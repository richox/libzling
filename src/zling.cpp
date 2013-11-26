/**
 * zling:
 *  light-weight lossless data compression utility.
 *
 * Copyright (C) 2012-2013 by Zhang Li <zhangli10 at baidu.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @author zhangli10<zhangli10@baidu.com>
 * @brief  zling main.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if HAS_CXX11_SUPPORT
#include <cstdint>
#else
#include <boost/cstdint.hpp>
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <fcntl.h>  // setmode()
#include <io.h>
#endif

#include "src/zling_codebuf.h"
#include "src/zling_huffman.h"
#include "src/zling_lz.h"

using baidu_zhangli10::zling::codebuf::ZlingCodebuf;
using baidu_zhangli10::zling::huffman::ZlingMakeLengthTable;
using baidu_zhangli10::zling::huffman::ZlingMakeEncodeTable;
using baidu_zhangli10::zling::huffman::ZlingMakeDecodeTable;
using baidu_zhangli10::zling::lz::ZlingRolzEncoder;
using baidu_zhangli10::zling::lz::ZlingRolzDecoder;

using baidu_zhangli10::zling::huffman::kHuffmanSymbols;
using baidu_zhangli10::zling::lz::kBucketItemSize;

static const int kHuffmanMaxLen1   = 15;
static const int kHuffmanMaxLen2   = 8;

static const int kBlockSizeIn      = 16777216;
static const int kBlockSizeRolz    = 262144;
static const int kBlockSizeHuffman = 393216;

static const unsigned char matchidx_bitlen[] = {
    /* 0 */ 0, 0, 0, 0,
    /* 4 */ 1, 1,
    /* 6 */ 2, 2,
    /* 8 */ 3, 3,
    /* 10*/ 4, 4,
    /* 12*/ 5, 5,
    /* 14*/ 6, 6,
    /* 16*/ 7, 7,
    /* 18*/ 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    /* 32*/
};
static const int kMatchidxCodeSymbols = sizeof(matchidx_bitlen) / sizeof(matchidx_bitlen[0]);
static const int kMatchidxMaxBitlen = 8;

static unsigned char matchidx_code[kBucketItemSize];
static unsigned char matchidx_bits[kBucketItemSize];
static uint16_t      matchidx_base[kMatchidxCodeSymbols];

static inline void InitMatchidxCode() {
    int code = 0;
    int bits = 0;

    for (int i = 0; i < kBucketItemSize; i++) {
        matchidx_code[i] = code;
        matchidx_bits[i] = bits;

        if ((++bits) >> matchidx_bitlen[code] != 0) {
            bits = 0;
            matchidx_base[++code] = i + 1;
        }
    }
    return;
}

static inline uint32_t IdxToCode(uint32_t idx) {
    return matchidx_code[idx];
}
static inline uint32_t IdxToBits(uint32_t idx) {
    return matchidx_bits[idx];
}
static inline uint32_t IdxToBitlen(uint32_t idx) {
    return matchidx_bitlen[matchidx_code[idx]];
}

static inline uint32_t IdxBitlenFromCode(uint32_t code) {
    return matchidx_bitlen[code];
}
static inline uint32_t IdxFromCodeBits(uint32_t code, uint32_t bits) {
    return matchidx_base[code] | bits;
}

static unsigned char ibuf[kBlockSizeIn];
static unsigned char obuf[kBlockSizeHuffman + 16];  // avoid overflow on decoding
static uint16_t      tbuf[kBlockSizeRolz];

