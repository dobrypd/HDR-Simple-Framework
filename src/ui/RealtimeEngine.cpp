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
#include "RealtimeEngine.hpp"

#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "kernel/ExposureValue.hpp"

/**
 * It's not perfect...
 * TODOs:
 * *libusb does not like threads
 * *memory corruption on device?
 * *save only HDR image, not video
 * *Do i want to show output?
 */

namespace ui
{

RealtimeEngine::RealtimeEngine(const GlobalArgs_t & globalArgs, int exposuresPerHDR)
        : createOutput(globalArgs.outputFile != NULL), hdrCreator(globalArgs), tmo(globalArgs), exposuresPerHDR(
                exposuresPerHDR), semSwitch(0), semCapture(0), fps(globalArgs.inputFPS), exposureCompensactionRange(
                1), initializedOnlyGenericDevice(true), globalArgs(globalArgs)
{
    using kernel::GenericFramePtr;
    using kernel::GenericFrame;
    using boost::shared_ptr;

    if (globalArgs.inputs > 0)
    {
        videoCapture = cv::VideoCapture(globalArgs.inputFiles[0]);
    }
    else
    {
        videoCapture = cv::VideoCapture(1);
    }

    work = vectorOfFramePtrsPtr_t(new std::vector<kernel::GenericFramePtr>());
    buffer = vectorOfFramePtrsPtr_t(new std::vector<kernel::GenericFramePtr>());

    for (int i = 0; i < exposuresPerHDR; ++i)
    {
        work->push_back(GenericFramePtr(new GenericFrame(globalArgs)));
        buffer->push_back(GenericFramePtr(new GenericFrame(globalArgs)));
    }

    quit = false;

    if (fps > 0)
    {
        delayMs = 1e3 / fps;
    }
    else
    {
        delayMs = 0;
    }

}

#define CR(TOKEN) do {\
    if ((TOKEN) == NULL) {\
        std::cerr << "Cannot parse settings. Switching to generic mode." << std::endl;\
        initializedOnlyGenericDevice = true;\
        break;\
    }\
} while(0)

bool RealtimeEngine::initializeDevice()
{
    using namespace cv;
    exposureCompensactionRange = 0;
    if (videoCapture.isOpened())
    {
        initializedOnlyGenericDevice = videoCapture.get(CAP_PROP_GPHOTO2_WIDGET_ENUMERATE) == -1;
        if (!initializedOnlyGenericDevice)
        {
            const char * deviceAdvancedSettings = ((const char*) (intptr_t) videoCapture.get(
                    CAP_PROP_GPHOTO2_WIDGET_ENUMERATE));
            videoCapture.set(CAP_PROP_FOCUS, (double) true);
            std::istringstream settings;
            char setting[4096] = "";
            const char * expositionSettingName = "exposurecompensation";
            const char * knownType = "radio";
            settings.str(deviceAdvancedSettings);
            while (settings.getline(setting, 4096))
            {
                char * saveptrDelim1, *saveptrDelim2;
                strtok_r(setting, ",", &saveptrDelim1); // Text description
                char * token = strtok_r(NULL, ",", &saveptrDelim1); // Label
                token = strtok_r(NULL, ",", &saveptrDelim1); // Name
                if (strcmp(expositionSettingName, token) == 0)
                {
                    CR(token = strtok_r(NULL, ",", &saveptrDelim1)); // readonly
                    CR(token = strtok_r(NULL, ",", &saveptrDelim1)); // type
                    if (strcmp(knownType, token) != 0)
                    {
                        std::cerr << "I dont know how do deal with " << token
                                << " type of setting. Switching to generic mode." << std::endl;
                        initializedOnlyGenericDevice = true;
                        break;
                    }
                    CR(token = strtok_r(NULL, ",", &saveptrDelim1)); // value
                    char * values = strtok_r(token, ";", &saveptrDelim2); // Count values
                    while (values && strtok_r(NULL, ";", &saveptrDelim2))
                    {
                        exposureCompensactionRange++;
                    }
                }
            }
        }

        if (initializedOnlyGenericDevice)
        {
            exposureCompensactionRange = 1;
        }
    }
    else
    {
        return false;
    }
    return true;
}
#undef CR

kernel::ExposureValue RealtimeEngine::changeDeviceSettings(int expNo, bool & success)
{
    using namespace cv;
    float exposure = ((float) (expNo + 1) / ((float) (exposuresPerHDR + 1)));
    if (!initializedOnlyGenericDevice)
    {
        exposure *= exposureCompensactionRange;
    }
    int exposureRadioSet = static_cast<int>(exposure);

    success = videoCapture.set(CAP_PROP_EXPOSURE, (double) exposureRadioSet);

    // EV=1 step assumption
    return kernel::ExposureValue((exposureCompensactionRange/2) - exposureRadioSet);
}

bool RealtimeEngine::openVideoWriter(kernel::GenericFramePtr & frame)
{
    using namespace cv;
    Size S = frame->getRawFrame().size();
    videoWriter.open(globalArgs.outputFile, CV_FOURCC('M', 'J', 'P', 'G'), globalArgs.outputFPS, S,
            true);

    return videoWriter.isOpened();
}

