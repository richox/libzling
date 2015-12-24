libzling
========

**fast and lightweight compression library and utility.**

Introduction
============

Libzling is an improved lightweight compression utility and library. libzling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it **3 times as fast as gzip on compressing, while still getting much better compression ratio and decompression speed**.

Simple benchmark with **enwik8**(100,000,000 bytes), also on [Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html#2702) (thanks to Matt Mahoney)

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
xz      | 26377KB         | 82.94s | 2.05s  |
bzip2   | 29009KB         | 10.81s | 4.58s  |
e4      | 29790KB         |  4.19s | 0.91s  |
e3      | 30066KB         |  3.55s | 0.92s  |
e2      | 30541KB         |  2.98s | 0.92s  |
e1      | 30919KB         |  2.64s | 0.93s  |
e0      | 31521KB         |  2.35s | 0.94s  |
gzip    | 36518KB         |  6.52s | 1.03s  |

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
