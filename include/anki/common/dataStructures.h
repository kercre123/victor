//
//  dataStructures.h
//  CoreTech_Common
//
//  Created by Peter Barnum on 8/7/13.
//  Copyright (c) 2013 Peter Barnum. All rights reserved.
//

#ifndef _ANKICORETECH_COMMON_DATASTRUCTURES_H_
#define _ANKICORETECH_COMMON_DATASTRUCTURES_H_

#include "anki/common/config.h"

namespace Anki
{
  // Return values:
  typedef enum Result_ {
    RESULT_OK = 0,
    RESULT_FAIL = 1,
    RESULT_FAIL_MEMORY = 10000,
    RESULT_FAIL_OUT_OF_MEMORY = 10001,
    RESULT_FAIL_IO = 20000,
    RESULT_FAIL_INVALID_PARAMETERS = 30000
  } Result;

} // namespace Anki

#endif // #ifndef _ANKICORETECH_COMMON_DATASTRUCTURES_H_
