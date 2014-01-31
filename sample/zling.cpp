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
 * @brief  zling main.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <fcntl.h>  // setmode()
#include <io.h>
#endif

#include "libzling.h"
#include "libzling_utils.h"

struct EncodeDemoHandler: baidu::zling::IActionHandler {
    EncodeDemoHandler() {
    }
    void OnProcess(baidu::zling::IInputer* inputer_, baidu::zling::IOutputer* outputer_) {
        baidu::zling::FileInputer* inputer   = dynamic_cast<baidu::zling::FileInputer*>(inputer_);
        baidu::zling::FileOutputer* outputer = dynamic_cast<baidu::zling::FileOutputer*>(outputer_);

        fprintf(
            stderr,
            "%8.2f MB => %8.2f MB\n",
            inputer->GetInputSize()   / 1e6,
            outputer->GetOutputSize() / 1e6);
    }
};
int main(int argc, char** argv) {
    baidu::zling::FileInputer inputer(stdin);
    baidu::zling::FileOutputer outputer(stdout);
    EncodeDemoHandler encode_handler;

#if defined(__MINGW32__) || defined(__MINGW64__)
    setmode(fileno(stdin),  O_BINARY);  // set stdio to binary mode for windows
    setmode(fileno(stdout), O_BINARY);
#endif

    // welcome message
    fprintf(stderr, "zling:\n");
    fprintf(stderr, "   light-weight lossless data compression utility\n");
    fprintf(stderr, "   by Zhang Li <zhangli10 at baidu.com>\n");
    fprintf(stderr, "\n");

    // zling <e/d> __argv2__ __argv3__
    if (argc == 4) {
        if (freopen(argv[3], "wb", stdout) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for write.\n", argv[3]);
            return -1;
        }
        argc = 3;
    }

    // zling <e/d> __argv2__ (stdout)
    if (argc == 3) {
        if (freopen(argv[2], "rb", stdin) == NULL) {
            fprintf(stderr, "error: cannot open file '%s' for read.\n", argv[2]);
            return -1;
        }
        argc = 2;
    }

    // zling <e/d> (stdin) (stdout)
    if (argc == 2 && strcmp(argv[1], "e") == 0) {
        return baidu::zling::Encode(&inputer, &outputer, &encode_handler);
    }
    if (argc == 2 && strcmp(argv[1], "d") == 0) {
        return baidu::zling::Decode(&inputer, &outputer);
    }

    // help message
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "   zling e source target\n");
    fprintf(stderr, "   zling d source target\n");
    fprintf(stderr, "    * source: default to stdin\n");
    fprintf(stderr, "    * target: default to stdout\n");
    return -1;
}
