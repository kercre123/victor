/**
 * File: opencvThreading.cpp
 *
 * Author: Humphrey Hu
 * Date:   6/15/2018
 *
 * Description: Wrappers for interfacing with OpenCV's parallelism/threading
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/opencvThreading.h"
#include "util/logging/logging.h"

#include "opencv2/core.hpp"

namespace Anki
{

Result SetNumOpencvThreads( unsigned int n, const std::string& debugName )
{
    PRINT_NAMED_INFO( (debugName + ".SetOpenCvThreading").c_str(),
                      "Setting %d threads for OpenCV", n );
    cv::setNumThreads( n );
    const int expectedThreads = (n == 0) ? 1 : n;
    if( cv::getNumThreads() != expectedThreads )
    {
      PRINT_NAMED_ERROR( (debugName + ".SetOpenCvThreadingError").c_str(),
                        "Expected %d but got %d threads!", expectedThreads, cv::getNumThreads());
      return RESULT_FAIL;
    }
    return RESULT_OK;
}

unsigned int GetNumOpencvThreads()
{
  return cv::getNumThreads();
}

} // namespace Anki
