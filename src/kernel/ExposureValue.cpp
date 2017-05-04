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
#include "ExposureValue.hpp"

#include <cmath>
#include <string>
#include <ostream>

#include <exiv2/exiv2.hpp>

namespace kernel
{

ExposureValue::ExposureValue(float value)
        : ExposureValue(value, Auto)
{
}

ExposureValue::ExposureValue(float value, Program p)
{
    expVal = 0;
    shutterSpeed = 1;
    aperture = 1;
    iso = 100;
    setExpVal(value, p);
}

ExposureValue::ExposureValue(float speed, float aperture, float iso)
        : ExposureValue(0)
{
    setShutterSpeed(speed);
    setAperture(aperture);
    setISO(iso);
}

ExposureValue::ExposureValue(float value, float speed, float aperture, float iso)
        : expVal(value), shutterSpeed(speed), aperture(aperture), iso(iso)
{
}

void ExposureValue::setExpVal(float value, Program p)
{
    switch (p)
    {
        default:
        case Auto:
            // FUTURE: not that important.
            setShutterSpeed(shutterSpeed * pow(2, expVal - value));
        break;
        case Speed:
            setShutterSpeed(shutterSpeed * pow(2, expVal - value));
        break;
        case Aperture:
            setAperture(aperture * pow(sqrt(2), value - expVal));
        break;
        case ISO:
            setISO(iso * pow(2, expVal - value));
        break;
    }
}

void ExposureValue::setShutterSpeed(float value)
{
    expVal += log2(shutterSpeed / value);
    shutterSpeed = value;
}

void ExposureValue::setAperture(float aperture)
{
    expVal -= 2 * (log2(this->aperture / aperture));
    this->aperture = aperture;
}

void ExposureValue::setISO(float iso)
{
    expVal -= log2(iso / this->iso);
    this->iso = iso;
}

// From Libpfs
void ExposureValue::setFromExif(const std::string& filename)
{
    typedef ::Exiv2::Image::AutoPtr ImgPtr;
    typedef ::Exiv2::ExifData ExifData;
    typedef ::Exiv2::ExifData::const_iterator DataIterator;
    try
    {
        ImgPtr image = Exiv2::ImageFactory::open(filename);
        image->readMetadata();
        ExifData &exifData = image->exifData();

        if (exifData.empty()) return;

        DataIterator it = exifData.end();
        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"))) != exifData.end())
        {
            setShutterSpeed(it->toFloat());
        }
        else if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue")))
                != exifData.end())
        {
            long num = 1;
            long div = 1;
            float tmp = std::exp(std::log(2.0f) * it->toFloat());
            if (tmp > 1)
            {
                div = static_cast<long>(tmp + 0.5f);
            }
            else
            {
                num = static_cast<long>(1.0f / tmp + 0.5f);
            }
            setShutterSpeed(static_cast<float>(num) / div);
        }

        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"))) != exifData.end())
        {
            setAperture(it->toFloat());
        }
        else if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ApertureValue")))
                != exifData.end())
        {
            setAperture(static_cast<float>(expf(logf(2.0f) * it->toFloat() / 2.f)));
        }

        if ((it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"))) != exifData.end())
        {
            setISO(it->toFloat());
        }
    }
    catch (Exiv2::AnyError& e)
    {
        return;
    }
}

float ExposureValue::get() const
{
    return expVal;
}

std::ostream & operator<<(std::ostream & os, const ExposureValue & v)
{
    os << "EV=" << v.expVal << ", (" << v.shutterSpeed << "s, f/" << v.aperture << ", iso" << v.iso
            << ")";
    return os;
}

} /* namespace kernel */
