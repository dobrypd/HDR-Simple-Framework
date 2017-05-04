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
#include <gtest/gtest.h>
#include <boost/assert.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "kernel/GenericFrame.hpp"
#include "testArgs.hpp"

using namespace std;
using namespace kernel;
using namespace cv;

class FrameTestCase: public ::testing::Test
{
public:
    FrameTestCase()
            : gf(argsHDR)
    {
    }
protected:
    virtual void SetUp()
    {
        gf.getFrameFromFile(argsHDR.inputFiles[0]);
        frame = gf.getRawFrame();
    }

    // virtual void TearDown() {}

    GenericFrame gf;
    Mat frame;
};

TEST_F(FrameTestCase, FrameCreateNew)
{
    ASSERT_FALSE(frame.empty());
}

TEST(ExposureValueCase, TestShutterSpeedChange)
{
    double eps = 0.1;
    ExposureValue ev(0);
    cout << ev << endl;

    ev.setShutterSpeed(2);
    cout << ev << endl;
    EXPECT_NEAR(-1, ev.get(), eps);

    ev.setShutterSpeed(0.5);
    cout << ev << endl;
    EXPECT_NEAR(1, ev.get(), eps);

    ev.setShutterSpeed((float)1/8000);
    cout << ev << endl;
    EXPECT_NEAR(13, ev.get(), eps);

    ev.setShutterSpeed(60);
    cout << ev << endl;
    EXPECT_NEAR(-6, ev.get(), eps);
}

TEST(ExposureValueCase, TestApertureSpeedChange)
{
    double eps = 0.1;
    ExposureValue ev(0);
    cout << ev << endl;

    ev.setAperture(0.7071067811865475); // = pow(sqrt(2), -1)
    cout << ev << endl;
    EXPECT_NEAR(-1, ev.get(), eps);

    ev.setAperture(1.4142135623730951); // = pow(sqrt(2), 1)
    cout << ev << endl;
    EXPECT_NEAR(1, ev.get(), eps);

    ev.setAperture(90.50966799187816); // = pow(sqrt(2), 13)
    cout << ev << endl;
    EXPECT_NEAR(13, ev.get(), eps);

    ev.setAperture(0.12499999999999994); // = pow(sqrt(2), -6)
    cout << ev << endl;
    EXPECT_NEAR(-6, ev.get(), eps);
}

TEST(ExposureValueCase, TestISO)
{
    double eps = 0.1;
    ExposureValue ev(0);
    cout << ev << endl;

    ev.setISO(200);
    cout << ev << endl;
    EXPECT_NEAR(-1, ev.get(), eps);

    ev.setISO(50);
    cout << ev << endl;
    EXPECT_NEAR(1, ev.get(), eps);

    ev.setISO(100 * pow(2, -13));
    cout << ev << endl;
    EXPECT_NEAR(13, ev.get(), eps);

    ev.setISO(100 * pow(2, 6));
    cout << ev << endl;
    EXPECT_NEAR(-6, ev.get(), eps);
}


TEST(ExposureValueCase, TestAllSplet)
{
    double eps = 0.1;
    ExposureValue ev(0);
    cout << ev << endl;

    ev.setISO(100);
    ev.setAperture(2.0);
    EXPECT_NEAR(2, ev.get(), eps);
    ev.setShutterSpeed(15.0);
    EXPECT_NEAR(-2, ev.get(), eps);
    ev.setAperture(11.0);
    EXPECT_NEAR(3, ev.get(), eps);
    ev.setAperture(8.0);
    EXPECT_NEAR(2, ev.get(), eps);
    ev.setShutterSpeed(0.5);
    cout << ev << endl;
    EXPECT_NEAR(7, ev.get(), eps);
}

TEST(ExposureValueCase, TestPrograms)
{
    double eps = 1e-6;
    ExposureValue ev(0);
    cout << ev << endl;
    ev.setExpVal(1, ExposureValue::Speed);
    cout << ev << endl;
    EXPECT_NEAR(1, ev.get(), eps);
    ev.setExpVal(2, ExposureValue::Aperture);
    cout << ev << endl;
    EXPECT_NEAR(2, ev.get(), eps);
    ev.setExpVal(3, ExposureValue::ISO);
    cout << ev << endl;
    EXPECT_NEAR(3, ev.get(), eps);
    ev.setExpVal(4, ExposureValue::Auto);
    cout << ev << endl;
    EXPECT_NEAR(4, ev.get(), eps);
    ev.setExpVal(4, ExposureValue::ISO);
    cout << ev << endl;
    EXPECT_NEAR(4, ev.get(), eps);
    ev.setExpVal(3.5, ExposureValue::Aperture);
    cout << ev << endl;
    EXPECT_NEAR(3.5, ev.get(), eps);
    ev.setExpVal(3.3, ExposureValue::Speed);
    cout << ev << endl;
    EXPECT_NEAR(3.3, ev.get(), eps);
    ev.setExpVal(2, ExposureValue::Auto);
    cout << ev << endl;
    EXPECT_NEAR(2, ev.get(), eps);
    ev.setExpVal(-1.5, ExposureValue::ISO);
    cout << ev << endl;
    EXPECT_NEAR(-1.5, ev.get(), eps);
    ev.setExpVal(0, ExposureValue::Aperture);
    cout << ev << endl;
    EXPECT_NEAR(0, ev.get(), eps);

}
