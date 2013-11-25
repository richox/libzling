// Copyright (c) 2013, Zhang Li.
// author zhangli10<zhangli10@baidu.com>
// brief  unittest for zling_codebuf
#include <gtest/gtest.h>
#include "src/zling_codebuf.h"

TEST(ZlingCodebuf, Main) {
    baidu_zhangli10::zling::codebuf::ZlingCodebuf codebuf;
    EXPECT_EQ(codebuf.GetLength(), 0);

    codebuf.Input(0x101, 12);
    codebuf.Input(0x101, 12);
    EXPECT_EQ(codebuf.GetLength(), 24);

    EXPECT_EQ(codebuf.Peek(16), 0x1101u);
    EXPECT_EQ(codebuf.Peek(24), 0x101101u);

    EXPECT_EQ(codebuf.Output(16), 0x1101u);
    EXPECT_EQ(codebuf.Output(8),  0x10u);
}
