// Copyright (c) 2013, Zhang Li.
// author zhangli10<zhangli10@baidu.com>
// brief  unittest for zling_codebuf
#include <gtest/gtest.h>
#include "src/zling_lz.h"

TEST(ZlingLZ, Main) {
    baidu::zling::lz::ZlingRolzEncoder* encoder = new baidu::zling::lz::ZlingRolzEncoder();
    baidu::zling::lz::ZlingRolzDecoder* decoder = new baidu::zling::lz::ZlingRolzDecoder();

    char data[2][512] = {
            "0.1.2.3.4.5.6.7.8.9"
            "9.8.7.6.5.4.3.2.1.0"
            ".match:123456"
            ".match:123789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ)!@#$%^&*(__(*&^%$#@!)XYXWVUTSRQPONMLKJIHGFEDCBA"
            "abcdefghijklmnopqrstuvwxyz0123456789==9876543210xyxwvutsrqponmlkjihgfedcba"
            "aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz"
            "AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ"
    };
    int test_len = strlen(data[0]) + 1;

    uint16_t test_encoded[512];
    int encpos = 0;
    int decpos = 0;

    fprintf(stderr, "0\n");
    fflush(stderr);

    int rlen = encoder->Encode(
        reinterpret_cast<unsigned char*>(data[0]),
        test_encoded,
        test_len,
        test_len,
        &encpos);
    fprintf(stderr, "1\n");
    fflush(stderr);

    EXPECT_EQ(test_len, encpos);
    EXPECT_EQ(test_len, rlen + 7);  // "match:123" encoded into 2 symbols

    fprintf(stderr, "2\n");
    fflush(stderr);
    int olen = decoder->Decode(
        test_encoded,
        reinterpret_cast<unsigned char*>(data[1]),
        rlen,
        &decpos);
    fprintf(stderr, "3\n");
    fflush(stderr);
    EXPECT_EQ(decpos, test_len);
    EXPECT_EQ(olen, rlen);
    EXPECT_STREQ(data[0], data[1]);

    delete decoder;
    delete encoder;
}
