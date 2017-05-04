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
#include "LuminanceProcessor.hpp"

#include <kernel/HDRExposition.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

namespace HDRCreation
{

LuminanceProcessor::LuminanceProcessor(std::vector<kernel::GenericFramePtr> & originalInputs)
        : originalInputs(originalInputs)
{
}

Mat & LuminanceProcessor::CameraCorrection::preprocess(Mat & m)
{
    pow(m, 0.7, m);
    return m;
}

Mat & LuminanceProcessor::CameraCorrectionPost::preprocess(Mat & m)
{
    pow(m, 1./0.7, m);
    return m;
}

void LuminanceProcessor::ThresholdBasedPartitionBuilder::createNewThresholdArea(Mat & m,
        Mat & partitions, vector<unsigned int> & thisPartition, unsigned int ident)
{
    assert(ident > 0);

    MatIterator_<unsigned char> partitionsIt = partitions.begin<unsigned char>();
    MatIterator_<unsigned char> partitionsEnd = partitions.end<unsigned char>();

    unsigned int shift = 0;
    for_each(m.begin<float>(), m.end<float>(),
            [this, &partitionsIt, &partitionsEnd, &thisPartition, &shift, &ident](float v)
            {
                assert(partitionsIt != partitionsEnd);

                // Create area only in blank.
                if (*partitionsIt == 0)
                {
                    if ((v > t0) && (v < t1))
                    {
                        *partitionsIt = ident;
                        thisPartition.push_back(shift);
                        shift = 0;
                    }
                }
                shift++;
                partitionsIt++;
            });
    assert(partitionsIt == partitionsEnd);
}

LuminanceProcessor::ThresholdBasedPartitionBuilder::ThresholdBasedPartitionBuilder(Size s, float t0,
        float t1)
        : t0(t0), t1(t1), s(s)
{
    // This creates assumption that maximum number of expositions cannot extend 255
    // (256 - 1 (blank area)). Any new exposition can create new partition area.
}

void LuminanceProcessor::ThresholdBasedPartitionBuilder::apply(vector<Mat> & inputs)
{
    unsigned char currentProperAreaNo = 0;
    Mat partitionsTmp = Mat::zeros(s, CV_8U);
    for_each(inputs.begin(), inputs.end(),
            [this, &partitionsTmp, &currentProperAreaNo](Mat & m)
            {
                vector<unsigned int> newPartition;
                createNewThresholdArea(m, partitionsTmp, newPartition, currentProperAreaNo + 1);
                partitions.push_back(pair<unsigned char, vector<unsigned int>>(currentProperAreaNo++, newPartition));
            });

    // Add 0 to as blank partition
    unsigned int shift = 0;
    vector<unsigned int> newPartition;
    for_each(partitionsTmp.begin<unsigned char>(), partitionsTmp.end<unsigned char>(),
            [&shift, &newPartition](unsigned char v)
            {
                if (v == 0)
                {
                    newPartition.push_back(shift);
                    shift = 0;
                }
                shift++;
            });
    partitions.push_back(
            pair<unsigned char, vector<unsigned int>>(currentProperAreaNo++, newPartition));

    noOfPartitions = currentProperAreaNo;
}

Mat & LuminanceProcessor::ThresholdBasedPartitionBuilder::preprocess(Mat & m)
{
    return m;
}

LuminanceProcessor::PartitionDataCollector::PartitionDataCollector(
        kernel::Partition<float, unsigned char, unsigned int> & partitions,
        std::vector<kernel::GenericFramePtr> & originalInputs)
        : kernel::LocalOperation<float, unsigned char, unsigned int>(partitions), originalInputs(
                originalInputs)
{
    debug_print(LVL_DEBUG, "Creating new partition datas for %d areas.\n", partitions.size());
    data.resize(size()); // most likely size == 0 while constructing.
}

float LuminanceProcessor::PartitionDataCollector::process(float inputs[], unsigned int exps)
{
    data[ident].noOfPixels++;
    for (unsigned char exp = 0; exp < exps; ++exp)
    {
        data[ident].avgOfAllExp[exp] += inputs[exp];
    }
    return inputs[0]; // doesn't matter
}
void LuminanceProcessor::PartitionDataCollector::enterArea(unsigned char ident)
{
    this->ident = ident;
    debug_print(LVL_DEBUG, "Creating new partition data %d/%d.\n", ident + 1, size());
    data.resize(size()); // most likely size == 0 while constructing.
    data[ident] = PartitionData
    { 0, vector<double>(), 0, 0 };
    data[ident].avgOfAllExp.resize(originalInputs.size());
    for_each(data[ident].avgOfAllExp.begin(), data[ident].avgOfAllExp.end(), [](double & avgVal){
        avgVal = 0;
    });
}
void LuminanceProcessor::PartitionDataCollector::leaveArea(unsigned char ident)
{
    // aggregate
    unsigned long long & noOfPixels = data[ident].noOfPixels;
    unsigned char exp = 0;
    for_each(data[ident].avgOfAllExp.begin(), data[ident].avgOfAllExp.end(), [&noOfPixels, &exp, &ident](double & avgVal){
        avgVal /= noOfPixels;
        debug_print(LVL_INFO, "Area %d, exposition %d, avg pixel val %f\n", ident, exp, avgVal);
        exp++;
    });

    if (ident >= originalInputs.size())
    {
        // this area wasn't catched in any exposition
        data[ident].avgValOfMaxPriorExp = data[ident].avgOfAllExp[ident];
        double minDist = 1;
        double closestTo = 0.5;
        unsigned char minDistIndex = 0;
        unsigned char currentIndex = 0;
        std::for_each(data[ident].avgOfAllExp.begin(), data[ident].avgOfAllExp.end(),
                [&minDist, &closestTo, &minDistIndex, &currentIndex](double & avgVal)
                {
                    if (abs(avgVal - closestTo) < minDist)
                    {
                        minDist = abs(avgVal - closestTo);
                        minDistIndex = currentIndex;
                    }
                    currentIndex++;
                });
        data[ident].avgValOfMaxPriorExp = data[ident].avgOfAllExp[minDistIndex];
        data[ident].evShift = originalInputs[ident - 1]->getEV().get();
    }
    else
    {
        data[ident].avgValOfMaxPriorExp = data[ident].avgOfAllExp[ident];
        data[ident].evShift = originalInputs[ident]->getEV().get();
    }

}
std::vector<LuminanceProcessor::PartitionData> & LuminanceProcessor::PartitionDataCollector::getData()
{
    return data;
}

LuminanceProcessor::HistogramShifter::HistogramShifter(
        kernel::Partition<float, unsigned char, unsigned int> & partitions,
        std::vector<LuminanceProcessor::PartitionData> & data)
        : kernel::LocalOperation<float, unsigned char, unsigned int>(partitions), data(data)
{
}

float LuminanceProcessor::HistogramShifter::process(float inputs[], unsigned int exps)
{
#ifndef NDEBUG
    c++;
#endif
    return inputs[min((unsigned int) ident, exps - 1)] + areaShift;;
}
void LuminanceProcessor::HistogramShifter::enterArea(unsigned char ident)
{
    areaShift = log1p(data[ident].avgValOfMaxPriorExp);
    debug_print(LVL_DEBUG, "Entering partition %d, global shift by= %f.\n", ident, areaShift);
    this->ident = ident;
#ifndef NDEBUG
    c = 0;
#endif
}
void LuminanceProcessor::HistogramShifter::leaveArea(unsigned char ident)
{
#ifndef NDEBUG
    debug_print(LVL_DEBUG, "Leaving partition %u, no of pix %lu\n", (unsigned int )ident, c);
#endif
}

bool LuminanceProcessor::mapLuminance(Mat & output, vector<Mat> & inputs)
{
    // Inputs will be overridden.

    kernel::HDRExposition<float> expositions(output, inputs);

    CameraCorrection opCorrect(originalInputs);
    CameraCorrectionPost opPostCorrect(originalInputs);
    ThresholdBasedPartitionBuilder opPartition(inputs.front().size(), 0.1, 0.9); // as argument -> FUTURE
    PartitionDataCollector opDataCollector(opPartition, originalInputs);
    HistogramShifter opHistogramShifer(opPartition, opDataCollector.getData());

    expositions.addOperation(opCorrect);
    expositions.addOperation(opPartition);
    expositions.addOperation(opDataCollector);
    expositions.addOperation(opHistogramShifer);
    expositions.addOperation(opPostCorrect);

    expositions.process();
    pow(output, 1./0.7, output);
    return true;
}

} /* namespace HDRCreation */
