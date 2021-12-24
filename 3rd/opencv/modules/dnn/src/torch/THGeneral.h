#ifndef TH_GENERAL_INC
#define TH_GENERAL_INC

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TH_API

#define THError(...) CV_Error(cv::Error::StsError, cv::format(__VA_ARGS__))
#define THArgCheck(cond, ...) CV_Assert(cond)

#define THAlloc malloc
#define THRealloc realloc
#define THFree free

#endif