void RealtimeEngine::videoCapturer()
{
    debug_print(LVL_INFO, "Capturing thread started for class ptr %p.\n", (void * )this);

    boost::system_time last_time = boost::get_system_time();
    boost::system_time current_time = boost::get_system_time();

    while (!quit)
    {
        debug_puts("Video capturer up to get lock 0.\n");
        semCapture.wait();
        debug_puts("Video capturer has lock 0\n");
        if (quit)
        {
            semSwitch.post();
            return;
        }
        // capture
        int expNo = 0;
        for (auto it = buffer->begin(); it != buffer->end(); it++, expNo++)
        {
            kernel::GenericFramePtr frame = *it;
            // Capture.
            debug_puts("Capturer is about to change settings.\n");
            bool changed = false;
            kernel::ExposureValue ev = changeDeviceSettings(expNo, changed);
            if (!changed)
                verbose_print(globalArgs.verbosity,
                        "Cannot change camera setting for exposition %d.", expNo);
            debug_puts("Capturer changed settings.\n");
            debug_puts("Capturer is about to capture.\n");
            time_t start_time = time(0);
            // With Timeout
            frame->getFrameFromDevice(videoCapture);
            frame->setEV(ev);
            debug_print(LVL_INFO, "Captured in delta = %ld.\n", time(0) - start_time);
            if (!(frame->isValid()))
            {
                verbose_print(globalArgs.verbosity,
                        "Exposure %d is not valid, check if device is connected.", expNo);
                quit = true;
                semSwitch.post();
                return;
            }
        }
        debug_puts("Video capturer is unlocking 1\n");
        semSwitch.post();
        // All exposures capture time.
        // Control FPS
        last_time = current_time;
        current_time = boost::get_system_time();
        auto time_duration = current_time - last_time;
        if (delayMs - time_duration.total_milliseconds() > 0)
        {
            if (quit) return;
            verbose_print(globalArgs.verbosity, "Waiting for %ld ms to meet fps (%f).",
                    delayMs - time_duration.total_milliseconds(), fps);
            boost::this_thread::sleep_for(
                    boost::chrono::milliseconds(delayMs - time_duration.total_milliseconds()));
        }
    }
    semSwitch.post();
}

void RealtimeEngine::creator()
{
    using kernel::GenericFramePtr;
    debug_print(LVL_INFO, "HDR creator thread started with for class ptr %p.\n", (void * )this);
    GenericFramePtr hdrImage(new kernel::GenericFrame(globalArgs));
    GenericFramePtr ldrImage(new kernel::GenericFrame(globalArgs));
    cv::Mat resized;

    while (!quit)
    {
        debug_puts("Creator up to get lock 1\n");
        semSwitch.wait();
        debug_puts("Creator has lock 1\n");
        if (quit)
        {
            semCapture.post();
            return;
        }
        vectorOfFramePtrsPtr_t tmp;
        tmp = work;
        work = buffer;
        buffer = tmp;
        debug_puts("Creator is unlocking 0\n");
        semCapture.post();

        if (!hdrCreator.create(hdrImage, *work) || hdrImage == 0 || !hdrImage->isValid())
        {
            debug_print(LVL_DEBUG, "Waiting for %d ms for device to start (HDR).\n", 1000);
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
            continue;
        }
        if (!tmo.create(ldrImage, hdrImage) || ldrImage == 0 || !ldrImage->isValid())
        {
            debug_print(LVL_DEBUG, "Waiting for %d ms for device to start. (LDR)\n", 1000);
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
            continue;
        }

        if (createOutput)
        {
            // Also save the output.
            if (!videoWriter.isOpened())
            {
                //open please
                if (!openVideoWriter(ldrImage))
                {
                    std::cerr << "Output video won't be created. File `" << globalArgs.outputFile
                            << "` error." << std::endl;
                    createOutput = false;
                }
            }
            if (createOutput) // I have to check again? TODO
                videoWriter << ldrImage->getRawFrame();
        }

        if (!quit)
        {
            ldrImage->convertToDepth(CV_8UC3);
            ldrImage->convertToColorSpace(kernel::GenericFrame::COLOR_BGR);
            int w = ldrImage->getRawFrame().size().width;
            int h = ldrImage->getRawFrame().size().height;
            if (w > 800)
            {
                float aspect = ((float) w / (float) h);
                cv::resize(ldrImage->getRawFrame(), resized,
                        cv::Size(800, (int) ((float) 800 / aspect)), 0, 0, cv::INTER_NEAREST);
                char buf[128] = "";
                sprintf(buf, "Scalled. Frame %d.",
                        ((int) videoCapture.get(cv::CAP_PROP_FRAME_COUNT)) / exposuresPerHDR);
                cv::putText(resized, buf, cvPoint(30, 30), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8,
                        cvScalar(200, 200, 250), 1, CV_AA);
                cv::imshow(windowOriginalLabel, resized);
            }
            else
            {
                cv::imshow(windowOriginalLabel, ldrImage->getRawFrame());\
            }
        }
    }
    semCapture.post();
}

void RealtimeEngine::run()
{
    using namespace cv;

    namedWindow(windowOriginalLabel, CV_WINDOW_NORMAL);
    quit = !initializeDevice();

    Mat empty = Mat::ones(200, 300, CV_8U);
    cv::putText(empty, "Waiting for frames...", cvPoint(30, 30), cv::FONT_HERSHEY_COMPLEX_SMALL,
            0.8, cvScalar(200, 200, 250), 1, CV_AA);
    cv::imshow(windowOriginalLabel, empty);

    boost::thread videoCapturer(&RealtimeEngine::videoCapturer, this);
    boost::thread creator(&RealtimeEngine::creator, this);

    semCapture.post();
    while (!quit)
    {
        if (cv::waitKey(1000) >= 0)
        { // flush.
            std::cout << "Waiting for last capture." << std::endl;
            quit = true;
        }
    }

    videoCapturer.join();
    creator.join();
}

} /* namespace ui */
