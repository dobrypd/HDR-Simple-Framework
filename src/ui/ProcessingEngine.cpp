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

#include "ProcessingEngine.hpp"
#include "config.h"
#include "kernel/HdrCreation/HDRCreator.hpp"
#include "kernel/TonemappingOperators/dobrowolski15/Dobrowolski15.hpp"
#include "kernel/GenericFrame.hpp"

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <vector>

using std::string;
using std::cout;
using std::endl;

namespace ui
{

const char * ProcessingEngine::tmp_root = "/tmp/HdrSimpleFreamwork/";

ProcessingEngine::ProcessingEngine(const GlobalArgs_t & globalArgs)
        : globalArgs(globalArgs)
{

}

ProcessingEngine::~ProcessingEngine()
{
}

void ProcessingEngine::process()
{
    //create_TMP();
}

const boost::filesystem::path & ProcessingEngine::create_TMP()
{
    temp_path = boost::filesystem::path(tmp_root);
    temp_path += boost::filesystem::unique_path();
    boost::filesystem::create_directories(temp_path);
    debug_print(LVL_INFO, "Temporary files will be stored in %s.\n", temp_path.native().c_str());

    return temp_path;
}

ProcessingHDRCreatorModel::ProcessingHDRCreatorModel(const GlobalArgs_t & globalArgs)
        : super(globalArgs)
{

}

void ProcessingHDRCreatorModel::process()
{
    using kernel::GenericFramePtr;
    super::process();
    HDRCreation::HDRCreator creator(globalArgs);
    std::vector<GenericFramePtr> frames;
    for (int i = 0; i < globalArgs.inputs; ++i)
    {
        frames.push_back(GenericFramePtr(new kernel::GenericFrame(globalArgs)));
    }
    // Load files
    std::vector<boost::thread *> threads;
    int i = 0;
    std::for_each(frames.begin(), frames.end(),
            [this, &threads, &i](GenericFramePtr & frame)
            {
                threads.push_back(new boost::thread(&kernel::GenericFrame::getFrameFromFile, frame, globalArgs.inputFiles[i++]));
            });
    std::for_each(threads.begin(), threads.end(), [](boost::thread* t)
    {   t->join(); delete t;});

    GenericFramePtr hdrImage(new kernel::GenericFrame(globalArgs));
#ifndef NDEBUG
    const char * window = "DEBUG WINDOW - processing engine";
    cv::namedWindow(window, CV_WINDOW_NORMAL);
    std::for_each(frames.begin(), frames.end(), [&window](GenericFramePtr & ptr)
    {
        cv::Mat bgr = ptr->getRawFrame().clone();
        if (ptr->getColorSpace() != kernel::GenericFrame::COLOR_BGR)
        cvtColor(bgr, bgr, cv::COLOR_Lab2BGR);
        if (!bgr.empty())
        cv::imshow(window, bgr);
        cv::waitKey(0);
    });
#endif

    if (creator.create(hdrImage, frames) && hdrImage != 0 && hdrImage->isValid())
    {
#ifndef NDEBUG
        cv::Mat bgr = hdrImage->getRawFrame().clone();
        cvtColor(bgr, bgr, cv::COLOR_Lab2BGR);
        cv::imshow(window, hdrImage->getRawFrame());
        cv::waitKey(0);
#endif
        if (hdrImage->saveFrameToFile(globalArgs.outputFile))
        {
            std::cout << "File saved to " << globalArgs.outputFile << std::endl;
        }
        else
        {
            std::cout << "Cannot save file to " << globalArgs.outputFile << std::endl;
        }
    }
    else
    {
        std::cout << "Unfortunately due to errors the output file wont be saved." << std::endl;
    }
}

ProcessingToneMapper::ProcessingToneMapper(const GlobalArgs_t & globalArgs)
        : super(globalArgs)
{

}

void ProcessingToneMapper::process()
{
    using kernel::GenericFramePtr;
    super::process();
    TMO::Dobrowolski15 toneMapper(globalArgs);
    GenericFramePtr frame(new kernel::GenericFrame(globalArgs));
    frame->getFrameFromFile(globalArgs.inputFiles[0]); // Load file;
    GenericFramePtr ldrImage(new kernel::GenericFrame(globalArgs));

#ifndef NDEBUG
    const char * window = "DEBUG WINDOW - processing engine";
    cv::namedWindow(window, CV_WINDOW_NORMAL);
    cv::Mat bgr = frame->getRawFrame().clone();
    if (frame->getColorSpace() != kernel::GenericFrame::COLOR_BGR)
        cvtColor(bgr, bgr, cv::COLOR_Lab2BGR);
    if (!bgr.empty())
    {
        cv::imshow(window, bgr);
        cv::waitKey(0);
    }
#endif

    if (toneMapper.create(ldrImage, frame) && ldrImage != 0 && ldrImage->isValid())
    {
#ifndef NDEBUG
        cv::imshow(window, ldrImage->getRawFrame());
        cv::waitKey(0);
#endif
        if (ldrImage->saveFrameToFile(globalArgs.outputFile))
        {
            std::cout << "File saved to " << globalArgs.outputFile << std::endl;
        }
        else
        {
            std::cout << "Cannot save file to " << globalArgs.outputFile << std::endl;
        }
    }
    else
    {
        std::cout << "Unfortunately due to errors the output file wont be saved." << std::endl;
    }
}

ProcessingHDRCreatorAndToneMapper::ProcessingHDRCreatorAndToneMapper(
        const GlobalArgs_t & globalArgs)
        : super(globalArgs)
{

}

void ProcessingHDRCreatorAndToneMapper::process()
{
    using kernel::GenericFramePtr;
    super::process();
    HDRCreation::HDRCreator creator(globalArgs);
    TMO::Dobrowolski15 toneMapper(globalArgs);
    std::vector<GenericFramePtr> frames;
    for (int i = 0; i < globalArgs.inputs; ++i)
    {
        frames.push_back(GenericFramePtr(new kernel::GenericFrame(globalArgs)));
    }

    // Load files
    std::vector<boost::thread *> threads;
    int i = 0;
    std::for_each(frames.begin(), frames.end(),
            [this, &threads, &i](GenericFramePtr & frame)
            {
                threads.push_back(new boost::thread(&kernel::GenericFrame::getFrameFromFile, frame, globalArgs.inputFiles[i++]));
            });
    std::for_each(threads.begin(), threads.end(), [](boost::thread* t)
    {   t->join(); delete t;});

    GenericFramePtr hdrImage(new kernel::GenericFrame(globalArgs));
    /** CREATE HDR */
    if (creator.create(hdrImage, frames) && hdrImage != 0 && hdrImage->isValid())
    {
        /** CREATE LDR */
        GenericFramePtr ldrImage(new kernel::GenericFrame(globalArgs));
        if (toneMapper.create(ldrImage, hdrImage) && ldrImage != 0 && ldrImage->isValid())
        {
            /** SAVE LDR */
            if (ldrImage->saveFrameToFile(globalArgs.outputFile))
            {
                std::cout << "File saved to " << globalArgs.outputFile << std::endl;
            }
            else
            {
                std::cout << "Cannot save file to " << globalArgs.outputFile << std::endl;
            }

            /** SAVE HDR if declared to be saved */
            if (globalArgs.createHDR)
            {
                std::string exrFileName(globalArgs.outputFile);
                exrFileName += ".exr";
                if (hdrImage->saveFrameToFile(exrFileName))
                {
                    std::cout << "File saved to " << exrFileName << std::endl;
                }
                else
                {
                    std::cout << "Cannot save file to " << exrFileName << std::endl;
                }
            }
        }
        else
        {
            std::cout << "Unfortunately due to errors the output file wont be saved." << std::endl;
        }

    }
    else
    {
        std::cout << "Unfortunately due to errors the output file wont be saved." << std::endl;
    }
}

} /* namespace ui */
