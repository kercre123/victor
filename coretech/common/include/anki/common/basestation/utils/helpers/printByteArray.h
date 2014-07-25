//
//  printByteArray.h
//
//  Created by Kevin Yoon on 7/24/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef CORETECH_BASESTATION_UTIL_PRINT_BYTE_ARRAY
#define CORETECH_BASESTATION_UTIL_PRINT_BYTE_ARRAY

namespace Anki {
  
  // Print each byte in hex
  void PrintBytesHex(const char* bytes, int num_bytes);
  
  // Print each byte as unsigned int
  void PrintBytesUInt(const char* bytes, int num_bytes);
}

#endif // CORETECH_BASESTATION_UTIL_PRINT_BYTE_ARRAY