static int main_encode() {
    ZlingRolzEncoder* lzencoder = new ZlingRolzEncoder();
    uint64_t size_src = 0;
    uint64_t size_dst = 0;
    int ilen = 0;
    int rlen = 0;
    int olen = 0;
    clock_t clock_start          = clock();
    clock_t clock_during_rolz    = 0;
    clock_t clock_during_huffman = 0;
    clock_t checkpoint;

    while ((ilen = fread(ibuf, 1, kBlockSizeIn, stdin)) > 0) {
        fputc(0, stdout);  // flag: start rolz round
        size_dst += 1;
        size_src += ilen;

        int encpos = 0;
        lzencoder->Reset();

        while (encpos < ilen) {
            fputc(1, stdout);  // flag: continue rolz round
            size_dst += 1;

            checkpoint = clock();  // rolz-encode
            {
                rlen = lzencoder->Encode(ibuf, tbuf, ilen, kBlockSizeRolz, &encpos);
            }
            clock_during_rolz += clock() - checkpoint;

            checkpoint = clock();  // huffman-encode
            {
                ZlingCodebuf codebuf;
                int opos = 0;
                uint32_t freq_table1[kHuffmanSymbols] = {0};
                uint32_t freq_table2[kHuffmanSymbols] = {0};
                uint32_t length_table1[kHuffmanSymbols] = {0};
                uint32_t length_table2[kHuffmanSymbols] = {0};
                uint16_t encode_table1[kHuffmanSymbols] = {0};
                uint16_t encode_table2[kHuffmanSymbols] = {0};

                for (int i = 0; i < rlen; i++) {
                    freq_table1[tbuf[i]] += 1;
                    if (tbuf[i] >= 256) {
                        freq_table2[matchidx_code[tbuf[++i]]] += 1;
                    }
                }
                ZlingMakeLengthTable(freq_table1, length_table1, 0, kHuffmanMaxLen1);
                ZlingMakeLengthTable(freq_table2, length_table2, 0, kHuffmanMaxLen2);

                ZlingMakeEncodeTable(length_table1, encode_table1, kHuffmanMaxLen1);
                ZlingMakeEncodeTable(length_table2, encode_table2, kHuffmanMaxLen2);

                // write length table
                for (int i = 0; i < kHuffmanSymbols; i += 2) {
                    obuf[opos++] = length_table1[i] * 16 + length_table1[i + 1];
                }
                for (int i = 0; i < kMatchidxCodeSymbols; i += 2) {
                    obuf[opos++] = length_table2[i] * 16 + length_table2[i + 1];
                }

                // encode
                for (int i = 0; i < rlen; i++) {
                    codebuf.Input(encode_table1[tbuf[i]], length_table1[tbuf[i]]);
                    if (tbuf[i] >= 256) {
                        i++;
                        codebuf.Input(
                            encode_table2[IdxToCode(tbuf[i])],
                            length_table2[IdxToCode(tbuf[i])]);
                        codebuf.Input(
                            IdxToBits(tbuf[i]),
                            IdxToBitlen(tbuf[i]));
                    }
                    while (codebuf.GetLength() >= 32) {
                        obuf[opos++] = codebuf.Output(8);
                        obuf[opos++] = codebuf.Output(8);
                        obuf[opos++] = codebuf.Output(8);
                        obuf[opos++] = codebuf.Output(8);
                    }
                }
                while (codebuf.GetLength() > 0) {
                    obuf[opos++] = codebuf.Output(8);
                }
                olen = opos;
            }
            clock_during_huffman += clock() - checkpoint;

            // output
            fputc(rlen / 16777216 % 256, stdout);
            fputc(olen / 16777216 % 256, stdout);
            fputc(rlen / 65536 % 256, stdout);
            fputc(olen / 65536 % 256, stdout);
            fputc(rlen / 256 % 256, stdout);
            fputc(olen / 256 % 256, stdout);
            fputc(rlen % 256, stdout);
            fputc(olen % 256, stdout);
            fwrite(obuf, 1, olen, stdout);
            size_dst += 8;
            size_dst += olen;
        }
        fprintf(stderr, "%6.2f MB => %6.2f MB %.2f%%, %.3f sec\n",
                size_src / 1e6,
                size_dst / 1e6,
                1e2 * size_dst / size_src, (clock() - clock_start) * 1.0 / CLOCKS_PER_SEC);
        fflush(stderr);
    }
    delete lzencoder;

    if (ferror(stdin) || ferror(stdout)) {
        fprintf(stderr, "error: I/O error.\n");
        return -1;
    }
    fprintf(stderr,
            "\nencode: %llu => %llu, time=%.3f sec\n"
            "\ttime_rolz:  %.3f sec\n"
            "\ttime_huffman: %.3f sec\n",
            size_src,
            size_dst,
            (clock() - clock_start) * 1.0 / CLOCKS_PER_SEC,
            clock_during_rolz    * 1.0 / CLOCKS_PER_SEC,
            clock_during_huffman * 1.0 / CLOCKS_PER_SEC);
    return 0;
}

