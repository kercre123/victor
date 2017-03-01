//
//  printByteArray.cpp
//
//  Created by Kevin Yoon on 7/24/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "util/helpers/printByteArray.h"
#include "util/logging/logging.h"
#include <stdio.h>

namespace Anki {
  
  void PrintBytesHex(const char* bytes, int num_bytes)
  {
    for (int i=0; i<num_bytes;i++){
      printf("%02x", (unsigned char)bytes[i]);
    }
  }
  
  void PrintBytesUInt(const char* bytes, int num_bytes)
  {
    for (int i=0; i<num_bytes;i++){
      printf("%u ", (unsigned char)bytes[i]);
    }
  }
  
  void FormatBytesAsHex(const char* bytes, int num_bytes, char* output, int maxOutputSize)
  {
    if (output == nullptr) {
      PRINT_NAMED_WARNING("FormatBytesAsHex.NoOutputBufferProvided","");
      return;
    }
    
    if (maxOutputSize <= 2*num_bytes) {
      PRINT_NAMED_WARNING("FormatBytesAsHex.OutputBufferIsTooSmall","maxoutputSize (%d) must be greater than 2 x num_bytes (%d)", maxOutputSize, num_bytes);
      return;
    }
    
    for (int i=0; i<num_bytes;i++){
      sprintf(output + (i*2), "%02x", (unsigned char)bytes[i]);
    }
  }
  
}

