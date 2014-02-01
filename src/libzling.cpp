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
#include "libzling.h"
#include "libzling_huffman.h"
#include "libzling_lz.h"

namespace baidu {
namespace zling {

using huffman::ZlingMakeLengthTable;
using huffman::ZlingMakeEncodeTable;
using huffman::ZlingMakeDecodeTable;
using lz::ZlingRolzEncoder;
using lz::ZlingRolzDecoder;

using lz::kMatchMaxLen;
using lz::kMatchMinLen;
using lz::kBucketItemSize;

static const unsigned char matchidx_bitlen[] = {
#include "ztable_matchidx_blen.inc"  /* include auto-generated constant tables */
};

static const unsigned char matchidx_code[] = {
#include "ztable_matchidx_code.inc"  /* include auto-generated constant tables */
};
static const unsigned char matchidx_bits[] = {
#include "ztable_matchidx_bits.inc"  /* include auto-generated constant tables */
};
static const uint16_t matchidx_base[] = {
#include "ztable_matchidx_base.inc"  /* include auto-generated constant tables */
};

static const int kMatchidxCodeSymbols = sizeof(matchidx_bitlen) / sizeof(matchidx_bitlen[0]);
static const int kMatchidxMaxBitlen = 8;

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

#define CHECK_IO_ERROR(io) do { \
    if ((io)->IsErr()) { \
        goto EncodeOrDecodeFinished; \
    } \
} while(0)

static const int kFlagRolzContinue = 1;
static const int kFlagRolzStop     = 0;

int Encode(IInputer* inputer, IOutputer* outputer, IActionHandler* action_handler) {
    if (action_handler) {
        action_handler->SetInputerOutputer(inputer, outputer, true);
        action_handler->OnInit();
    }

    ZlingRolzEncoder* lzencoder = NULL;
    int rlen;
    int ilen;
    int olen;
    int encpos;
    unsigned char* ibuf = NULL;
    unsigned char* obuf = NULL;
    uint16_t* tbuf = NULL;

    lzencoder = new ZlingRolzEncoder();
    tbuf = new unsigned uint16_t[kBlockSizeRolz];
    ibuf = new unsigned char[kBlockSizeIn];
    obuf = new unsigned char[kBlockSizeHuffman + 16];  // avoid overflow on decoding

    if (!lzencoder || !tbuf || !ibuf || !obuf) {
        delete lzencoder;
        delete [] ibuf;
        delete [] obuf;
        delete [] tbuf;
        throw std::bad_alloc();
    }
    while (!inputer->IsEnd() && !inputer->IsErr()) {
        rlen = 0;
        ilen = 0;
        olen = 0;
        encpos = 0;

        while(!inputer->IsEnd() && !inputer->IsErr() && ilen < kBlockSizeIn) {
            ilen += inputer->GetData(ibuf + ilen, kBlockSizeIn - ilen);
            CHECK_IO_ERROR(inputer);
        }
        lzencoder->Reset();

        while (encpos < ilen) {
             outputer->PutChar(kFlagRolzContinue);
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
            outputer->PutUInt32(encpos); CHECK_IO_ERROR(outputer);
            outputer->PutUInt32(rlen);   CHECK_IO_ERROR(outputer);
            outputer->PutUInt32(olen);   CHECK_IO_ERROR(outputer);

            for (int ooff = 0; !outputer->IsErr() && ooff < olen; ) {
                ooff += outputer->PutData(obuf + ooff, olen - ooff);
                CHECK_IO_ERROR(outputer);
            }
        }
        outputer->PutChar(kFlagRolzStop);
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

    ZlingRolzDecoder* lzdecoder = NULL;
    int rlen;
    int olen;
    int encflag;
    int encpos;
    int decpos;
    unsigned char* ibuf = NULL;
    unsigned char* obuf = NULL;
    uint16_t* tbuf = NULL;

    lzdecoder = new ZlingRolzDecoder();
    tbuf = new unsigned uint16_t[kBlockSizeRolz];
    ibuf = new unsigned char[kBlockSizeIn];
    obuf = new unsigned char[kBlockSizeHuffman + 16];  // avoid overflow on decoding

    if (!lzdecoder || !tbuf || !ibuf || !obuf) {
        delete lzdecoder;
        delete [] ibuf;
        delete [] obuf;
        delete [] tbuf;
        throw std::bad_alloc();
    }
    while (!inputer->IsEnd()) {
        olen = 0;
        rlen = 0;
        decpos = 0;
        lzdecoder->Reset();

        while (!inputer->IsEnd()) {
            encflag = inputer->GetChar();

            if (encflag != kFlagRolzStop && encflag != kFlagRolzContinue) {
                delete lzdecoder;  /* error: invalid encflag */
                delete [] ibuf;
                delete [] obuf;
                delete [] tbuf;
                throw std::runtime_error("baidu::zling::Decode(): invalid encflag.");
            }
            if (encflag == kFlagRolzStop) {
                break;
            }

            encpos = inputer->GetUInt32(); CHECK_IO_ERROR(inputer);
            rlen   = inputer->GetUInt32(); CHECK_IO_ERROR(inputer);
            olen   = inputer->GetUInt32(); CHECK_IO_ERROR(inputer);

            for (int ooff = 0; !inputer->IsEnd() && ooff < olen; ) {
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
                while (codebuf.GetLength() < 32) {
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

                if (tbuf[i] >= kHuffmanCodes1) {
                    delete lzdecoder;  /* error: literal/length >= kHuffmanCodes1 */
                    delete [] ibuf;
                    delete [] obuf;
                    delete [] tbuf;
                    throw std::runtime_error("baidu::zling::Decode(): invalid huffman stream. (bad code1)");
                }
                codebuf.Output(length_table1[tbuf[i]]);

                if (tbuf[i] >= 256) {
                    uint32_t code;
                    uint32_t bitlen;
                    uint32_t bits;

                    if((code = decode_table2[codebuf.Peek(kHuffmanMaxLen2)]) >= kHuffmanCodes2) {
                        delete lzdecoder;  /* error: matchidx.code >= kHuffmanCodes2 */
                        delete [] ibuf;
                        delete [] obuf;
                        delete [] tbuf;
                        throw std::runtime_error("baidu::zling::Decode(): invalid huffman stream. (bad code2)");
                    }
                    codebuf.Output(length_table2[code]);

                    bitlen = IdxBitlenFromCode(code);
                    bits = codebuf.Output(bitlen);

                    if ((tbuf[++i] = IdxFromCodeBits(code, bits)) >= kBucketItemSize) {
                        delete lzdecoder;  /* error: matchidx >= kBucketItemSize */
                        delete [] ibuf;
                        delete [] obuf;
                        delete [] tbuf;
                        throw std::runtime_error("baidu::zling::Decode(): invalid huffman stream. (bad ex-bits)");
                    }
                }
            }

            // ROLZ decode
            // ============================================================
            if (lzdecoder->Decode(tbuf, ibuf, rlen, encpos, &decpos) == -1) {
                delete lzdecoder;  /* error: lz.Decode failed */
                delete [] ibuf;
                delete [] obuf;
                delete [] tbuf;
                throw std::runtime_error("baidu::zling::Decode(): lzdecode failed.");
            }
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
