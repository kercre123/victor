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
  
  // Format each byte as hex in the given output buffer
  void FormatBytesAsHex(const char* bytes, int num_bytes, char* output, int maxOutputSize);
}

#endif // CORETECH_BASESTATION_UTIL_PRINT_BYTE_ARRAY
