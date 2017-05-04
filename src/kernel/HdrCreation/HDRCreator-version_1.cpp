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

#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions.hpp>
#include <vector>
#include <cmath>

namespace HDRCreation
{

using namespace std;

HDRCreator::HDRCreator(const GlobalArgs_t & globalArgs)
    : globalArgs(globalArgs)
{
    // TODO Auto-generated constructor stub
}

typedef float(*luminanceCorrection_t)(float);

float logLumiCorrection(float value) {
    double base = 10; // TODO:

    double baseFraction = log(base);
    double logValue = log1p(value);

    return logValue / baseFraction;
}

float powLumiCorrection(float value) {
    double power = 3.3; // TODO:

    double powValue = pow(value + 1, power);

    return powValue;
}

void correctLuminocity(luminanceCorrection_t lumiCorrection, pfs::FramePtr frame) {
    using namespace pfs;
    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels(X, Y, Z);
    for (auto itX = X->begin(), itY = Y->begin(), itZ = Z->begin();
            itX != X->end(); itX++, itY++, itZ++) {
        (*itX) = lumiCorrection(*itX);
        (*itY) = lumiCorrection(*itY);
        (*itZ) = lumiCorrection(*itZ);
    }
}

typedef float(*agrFun_t)(pfs::FramePtr);

float avgAgr(pfs::FramePtr frame) {
    using namespace pfs;
    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels(X, Y, Z);
    double sumX=0, sumY=0, sumZ=0;
    for (auto itX = X->begin(), itY = Y->begin(), itZ = Z->begin();
            itX != X->end(); itX++, itY++, itZ++) {
        sumX += *itX;
        sumY += *itY;
        sumZ += *itZ;
    }
    sumX /= frame->getWidth() * frame->getHeight();
    sumY /= frame->getWidth() * frame->getHeight();
    sumZ /= frame->getWidth() * frame->getHeight();

    return (sumX + sumY + sumZ) / 3; // dummy heuristic, use colorspace characteristic
}

float heuristicRelativeEV(agrFun_t aggregation, pfs::FramePtr frame)
{
    return aggregation(frame);
}

void getChannelsIterators(pfs::Channel *X, pfs::Channel *Y, pfs::Channel *Z,
        pfs::Channel::iterator & itX, pfs::Channel::iterator & itY, pfs::Channel::iterator & itZ)
{
    itX = X->begin();
    itY = Y->begin();
    itZ = Z->begin();
}

void getChannelsIterators(pfs::FramePtr frame,
        pfs::Channel::iterator & itX, pfs::Channel::iterator & itY, pfs::Channel::iterator & itZ)
{
    using pfs::Channel;
    Channel *X, *Y, *Z;
    frame->getXYZChannels(X, Y, Z);
    getChannelsIterators(X, Y, Z, itX, itY, itZ);
}

void getChannelsIterators(pfs::FramePtr frame,
        pfs::Channel::iterator & itX, pfs::Channel::iterator & itY, pfs::Channel::iterator & itZ,
        pfs::Channel::iterator & endX)
{
    using pfs::Channel;
    Channel *X, *Y, *Z;
    frame->getXYZChannels(X, Y, Z);
    getChannelsIterators(X, Y, Z, itX, itY, itZ);
    endX = X->end();
}

void createBeginIterators(vector<pfs::FramePtr> & frames,
        vector<pfs::Channel::iterator> & XChannelsIt,
        vector<pfs::Channel::iterator> & YChannelsIt,
        vector<pfs::Channel::iterator> & ZChannelsIt)
{
    XChannelsIt.resize(frames.size());
    YChannelsIt.resize(frames.size());
    ZChannelsIt.resize(frames.size());
    int expNo = 0;
    for (auto frameIt = frames.begin(); frameIt != frames.end(); ++frameIt, ++expNo)
    {
        pfs::Channel::iterator itX, itY, itZ;
        getChannelsIterators(*frameIt, itX, itY, itZ);
        XChannelsIt[expNo] = itX;
        YChannelsIt[expNo] = itY;
        ZChannelsIt[expNo] = itZ;
    }
}

/**
 * Assumptions:
 * 1 - all frames are aligned, with fixed size.
 * 2 - frames are exposure value ordered
 */
bool HDRCreator::create(kernel::GenericFramePtr output, std::vector<kernel::GenericFramePtr> & framesGeneric)
{
    /**
     * Assertions
     */
    debug_print(LVL_INFO, "Found %d frame(s) to process.\n", framesGeneric.size());
    if (output == 0) {
    	return false;
    }

    if (!output->isValid()) {
    	if (framesGeneric.front()->isValid())
    	{
    		if (!output->setEmptyFrameFrom(framesGeneric.front()->getRawFramePFS())) {
    		    return false;
    		}
    	} else {
    		return false;
    	}
    }
    assert(output->isValid());
    pfs::FramePtr outputFrame = output->getRawFramePFS();

    /**
     * Create pointers.
     * Map luminocity in each frame.
     */
    int noOfExposures = framesGeneric.size();
    size_t frameSize = outputFrame->getWidth() * outputFrame->getHeight();
    vector<pfs::FramePtr> frames;
    vector<double> framesRelativeEVComputed; // X translation
    vector<double> avgLuminocityRelativeFractionX, avgLuminocityRelativeFractionY,avgLuminocityRelativeFractionZ; // Y translation, than frame 1
    for (auto frameGenIt = framesGeneric.begin(); frameGenIt != framesGeneric.end(); frameGenIt++) {
        pfs::FramePtr frame = (*frameGenIt)->getRawFramePFS();
        frames.push_back(frame);
        correctLuminocity(&logLumiCorrection, frame);
        framesRelativeEVComputed.push_back(heuristicRelativeEV(&avgAgr, frame));

        avgLuminocityRelativeFractionX.push_back(0.0f);
        avgLuminocityRelativeFractionY.push_back(0.0f);
        avgLuminocityRelativeFractionZ.push_back(0.0f);
    }


    /**
     * Prepare vectors of channels. Channel data iterators.
     */
    pfs::Channel::iterator itX, itY, itZ, endX;


    /**
     *
     */
    vector<pfs::Channel::iterator> XChannelsIt, YChannelsIt, ZChannelsIt;
    createBeginIterators(frames, XChannelsIt, YChannelsIt, ZChannelsIt);
    float relativeX = **XChannelsIt.begin(), relativeY = **YChannelsIt.begin(), relativeZ = **ZChannelsIt.begin();
    int expNo = 1;
    for (auto XChannelsDataIt = ++XChannelsIt.begin(), YChannelsDataIt = ++YChannelsIt.begin(), ZChannelsDataIt = ++ZChannelsIt.begin();
            XChannelsDataIt != XChannelsIt.end();
            XChannelsDataIt++, YChannelsDataIt++, ZChannelsDataIt++, expNo++)
    {
        auto XValueIt = *XChannelsDataIt, YValueIt = *YChannelsDataIt, ZValueIt = *ZChannelsDataIt;
        for (size_t pixel = 0; pixel < frameSize;
                ++pixel, ++XValueIt, ++YValueIt, ++ZValueIt)
        {
            auto XValue = *XValueIt, YValue = *YValueIt, ZValue = *ZValueIt;
            avgLuminocityRelativeFractionX[expNo] += XValue - relativeX;
            avgLuminocityRelativeFractionY[expNo] += YValue - relativeY;
            avgLuminocityRelativeFractionZ[expNo] += ZValue - relativeZ;
        }
    }
    // Normalize
    for(auto avgLRFXIt = avgLuminocityRelativeFractionX.begin(), avgLRFYIt = avgLuminocityRelativeFractionY.begin(),
            avgLRFZIt = avgLuminocityRelativeFractionZ.begin();
            avgLRFXIt != avgLuminocityRelativeFractionX.end();
            avgLRFXIt++, avgLRFYIt++, avgLRFZIt++)
    {
        (*avgLRFXIt) /= frameSize;
        (*avgLRFYIt) /= frameSize;
        (*avgLRFZIt) /= frameSize;
        if (boost::math::isnan((*avgLRFXIt))) { debug_print(LVL_LOW, "*avgLRFXIt isn't a number = %f\n", *avgLRFXIt); }
        if (boost::math::isnan((*avgLRFYIt))) { debug_print(LVL_LOW, "*avgLRFYIt isn't a number = %f\n", *avgLRFYIt); }
        if (boost::math::isnan((*avgLRFZIt))) { debug_print(LVL_LOW, "*avgLRFZIt isn't a number = %f\n", *avgLRFZIt); }
    }

    /**
     * Translate vector.
     */
    createBeginIterators(frames, XChannelsIt, YChannelsIt, ZChannelsIt);
    expNo = 0;
    for (auto XChannelsDataIt = XChannelsIt.begin(), YChannelsDataIt = YChannelsIt.begin(), ZChannelsDataIt = ZChannelsIt.begin();
            XChannelsDataIt != XChannelsIt.end();
            XChannelsDataIt++, YChannelsDataIt++, ZChannelsDataIt++, expNo++)
    {
        auto & XValueIt = *XChannelsDataIt, & YValueIt = *YChannelsDataIt, & ZValueIt = *ZChannelsDataIt;
        auto expLumin = avgLuminocityRelativeFractionX[expNo];
        auto frameRelEV = framesRelativeEVComputed[expNo];
        auto pixelFraction = expLumin + frameRelEV;
        if (boost::math::isnan(pixelFraction)) { debug_print(LVL_LOW, "pixelFraction isn't a number = %f\n", pixelFraction); }
        for (size_t pixel = 0; pixel < frameSize;
                ++pixel, ++XValueIt, ++YValueIt, ++ZValueIt)
        {
            auto & XValue = *XValueIt, YValue = *YValueIt, ZValue = *ZValueIt;
            XValue += pixelFraction;
            YValue += pixelFraction;
            ZValue += pixelFraction;
        }
    }


    // Create HDR Image
    int it = 0;
    createBeginIterators(frames, XChannelsIt, YChannelsIt, ZChannelsIt);
    auto XChannelsDataIt = XChannelsIt.begin(), YChannelsDataIt = YChannelsIt.begin(), ZChannelsDataIt = ZChannelsIt.begin();
    float minXV = 1000, minYV = 1000, minZV = 1000;
    float maxXV = -1, maxYV = -1, maxZV = -1;
    pfs::Channel *X, *Y, *Z;
    outputFrame->createXYZChannels(X, Y, Z);
    endX = X->end();
    for(getChannelsIterators(X, Y, Z, itX, itY, itZ); itX != endX;
            ++itX, ++itY, ++itZ, it++) {
        auto & XOutputVal = *itX, & YOutputVal = *itY, & ZOutputVal = *itZ;
        XOutputVal = 0, YOutputVal = 0, ZOutputVal = 0;
        XChannelsDataIt = XChannelsIt.begin(), YChannelsDataIt = YChannelsIt.begin(), ZChannelsDataIt = ZChannelsIt.begin();
        for (int expNo = 0; expNo < noOfExposures; XChannelsDataIt++, YChannelsDataIt++, ZChannelsDataIt++, expNo++)
        {
            auto & XValueIt = *XChannelsDataIt, & YValueIt = *YChannelsDataIt, & ZValueIt = *ZChannelsDataIt;
            auto XValue = *XValueIt, YValue = *YValueIt, ZValue = *ZValueIt;

            XOutputVal += XValue;
            YOutputVal += YValue;
            ZOutputVal += ZValue;

            ++XValueIt;
            ++YValueIt;
            ++ZValueIt;
        }
        if (minXV > XOutputVal) minXV = XOutputVal;
        if (minYV > YOutputVal) minYV = YOutputVal;
        if (minZV > ZOutputVal) minZV = ZOutputVal;
        if (maxXV < XOutputVal) maxXV = XOutputVal;
        if (maxYV < YOutputVal) maxYV = YOutputVal;
        if (maxZV < ZOutputVal) maxZV = ZOutputVal;
        //debug_print(LVL_LOW, "X = %f, Y = %f, Z = %f,\n", XOutputVal, YOutputVal, ZOutputVal);
        if (boost::math::isnan(XOutputVal)) { debug_print(LVL_LOW, "XOutputVal isn't a number = %f\n", XOutputVal); }
        if (boost::math::isnan(YOutputVal)) { debug_print(LVL_LOW, "YOutputVal isn't a number = %f\n", YOutputVal); }
        if (boost::math::isnan(ZOutputVal)) { debug_print(LVL_LOW, "ZOutputVal isn't a number = %f\n", ZOutputVal); }
    }

    float rangeX = abs(maxXV - minXV), rangeY = abs(maxYV - minYV), rangeZ = abs(maxZV - minZV);
    debug_print(LVL_DEBUG, "rangeX=(%f - %f) rangeY=(%f - %f) rangeZ=(%f - %f)\n", minXV, maxXV, minYV, maxYV, minZV, maxZV);
    long nanX = 0, nanY = 0, nanZ = 0;

    // Normalize output (TONE MAP)
    for(getChannelsIterators(outputFrame, itX, itY, itZ, endX); itX != endX;
            ++itX, ++itY, ++itZ) {
        auto & XOutputVal = *itX, & YOutputVal = *itY, & ZOutputVal = *itZ;
        XOutputVal = (XOutputVal - minXV) / rangeX;
        YOutputVal = (YOutputVal - minYV) / rangeY;
        ZOutputVal = (ZOutputVal - minZV) / rangeZ;

        if (boost::math::isnan(XOutputVal)) {
            XOutputVal = 0;
            nanX++;
        }
        if (boost::math::isnan(YOutputVal)) {
            YOutputVal = 0;
            nanY++;
        }
        if (boost::math::isnan(ZOutputVal)) {
            ZOutputVal = 0;
            nanZ++;
        }
    }
    debug_print(LVL_DEBUG, "NanX = %f, NanY = %f, NanZ = %f\n", (float)nanX / (float)frameSize,(float)nanY / (float)frameSize,(float)nanZ / (float)frameSize);

    // Create output.
    return true; // XXX
}

} /* namespace HDRCreation */
