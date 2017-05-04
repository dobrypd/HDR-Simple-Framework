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
#include "HDRCreator.hpp"

#include <chrono>
#include <vector>
#include <cmath>

#include <boost/thread.hpp>

#include "kernel/HDRExposition.hpp"
#include "LuminanceProcessor.hpp"

namespace HDRCreation
{

using namespace std;
using namespace cv;

HDRCreator::HDRCreator(const GlobalArgs_t & globalArgs)
        : globalArgs(globalArgs)
{
}

inline void getSize(Mat& m, unsigned int & w, unsigned int & h)
{
    Size s = m.size();
    w = s.width;
    h = s.height;
}

template<typename ChromaticityMatType>
class ChooseClosestColor: public kernel::GlobalOperation<float>
{
private:
    vector<kernel::GenericFramePtr> & originalInputs;

    Mat & outputColor;

    Mat & hdrLuminance;
    vector<Mat> & inputsCH;

    MatIterator_<float> hdrIt;
    vector<MatIterator_<ChromaticityMatType>> inputsCHIt;
    MatIterator_<ChromaticityMatType> outputIt;

public:
#ifndef NDEBUG
    vector<unsigned long int> pixFromExp;
#endif

    explicit ChooseClosestColor(vector<kernel::GenericFramePtr> & originalInputs,
            Mat & hdrLuminance, vector<Mat> & inputsCH, Mat & outputColor)
            : originalInputs(originalInputs), outputColor(outputColor), hdrLuminance(hdrLuminance), inputsCH(
                    inputsCH)
    {
        debug_print(LVL_DEBUG, "Creating color picker for hdr mat %p.\n", (void* ) &hdrLuminance);
    }

    virtual float process(float inputs[], unsigned int exps)
    {
        float closestDst = 1e10; // Infty
        unsigned int closestExpNo = 0;
        float hdrValue = *hdrIt; // assumption that sizes are proper
        for (int i = 0; i < exps; ++i)
        {
//            float ev = originalInputs[i]->getEV().get();
            float currentDst = abs(inputs[i] - 0.5 /*hdrValue*/);
            if (currentDst < closestDst)
            {
                closestDst = currentDst;
                closestExpNo = i;
            }
        }
        // Get color from exposition closestExpNo
#ifndef NDEBUG
        pixFromExp[closestExpNo]++;
#endif
        *outputIt = *(inputsCHIt[closestExpNo]);
        // Move
        hdrIt++;
        outputIt++;
        for_each(inputsCHIt.begin(), inputsCHIt.end(), [](MatIterator_<ChromaticityMatType>& it)
        {   it++;});
        return hdrValue; // Should do nothing if output is HDR Image
    }

    virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs)
    {
        outputIt = outputColor.begin<ChromaticityMatType>();
        hdrIt = hdrLuminance.begin<float>();
        for_each(inputsCH.begin(), inputsCH.end(), [this](Mat& m)
        {
            inputsCHIt.push_back(m.begin<ChromaticityMatType>());
        });
#ifndef NDEBUG
        pixFromExp.resize(inputs.size());
        for (int i = 0; i < inputs.size(); i++)
            pixFromExp[i] = 0;
#endif
        kernel::GlobalOperation<float>::apply(output, inputs);
        GaussianBlur(output, output, Size(3, 3), 1.5, 1.5);
    }
};

bool extractColor(vector<kernel::GenericFramePtr> & originalInputs, Mat & output,
        Mat & hdrLuminance, vector<Mat> & inputsL, vector<Mat> & inputsCH)
{
    ChooseClosestColor<Vec2f> opColorPicker(originalInputs, hdrLuminance, inputsCH, output);
    kernel::HDRExposition<float> colorPicker(hdrLuminance, inputsL);
    colorPicker.addOperation(opColorPicker);
    colorPicker.process();
#ifndef NDEBUG
    for (unsigned int i = 0; i < inputsL.size(); ++i)
    {
        debug_print(LVL_DEBUG, "Color pixels picked from exp %d == %lu\n", i,
                opColorPicker.pixFromExp[i]);
    }
#endif
    return true;
}

