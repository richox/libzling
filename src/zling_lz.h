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
 * @brief  manipulate ROLZ (reduced offset Lempel-Ziv) compression.
 */
#ifndef SRC_ZLING_LZ_H
#define SRC_ZLING_LZ_H

#include <cstring>

#if HAS_CXX11_SUPPORT
#include <cstdint>
#else
#include <stdint.h>
#include <inttypes.h>
#endif

namespace baidu {
namespace zling {
namespace lz {

static const int kBucketItemSize = 4096;
static const int kBucketItemHash = 8192;
static const int kMatchDiscardMinLen = 3000;
static const int kMatchDepth = 8;
static const int kMatchMinLen = 4;
static const int kMatchMaxLen = 259;

class ZlingRolzEncoder {
public:
    ZlingRolzEncoder() {
        Reset();
    }

    /* Encode:
     *  arg ibuf:   input data
     *  arg obuf:   output data (compressed)
     *  arg ilen:   input data length
     *  arg olen:   input data length
     *  arg decpos: start encoding at ibuf[encpos], limited by ilen and olen
     */
    int  Encode(unsigned char* ibuf, uint16_t* obuf, int ilen, int olen, int* encpos);
    void Reset();

private:
    int  Match(unsigned char* buf, int pos, int* match_idx, int* match_len);
    void Update(unsigned char* buf, int pos);

    struct ZlingEncodeBucket {
        uint16_t suffix[kBucketItemSize];
        uint32_t offset[kBucketItemSize];
        uint16_t head;
        uint16_t hash[kBucketItemHash];
    };
    ZlingEncodeBucket m_buckets[256];

    ZlingRolzEncoder(const ZlingRolzEncoder&);
    ZlingRolzEncoder& operator = (const ZlingRolzEncoder&);
};

class ZlingRolzDecoder {
public:
    ZlingRolzDecoder() {
        Reset();
    }

    /* Decode:
     *  arg ibuf:   input data (compressed)
     *  arg obuf:   output data
     *  arg ilen:   input data length
     *  arg decpos: start decoding at obuf[decpos], limited by ilen
     */
    int  Decode(uint16_t* ibuf, unsigned char* obuf, int ilen, int* decpos);
    void Reset();

private:
    int  GetMatch(unsigned char* buf, int pos, int idx);
    void Update(unsigned char* buf, int pos);

    struct ZlingDecodeBucket {
        uint32_t offset[kBucketItemSize];
        uint16_t head;
    };
    ZlingDecodeBucket m_buckets[256];

    ZlingRolzDecoder(const ZlingRolzDecoder&);
    ZlingRolzDecoder& operator = (const ZlingRolzDecoder&);
};

}  // namespace lz
}  // namespace zling
}  // namespace baidu
#endif  // SRC_ZLING_LZ_H
