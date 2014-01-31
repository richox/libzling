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
 * @brief  libzling.
 */
#include "stdio.h"
#include "libzling.h"
#include "zling_codebuf.h"
#include "zling_huffman.h"
#include "zling_lz.h"

namespace baidu {
namespace zling {

using codebuf::ZlingCodebuf;
using huffman::ZlingMakeLengthTable;
using huffman::ZlingMakeEncodeTable;
using huffman::ZlingMakeDecodeTable;
using lz::ZlingRolzEncoder;
using lz::ZlingRolzDecoder;

using lz::kMatchMaxLen;
using lz::kMatchMinLen;
using lz::kBucketItemSize;

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

static unsigned char matchidx_code[kBucketItemSize] = {
#include "ztable_matchidx_code.inc"  /* include auto-generated constant tables */
};
static unsigned char matchidx_bits[kBucketItemSize] = {
#include "ztable_matchidx_bits.inc"  /* include auto-generated constant tables */
};
static uint16_t matchidx_base[kMatchidxCodeSymbols] = {
#include "ztable_matchidx_base.inc"  /* include auto-generated constant tables */
};

#if 0  /* no longer needed -- use constant tables instead. */
static inline void InitMatchidxCode() {
    int code = 0;
    int bits = 0;

    for (int i = 0; i < kBucketItemSize; i++) {
        matchidx_code[i] = code;
        matchidx_bits[i] = bits;

        if (i + 1 < kBucketItemSize && (++bits) >> matchidx_bitlen[code] != 0) {
            bits = 0;
            matchidx_base[++code] = i + 1;
        }
    }

    /* print tables */
    for (int i = 0; i < kBucketItemSize; i++) {
        fprintf(stderr, "%2hhu,%s", matchidx_code[i], ((i % 16 == 15) ? "\n" : "\x20"));
    }
    for (int i = 0; i < kBucketItemSize; i++) {
        fprintf(stderr, "%3hhu,%s", matchidx_bits[i], ((i % 16 == 15) ? "\n" : "\x20"));
    }
    for (int i = 0; i < kMatchidxCodeSymbols; i++) {
        fprintf(stderr, "%4hu,%s", matchidx_base[i], ((i % 16 == 15) ? "\n" : "\x20"));
    }
    return;
}
#endif

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

static const int kHuffmanCodes1      = 256 + (kMatchMaxLen - kMatchMinLen + 1);  // must be even
static const int kHuffmanCodes2      = kMatchidxCodeSymbols;                     // must be even
static const int kHuffmanMaxLen1     = 15;
static const int kHuffmanMaxLen2     = 8;
static const int kHuffmanMaxLen1Fast = 10;

static const int kBlockSizeIn      = 16777216;
static const int kBlockSizeRolz    = 262144;
static const int kBlockSizeHuffman = 393216;

static inline int GetChar(IInputer* inputer) {
    unsigned char ch;
    inputer->GetData(&ch, 1);
    return ch;
}
static inline int PutChar(IOutputer* outputer, int c) {
    unsigned char ch = c;
    outputer->PutData(&ch, 1);
    return ch;
}

#define CHECK_IO_ERROR(io) do { \
    if ((io)->IsErr()) { \
        goto EncodeOrDecodeFinished; \
    } \
} while(0)

static const int kFlagRolzStart    = 1;
static const int kFlagRolzContinue = 2;
static const int kFlagRolzStop     = 0;

int Encode(IInputer* inputer, IOutputer* outputer, IActionHandler* action_handler) {
    if (action_handler) {
        action_handler->SetInputerOutputer(inputer, outputer, true);
        action_handler->OnInit();
    }

    ZlingRolzEncoder* lzencoder = new ZlingRolzEncoder();
    int ilen;
    int olen;
    int rlen;
    int encpos;

    unsigned char* ibuf = new unsigned char[kBlockSizeIn];
    unsigned char* obuf = new unsigned char[kBlockSizeHuffman + 16];  // avoid overflow on decoding
    uint16_t*      tbuf = new unsigned uint16_t[kBlockSizeRolz];

    while (!inputer->IsEnd() && !inputer->IsErr()) {
        ilen = 0;
        olen = 0;
        rlen = 0;
        encpos = 0;

        while(!inputer->IsEnd() && !inputer->IsErr() && ilen < kBlockSizeIn) {
            ilen += inputer->GetData(ibuf + ilen, kBlockSizeIn - ilen);
            CHECK_IO_ERROR(inputer);
        }

        PutChar(outputer, kFlagRolzStart);
        CHECK_IO_ERROR(outputer);

        lzencoder->Reset();

        while (encpos < ilen) {
            PutChar(outputer, kFlagRolzContinue);
            CHECK_IO_ERROR(outputer);

            // ROLZ encode
            // ============================================================
            rlen = lzencoder->Encode(ibuf, tbuf, ilen, kBlockSizeRolz, &encpos);

            // HUFFMAN encode
            // ============================================================
            ZlingCodebuf codebuf;
            int opos = 0;
            uint32_t freq_table1[kHuffmanCodes1] = {0};
            uint32_t freq_table2[kHuffmanCodes2] = {0};
            uint32_t length_table1[kHuffmanCodes1];
            uint32_t length_table2[kHuffmanCodes2];
            uint16_t encode_table1[kHuffmanCodes1];
            uint16_t encode_table2[kHuffmanCodes2];

            for (int i = 0; i < rlen; i++) {
                freq_table1[tbuf[i]] += 1;
                if (tbuf[i] >= 256) {
                    freq_table2[IdxToCode(tbuf[++i])] += 1;
                }
            }
            ZlingMakeLengthTable(freq_table1, length_table1, 0, kHuffmanCodes1, kHuffmanMaxLen1);
            ZlingMakeLengthTable(freq_table2, length_table2, 0, kHuffmanCodes2, kHuffmanMaxLen2);

            ZlingMakeEncodeTable(length_table1, encode_table1, kHuffmanCodes1, kHuffmanMaxLen1);
            ZlingMakeEncodeTable(length_table2, encode_table2, kHuffmanCodes2, kHuffmanMaxLen2);

            // write length table
            for (int i = 0; i < kHuffmanCodes1; i += 2) {
                obuf[opos++] = length_table1[i] * 16 + length_table1[i + 1];
            }
            for (int i = 0; i < kHuffmanCodes2; i += 2) {
                obuf[opos++] = length_table2[i] * 16 + length_table2[i + 1];
            }
            if (opos % 4 != 0) obuf[opos++] = 0;  // keep aligned
            if (opos % 4 != 0) obuf[opos++] = 0;  // keep aligned
            if (opos % 4 != 0) obuf[opos++] = 0;  // keep aligned
            if (opos % 4 != 0) obuf[opos++] = 0;  // keep aligned

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
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    *reinterpret_cast<uint32_t*>(obuf + opos) = codebuf.Output(32);
                    opos += 4;
#else
                    obuf[opos++] = codebuf.Output(8);
                    obuf[opos++] = codebuf.Output(8);
                    obuf[opos++] = codebuf.Output(8);
                    obuf[opos++] = codebuf.Output(8);
#endif
                }
            }
            while (codebuf.GetLength() > 0) {
                obuf[opos++] = codebuf.Output(8);
            }
            olen = opos;

            // outputer
            PutChar(outputer, rlen / 16777216 % 256); CHECK_IO_ERROR(outputer);
            PutChar(outputer, olen / 16777216 % 256); CHECK_IO_ERROR(outputer);
            PutChar(outputer, rlen / 65536 % 256);    CHECK_IO_ERROR(outputer);
            PutChar(outputer, olen / 65536 % 256);    CHECK_IO_ERROR(outputer);
            PutChar(outputer, rlen / 256 % 256);      CHECK_IO_ERROR(outputer);
            PutChar(outputer, olen / 256 % 256);      CHECK_IO_ERROR(outputer);
            PutChar(outputer, rlen % 256);            CHECK_IO_ERROR(outputer);
            PutChar(outputer, olen % 256);            CHECK_IO_ERROR(outputer);

            for (int ooff = 0; !outputer->IsErr() && ooff < olen; ) {
                ooff += outputer->PutData(obuf + ooff, olen - ooff);
                CHECK_IO_ERROR(outputer);
            }
        }
        PutChar(outputer, kFlagRolzStop);
        CHECK_IO_ERROR(outputer);

