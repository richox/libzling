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

#if HAS_CXX11_SUPPORT
#include <cstdint>
#include <cinttypes>
#else
#include <stdint.h>
#include <inttypes.h>
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <fcntl.h>  // setmode()
#include <io.h>
#endif

#include "libzling.h"
#include "libzling_utils.h"

struct DemoActionHandler: baidu::zling::IActionHandler {
    DemoActionHandler() {
        m_clockstart = clock();
    }
    void OnInit() {
        m_inputer  = dynamic_cast<baidu::zling::FileInputer*>(GetInputer());
        m_outputer = dynamic_cast<baidu::zling::FileOutputer*>(GetOutputer());
    }

    void OnDone() {
        uint64_t isize = m_inputer->GetInputSize();
        uint64_t osize = m_outputer->GetOutputSize();
        double cost_seconds = double(clock() - m_clockstart) / CLOCKS_PER_SEC;

        if (IsEncode()) {
            fprintf(stderr, "encode: %"PRIu64" => %"PRIu64", time=%.3f sec, speed=%.3f MB/sec\n",
                    isize,
                    osize,
                    cost_seconds,
                    isize / cost_seconds / 1e6);
        } else {
            fprintf(stderr, "encode: %"PRIu64" <= %"PRIu64", time=%.3f sec, speed=%.3f MB/sec\n",
                    osize,
                    isize,
                    cost_seconds,
                    isize / cost_seconds / 1e6);
        }
        fflush(stderr);
    }

    void OnProcess() {
        uint64_t isize = m_inputer->GetInputSize();
        uint64_t osize = m_outputer->GetOutputSize();
        double cost_seconds = double(clock() - m_clockstart) / CLOCKS_PER_SEC;

        if (IsEncode()) {
            fprintf(stderr, "%6.2f MB => %6.2f MB %.2f%%, %.3f sec, speed=%.3f MB/sec\n",
                    isize / 1e6,
                    osize / 1e6,
                    osize * 1e2 / isize,
                    cost_seconds,
                    isize / cost_seconds / 1e6);
        } else {
            fprintf(stderr, "%6.2f MB <= %6.2f MB %.2f%%, %.3f sec, speed=%.3f MB/sec\n",
                    osize / 1e6,
                    isize / 1e6,
                    isize * 1e2 / osize,
                    cost_seconds,
                    osize / cost_seconds / 1e6);
        }
        fflush(stderr);
    }

private:
    baidu::zling::FileInputer*  m_inputer;
    baidu::zling::FileOutputer* m_outputer;
    clock_t m_clockstart;
};

int main(int argc, char** argv) {
    baidu::zling::FileInputer  inputer(stdin);
    baidu::zling::FileOutputer outputer(stdout);
    DemoActionHandler demo_handler;

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
        return baidu::zling::Encode(&inputer, &outputer, &demo_handler);
    }
    if (argc == 2 && strcmp(argv[1], "d") == 0) {
        return baidu::zling::Decode(&inputer, &outputer, &demo_handler);
    }

    // help message
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "   zling e source target\n");
    fprintf(stderr, "   zling d source target\n");
    fprintf(stderr, "    * source: default to stdin\n");
    fprintf(stderr, "    * target: default to stdout\n");
    return -1;
}
