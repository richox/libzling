zling
=====

</b>fast and lightweight compression utility and library.</b>

Introduction
============

Zling is an improved lightweight compression utility and library. zling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it <b>twice as fast as gzip on compressing, while still getting better compression ratio and decompression speed</b>.

Simple benchmark with <b>enwik8</b>(100,000,000 bytes)

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>32196557</td>        <td>3.18s</td>  <td>1.02s</td></tr>
 <tr><td>gzip</td>  <td>36518322</td>        <td>8.13s</td>  <td>1.47s</td></tr>
</table>

Simple benchmark with <b>fp.log</b>(20,617,071 bytes)

<table border="1">
 <tr><td>Tool</td>  <td>Compressed Size</td> <td>Encode</td> <td>Decode</td></tr>
 <tr><td>zling</td> <td>897456</td>          <td>0.13s</td> <td>0.07s</td></tr>
 <tr><td>gzip</td>  <td>1448582</td>         <td>0.41s</td> <td>0.14s</td></tr>
</table>

Build & Install
===============

You can build zling automatically by cmake with the following command:

    cmake . -DCMAKE_INSTALL_PREFIX=/path/to/install
    make
    make install

Usage
=====

Zling provides simple and lightweight interface. here is a simple program showing how to work with libzling.

    #include "libzling.h"

    int main() {
        // compress
        {
            FILE* fin = fopen("./1.txt", "rb");
            FILE* fout = fopen("./1.txt.zlng", "wb");

            baidu::zling::FileInputer  inputer(fin);
            baidu::zling::FileOutputer outputer(fout);

            baidu::zling::Encode(&inputer, &outputer);
            fclose(fin);
            fclose(fout);
        }

        // decompress
        {
            FILE* fin = fopen("./1.txt.zlng", "rb");
            FILE* fout = fopen("./2.txt", "wb");

            baidu::zling::FileInputer  inputer(fin);
            baidu::zling::FileOutputer outputer(fout);
            baidu::zling::Decode(&inputer, &outputer);
        }
        return 0;
    }

However zling supports more complicated interface, see <b>./sample/zling.cpp</b> for details.
