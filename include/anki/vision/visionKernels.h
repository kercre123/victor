#ifndef _ANKICORETECH_VISION_VISIONKERNELS_H_
#define _ANKICORETECH_VISION_VISIONKERNELS_H_

#include "anki/vision/config.h"
#include "anki/common.h"

Anki::Result BinomialFilter(const Anki::Matrix<u8> &img, Anki::Matrix<u8> &filteredImg, Anki::MemoryStack scratch);

#endif //_ANKICORETECH_VISION_VISIONKERNELS_H_
