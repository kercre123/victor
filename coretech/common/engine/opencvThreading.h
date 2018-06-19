/**
 * File: opencvThreading.h
 *
 * Author: Humphrey Hu
 * Date:   6/15/2018
 *
 * Description: Wrappers for interfacing with OpenCV's parallelism/threading
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef ANKI_CORETECH_COMMON_OPENCV_THREADING_H_
#define ANKI_CORETECH_COMMON_OPENCV_THREADING_H_

#include "coretech/common/shared/types.h"
#include <string>

namespace Anki
{

// Tells OpenCV to use the specified number of threads in addition to the main thread
// Note that 0 means to use sequential operations only
Result SetNumOpencvThreads( unsigned int n, const std::string& debugName );

// Returns the number of threads OpenCV is allowed to use, including the main thread
unsigned int GetNumOpencvThreads();

} // namespace Anki

#endif // ANKI_CORETECH_COMMON_OPENCV_THREADING_H_