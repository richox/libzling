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
 * @brief  manipulate huffman encoding.
 */
#ifndef SRC_ZLING_HUFFMAN_H
#define SRC_ZLING_HUFFMAN_H

#include <algorithm>
#include <cstring>

#if HAS_CXX11_SUPPORT
#include <cstdint>
#else
#include <boost/cstdint.hpp>
#endif

namespace baidu_zhangli10 {
namespace zling {
namespace huffman {

static const int kHuffmanSymbols = 384; // should be even
static const int kHuffmanMaxLen = 15;   // should be < 16 -- packing two length values into a byte

void ZlingMakeLengthTable(const uint32_t* freq_table, uint32_t* length_table, int scaling);
void ZlingMakeEncodeTable(const uint32_t* length_table, uint16_t* encode_table);
void ZlingMakeDecodeTable(const uint32_t* length_table, uint16_t* decode_table);

}  // namespace huffman
}  // namespace zling
}  // namespace baidu_zhangli10
#endif  // SRC_ZLING_HUFFMAN_H
