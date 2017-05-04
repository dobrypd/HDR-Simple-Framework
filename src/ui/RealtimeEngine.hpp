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
#ifndef REALTIMEENGINE_HPP_
#define REALTIMEENGINE_HPP_

#include "config.h"
#include "ProcessingEngine.hpp"
#include "kernel/GenericFrame.hpp"
#include "kernel/ExposureValue.hpp"
#include "kernel/HdrCreation/HDRCreator.hpp"
#include "kernel/TonemappingOperators/dobrowolski15/Dobrowolski15.hpp"

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

namespace ui
{

/*
 *
 */
class RealtimeEngine
{
private:
    const char* windowOriginalLabel = "Original image";

    cv::VideoCapture videoCapture;
    cv::VideoWriter videoWriter;
    bool createOutput;

    HDRCreation::HDRCreator hdrCreator;
    TMO::Dobrowolski15 tmo;

    unsigned int exposuresPerHDR;
    boost::interprocess::interprocess_semaphore semSwitch;
    boost::interprocess::interprocess_semaphore semCapture;

    typedef boost::shared_ptr<std::vector<kernel::GenericFramePtr> > vectorOfFramePtrsPtr_t;
    vectorOfFramePtrsPtr_t buffer;
    vectorOfFramePtrsPtr_t work;

    bool quit;

    float fps;
    unsigned int delayMs;

    std::string deviceName;
    unsigned int exposureCompensactionRange;
    bool initializedOnlyGenericDevice;

    const GlobalArgs_t & globalArgs;

    bool initializeDevice();
    kernel::ExposureValue changeDeviceSettings(int expNo, bool & success);
    bool openVideoWriter(kernel::GenericFramePtr & frame);

    void videoCapturer();
    void creator();

public:
    explicit RealtimeEngine(const GlobalArgs_t & globalArgs, int exposuresPerHDR = 1);

    void run();
};

} /* namespace ui */

#endif /* REALTIMEENGINE_HPP_ */