        if (action_handler) {
            action_handler->OnProcess();
        }
    }

EncodeOrDecodeFinished:
    if (action_handler) {
        action_handler->OnDone();
    }
    delete lzencoder;
    delete [] ibuf;
    delete [] obuf;
    delete [] tbuf;
    return (inputer->IsErr() || outputer->IsErr()) ? -1 : 0;
}

int Decode(IInputer* inputer, IOutputer* outputer, IActionHandler* action_handler) {
    if (action_handler) {
        action_handler->SetInputerOutputer(inputer, outputer, false);
        action_handler->OnInit();
    }

    ZlingRolzDecoder* lzdecoder = new ZlingRolzDecoder();
    int rlen;
    int olen;
    int decpos;

    unsigned char* ibuf = new unsigned char[kBlockSizeIn];
    unsigned char* obuf = new unsigned char[kBlockSizeHuffman + 16];  // avoid overflow on decoding
    uint16_t*      tbuf = new unsigned uint16_t[kBlockSizeRolz];

    while (!inputer->IsEnd() && GetChar(inputer) == kFlagRolzStart) {
        olen = 0;
        rlen = 0;
        decpos = 0;
        lzdecoder->Reset();

        while (GetChar(inputer) == kFlagRolzContinue) {
            rlen  = GetChar(inputer) * 16777216; CHECK_IO_ERROR(inputer);
            olen  = GetChar(inputer) * 16777216; CHECK_IO_ERROR(inputer);
            rlen += GetChar(inputer) * 65536;    CHECK_IO_ERROR(inputer);
            olen += GetChar(inputer) * 65536;    CHECK_IO_ERROR(inputer);
            rlen += GetChar(inputer) * 256;      CHECK_IO_ERROR(inputer);
            olen += GetChar(inputer) * 256;      CHECK_IO_ERROR(inputer);
            rlen += GetChar(inputer);            CHECK_IO_ERROR(inputer);
            olen += GetChar(inputer);            CHECK_IO_ERROR(inputer);

            for (int ooff = 0; !outputer->IsErr() && ooff < olen; ) {
                ooff += inputer->GetData(obuf + ooff, olen - ooff);
                CHECK_IO_ERROR(inputer);
            }

            // HUFFMAN DECODE
            // ============================================================
            ZlingCodebuf codebuf;
            int opos = 0;
            uint32_t length_table1[kHuffmanCodes1] = {0};
            uint32_t length_table2[kHuffmanCodes2] = {0};
            uint16_t decode_table1[1 << kHuffmanMaxLen1];
            uint16_t decode_table2[1 << kHuffmanMaxLen2];
            uint16_t decode_table1_fast[1 << kHuffmanMaxLen1Fast];
            uint16_t encode_table1[kHuffmanCodes1];
            uint16_t encode_table2[kHuffmanCodes2];

            // read length table
            for (int i = 0; i < kHuffmanCodes1; i += 2) {
                length_table1[i] =     obuf[opos] / 16;
                length_table1[i + 1] = obuf[opos] % 16;
                opos++;
            }
            for (int i = 0; i < kHuffmanCodes2; i += 2) {
                length_table2[i] =     obuf[opos] / 16;
                length_table2[i + 1] = obuf[opos] % 16;
                opos++;
            }
            if (opos % 4 != 0) opos++;  // keep aligned
            if (opos % 4 != 0) opos++;  // keep aligned
            if (opos % 4 != 0) opos++;  // keep aligned
            if (opos % 4 != 0) opos++;  // keep aligned

            ZlingMakeEncodeTable(length_table1, encode_table1, kHuffmanCodes1, kHuffmanMaxLen1);
            ZlingMakeEncodeTable(length_table2, encode_table2, kHuffmanCodes2, kHuffmanMaxLen2);

            // decode_table1: 2-level decode table
            ZlingMakeDecodeTable(length_table1,
                                 encode_table1,
                                 decode_table1,
                                 kHuffmanCodes1,
                                 kHuffmanMaxLen1);
            ZlingMakeDecodeTable(length_table1,
                                 encode_table1,
                                 decode_table1_fast,
                                 kHuffmanCodes1,
                                 kHuffmanMaxLen1Fast);

            // decode_table2: 1-level decode table
            ZlingMakeDecodeTable(length_table2,
                                 encode_table2,
                                 decode_table2,
                                 kHuffmanCodes2,
                                 kHuffmanMaxLen2);

            // decode
            for (int i = 0; i < rlen; i++) {
                while (/* opos < olen && */ codebuf.GetLength() < 32) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    codebuf.Input(*reinterpret_cast<uint32_t*>(obuf + opos), 32);
                    opos += 4;
#else
                    codebuf.Input(obuf[opos++], 8);
                    codebuf.Input(obuf[opos++], 8);
                    codebuf.Input(obuf[opos++], 8);
                    codebuf.Input(obuf[opos++], 8);
#endif
                }

                tbuf[i] = decode_table1_fast[codebuf.Peek(kHuffmanMaxLen1Fast)];
                if (tbuf[i] == uint16_t(-1)) {
                    tbuf[i] = decode_table1[codebuf.Peek(kHuffmanMaxLen1)];
                }
                codebuf.Output(length_table1[tbuf[i]]);

                if (tbuf[i] >= 256) {
                    uint32_t code = decode_table2[codebuf.Peek(kHuffmanMaxLen2)];
                    uint32_t bitlen = IdxBitlenFromCode(code);
                    codebuf.Output(length_table2[code]);
                    tbuf[++i] = IdxFromCodeBits(code, codebuf.Output(bitlen));
                }
            }

            // ROLZ decode
            // ============================================================
            rlen = lzdecoder->Decode(tbuf, ibuf, rlen, &decpos);
        }

        // output
        for (int ioff = 0; !outputer->IsErr() && ioff < decpos; ) {
            ioff += outputer->PutData(ibuf + ioff, decpos - ioff);
            CHECK_IO_ERROR(outputer);
        }

        if (action_handler) {
            action_handler->OnProcess();
        }
    }

EncodeOrDecodeFinished:
    if (action_handler) {
        action_handler->OnDone();
    }
    delete lzdecoder;
    delete [] ibuf;
    delete [] obuf;
    delete [] tbuf;
    return (inputer->IsErr() || outputer->IsErr()) ? -1 : 0;
    return 0;
}

}  // namespace zling
}  // namespace baidu
