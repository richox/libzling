#!/usr/bin/env python

# table auto-generator for zling.
# author: Zhang Li <zhangli10@baidu.com>

kBucketItemSize = 4096

matchidx_blen = [0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7] + [8] * 1024
matchidx_code = []
matchidx_bits = []
matchidx_base = []

#for i in range(0, kBucketItemSize):
while matchidx_code.__len__() < kBucketItemSize:
    matchidx_base.append(matchidx_code.__len__())

    for bits in range(0, 1 << matchidx_blen[matchidx_base.__len__() - 1]):
        matchidx_bits.append(bits)
        matchidx_code.append(matchidx_base.__len__() - 1)

f_blen = open("ztable_matchidx_blen.inc", "w")
f_base = open("ztable_matchidx_base.inc", "w")
f_code = open("ztable_matchidx_code.inc", "w")
f_bits = open("ztable_matchidx_bits.inc", "w")

for i in range(0, matchidx_base.__len__()):
    f_blen.write("%4u," % matchidx_blen[i] + "\n\x20" [int(i % 16 != 15)])
    f_base.write("%4u," % matchidx_base[i] + "\n\x20" [int(i % 16 != 15)])

for i in range(0, matchidx_code.__len__()):
    f_code.write("%4u," % matchidx_code[i] + "\n\x20" [int(i % 16 != 15)])
    f_bits.write("%4u," % matchidx_bits[i] + "\n\x20" [int(i % 16 != 15)])
