# HDR Simple Framework

Simple image processing framework. Realtime HDR. HDR tonemapper.

## Examples

#### 1
| EV- | EV0 | EV+ |
| --- | --- | --- |
| ![EVmin](doc/1/0.jpg) | ![EVzero](doc/1/1.jpg) | ![EVmmax](doc/1/2.jpg) |

![EVmmax](doc/1/output.jpg)

#### 2

| EV- | EV0 | EV+ |
| --- | --- | --- |
| ![EVmin](doc/2/0.jpg) | ![EVzero](doc/2/1.jpg) | ![EVmmax](doc/2/2.jpg) |

![EVmmax](doc/2/output.jpg)

#### 3

| EV- | EV+ |
| --- | --- |
| ![EVmin](doc/5/0.jpg) |![EVzero](doc/5/1.jpg) |

![EVmmax](doc/5/output.jpg)

#### 4

| EV-  | EV+ |
| ---  | --- |
| ![EVmin](doc/9/0.jpg) | ![EVzero](doc/9/1.jpg) |

![EVmmax](doc/9/output.jpg)


## Build
Build with CMake.

Requirements:
 * OpenCV
 * LibRaw
 * C++ Boost
 * Google Test
 * evif2

## Usage


Application is executable only from command line.

Usage: ./HDRSimpleFramework [OPTION]... [FILE]...

Application HDR Simple Framework can be used in different ways.

As a HDR file creator (with option -c/ –createHDR). As an LDR file creator (with option -l / –createLDR). Or can work in real time – it will create animation (with option -r / –realTime).

Options are mentioned below:
 * -c, --createHDR from input files create one HDR file, if not set only Tone Mapping will be executed,
 * -l, --createLDR from input files create one LDR file, do the whole process, create HDR and then LDR, if not set only Tone Mapping will be executed,
 * -o, --output FILE save in filename, if exists will be overriden,
 * -r, --realTime try to work on physical camera at real-time, works like with –createLDR, output is
a movie,
 * -f, --realTimeFPSOutput F if in real time mode, declare output video FPS F - is a floating point number, 20 by default,
 * -i, --realTimeFPSInput F if in real time mode, declare input capture FPS F - is a floating point number, 0.5 by default,
 * -e, --realTimeExpPerHDR U if in real time mode, declare how many exposures will be done per one HDR Image, 3 by default, U - is a natural number.
 * -v, --verbose increase verbosity
 * --help display this help and exit,
 * --version output version information and exit.

