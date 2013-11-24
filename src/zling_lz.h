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
#ifndef ZLING_LZ_H
#define ZLING_LZ_H

#include <cstring>

namespace baidu_zhangli10 {
namespace zling {
namespace lz {

static const int kBucketItemSize = 4096;
static const int kBucketItemHash = 2048;
static const int kMatchDiscardMinLen = 1333;
static const int kMatchDepth = 8;
static const int kMatchMinLen = 4;
static const int kMatchMaxLen = 120;

class ZlingRolzEncoder {
public:
    ZlingRolzEncoder() {
        Reset();
    }

    int  Encode(unsigned char* ibuf, unsigned short* obuf, int ilen, int olen, int* encpos);
    void Reset();

private:
    int  Match(unsigned char* buf, int pos, int* match_idx, int* match_len);
    void Update(unsigned char* buf, int pos);

    struct ZlingEncodeBucket {
        unsigned short suffix[kBucketItemSize];
        unsigned       offset[kBucketItemSize];
        unsigned short head;
        unsigned short hash[kBucketItemHash];
    };

    ZlingEncodeBucket m_buckets[256];
};

class ZlingRolzDecoder {
public:
    ZlingRolzDecoder() {
        Reset();
    }

    int  Decode(unsigned short* ibuf, unsigned char* obuf, int ilen, int* encpos);
    void Reset();

private:
    int  GetMatch(unsigned char* buf, int pos, int idx);
    void Update(unsigned char* buf, int pos);

    struct ZlingDecodeBucket {
        unsigned       offset[kBucketItemSize];
        unsigned short head;
    };

    ZlingDecodeBucket m_buckets[256];
};

}  // namespace lz
}  // namespace zling
}  // namespace baidu_zhangli10
#endif
