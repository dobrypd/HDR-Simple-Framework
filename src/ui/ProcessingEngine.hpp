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

#ifndef PROCESSINGENGINE_H_
#define PROCESSINGENGINE_H_

#include "config.h"
#include <boost/filesystem.hpp>
#include <string>
using std::string;

namespace ui
{

/*
 *
 */
class ProcessingEngine
{
protected:
    static const char * tmp_root;

    const GlobalArgs_t & globalArgs;

    boost::filesystem::path temp_path;

    const boost::filesystem::path & create_TMP();

public:
    explicit ProcessingEngine(const GlobalArgs_t & globalArgs);
    virtual ~ProcessingEngine();

    virtual void process();
};

class ProcessingHDRCreatorModel: public ProcessingEngine
{
private:
    typedef ProcessingEngine super;

public:
    explicit ProcessingHDRCreatorModel(const GlobalArgs_t & globalArgs);

    virtual void process();
};

class ProcessingToneMapper: public ProcessingEngine
{
private:
    typedef ProcessingEngine super;

public:
    explicit ProcessingToneMapper(const GlobalArgs_t & globalArgs);

    virtual void process();
};

class ProcessingHDRCreatorAndToneMapper: public ProcessingEngine
{
private:
    typedef ProcessingEngine super;

public:
    explicit ProcessingHDRCreatorAndToneMapper(const GlobalArgs_t & globalArgs);

    virtual void process();
};

} /* namespace ui */
#endif /* PROCESSINGENGINE_H_ */
