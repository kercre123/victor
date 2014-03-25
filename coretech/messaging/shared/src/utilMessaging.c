/**
 * File: utilMessaging.c
 *
 * Author: Mark Palatucci (markmp)
 * Created: 11/29/2008
 *
 *
 * Description: Utility functions for message communication.
 *
 * Copyright: Anki, Inc. 2008-2011
 *
 **/
 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
//#include <arpa/inet.h>
#include <string.h>
#include "anki/messaging/shared/utilMessaging.h"
#include <assert.h>
#include <limits.h>

#ifndef UINT8_MAX
#define UINT8_MAX UCHAR_MAX
#endif

void* WriteU64(void *dest, unsigned long long src) {
  memcpy((char *) dest, (char*)&src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Writes a uint32 to the dest buffer after converting to network byte order.
// Returns the pointer incremented by the number of bytes written.
/*
 void * WriteNetU32(void *dest, unsigned int src)
 {
 unsigned int hostInt = htonl(src);
 memcpy((char *)dest, (char*) &hostInt, sizeof(hostInt));
 return((char *)dest + sizeof(hostInt));
 }
 */

// Writes a uint32 to the dest buffer. Does not convert to network byte order.
// Returns the pointer incremented by the number of bytes written.
void * WriteU32(void *dest, unsigned int src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Writes a uint16 to the dest buffer after converting to network byte order.
// Returns the pointer incremented by the number of bytes written.
/* Unused
 void * WriteNetU16(void *dest, unsigned short src)
 {
 unsigned short hostInt = htons(src);
 memcpy((char *)dest, (char*) &hostInt, sizeof(hostInt));
 return((char *)dest + sizeof(hostInt));
 }
 */

// Writes a uint16 to the dest buffer. Does not convert to network byte order.
// Returns the pointer incremented by the number of bytes written.
void * WriteU16(void *dest, unsigned short src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Writes a char to the dest buffer. Returns the pointer incremented
// by the number of bytes written.
void * WriteChar(void *dest, unsigned char src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Do not marshal the float values into htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * WriteFloat(void *dest, float src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Do not marshal the float values into htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * WriteDouble(void *dest, double src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Reading

void * ReadU64(unsigned long long *dest, void *src) {
  memcpy((char *)dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads a uint32 to the dest buffer after converting to network byte order.
// Returns the src pointer incremented by the number of bytes written.
/*
 void * ReadNetU32(unsigned int *dest, void *src)
 {
 unsigned int hostInt;
 memcpy((char *) &hostInt, (char*) src, sizeof(*dest));
 *dest = ntohl(hostInt);
 return((char *)src + sizeof(*dest));
 }
 */

// Reads a uint32 to the dest buffer. Does not convert to network byte order.
// Returns the src pointer incremented by the number of bytes written.
void * ReadU32(unsigned int *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads a uint16 to the dest buffer after converting to network byte order.
// Returns the src pointer incremented by the number of bytes written.
/* Unused
 void * ReadNetU16(unsigned short *dest, void *src)
 {
 unsigned short hostShort;
 memcpy((char *) &hostShort, (char*) src, sizeof(*dest));
 *dest = ntohs(hostShort);
 return((char *)src + sizeof(*dest));
 }
 */

// Reads a uint16 to the dest buffer. Does not convert to network byte order.
// Returns the src pointer incremented by the number of bytes written.
void * ReadU16(unsigned short *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads a char to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
void * ReadChar(unsigned char *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads a 4 byte float to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
// Do not unmarshall the float values through htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * ReadFloat(float *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads an 8 byte float to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
void * ReadDouble(double *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Packs a list of variables into the 'dst' byte array. You must
// pass a 'packStr' to inform the function what the type of the
// variables are coming the variable arg list. The possible types
// are 'i' for int (4 bytes), 'f' for float (4 bytes), 'd' for double
// (8 bytes), 'h' for short (2 bytes), and 'c' for char (1 byte). For array, use
// 'a' followed by the type of the array (ex: 'ad') and use the variable
// corresponding to the 'a' to use an integer to specify the number of elements.
// For example, to pack two chars, a float, an array of shorts (with number of elements char),
//  and two ints use"ccfahii" as the packStr.
UtilMsgError UtilMsgPack(void *dst, unsigned int dstBytes, unsigned int *bytesPacked, const char *packStr, ...)
{
  long long int l; // 8 bytes
  int i;    //4 bytes
  float f;  //4 bytes
  double d; //8 bytes
  short h;  //2 bytes
  char c;   //1 byte

  int idx;
  int variableNum;
  char *temp = (char *) dst;
  va_list arglist;
  UtilMsgError error = UTILMSG_OK;
	
  // Always set this first.  We want this to be valid even if we fail.
  if ( bytesPacked ) {
    *bytesPacked = 0;
  }
  
  // Don't even try to pack any data if the supplied buffer isnt allocated.
  if ( ( dst == NULL ) || ( dstBytes <= 0 ) )
  {
#ifndef UNIT_TEST
    assert(0);
#endif
    
    return UTILMSG_ZEROBUFFER;
  }
  
  va_start(arglist, packStr);
  for(idx = 0; (packStr[idx] != '\0') && (error == UTILMSG_OK); ++idx) {
    
    // If we're writing an array, the variable corresponding to the 'a'
    // character contains the number we will write, and the character at the
    // following index represents the type.
    int numVariablesToWrite = 1; // By default, writing one variable at a time
    void* arrPtr = NULL;
    if(packStr[idx] == 'a') {
      numVariablesToWrite = va_arg(arglist, int);
      
#ifndef UNIT_TEST
      assert( ( numVariablesToWrite >= 0 ) && ( numVariablesToWrite <= UINT8_MAX ) );
#endif
      
      // We can only unpack the array size as an unsigned char, so make sure we're not packing more than 255 elements.
      numVariablesToWrite = ( ( numVariablesToWrite <= 0 ) ? 0 : ( ( numVariablesToWrite >= UINT8_MAX ) ? UINT8_MAX : numVariablesToWrite ) );
      
      // Check for buffer overrun prior to writing to the buffer.
      if ( ((temp - (char*)dst) + sizeof(char)) > dstBytes ) {
        error = UTILMSG_BUFFEROVRN;
        break;
      }
      
      temp = (char *)WriteChar(temp, (char) numVariablesToWrite); // change pack to always write out the num of array elements
      
      arrPtr = (void*)va_arg(arglist, void*);  // get pointer to the array
      idx++; // To go to type character for array
    }
    
    // Write each variable (1 or for each one in array)
    for(variableNum = 0; variableNum < numVariablesToWrite; variableNum++) {
      
      // floats promoted to double and chars and shorts promoted to int
      switch(packStr[idx]) {   // Type to expect.
        case 'i':
          if(arrPtr) {
            i = ((int*)arrPtr)[variableNum];
          } else {
            i = va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(i)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)WriteU32(temp, i);
          break;
          
        case 'f':
          if(arrPtr) {
            f = ((float*)arrPtr)[variableNum];
          } else {
            f = (float) va_arg(arglist, double);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(f)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)WriteFloat(temp, f);
          break;
          
        case 'd':
          if(arrPtr) {
            d = ((double*)arrPtr)[variableNum];
          } else {
            d = va_arg(arglist, double);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(d)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)WriteDouble(temp, d);
          break;
          
        case 'h':
          if(arrPtr) {
            h = ((short*)arrPtr)[variableNum];
          } else {
            h = (short) va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(h)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)WriteU16(temp, h);
          break;
          
        case 'c':
          if(arrPtr) {
            c = ((char*)arrPtr)[variableNum];
          } else {
            c = (char) va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(c)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)WriteChar(temp, c);
          break;
          
        case 'l':
          if(arrPtr) {
            l = ((long long int*)arrPtr)[variableNum];
          } else {
            l = (long long int)va_arg(arglist,long long int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (char*)dst) + sizeof(l)) > dstBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char*)WriteU64(temp, l);
          break;
          
        default:
          error = UTILMSG_INVALIDARG;
          break;
      }
      
      if (error != UTILMSG_OK) {
        break;
      }
    }
  }
  
  va_end(arglist);
  
#ifndef UNIT_TEST
  assert(error == UTILMSG_OK);
#endif
  
  // If they requested the num bytes written, fill it in.
  if ( bytesPacked ) {
    *bytesPacked = ( temp - (char*)dst );
  }
  
  return error;
}



// Unpacks a list of variables from the 'src' byte array. You must
// pass a 'packStr' to inform the function what the type of the
// variables are in the buffer. The possible types
// are 'i' for int (4 bytes), 'f' for float (4 bytes), 'd' for double
// (8 bytes), 'h' for short (2 bytes), and 'c' for char (1 byte). To unpack
// you pass pointers to storage for the appropriate type. For example,
// to unpack two chars, a float and two ints use "ccfii" as the
// packStr. Suppose you have char c1,c2 and float f, and int i1,i2.
// Then you would unpack by passing the pointers
// UtilMsgUnpack(src, "ccfii", &c1, &c2, &f, &i1, &i2)
UtilMsgError UtilMsgUnpack(const void *src, unsigned int srcBytes, unsigned int *bytesUnpacked, const char *packStr, ...)
{
  unsigned long long l;
  unsigned long long *pL = 0;
  unsigned int i;    //4 bytes
  unsigned int *pI = 0;
  float f;  //4 bytes
  float *pF = 0;
  double d; //8 bytes
  double *pD = 0;
  unsigned short h;  //2 bytes
  unsigned short *pH = 0;
  unsigned char c;   //1 byte
  unsigned char *pC = 0;

  int idx;
  int variableNum;
  va_list arglist;
  char *temp = (char *) src;
  UtilMsgError error = UTILMSG_OK;
  
  if ( bytesUnpacked ) {
    *bytesUnpacked = 0;
  }
  
  // Don't even try to pack any data if the supplied buffer isnt allocated.
  if ( ( src == NULL ) || ( srcBytes <= 0 ) )
  {
#ifndef UNIT_TEST
    assert(0);
#endif
    
    return UTILMSG_ZEROBUFFER;
  }
 
  
  va_start(arglist, packStr);
  
  for(idx = 0; (packStr[idx] != '\0') && (error == UTILMSG_OK); ++idx) {
    
    // If we're reading an array, the variable corresponding to the 'a'
    // character contains the number we will read, and the character at
    // the following index represents the type.
    int numVariablesToRead = 1; // By default, writing one variable at a time
    if(packStr[idx] == 'a') {
      // Make sure we don't read past the end of the buffer
      if ( ((temp - (char*)src) + sizeof(char)) > srcBytes ) {
        error = UTILMSG_BUFFEROVRN;
        break;
      }
      
      temp = (char *)ReadChar(&c, temp);   // Read the number of things to read
      pC = (unsigned char *) va_arg(arglist, void *);
      *pC = c;    // Set this number in the struct. (Not really used for anything, but here for completeness.)
      numVariablesToRead = (int)c;
      if (numVariablesToRead == 0) {
        // Empty array, no variables will be assigned.
        // Get the next variable in the arg list
        pC = (unsigned char *) va_arg(arglist, void *);
      }
      idx++; // To go to type character for array
    }
    
    // Write each variable (1 or for each one in array)
    for(variableNum = 0; variableNum < numVariablesToRead; variableNum++) {
      
      // floats promoted to double and chars and shorts promoted to int
      switch(packStr[idx]) {   // Type to expect.
        case 'i':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(i)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadU32(&i, temp);
          if(variableNum == 0) {
            pI = (unsigned int *) va_arg(arglist, void *);
          } else {
            pI++;
          }
          *pI = i;
          break;
          
        case 'f':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(f)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadFloat(&f, temp);
          if(variableNum == 0) {
            pF = (float *) va_arg(arglist, void *);
          } else {
            pF++;
          }
          *pF = f;
          break;
          
        case 'd':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(d)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadDouble(&d, temp);
          if(variableNum == 0) {
            pD = (double *) va_arg(arglist, void *);
          } else {
            pD++;
          }
          *pD = d;
          break;
          
        case 'h':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(h)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadU16(&h, temp);
          if(variableNum == 0) {
            pH = (unsigned short *) va_arg(arglist, void *);
          } else {
            pH++;
          }
          *pH = h;
          break;
          
        case 'c':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(c)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadChar(&c, temp);
          if(variableNum == 0) {
            pC = (unsigned char *) va_arg(arglist, void *);
          } else {
            pC++;
          }
          *pC = c;
          break;
          
        case 'l':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (char*)src) + sizeof(l)) > srcBytes ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (char *)ReadU64(&l, temp);
          if(variableNum == 0) {
            pL = (unsigned long long *)va_arg(arglist, void *);
          }
          else {
            pL++;
          }
          *pL = l;
          break;
          
        default:
          error = UTILMSG_INVALIDARG;
          break;
      }
      
      // If we failed to unpack the data, bail out of the function.
      if (error != UTILMSG_OK) {
        break;
      }
    }
  }
  
  va_end(arglist);
  
#ifndef UNIT_TEST
  assert(error == UTILMSG_OK);
#endif
  
  // Write out the number of bytes we unpacked if it was requested.
  if ( bytesUnpacked ) {
    *bytesUnpacked = ( temp - (char*)src );
  }
  
  return error;
}

