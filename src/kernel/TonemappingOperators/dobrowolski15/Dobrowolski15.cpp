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
#include "Dobrowolski15.hpp"

using namespace cv;

namespace TMO
{

Dobrowolski15::Dobrowolski15(const GlobalArgs_t & globalArgs)
        : globalArgs(globalArgs)
{
    // TODO Auto-generated constructor stub

}

bool Dobrowolski15::create(kernel::GenericFramePtr outputF, kernel::GenericFramePtr frame)
{
    debug_puts("Will tonemap new frame.\n");

    if (outputF == 0)
    {
        outputF = kernel::GenericFramePtr(new kernel::GenericFrame(globalArgs));
    }
    if (!outputF->isValid())
    {
        if (frame->isValid())
        {
            outputF->setEmptyFrameFrom(frame->getRawFrame(), frame->getColorSpace());
        }
        else
        {
            return false;
        }
    }
    assert(outputF->isValid());

    Mat output = frame->getRawFrame();

    outputF->assignFrameTo(output, frame->getColorSpace());
    outputF->convertToColorSpace(kernel::GenericFrame::COLOR_BGR);
    outputF->convertToDepth(CV_8UC3); // 16 in future
    return true;
}

} /* namespace TMO */