static int main_decode() {
    ZlingRolzDecoder* lzdecoder = new ZlingRolzDecoder();
    uint64_t size_src = 0;
    uint64_t size_dst = 0;
    int rlen = 0;
    int olen = 0;
    clock_t clock_start          = clock();
    clock_t clock_during_rolz    = 0;
    clock_t clock_during_huffman = 0;
    clock_t checkpoint;

    while (ungetc(fgetc(stdin), stdin) == 0) {  // flag: start rolz round
        fgetc(stdin);
        size_dst += 1;

        int decpos = 0;
        lzdecoder->Reset();

        while (ungetc(fgetc(stdin), stdin) == 1) {  // flag: continue rolz round
            fgetc(stdin);
            size_dst += 1;

            rlen  = fgetc(stdin) * 16777216;
            olen  = fgetc(stdin) * 16777216;
            rlen += fgetc(stdin) * 65536;
            olen += fgetc(stdin) * 65536;
            rlen += fgetc(stdin) * 256;
            olen += fgetc(stdin) * 256;
            rlen += fgetc(stdin);
            olen += fgetc(stdin);
            if (fread(obuf, 1, olen, stdin) != static_cast<size_t>(olen)) {
                fprintf(stderr, "error: reading block with size '%d' error.", olen);
                return -1;
            }
            size_dst += 8;
            size_dst += olen;

            // huffman-decode
            checkpoint = clock();
            {
                ZlingCodebuf codebuf;
                int opos = 0;
                uint32_t length_table1[kHuffmanSymbols] = {0};
                uint32_t length_table2[kHuffmanSymbols] = {0};
                uint16_t decode_table1[1 << kHuffmanMaxLen1] = {0};
                uint16_t decode_table2[1 << kHuffmanMaxLen2] = {0};

                // read length table
                for (int i = 0; i < kHuffmanSymbols; i += 2) {
                    length_table1[i] =     obuf[opos] / 16;
                    length_table1[i + 1] = obuf[opos] % 16;
                    opos++;
                }
                for (int i = 0; i < kMatchidxCodeSymbols; i += 2) {
                    length_table2[i] =     obuf[opos] / 16;
                    length_table2[i + 1] = obuf[opos] % 16;
                    opos++;
                }
                ZlingMakeDecodeTable(length_table1, decode_table1, kHuffmanMaxLen1);
                ZlingMakeDecodeTable(length_table2, decode_table2, kHuffmanMaxLen2);

                // decode
                for (int i = 0; i < rlen; i++) {
                    while (/* opos < olen && */ codebuf.GetLength() < 32) {
                        codebuf.Input(obuf[opos++], 8);
                        codebuf.Input(obuf[opos++], 8);
                        codebuf.Input(obuf[opos++], 8);
                        codebuf.Input(obuf[opos++], 8);
                    }
                    tbuf[i] = decode_table1[codebuf.Peek(kHuffmanMaxLen1)];
                    codebuf.Output(length_table1[tbuf[i]]);

                    if (tbuf[i] >= 256) {
                        uint32_t code = decode_table2[codebuf.Peek(kHuffmanMaxLen2)];
                        uint32_t bitlen = IdxBitlenFromCode(code);
                        codebuf.Output(length_table2[code]);
                        tbuf[++i] = IdxFromCodeBits(code, codebuf.Output(bitlen));
                    }
                }
            }
            clock_during_huffman += clock() - checkpoint;

            // rolz-decode
            checkpoint = clock();
            {
                rlen = lzdecoder->Decode(tbuf, ibuf, rlen, &decpos);
            }
            clock_during_rolz += clock() - checkpoint;
        }

        // output
        fwrite(ibuf, 1, decpos, stdout);
        size_src += decpos;
        fprintf(stderr, "%6.2f MB <= %6.2f MB %.2f%%, %.3f sec\n",
                size_src / 1e6,
                size_dst / 1e6,
                1e2 * size_dst / size_src, (clock() - clock_start) * 1.0 / CLOCKS_PER_SEC);
        fflush(stderr);
    }
    delete lzdecoder;

    if (ferror(stdin) || ferror(stdout)) {
        fprintf(stderr, "error: I/O error.\n");
        return -1;
    }

    fprintf(stderr,
            "\ndecode: %llu <= %llu, time=%.3f sec\n"
            "\ttime_rolz:    %.3f sec\n"
            "\ttime_huffman: %.3f sec\n",
            size_src,
            size_dst,
            (clock() - clock_start) * 1.0 / CLOCKS_PER_SEC,
            clock_during_rolz    * 1.0 / CLOCKS_PER_SEC,
            clock_during_huffman * 1.0 / CLOCKS_PER_SEC);
    return 0;
}

int main(int argc, char** argv) {

    // set stdio to binary mode for windows
#if defined(__MINGW32__) || defined(__MINGW64__)
    setmode(fileno(stdin),  O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    // welcome message
    fprintf(stderr, "zling:\n");
    fprintf(stderr, "   light-weight lossless data compression utility\n");
    fprintf(stderr, "   by Zhang Li <zhangli10 at baidu.com>\n");
    fprintf(stderr, "\n");

    // zling <e/d> __argv2__ __argv3__
    if (argc == 4) {
        if (freopen(argv[3], "wb", stdout) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for write.\n", argv[3]);
            return -1;
        }
        argc = 3;
    }

    // zling <e/d> __argv2__ (stdout)
    if (argc == 3) {
        if (freopen(argv[2], "rb", stdin) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for read.\n", argv[2]);
            return -1;
        }
        argc = 2;
    }

    // zling <e/d> (stdin) (stdout)
    if (argc == 2 && strcmp(argv[1], "e") == 0) {
        InitMatchidxCode();
        return main_encode();
    }

    if (argc == 2 && strcmp(argv[1], "d") == 0) {
        InitMatchidxCode();
        return main_decode();
    }

    // help message
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "   zling e source target\n");
    fprintf(stderr, "   zling d source target\n");
    fprintf(stderr, "    * source: default to stdin\n");
    fprintf(stderr, "    * target: default to stdout\n");
    return -1;
}
