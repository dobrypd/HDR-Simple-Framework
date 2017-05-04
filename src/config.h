/*
 *  MIT License
 *
 *  Copyright (c) 2017 Piotr Dobrowolski
 *
 *  permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */
/*
 * Author: Piotr Dobrowolski
 * dobrypd[at]gmail[dot]com
 *
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <sys/time.h>
#include <cstring>
#include <cstdio>

#ifndef NDEBUG
#ifndef DBGLVL
#define DBGLVL 1000
#endif
#else
#define DBGLVL 0
#endif

#define LVL_URGENT 0
#define LVL_ERROR 10
#define LVL_WARNING 20
#define LVL_INFO 30
#define LVL_DEBUG 35
#define LVL_LOW 40
#define LVL_LOWER 50
#define LVL_LOWEST 60
#define LVL_ALL 1000

#define NAME_OF_THE_CURRENT_SOURCE_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
 * This macro (code where this macro is used)
 * should be removed by gcc optimizations if not in debug mode.
 */
#define CREATE_TIME() \
    struct timeval tp;\
    gettimeofday(&tp, 0);\
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000
#define debug_print(lvl, fmt, ...) \
    do { if (DBGLVL >= lvl) {\
        CREATE_TIME();\
        fprintf(stderr, "[%ld][%d]%s:%d:%s():\t" fmt, ms, lvl, (NAME_OF_THE_CURRENT_SOURCE_FILE),\
                __LINE__, __func__, __VA_ARGS__);\
    } } while (0)
#define debug_puts(fmt) \
    do { if (DBGLVL >= LVL_INFO) {\
        CREATE_TIME();\
        fprintf(stderr, "[%ld][%d]%s:%d:%s():\t" fmt, ms, LVL_INFO, (NAME_OF_THE_CURRENT_SOURCE_FILE),\
                __LINE__, __func__);\
    } } while (0)
#define verbose_print(verbosity, fmt, ...)\
    do { if (verbosity > 0){\
        CREATE_TIME();\
        fprintf(stdout, "[%ld]" fmt "\n", ms, __VA_ARGS__);\
    } } while (0)

struct GlobalArgs_t
{
    const char * * inputFiles;
    const char * outputFile;
    bool createHDR;
    bool createLDR;
    bool realTime;
    float inputFPS; // if realTime
    float outputFPS; // if realTime
    unsigned int expPerHDR; // if realTime
    int verbosity;
    int inputs;
};

#endif /* CONFIG_H_ */

