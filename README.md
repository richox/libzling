libzling
========

**fast and lightweight compression library and utility.**

Introduction
============

Libzling is an improved lightweight compression utility and library. libzling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it **3 times as fast as gzip on compressing, while still getting much better compression ratio and decompression speed**.

Simple benchmark with **enwik8**(100,000,000 bytes), also on [Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html#2702) (thanks to Matt Mahoney)

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
zling e0| 32378KB         | 2.42s  | 1.03s  |
zling e1| 31720KB         | 2.73s  | 1.02s  |
zling e2| 31341KB         | 3.14s  | 1.00s  |
zling e3| 30980KB         | 3.73s  | 0.99s  |
zling e4| 30707KB         | 4.36s  | 0.98s  |
gzip    | 36520KB         | 8.13s  | 1.47s  |

Simple benchmark with **fp.log**(20,617,071 bytes)

Tool  | Compressed Size | Encode | Decode |
------|-----------------|--------|--------|
zling e0| 973KB           | 0.10s  | 0.07s  |
zling e1| 914KB           | 0.11s  | 0.07s  |
zling e2| 903KB           | 0.12s  | 0.07s  |
zling e3| 914KB           | 0.15s  | 0.07s  |
zling e4| 901KB           | 0.19s  | 0.07s  |
gzip    | 1449KB          | 0.41s  | 0.14s  |

Build & Install
===============

You can build and install libzling automatically by **cmake** with the following command:

    cd ./build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install
    make
    make install

Usage
=====

Libling provides simple and lightweight interface. here is a simple program showing the basic usage of libzling. (compiled with `g++ -Wall -O3 zling_sample.cpp -o zling_sample -lzling`)

```C++
#include "libzling/libzling.h"

int main() {
    // compress
    {
        const int level = 0;  // valid levels: 0, 1, 2, 3, 4
        FILE* fin = fopen("./1.txt", "rb");
        FILE* fout = fopen("./1.txt.zlng", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Encode(&inputter, &outputter, level);
        fclose(fin);
        fclose(fout);
    }

    // decompress
    {
        FILE* fin = fopen("./1.txt.zlng", "rb");
        FILE* fout = fopen("./2.txt", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Decode(&inputter, &outputter);
        fclose(fin);
        fclose(fout);
    }
    return 0;
}
```
However libzling supports more complicated interface, see **./demo/zling.cpp** for details.
