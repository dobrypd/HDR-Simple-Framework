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
#ifndef GenericFrame_HPP_
#define GenericFrame_HPP_

#include "config.h"
#include "ExposureValue.hpp"

#include <opencv2/opencv.hpp>
#include <boost/shared_ptr.hpp>
#ifdef __APPLE__
#include <libraw.h>
#else
#include <libraw/libraw.h>
#endif

namespace kernel
{

/*
 * This is a cv::Mat wrapper.
 */
class GenericFrame
{
public:
    enum ColorSpace {
        COLOR_BGR, COLOR_RGB, COLOR_CIEXYZ, COLOR_CIELab, COLOR_UNDEFINED
    };
private:
    bool dirty;
    cv::Mat frame;
    ColorSpace color;
    ExposureValue ev;

    const GlobalArgs_t & globalArgs;

    inline void frameModified();
    bool readRaw(const std::string & filename);
    void setParams(LibRaw & processor);

public:
    GenericFrame(const GlobalArgs_t & globalArgs);
	GenericFrame(const GlobalArgs_t & globalArgs, cv::Mat & frame, ColorSpace color);

	// 2 color space setters
	bool convertToColorSpace(ColorSpace color);
	void declareColorSpace(ColorSpace color);
	// and 1 getter
	ColorSpace getColorSpace();

	bool convertToDepth(int depth);


	ExposureValue getEV();
	void setEV(ExposureValue info);

    /**
     * Get cv::Mat frame.
     */
    cv::Mat & getRawFrame();

    /**
     * Frame creator.
     */
    bool setEmptyFrameFrom(cv::Mat & frame, ColorSpace color);

    /**
     * Frame creator.
     */
    bool copyFrameFrom(cv::Mat & frame, ColorSpace color);

    /**
     * Won't be coppied.
     */
    bool assignFrameTo(cv::Mat & frame, ColorSpace color);

	/**
	 * Read file
	 */
	bool getFrameFromFile(const std::string & filename);

	/**
	 * Get frame from device
	 */
	bool getFrameFromDevice(cv::VideoCapture & cap);

    /**
	 * Save frame.
	 */
    bool saveFrameToFile(const std::string & filename);

    /**
     * Validator.
     */
    bool isValid() const;
};

typedef boost::shared_ptr<kernel::GenericFrame> GenericFramePtr;

} /* namespace kernel */

#endif /* GenericFrame_HPP_ */
