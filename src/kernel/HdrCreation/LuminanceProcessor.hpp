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
#ifndef LUMINANCEPROCESSOR_HPP_
#define LUMINANCEPROCESSOR_HPP_

#include <opencv2/opencv.hpp>
#include <vector>

#include "kernel/HDRExposition.hpp"
#include "kernel/GenericFrame.hpp"

namespace HDRCreation
{

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

/*
 *
 */
class LuminanceProcessor
{
protected:
    std::vector<kernel::GenericFramePtr> & originalInputs;

    class CameraCorrection: public kernel::Preprocess<float>
    {
    private:
        std::vector<kernel::GenericFramePtr> & originalInputs;
    public:
        CameraCorrection(std::vector<kernel::GenericFramePtr> & originalInputs)
                : kernel::Preprocess<float>(), originalInputs(originalInputs)
        { }
        virtual cv::Mat & preprocess(cv::Mat & m);
        virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs)
        { }
    };

    class CameraCorrectionPost: public kernel::Preprocess<float>
    {
    private:
        std::vector<kernel::GenericFramePtr> & originalInputs;
    public:
        CameraCorrectionPost(std::vector<kernel::GenericFramePtr> & originalInputs)
                : kernel::Preprocess<float>(), originalInputs(originalInputs)
        { }
        virtual cv::Mat & preprocess(cv::Mat & m);
        virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs)
        { }
    };

    class ThresholdBasedPartitionBuilder: public kernel::Partition<float, unsigned char,
            unsigned int>
    {
    private:
        float t0 /* underexposure threshold */, t1 /* overexposure threshold */;
        cv::Size s;
        void createNewThresholdArea(cv::Mat & m, cv::Mat & partitions,
                std::vector<unsigned int> & thisPartition, unsigned int ident);
    public:
        ThresholdBasedPartitionBuilder(cv::Size s, float t0, float t1);

        virtual void apply(std::vector<cv::Mat> & inputs);
        virtual cv::Mat & preprocess(cv::Mat & m);
    };

    struct PartitionData
    {
        double avgValOfMaxPriorExp;
        std::vector<double> avgOfAllExp;
        unsigned long long int noOfPixels;

        float evShift;
    };

    class PartitionDataCollector: public kernel::LocalOperation<float, unsigned char, unsigned int>
    {
    private:
        std::vector<kernel::GenericFramePtr> & originalInputs;
        std::vector<PartitionData> data;

        unsigned char ident;
    public:
        PartitionDataCollector(kernel::Partition<float, unsigned char, unsigned int> & partitions,
                std::vector<kernel::GenericFramePtr> & originalInputs);

        virtual float process(float inputs[], unsigned int exps);
        virtual void enterArea(unsigned char ident);
        virtual void leaveArea(unsigned char ident);

        std::vector<PartitionData> & getData();
    };

    class HistogramShifter: public kernel::LocalOperation<float, unsigned char, unsigned int>
    {
    private:
#ifndef NDEBUG
        size_t c;
#endif
        std::vector<PartitionData> & data;

        // Visitor-like
        unsigned char ident;
        float areaShift;
    public:
        HistogramShifter(kernel::Partition<float, unsigned char, unsigned int> & partitions,
                std::vector<PartitionData> & data);
        virtual float process(float inputs[], unsigned int exps);
        virtual void enterArea(unsigned char ident);
        virtual void leaveArea(unsigned char ident);
    };

public:
    LuminanceProcessor(std::vector<kernel::GenericFramePtr> & originalInputs);

    bool mapLuminance(cv::Mat & output, std::vector<cv::Mat> & inputs);
};

} /* namespace kernel */

#endif /* LUMINANCEPROCESSOR_H_ */
