#!/usr/bin/env python

matchidx_bitlen = [
    0, 0, 0, 0,
    1, 1,
    2, 2,
    3, 3,
    4, 4,
    5, 5,
    6, 6,
    7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8]

kBucketItemSize      = 4096
kMatchidxMaxBitlen   = 8
kMatchidxCodeSymbols = len(matchidx_bitlen)

matchidx_code = [0] * kBucketItemSize
matchidx_bits = [0] * kBucketItemSize
matchidx_base = [0] * kMatchidxCodeSymbols

code = 0
bits = 0

for i in range(0, kBucketItemSize):
    matchidx_code[i] = code
    matchidx_bits[i] = bits

    if i + 1 < kBucketItemSize:
        bits += 1
        if bits >> matchidx_bitlen[code] != 0:
            bits  = 0
            code += 1
            matchidx_base[code] = i + 1

with open("ztable_matchidx_blen.inc", "w") as f:
    for i in range(0, kMatchidxCodeSymbols):
        f.write("%1u," % matchidx_bitlen[i] + "\n\x20" [int(i % 16 != 15)])

with open("ztable_matchidx_code.inc", "w") as f:
    for i in range(0, kBucketItemSize):
        f.write("%2u," % matchidx_code[i] + "\n\x20" [int(i % 16 != 15)])

with open("ztable_matchidx_bits.inc", "w") as f:
    for i in range(0, kBucketItemSize):
        f.write("%3u," % matchidx_bits[i] + "\n\x20" [int(i % 16 != 15)])

with open("ztable_matchidx_base.inc", "w") as f:
    for i in range(0, kMatchidxCodeSymbols):
        f.write("%4u," % matchidx_base[i] + "\n\x20" [int(i % 16 != 15)])
