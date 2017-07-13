//
//  UUID.c
//  RushHour
//
//  Created by Mark Pauley on 8/10/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "UUID.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


const UUIDBytesRef UUIDEmpty() {
  static UUIDBytes emptyUUID = {{0}};
  static UUIDBytes returnedUUID;
  
  // just a touch of paranoia.
  UUIDCopy(&returnedUUID, &emptyUUID);
  return &returnedUUID;
}

int UUIDBytesFromString(UUIDBytesRef uuid, const char* uuidStr) {
  unsigned char* curByte;
  int result;

  memset(uuid, 0, sizeof(UUIDBytes));
  
  curByte = &uuid->bytes[0];
  int n = 0;
  result = sscanf(uuidStr, "%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%n",
                  &curByte[0], &curByte[1], &curByte[2], &curByte[3], &curByte[4], &curByte[5], &curByte[6], &curByte[7],
                  &curByte[8], &curByte[9], &curByte[10], &curByte[11], &curByte[12], &curByte[13], &curByte[14], &curByte[15], &n);
  return (n != 36 || uuidStr[n] != '\0' || result != 16) ? -1 : 0;
}

int IsValidUUIDString(const char* uuidStr) {
  UUIDBytes uuid;
  return (0 == UUIDBytesFromString(&uuid, uuidStr));
}


static void StringToUpper(unsigned char* str) {
  while(*str) {
    *str = toupper(*str) & 0xff;
    str++;
  }
}

const char* StringFromUUIDBytes(const UUIDBytesRef uuid) {
  static char buffer[37];
  memset(buffer, 0, 37);
  
  const unsigned char* bytes = &uuid->bytes[0];
  sprintf(buffer, "%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
          bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
  
  if (strlen(buffer) != 36)
    return NULL;
  
  StringToUpper((unsigned char*)buffer);
  return buffer;
}

int UUIDCompare(const UUIDBytesRef first, const UUIDBytesRef second) {
  return memcmp(first, second, sizeof(UUIDBytes));
}

void UUIDCopy(UUIDBytesRef destination, const UUIDBytesRef source) {
  if(destination != source) {
    memcpy(destination, source, sizeof(UUIDBytes));
  }
}

