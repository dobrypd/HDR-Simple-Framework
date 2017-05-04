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
#include "HDRExposition.hpp"

#include <vector>
#include <opencv2/opencv.hpp>

namespace kernel
{

template<typename PixelType>
void GlobalOperation<PixelType>::apply(cv::Mat & output, std::vector<cv::Mat> & inputs)
{
    typedef cv::MatConstIterator_<PixelType> it_t;
    unsigned int noOfExp = inputs.size();
    PixelType* expositions = new PixelType[noOfExp];
    it_t* itrtrs = new it_t[noOfExp];
    it_t* ends = new it_t[noOfExp];
    unsigned int exp = 0;
    for_each(inputs.begin(), inputs.end(), [&itrtrs, &ends, &exp](cv::Mat & input)
    {
        ends[exp] = input.end<PixelType>();
        itrtrs[exp++] = input.begin<PixelType>();
    });

    std::for_each(output.begin<PixelType>(), output.end<PixelType>(),
            [this, &noOfExp, &itrtrs, &ends, &expositions](PixelType &p)
            {
                for (int exp = 0; exp < noOfExp; exp++)
                {
                    assert(itrtrs[exp] != ends[exp]);
                    expositions[exp] = *itrtrs[exp];
                    itrtrs[exp]++;
                }
                p = process(expositions, noOfExp);
            });

    delete[] ends;
    delete[] itrtrs;
    delete[] expositions;
}

template<typename PixelType, typename PartitionType, typename PartitionShift>
void LocalOperation<PixelType, PartitionType, PartitionShift>::apply(cv::Mat & output,
        std::vector<cv::Mat> & inputs)
{
    typedef typename Partition<PixelType, PartitionType, PartitionShift>::template PartitionIterator<
            PixelType> iterator;
    std::vector<std::vector<iterator>> partitionsIts;
    std::vector<std::vector<iterator>> partitionsEnds;

    unsigned int noOfExp = inputs.size();
    PixelType* expositions = new PixelType[noOfExp];
    debug_print(LVL_DEBUG, "Applying local, with partitions. No of expositions = %d.\n", noOfExp);

    std::for_each(inputs.begin(), inputs.end(),
            [this, &partitionsIts, &partitionsEnds](cv::Mat & input)
            {
                partitionsIts.push_back(partitions.template listOfBegins<PixelType>(input));
                partitionsEnds.push_back(partitions.template listOfEnds<PixelType>(input));
            });
    std::vector<iterator> outputPartitionsIts, outputPartitionsEnds;
    outputPartitionsIts = partitions.template listOfBegins<PixelType>(output);
    outputPartitionsEnds = partitions.template listOfEnds<PixelType>(output);
    debug_print(LVL_DEBUG, "Divided into %lu partitions.\n", outputPartitionsIts.size());

    assert(partitions.size() == outputPartitionsIts.size());
    for (PartitionType ident = 0; ident < partitions.size(); ++ident)
    {
        enterArea(ident);

        std::vector<iterator> inputsIt;
        std::for_each(partitionsIts.begin(), partitionsIts.end(),
                [&inputsIt, &ident](std::vector<iterator> & currentExpositionIterators)
                {
                    inputsIt.push_back(currentExpositionIterators[ident]);
                });

        std::for_each(outputPartitionsIts[ident], outputPartitionsEnds[ident],
                [this, &expositions, &inputsIt, &noOfExp, &ident, &partitionsEnds](PixelType & outputValue)
                {
                    unsigned int exp = 0;
                    std::for_each(inputsIt.begin(), inputsIt.end(), [&exp, &expositions, &ident, &partitionsEnds](iterator & inputExpValue)
                            {
                                assert(inputExpValue != partitionsEnds[exp][ident]);
                                expositions[exp] = *inputExpValue;
                                ++inputExpValue;
                                exp++;
                            });
                    assert(noOfExp == exp);
                    outputValue = process(expositions, noOfExp);
                });

        leaveArea(ident);
    }

    delete[] expositions;
}

template<typename PixelType>
HDRExposition<PixelType>::HDRExposition(cv::Mat& output, std::vector<cv::Mat> inputs)
        : output(output), inputs(inputs)
{
}

template<typename PixelType>
HDRExposition<PixelType>& HDRExposition<PixelType>::addOperation(Operation<PixelType> & operation)
{
    operations.push_back(operation);
    return *this;
}

template<typename PixelType>
cv::Mat& HDRExposition<PixelType>::process()
{
    std::for_each(operations.begin(), operations.end(), [this](Operation<PixelType> & op)
    {
        std::for_each(inputs.begin(), inputs.end(), [&op](cv::Mat& m)
                {   m = op.preprocess(m);});
        op.apply(output, inputs);
        std::for_each(inputs.begin(), inputs.end(), [&op](cv::Mat& m)
                {   m = op.postprocess(m);});
    });
    return output;
}

template class GlobalOperation<float> ;
template class LocalOperation<float, unsigned char, unsigned int> ;
template class HDRExposition<float> ;
//template class HDRExposition<cv::Vec3f> ;
//template class HDRExposition<cv::Vec3b> ;

} /* namespace kernel */
