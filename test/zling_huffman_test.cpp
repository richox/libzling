// Copyright (c) 2013, Zhang Li.
// author zhangli10<zhangli10@baidu.com>
// brief  unittest for zling_codebuf
#include <gtest/gtest.h>
#include "src/zling_huffman.h"

TEST(ZlingHuffman, MakeLengthTable) {
    uint32_t freq_table[4] = {
        0,
        10000,
        1,
        1,
    };
    uint32_t length_table[4];

    baidu::zling::huffman::ZlingMakeLengthTable(freq_table, length_table, 0, 4, 8);
    EXPECT_EQ(length_table[0], 0u);
    EXPECT_EQ(length_table[1], 1u);
    EXPECT_EQ(length_table[2], 2u);
    EXPECT_EQ(length_table[3], 2u);
}

TEST(ZlingHuffman, MakeEncodeTable) {
    uint32_t length_table[4] = {
        0,
        1,
        2,
        2,
    };
    uint16_t encode_table[4];

    baidu::zling::huffman::ZlingMakeEncodeTable(length_table, encode_table, 4, 8);
    EXPECT_EQ(encode_table[0], 0);
    EXPECT_EQ(encode_table[1], 0);  // rev(0)
    EXPECT_EQ(encode_table[2], 1);  // rev(10)
    EXPECT_EQ(encode_table[3], 3);  // rev(11)
}

TEST(ZlingHuffman, MakeDecodeTable) {
    uint32_t length_table[4] = {
        0,
        1,
        2,
        2,
    };
    uint16_t encode_table[4];
    uint16_t decode_table[1 << 8];

    baidu::zling::huffman::ZlingMakeEncodeTable(length_table, encode_table, 4, 8);
    baidu::zling::huffman::ZlingMakeDecodeTable(length_table, encode_table, decode_table, 4, 8);
    EXPECT_EQ(decode_table[0], 1);  // 0 ->1
    EXPECT_EQ(decode_table[1], 2);  // 10->2
    EXPECT_EQ(decode_table[2], 1);  // 01->1
    EXPECT_EQ(decode_table[3], 3);  // 11->3
}
