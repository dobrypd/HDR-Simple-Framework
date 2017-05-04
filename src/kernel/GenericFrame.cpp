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
#include "GenericFrame.hpp"
#include <algorithm>
#include <string>
#ifdef __APPLE__
#include <libraw.h>
#else
#include <libraw/libraw.h>
#endif
#include <sstream>

namespace kernel
{

GenericFrame::GenericFrame(const GlobalArgs_t & globalArgs)
        : dirty(true), frame(), color(COLOR_UNDEFINED), ev(0), globalArgs(globalArgs)
{
    debug_print(LVL_LOW, "Creating empty frame %p.\n", (void * ) this);
}
GenericFrame::GenericFrame(const GlobalArgs_t & globalArgs, cv::Mat & frame, ColorSpace color)
        : GenericFrame(globalArgs)
{
    this->frame = frame;
    this->color = color;
    frameModified();
    debug_print(LVL_LOW, "Wrapping cv frame %p into %p.\n", (void * ) &(frame), (void * ) this);
}

/**
 * TODO: !!!OpenCV CIE L*a*b* is actually for D65 whitepoint, which is not exactly what I want.
 */
bool GenericFrame::convertToColorSpace(ColorSpace color)
{
    using namespace cv;

    if (!isValid()) return false;

    if (color == this->color) return true;

    int code = -1;
    switch (this->color)
    {
        case COLOR_BGR:
            switch (color)
            {
                case COLOR_RGB:
                    code = COLOR_BGR2RGB;
                break;
                case COLOR_CIELab:
                    code = COLOR_BGR2Lab;
                break;
                case COLOR_CIEXYZ:
                    code = COLOR_BGR2XYZ;
                break;
                default:
                    return false;
            }
        break;
        case COLOR_RGB:
            switch (color)
            {
                case COLOR_BGR:
                    code = COLOR_RGB2BGR;
                break;
                case COLOR_CIELab:
                    code = COLOR_RGB2Lab;
                break;
                case COLOR_CIEXYZ:
                    code = COLOR_RGB2XYZ;
                break;
                default:
                    return false;
            }
        break;
        case COLOR_CIELab:
            switch (color)
            {
                case COLOR_RGB:
                    code = COLOR_Lab2RGB;
                break;
                case COLOR_BGR:
                    code = COLOR_Lab2BGR;
                break;
                case COLOR_CIEXYZ:
                    cvtColor(frame, frame, COLOR_Lab2BGR);
                    code = COLOR_BGR2XYZ;
                break;
                default:
                    return false;
            }
        break;
        case COLOR_CIEXYZ:
            switch (color)
            {
                case COLOR_RGB:
                    code = COLOR_XYZ2RGB;
                break;
                case COLOR_BGR:
                    code = COLOR_XYZ2BGR;
                break;
                case COLOR_CIELab:
                    cvtColor(frame, frame, COLOR_XYZ2BGR);
                    code = COLOR_BGR2Lab;
                break;
                default:
                    return false;
            }
        break;
        default:
            return false;
    }

    debug_print(LVL_INFO, "color space change code %d\n", code);
    cvtColor(frame, frame, code);
    declareColorSpace(color);
    return true;
}

void GenericFrame::declareColorSpace(ColorSpace color)
{
    this->color = color;
}
GenericFrame::ColorSpace GenericFrame::getColorSpace()
{
    return color;
}

/**
 * Only between CV_32F, CV_8U and CV_16U for only 1..4 channels.
 */
#define GLUE(X,Y,Z) X##Y##Z
#define REMOVE_CHANNEL(VAR,DEP) if ((VAR) == GLUE(CV_,DEP,C1) || (VAR) == GLUE(CV_,DEP,C2) || (VAR) == GLUE(CV_,DEP,C3) || (VAR) == GLUE(CV_,DEP,C4)) VAR = GLUE(CV_,DEP,)
#define MAX_8U   255
#define MAX_16U  65535
#define MAX_32F  1.0
bool GenericFrame::convertToDepth(int newDepth)
{
    using namespace cv;
    if (!isValid()) return false;
    int newDepthWithChannels = newDepth;
    int oldDepth = frame.depth();
    REMOVE_CHANNEL(oldDepth, 32F);
    REMOVE_CHANNEL(oldDepth, 8U);
    REMOVE_CHANNEL(oldDepth, 16U);
    REMOVE_CHANNEL(newDepth, 32F);
    REMOVE_CHANNEL(newDepth, 8U);
    REMOVE_CHANNEL(newDepth, 16U);

    if (newDepth == oldDepth) return true;

    switch (oldDepth)
    {
        case CV_32F:
            switch (newDepth)
            {
                case CV_16U:
                    debug_puts("depth change, from CV_32F to CV_16U\n");
                    frame.convertTo(frame, newDepthWithChannels, MAX_16U);
                break;
                case CV_8U:
                    debug_puts("depth change, from CV_32F to CV_8U\n");
                    frame.convertTo(frame, newDepthWithChannels, MAX_8U);
                break;
                default:
                    return false;
            }
        break;
        case CV_16U:
            switch (newDepth)
            {
                case CV_32F:
                    debug_puts("depth change, from CV_16U to CV_32F\n");
                    frame.convertTo(frame, newDepthWithChannels, 1. / MAX_16U);
                break;
                case CV_8U:
                    debug_puts("depth change, from CV_16U to CV_8U\n");
                    frame.convertTo(frame, newDepthWithChannels, 1. / MAX_8U);
                break;
                default:
                    return false;
            }
        break;
        case CV_8U:
            switch (newDepth)
            {
                case CV_16U:
                    debug_puts("depth change, from CV_8U to CV_16U\n");
                    frame.convertTo(frame, newDepthWithChannels, MAX_8U);
                break;
                case CV_32F:
                    debug_puts("depth change, from CV_8U to CV_32F\n");
                    frame.convertTo(frame, newDepthWithChannels, 1. / MAX_8U);
                break;
                default:
                    return false;
            }
        break;
        default:
            return false;
    }
    return true;
}
#undef GLUE
#undef REMOVE_CHANNEL
#undef MAX_8U
#undef MAX_16U
#undef MAX_32F

ExposureValue GenericFrame::getEV()
{
    return ev;
}

void GenericFrame::setEV(ExposureValue ev)
{
    this->ev = ev;
}

void GenericFrame::frameModified()
{
    dirty = false;
}

cv::Mat & GenericFrame::getRawFrame()
{
    assert(!dirty);
    return frame;
}

bool GenericFrame::setEmptyFrameFrom(cv::Mat & frame, ColorSpace color)
{
    cv::Size s = frame.size();
    this->frame = cv::Mat(s.width, s.height, frame.type());
    this->color = color;
    frameModified();
    return isValid();
}

bool GenericFrame::copyFrameFrom(cv::Mat & frame, ColorSpace color)
{
    this->frame = frame.clone();
    this->color = color;
    frameModified();
    return isValid();
}

bool GenericFrame::assignFrameTo(cv::Mat & frame, ColorSpace color)
{
    this->frame = frame;
    this->color = color;
    frameModified();
    return isValid();
}

bool filenameExtAimsRaw(const std::string & filename)
{
    // TODO
    const std::string allExtensions =
            "crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|3fr|rw2|mef|mos|erf|nrw|srw";

    std::string ext = "#";
    if (filename.find_last_of(".") != std::string::npos)
    {
        ext = filename.substr(filename.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return allExtensions.find(ext) != std::string::npos;
    }
    return false;
}

#define CR(F) if ((F) != LIBRAW_SUCCESS) goto err
#define Pm processor.imgdata.params
void GenericFrame::setParams(LibRaw & processor)
{
    Pm.exp_correc = 0;
    Pm.no_auto_bright = 1;
    Pm.use_auto_wb = 0;
    Pm.use_camera_wb = 1;
    Pm.use_camera_matrix = 1;
    Pm.output_bps = 16;
    Pm.output_color = 1; // sRGB
}
#undef Pm

bool GenericFrame::readRaw(const std::string & filename)
{
    int W = 0, H = 0;
    libraw_processed_image_t *image = NULL;
    cv::Mat preprocess;
    ExposureValue ev(0);

    LibRaw processor;
    setParams(processor);
    CR(processor.open_file(filename.c_str()));
    CR(processor.unpack());
    CR(processor.dcraw_process());
    debug_print(LVL_INFO, "Opened RAW file (%d x %d). Model %s, Shutter: %f, aperature: %f.\n",
            processor.imgdata.sizes.width, processor.imgdata.sizes.height,
            processor.imgdata.idata.model, processor.imgdata.other.shutter,
            processor.imgdata.other.aperture);
    ev = ExposureValue(processor.imgdata.other.shutter, processor.imgdata.other.aperture,
            processor.imgdata.other.iso_speed);
    setEV(ev);

    image = processor.dcraw_make_mem_image();
    if (!image) goto err;

    W = image->width;
    H = image->height;
    assert(W == processor.imgdata.sizes.width);
    assert(image->data_size == W * H * 3 * sizeof(uint16_t));

    preprocess = cv::Mat(H, W, CV_16UC3, image->data);
    frame = cv::Mat(H, W, CV_32FC3);
    debug_puts("START: Changind RAW colorspace to CIE L*a*b*\n");
    preprocess.convertTo(preprocess, CV_32FC3, 1. / 65535); // ((1 << 16) - 1)
    cvtColor(preprocess, frame, CV_RGB2Lab);
    debug_puts("DONE: Changind RAW colorspace to CIE L*a*b*\n");
    color = COLOR_CIELab;

    LibRaw::dcraw_clear_mem(image);
    processor.recycle();
    return true;
    err: processor.recycle();
    return false;
}
#undef CR

bool GenericFrame::getFrameFromFile(const std::string & filename)
{
    if (filenameExtAimsRaw(filename))
    {
        readRaw(filename);
    }
    else
    {
        frame = cv::imread(filename,
                CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_COLOR | CV_LOAD_IMAGE_UNCHANGED);
        color = COLOR_BGR;
        ev.setFromExif(filename);
    }
    std::stringstream ss;
    ss << ev;
    verbose_print(globalArgs.verbosity, "File %s (%d, %d; %s) loaded.", filename.c_str(),
            frame.size().width, frame.size().height, ss.str().c_str());
    frameModified();
    return isValid();
}

bool GenericFrame::getFrameFromDevice(cv::VideoCapture & cap)
{
    cap >> frame;
    color = COLOR_BGR;
    frameModified();
    return !frame.empty();
}

bool GenericFrame::saveFrameToFile(const std::string & filename)
{
    assert(!dirty);
    if (!convertToColorSpace(COLOR_BGR)) return false;
    cv::imwrite(filename, frame);
    return true;
}

bool GenericFrame::isValid() const
{
    cv::Size s = frame.size();
    return (!dirty) && (color != COLOR_UNDEFINED) && (!frame.empty()) && (frame.cols > 0)
            && (frame.rows > 0) && (s.width > 0) && (s.height > 0);
}

} /* namespace kernel */
