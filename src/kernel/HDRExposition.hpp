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
#ifndef HDREXPOSITION_HPP_
#define HDREXPOSITION_HPP_

#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/shared_ptr.hpp>

#include <config.h>

namespace kernel
{

// Declaration
template<typename PixelType, typename PartitionType, typename PartitionShift>
class Partition;

//*************************************************************************************************
/**
 *
 * This is an abstraction of operation on multiple exposures.
 * While processing HDR expositions (not joined yet) every
 * frame will be preprocessed, then operation will be applied
 * at the end postprocess will be executed.
 *
 * All classes in this file will which inherit from this abstraction
 * will create new abstraction for different kinds of Operator abstractions.
 *
 */
template<typename PixelType>
class Operation
{
public:
    virtual ~Operation()
    {
    }

    /**
     * Preprocess every image in HDR exposition before application.
     */
    virtual cv::Mat& preprocess(cv::Mat& m)
    {
        return m;
    }
    /**
     * Postprocess every image in HDR exposition after application.
     */
    virtual cv::Mat& postprocess(cv::Mat& m)
    {
        return m;
    }

    /**
     * Apply an operation to a set of input images.
     * Remember that most of operators will override output,
     * which means that only last operator will be able to create
     * proper output.
     */
    virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs) = 0;

    /**
     * Global matrix function application to every pixel.
     */
    template<typename F>
    cv::Mat & globalOperation(cv::Mat & m, F f)
    {
        std::for_each(m.begin<PixelType>(), m.end<PixelType>(), [&f](PixelType & v)
        {
            v = f(v);
        });
        return m;
    }
};
//*************************************************************************************************

//*************************************************************************************************
/**
 * ProcessingOperation is an abstraction of Operation {@see Operation}
 * where every pixel in image will be processed at some time.
 */
template<typename PixelType>
class ProcessingOperation: public Operation<PixelType>
{
public:
    virtual ~ProcessingOperation()
    {
    }

    /**
     * Processing operators will work on pixel array. Pixels of all exposition.
     * Processing mechanism will start at the cv::Mat.begin<_>() and will
     * iterate with "++" operator to finish just before cv::Mat.end<_>().
     */
    virtual PixelType process(PixelType[], unsigned int exps) = 0;
};

/**
 * Global operator is processing pixels without previous assumption.
 */
template<typename PixelType>
class GlobalOperation: public ProcessingOperation<PixelType>
{
public:
    virtual ~GlobalOperation()
    {
    }

    virtual PixelType process(PixelType[], unsigned int exps) = 0;

    virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs);
};

/**
 * Local operator is processing pixels with use of declared partitions.
 *
 * @method process, @method enterArea and @method leaveArea has to be implemented
 * to use this abstraction.
 *
 * Area with @field ident will be entered only once!
 */
template<typename PixelType, typename PartitionType, typename PartitionShift>
class LocalOperation: public ProcessingOperation<PixelType>
{
private:
    Partition<PixelType, PartitionType, PartitionShift> & partitions;
protected:
    PartitionType size() {
        return partitions.size();
    }
public:
    /**
     * Local operator needs partitions which were created by
     * executing implementation of Partition class.
     * Create LocalOperator with Partition.getPartition(), even if partitions aren't
     * created yet.
     */
    LocalOperation(Partition<PixelType, PartitionType, PartitionShift> & partitions)
            : partitions(partitions)
    {
    }
    virtual ~LocalOperation()
    {
    }

    virtual PixelType process(PixelType[], unsigned int exps) = 0;
    virtual void enterArea(unsigned char ident) = 0;
    virtual void leaveArea(unsigned char ident) = 0;

    virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs);
};
//*************************************************************************************************

//*************************************************************************************************
/**
 * Preprocess operator is an abstraction of processor on set of input matrices.
 */
template<typename PixelType>
class Preprocess: public Operation<PixelType>
{
public:
    virtual ~Preprocess()
    {
    }

    /**
     * Implement preprocess.
     */
    virtual cv::Mat& preprocess(cv::Mat& m) = 0;

    virtual void apply(cv::Mat & output, std::vector<cv::Mat> & inputs)
    {
        apply(inputs);
    }
    /**
     * Maybe implemented but not required
     */
    virtual void apply(std::vector<cv::Mat> & inputs)
    {
    }
};

/**
 * Partition is a image partition creator. This abstraction is
 * required for @see LocalOperation operator.
 */
template<typename PixelType, typename PartitionType, typename PartitionShift>
class Partition: public Preprocess<PixelType>
{
protected:
    // partition -> is vector of jumps in Mat (starting in Mat::begin())
    // example: for input where { A, B, B, A, B, A, A, A, B, B, B, A }
    // partition A              { 0,       3,    2, 1, 1,        , 4 }
    // partition B              {    1, 1,    2,          4, 1, 1    }
    typedef std::vector<std::pair<PartitionType, std::vector<PartitionShift>>>partition_t;
    partition_t partitions;

