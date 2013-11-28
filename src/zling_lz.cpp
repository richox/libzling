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
#include "src/zling_lz.h"

namespace baidu {
namespace zling {
namespace lz {

static inline uint32_t HashContext(unsigned char* ptr) {
    return (ptr[0] * 33337 + ptr[1] * 3337 + ptr[2] * 337 + ptr[3]);
}

static inline uint32_t RollingAdd(uint32_t x, uint32_t y) {
    return (x + y) & (kBucketItemSize - 1);
}
static inline uint32_t RollingSub(uint32_t x, uint32_t y) {
    return (x - y) & (kBucketItemSize - 1);
}

static inline int GetCommonLength(unsigned char* buf1, unsigned char* buf2, int maxlen) {
    unsigned char* p1 = buf1;
    unsigned char* p2 = buf2;

    while ((maxlen--) > 0 && *p1 == *p2) {
        p1++;
        p2++;
    }
    return p1 - buf1;
}

static inline void IncrementalCopyFastPath(unsigned char* src, unsigned char* dst, int len) {
    while (dst - src < 8) {
        *reinterpret_cast<uint64_t*>(dst) = *reinterpret_cast<uint64_t*>(src);
        len -= dst - src;
        dst += dst - src;
    }
    while (len > 0) {
        *reinterpret_cast<uint64_t*>(dst) = *reinterpret_cast<uint64_t*>(src);
        len -= 8;
        dst += 8;
        src += 8;
    }
    return;
}

int ZlingRolzEncoder::Encode(unsigned char* ibuf, uint16_t* obuf, int ilen, int olen, int* encpos) {
    int ipos = encpos[0];
    int opos = 0;

    // first byte
    if (ipos == 0 && opos < olen && ipos < ilen) {
        obuf[opos++] = ibuf[ipos++];
    }

    while (opos + 1 < olen && ipos + kMatchMaxLen < ilen) {
        int match_idx;
        int match_len;

        if (Match(ibuf, ipos, &match_idx, &match_len)) {
            obuf[opos++] = 256 + match_len - kMatchMinLen;  // encode as match
            obuf[opos++] = match_idx;
            Update(ibuf, ipos);
            ipos += match_len;

        } else {
            obuf[opos++] = ibuf[ipos];  // encode as literal
            Update(ibuf, ipos);
            ipos += 1;
        }
    }

    // rest byte
    while (opos < olen && ipos < ilen) {
        obuf[opos++] = ibuf[ipos];
        Update(ibuf, ipos);
        ipos += 1;
    }
    encpos[0] = ipos;
    return opos;
}

void ZlingRolzEncoder::Reset() {
    memset(m_buckets, 0, sizeof(m_buckets));
    return;
}

int ZlingRolzEncoder::Match(unsigned char* buf, int pos, int* match_idx, int* match_len) {
    int maxlen = kMatchMinLen - 1;
    int maxidx = 0;
    int hash = HashContext(buf + pos);
    int hash_check   = hash / kBucketItemHash % 256;
    int hash_context = hash % kBucketItemHash;
    int node;
    int i;
    ZlingEncodeBucket* bucket = &m_buckets[buf[pos - 1]];

    node = bucket->hash[hash_context];

    for (i = 0; i < kMatchDepth; i++) {
        int offset = bucket->offset[node] & 0xffffff;
        int check = bucket->offset[node] >> 24;

        if (check == hash_check && buf[pos + maxlen] == buf[offset + maxlen]) {
            int len = GetCommonLength(buf + pos, buf + offset, kMatchMaxLen);

            if (len > maxlen) {
                maxlen = len;
                maxidx = RollingSub(bucket->head, node);
                if (maxlen == kMatchMaxLen) {
                    break;
                }
            }
        }
        if (offset <= int(bucket->offset[bucket->suffix[node]] & 0xffffff)) {
            break;
        }
        node = bucket->suffix[node];
    }
    if (maxlen >= kMatchMinLen + (maxidx >= kMatchDiscardMinLen)) {
        *match_len = maxlen;
        *match_idx = maxidx;
        return 1;
    }
    return 0;
}

void ZlingRolzEncoder::Update(unsigned char* buf, int pos) {
    int hash = HashContext(buf + pos);
    int hash_check   = hash / kBucketItemHash % 256;
    int hash_context = hash % kBucketItemHash;
    ZlingEncodeBucket* bucket = &m_buckets[buf[pos - 1]];

    bucket->head = RollingAdd(bucket->head, 1);
    bucket->suffix[bucket->head] = bucket->hash[hash_context];
    bucket->offset[bucket->head] = pos | hash_check << 24;
    bucket->hash[hash_context] = bucket->head;
    return;
}

int ZlingRolzDecoder::Decode(uint16_t* ibuf, unsigned char* obuf, int ilen, int* decpos) {
    int opos = decpos[0];
    int ipos = 0;
    int match_idx;
    int match_len;
    int match_offset;

    // first byte
    if (opos == 0 && ipos < ilen) {
        obuf[opos++] = ibuf[ipos++];
    }

    // rest byte
    while (ipos < ilen) {
        if (ibuf[ipos] < 256) {  // process a literal byte
            obuf[opos] = ibuf[ipos++];
            Update(obuf, opos);
            opos++;

        } else {  // process a match
            match_len = ibuf[ipos++] - 256 + kMatchMinLen;
            match_idx = ibuf[ipos++];
            match_offset = GetMatch(obuf, opos, match_idx);
            Update(obuf, opos);

            IncrementalCopyFastPath(&obuf[match_offset], &obuf[opos], match_len);
            opos += match_len;
        }
    }
    decpos[0] = opos;
    return ipos;
}

void ZlingRolzDecoder::Reset() {
    memset(m_buckets, 0, sizeof(m_buckets));
    return;
}

int ZlingRolzDecoder::GetMatch(unsigned char* buf, int pos, int idx) {
    ZlingDecodeBucket* bucket = &m_buckets[buf[pos - 1]];
    int head = bucket->head;
    int node = RollingSub(head, idx);
    return bucket->offset[node];
}

void ZlingRolzDecoder::Update(unsigned char* buf, int pos) {
    ZlingDecodeBucket* bucket = &m_buckets[buf[pos - 1]];

    bucket->head = RollingAdd(bucket->head, 1);
    bucket->offset[bucket->head] = pos;
    return;
}

}  // namespace lz
}  // namespace zling
}  // namespace baidu
