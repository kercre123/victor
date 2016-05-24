/**
 * File: CConversionHelpers
 *
 * Author: Mark Wesley
 * Created: 10/14/2014
 *
 * Description: Helper functions for more easily copying types out (pure C interface)
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Util_Helpers_CConversionHelpers_H__
#define __Util_Helpers_CConversionHelpers_H__


#include "util/export/export.h"
#include "stdint.h"
#include "stddef.h"


ANKI_C_BEGIN


ANKI_EXPORT uint32_t   AnkiGetStringLength(const char* inString);
ANKI_EXPORT uint32_t   AnkiCopyStringIntoOutBuffer(const char* inString, char* outBuffer, uint32_t outBufferLen);


ANKI_C_END



#endif // #ifndef __Util_Helpers_CConversionHelpers_H__