    PartitionType noOfPartitions = 0;    // btw. index is in range [0, noOfPartitions)

public:
    template<typename AppliedMatPixelType>
    class PartitionIterator : public std::iterator<std::forward_iterator_tag, AppliedMatPixelType>
    {
    private:
        cv::MatIterator_<AppliedMatPixelType> matIt;
        typedef typename std::vector<PartitionShift>::const_iterator vecIterator;
        vecIterator partitionIt;
    public:
        PartitionIterator(cv::MatIterator_<AppliedMatPixelType> matIt, vecIterator partitionIt)
        : matIt(matIt), partitionIt(partitionIt)
        {}
        void swap(PartitionIterator& other) noexcept
        {
            using std::swap;
            swap(matIt, other.matIt);
            swap(partitionIt, other.partitionIt);
        }
        PartitionIterator& operator++ () // Pre-increment
        {
            matIt += *partitionIt;
            partitionIt++;
            return *this;
        }
        PartitionIterator operator++ (int) // Post-increment
        {
            PartitionIterator tmp(*this);
            matIt += *partitionIt;
            partitionIt++;
            return tmp;
        }
        /** Well, it can work little bit unexpectly,
         * basically this same partition iterators (on this same position)
         * can be equal on different working matrix.
         */
        template<class OtherType>
        bool operator == (const PartitionIterator<OtherType>& rhs) const
        {
            return partitionIt == rhs.partitionIt;
        }
        template<class OtherType>
        bool operator != (const PartitionIterator<OtherType>& rhs) const
        {
            return partitionIt != rhs.partitionIt;
        }
        AppliedMatPixelType& operator* () const
        {
            return *matIt;
        }
    };

    virtual ~Partition()
    {}
    /**
     * Fill @field partitions in implementation.
     */
    virtual void apply(std::vector<cv::Mat> & inputs) = 0;

    template<typename AppliedMatPixelType>
    std::vector<PartitionIterator<AppliedMatPixelType>> listOfBegins(cv::Mat & m)
    {
        std::vector<PartitionIterator<AppliedMatPixelType>> l;
        std::for_each(partitions.begin(), partitions.end(),
                [&m, &l](std::pair<PartitionType, std::vector<PartitionShift>> & p)
                {
                    l.push_back(PartitionIterator<AppliedMatPixelType>(m.begin<AppliedMatPixelType>(), p.second.begin()));
                });
        return l;
    }

    template<typename AppliedMatPixelType>
    std::vector<PartitionIterator<AppliedMatPixelType>> listOfEnds(cv::Mat & m)
    {
        std::vector<PartitionIterator<AppliedMatPixelType>> l;
        std::for_each(partitions.begin(), partitions.end(),
                [&m, &l](std::pair<PartitionType, std::vector<PartitionShift>> & p)
                {
                    l.push_back(PartitionIterator<AppliedMatPixelType>(m.end<AppliedMatPixelType>(), p.second.end()));
                });
        return l;
    }

    template<typename AppliedMatPixelType>
    PartitionIterator<AppliedMatPixelType> begin(cv::Mat & m, PartitionType id)
    {
        typedef typename partition_t::iterator iterator;
        iterator it = std::find_if(
                partitions.begin(), partitions.end(),
                [&id](std::pair<PartitionType, std::vector<PartitionShift>> & p)
                {
                    return (id == p.first);
                });
        if (it != partitions.end())
        {
            return PartitionIterator<AppliedMatPixelType>(m.begin<AppliedMatPixelType>(), it->second.begin());
        }
        debug_print(LVL_ERROR, "Cannot find partition id %d\n", id);
        return PartitionIterator<AppliedMatPixelType>(m.begin<AppliedMatPixelType>(), std::vector<PartitionShift>().begin());
    }

    template<typename AppliedMatPixelType>
    PartitionIterator<AppliedMatPixelType> end(cv::Mat & m, PartitionType id)
    {
        typedef typename partition_t::iterator iterator;
        iterator it = std::find_if(
                partitions.begin(), partitions.end(),
                [&id](std::pair<PartitionType, std::vector<PartitionShift>> & p)
                {
                    return (id == p.first);
                });
        if (it != partitions.end())
        {
            return PartitionIterator<AppliedMatPixelType>(m.end<AppliedMatPixelType>(), it->second.end());
        }
        debug_print(LVL_ERROR, "Cannot find partition id %d\n", id);
        return PartitionIterator<AppliedMatPixelType>(m.end<AppliedMatPixelType>(), std::vector<PartitionShift>().end());
    }

    PartitionType size()
    {
        return noOfPartitions;
    }

    size_t size(PartitionType id)
    {
        typedef typename partition_t::iterator iterator;
        iterator it = std::find_if(
                partitions.begin(), partitions.end(),
                [&id](std::pair<PartitionType, std::vector<PartitionShift>> & p)
                {
                    return (id == p.first);
                });
        if (it != partitions.end())
        {
            return it->second.size();
        }
        debug_print(LVL_DEBUG, "Cannot find id %d\n", id);
        return 0;
    }
}
;
//*************************************************************************************************

//*************************************************************************************************
/**
 * HDRExposition is a base class for HDR processing with Operator(s).
 */
template<typename PixelType>
class HDRExposition
{
private:
    cv::Mat & output;
    std::vector<cv::Mat> inputs;
    std::vector<std::reference_wrapper<Operation<PixelType>>>operations;

public:
    HDRExposition(cv::Mat& output, std::vector<cv::Mat> inputs);

    HDRExposition<PixelType>& addOperation(Operation<PixelType> &);

    cv::Mat& process();
};

}
/* namespace kernel */

#endif /* HDREXPOSITION_HPP_ */
