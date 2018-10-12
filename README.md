(this project is no longer maintained, please move to http://github.com/richox/orz)

libzling
========

**fast and lightweight compression library and utility.**

Introduction
============

Libzling is an improved lightweight compression utility and library. libzling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it **3 times as fast as gzip on compressing, while still getting much better compression ratio and decompression speed**.

Simple benchmark with **enwik8**(100,000,000 bytes), also on [Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html#2702) (thanks to Matt Mahoney)

>     CPU: Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz
>     MEM: 65726592 kB
>     OS:  Linux 2.6.32.43-tlinux-1.0.14-default

Tool    | Encode | Decode | Compressed Size | status
--------|--------|--------|-----------------|--------
xz          | 1m23.838s | 0m2.265s | 26375764 | PASS
bzip2       | 0m10.680s | 0m4.633s | 29008758 | PASS
libzling e4 |  0m4.058s | 0m0.989s | 29721410 | PASS
libzling e3 |  0m3.383s | 0m1.007s | 29999672 | PASS
libzling e2 |  0m2.860s | 0m1.021s | 30477912 | PASS
libzling e1 |  0m2.504s | 0m1.021s | 30855731 | PASS
libzling e0 |  0m2.208s | 0m1.043s | 31456189 | PASS
gzip        |  0m6.502s | 0m1.089s | 36518322 | PASS

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
