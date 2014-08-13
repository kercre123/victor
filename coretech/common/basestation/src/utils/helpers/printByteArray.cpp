//
//  printByteArray.cpp
//
//  Created by Kevin Yoon on 7/24/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include <stdio.h>

namespace Anki {
  
  void PrintBytesHex(const char* bytes, int num_bytes)
  {
    for (int i=0; i<num_bytes;i++){
      printf("0x%x ", (unsigned char)bytes[i]);
    }
  }
  
  void PrintBytesUInt(const char* bytes, int num_bytes)
  {
    for (int i=0; i<num_bytes;i++){
      printf("%u ", (unsigned char)bytes[i]);
    }
  }

  
}

