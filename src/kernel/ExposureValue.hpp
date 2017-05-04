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
#ifndef EXPOSUREVALUE_HPP_
#define EXPOSUREVALUE_HPP_

#include <ostream>
#include <string>

namespace kernel
{

/*
 *
 */
class ExposureValue
{
public:
    enum Program {Auto, Speed, Aperture, ISO};

    ExposureValue(float value);
    ExposureValue(float value, Program p);
    ExposureValue(float speed, float aperture, float iso);
    ExposureValue(float value, float speed, float aperture, float iso);

    void setExpVal(float value, Program p);
    void setShutterSpeed(float value);
    void setAperture(float aperture);
    void setISO(float iso);

    void setFromExif(const std::string & filename);

    float get() const;

    friend std::ostream & operator<<(std::ostream &, const ExposureValue &);

private:
    float expVal;

    float shutterSpeed;
    float aperture;
    float iso;
};

/*
 *
 */
std::ostream & operator<<(std::ostream & os, const ExposureValue & v);

} /* namespace kernel */

#endif /* EXPOSUREVALUE_HPP_ */
