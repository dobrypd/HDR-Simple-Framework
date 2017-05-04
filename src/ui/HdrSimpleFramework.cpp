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

#include "HdrSimpleFramework.hpp"

#include "config.h"
#include "ProcessingEngine.hpp"
#include "RealtimeEngine.hpp"

#include <cstdlib>
#include <getopt.h>
#include <string>
#include <cstdio>
#include <cassert>
#include <climits>
#include <vector>

static const char * version_no = "1.0";
static const char * default_output_filename = "out";
#define  version_str "HDR Simple Framework %s\n\
Piotr Dobrowolski (c) 2014-2015\n"

namespace ui
{

HdrSimpleFramework::HdrSimpleFramework()
{
}

} /* namespace ui */

static GlobalArgs_t globalArgs;

enum
{
    HELP_OPTION = CHAR_MAX + 1, VERSION_OPTION
};

static const struct option long_options[] =
{
{ "output", no_argument, NULL, 'o' },
{ "createHDR", no_argument, NULL, 'c' },
{ "createLDR", no_argument, NULL, 'l' },
{ "realTime", no_argument, NULL, 'r' },
{ "realTimeFPSInput", no_argument, NULL, 'i' },
{ "realTimeFPSOutput", no_argument, NULL, 'f' },
{ "realTimeExpPerHDR", no_argument, NULL, 'e' },
{ "verbose", no_argument, NULL, 'v' },
{ "help", no_argument, NULL, HELP_OPTION },
{ "version", no_argument, NULL, VERSION_OPTION },
{ NULL, no_argument, NULL, 0 } };

static char * program_name;

static void initializeGlobalArgs();
static void parseArgs(int argc, char * argv[]);
static void usage(int status);
static void version();

static void initializeGlobalArgs()
{
    globalArgs.createHDR = false; // Tone Map by default
    globalArgs.createLDR = false; // Tone Map by default
    globalArgs.realTime = false;
    globalArgs.outputFPS = 20; // if realTime
    globalArgs.inputFPS = 0.5; // if realTime
    globalArgs.expPerHDR = 3; // if realTime
    globalArgs.inputFiles = 0;
    globalArgs.outputFile = default_output_filename;
    globalArgs.verbosity = 0;
    globalArgs.inputs = 0;

}

static void parseArgs(int argc, char * argv[])
{
    debug_print(LVL_DEBUG, "Parsing %d arguments.\n", argc);
    int opt, oi = -1;
    while (1)
    {
        opt = getopt_long(argc, argv, "o:clrf:i:e:vh", long_options, &oi);
        if (opt == -1) break;

        switch (opt)
        {
            case 'c':
                globalArgs.createHDR = true;
                debug_puts("Will create HDR Image.\n");
            break;
            case 'l':
                globalArgs.createLDR = true;
                debug_puts("Will create LDR Image.\n");
            break;
            case 'o':
                globalArgs.outputFile = optarg;
                debug_print(LVL_INFO, "Setting output filename to %s.\n", optarg);
            break;
            case 'r':
                globalArgs.realTime = true;
                debug_puts("Will try to work realtime.\n");
            break;
            case 'f':
                sscanf(optarg, "%f", &globalArgs.outputFPS);
                debug_print(LVL_INFO, "Setting output fps to %s.\n", optarg);
            break;
            case 'i':
                sscanf(optarg, "%f", &globalArgs.inputFPS);
                debug_print(LVL_INFO, "Setting input fps to %s.\n", optarg);
            break;
            case 'e':
                sscanf(optarg, "%u", &globalArgs.expPerHDR);
                debug_print(LVL_INFO, "Setting real time number of expositions per HDR to %s.\n", optarg);
            break;
            case 'v':
                fputs("Verbosity set on.\n", stdout);
                globalArgs.verbosity = 1;
            break;
            case VERSION_OPTION:
                version();
                exit(EXIT_SUCCESS);
            break;
            case HELP_OPTION:
                usage(EXIT_SUCCESS);
            break;
            default:
                debug_print(LVL_INFO, "Option not found %c.\n", opt);
                usage(EXIT_FAILURE);
                /* no break */
        }
    }

    // Input files:
    debug_print(LVL_DEBUG, "Got %d files.\n", argc - optind);
    globalArgs.inputs = argc - optind;
    globalArgs.inputFiles = (const char * *) argv + optind;
}

int main(int argc, char * argv[])
{
    debug_print(LVL_LOWEST, "Starting with %d args.\n", argc);
    program_name = argv[0];
    initializeGlobalArgs();
    parseArgs(argc, argv);

    if (globalArgs.inputs == 0 && !globalArgs.realTime)
    {
        fputs("No files declared, exiting.\n", stdout);
        return EXIT_FAILURE;
    }

    verbose_print(globalArgs.verbosity, version_str, version_no);

    if (globalArgs.realTime)
    {
        ui::RealtimeEngine realtimeEngine(globalArgs, globalArgs.expPerHDR);
        realtimeEngine.run();
    }
    else
    {
        ui::ProcessingEngine * processingEngine;
        if (globalArgs.createLDR)
        {
            processingEngine = new ui::ProcessingHDRCreatorAndToneMapper(globalArgs);
        }
        else if (globalArgs.createHDR)
        {
            processingEngine = new ui::ProcessingHDRCreatorModel(globalArgs);
        }
        else
        {
            processingEngine = new ui::ProcessingToneMapper(globalArgs);
        }

        processingEngine->process();

        if (processingEngine != 0)
        {
            delete processingEngine;
        }
    }

    return EXIT_SUCCESS;
}

static void usage(int status)
{
    printf("Usage: %s [OPTION]... [FILE]...\n", program_name);
    fputs(
            "\
HDR Simple Framework\n\
Works in two different ways.\n\n\
HDR/LDR creator:\n\
  * as HDR image creator, with --createHDR, then FILES should contain\n\
      this same exposition with different exposures (output like OpenEXR),\n\
  * as Tone Mapper, when FILE is an HDR before tone mapping,\n\
      (input like OpenEXR),\n\
  * whole process can be executed with --createLDR.\n\n\
Real time HDR time lapse capturer will be executed with --realTime option.\n\
\n\n\
Mandatory arguments to long options are mandatory for short options too.\n\n\
  -c, --createHDR            from input files create one HDR file,\n\
                               if not set only Tone Mapping will be executed,\n\n\
  -l, --createLDR            from input files create one LDR file,\n\
                               do the whole process, create HDR and then LDR,\n\
                               if not set only Tone Mapping will be executed,\n\n\
  -o, --output FILE          save in filename, if exists will be overriden,\n\n\
  -r, --realTime             try to work on physical camera at real-time,\n\
                               works like with --createLDR, output is a movie,\n\n\
  -f, --realTimeFPSOutput F  if in real time mode, declare output video FPS\n\
                               F - is a floating point number, 20 by default,\n\n\
  -i, --realTimeFPSInput  F  if in real time mode, declare input capture FPS\n\
                               F - is a floating point number, 0.5 by default,\n\n\
  -e, --realTimeExpPerHDR U  if in real time mode, declare how many exposures\n\
                               will be done per one HDR Image, 3 by default,\n\
                               U - is a natural number.\n\n\
  -v, --verbose              increase verbosity\n\n\
      --help                 display this help and exit,\n\n\
      --version              output version information and exit.\n\
",
            stdout);
    exit(status);
}

static void version()
{
    fprintf(stdout, version_str, version_no);
}