bool HDRCreator::create(kernel::GenericFramePtr outputFrame,
        vector<kernel::GenericFramePtr> & inputs)
{
    unsigned int width = 0, height = 0;
    unsigned int exps = inputs.size();
    chrono::time_point < chrono::system_clock > lastTime;
    lastTime = chrono::system_clock::now();

    std::vector<boost::thread *> threads;

    verbose_print(globalArgs.verbosity, "HDR creation from %d input(s).", exps);

    if ((outputFrame == 0) || (exps <= 0) || (inputs.front() == NULL)) return false;

    getSize(inputs.front()->getRawFrame(), width, height);
    if ((width == 0) || (height == 0)) return false;

    vector<Mat> inputsL, inputsCH;
    inputsL.resize(exps);
    inputsCH.resize(exps);

    Mat hdrLuminance = cv::Mat(height, width, CV_32F); // 1 Luminance factor channel
    Mat hdrColor = cv::Mat(height, width, CV_32FC2); // 2 Chromaticity channels

    verbose_print(globalArgs.verbosity,
            "Phase 1. Change colorspace.\t\tInitialization took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    lastTime = chrono::system_clock::now();
    {
        bool properOut = true;
        std::for_each(inputs.begin(), inputs.end(),
                [&properOut, &threads](kernel::GenericFramePtr & gF)
                {
                    threads.push_back(new boost::thread(
                                    [](bool &properOut, kernel::GenericFramePtr & gF)
                                    {
                                        properOut &= gF->convertToDepth(CV_32FC3);
                                        properOut &= gF->convertToColorSpace(kernel::GenericFrame::COLOR_CIELab);
                                    }, properOut, gF));
                });
        for_each(threads.begin(), threads.end(), [](boost::thread* t)
        {   t->join(); delete t;});
        threads.clear();
        if (!properOut) return false;
    }

    verbose_print(globalArgs.verbosity, "Phase 2. Split channels.\t\tPhase 1 took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    lastTime = chrono::system_clock::now();
    {
        int exp = 0;
        std::for_each(inputs.begin(), inputs.end(),
                [&exp, &inputsL, &inputsCH, &threads, height, width](kernel::GenericFramePtr & gF)
                {
                    threads.push_back(new boost::thread(
                                    [&inputsL, &inputsCH, height, width](int exp, kernel::GenericFramePtr & gF)
                                    {
                                        Mat chromaOut = Mat(height, width, CV_32FC2);
                                        Mat channel[3];
                                        split(gF->getRawFrame(), channel);
                                        inputsL[exp] = channel[0] / 100; // L
                                        const int fromToChromaticity[] =
                                        {   0, 0, 1, 1}; // a*b*
                                        mixChannels(channel + 1, 2, &chromaOut, 1, fromToChromaticity, 2);
                                        inputsCH[exp] = chromaOut;
                                    }, exp, gF));
                    exp++;
                });
        for_each(threads.begin(), threads.end(), [](boost::thread* t)
        {   t->join(); delete t;});
        threads.clear();
    }

    verbose_print(globalArgs.verbosity, "Phase 3. Process luminance.\t\tPhase 2 took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    lastTime = chrono::system_clock::now();
    {
        LuminanceProcessor processor(inputs);
        if (!processor.mapLuminance(hdrLuminance, inputsL)) return false;
        //        *hdrLuminance.begin<float>() = 0;
        //        *(++hdrLuminance.begin<float>()) = 1;
        normalize(hdrLuminance, hdrLuminance, 0, 1, NORM_MINMAX);
    }

    verbose_print(globalArgs.verbosity, "Phase 4. Process chromaticity.\t\tPhase 3 took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    lastTime = chrono::system_clock::now();
    {
        if (!extractColor(inputs, hdrColor, hdrLuminance, inputsL, inputsCH)) return false;
    }

    threads.push_back(new boost::thread([&hdrLuminance]()
    {
        normalize(hdrLuminance, hdrLuminance, 0, 100, NORM_MINMAX);
    }));
    // Wont be used any more.
    inputsL.clear();
    inputsCH.clear();
    // Create output.
    Mat output = cv::Mat(height, width, CV_32FC3); // Lab

    for_each(threads.begin(), threads.end(), [](boost::thread* t)
    {   t->join(); delete t;});
    verbose_print(globalArgs.verbosity, "Phase 5. Mix channels.\t\tPhase 4 took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    lastTime = chrono::system_clock::now();
    {
        // Luminance
        const int fromToLuminanace[] =
        { 0, 0 }; // L*
        mixChannels(&hdrLuminance, 1, &output, 1, fromToLuminanace, 1);
        // Color
        const int fromToColor[] =
        { 0, 1, 1, 2 }; // a*b*
        mixChannels(&hdrColor, 1, &output, 1, fromToColor, 2);
    }

    outputFrame->assignFrameTo(output, kernel::GenericFrame::COLOR_CIELab);
    verbose_print(globalArgs.verbosity, "Finished. \t\tPhase 5 took [%ld ms].",
            (chrono::duration_cast < std::chrono::milliseconds
                    > (chrono::system_clock::now() - lastTime)).count());
    return true;
}

} /* namespace HDRCreation */
