/**
 * File: utilMessaging.c
 *
 * Author: Mark Palatucci (markmp)
 * Created: 11/29/2008
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Utility functions for message communication.
 *
 * Copyright: Anki, Inc. 2008-2011
 *
 **/
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
//#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include "util/utilMessaging/utilMessaging.h"

void* WriteU64(void *dest, uint64_t src) {
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
void * WriteU32(void *dest, uint32_t src)
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
void * WriteU16(void *dest, uint16_t src)
{
  memcpy((char *)dest, (char*) &src, sizeof(src));
  return((char *)dest + sizeof(src));
}

// Writes a char to the dest buffer. Returns the pointer incremented
// by the number of bytes written.
void * WriteChar(void *dest, uint8_t src)
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

void * ReadU64(uint64_t *dest, void *src) {
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
void * ReadU32(uint32_t *dest, void *src)
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
void * ReadU16(uint16_t *dest, void *src)
{
  memcpy((char *) dest, (char*) src, sizeof(*dest));
  return((char *)src + sizeof(*dest));
}

// Reads a char to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
void * ReadChar(uint8_t *dest, void *src)
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
static inline UtilMsgError _SafeUtilMsgPackIternal(void *dst, uint32_t dstBytesLen, uint32_t *oNumBytesPacked, const char *packStr, va_list arglist)
{
  uint64_t l;  //8 bytes
  uint32_t i;  //4 bytes
  float f;     //4 bytes
  double d;    //8 bytes
  uint16_t h;  //2 bytes
  uint8_t c;   //1 byte
  
  // Always set this first.  We want this to be valid even if we fail.
  if ( oNumBytesPacked ) {
    *oNumBytesPacked = 0;
  }
  
  // Don't even try to pack any data if the supplied buffer isnt allocated.
  if ( ( dst == NULL ) || ( dstBytesLen <= 0 ) )
  {
    return UTILMSG_ZEROBUFFER;
  }
  
  uint8_t *temp = (uint8_t *) dst;

//  va_list arglist;
//  va_start(arglist, packStr);
  
  UtilMsgError error = UTILMSG_OK;
  for(int idx = 0; (packStr[idx] != '\0') && (error == UTILMSG_OK); ++idx) {
    
    // If we're writing an array, the variable corresponding to the 'a'
    // character contains the number we will write, and the character at the
    // following index represents the type.
    int numVariablesToWrite = 1; // By default, writing one variable at a time
    void* arrPtr = NULL;
    if(packStr[idx] == 'a') {
      numVariablesToWrite = va_arg(arglist, int);
      
      // We can only unpack the array size as an unsigned char, so make sure we're not packing more than 255 elements.
      numVariablesToWrite = ( ( numVariablesToWrite <= 0 ) ? 0 : ( ( numVariablesToWrite >= UINT8_MAX ) ? UINT8_MAX : numVariablesToWrite ) );
      
      // Check for buffer overrun prior to writing to the buffer.
      if ( ((temp - (uint8_t*)dst) + sizeof(uint8_t)) > dstBytesLen ) {
        error = UTILMSG_BUFFEROVRN;
        break;
      }
      
      temp = (uint8_t *)WriteChar(temp, (uint8_t)numVariablesToWrite); // change pack to always write out the num of array elements
      
      arrPtr = (void*)va_arg(arglist, void*);  // get pointer to the array
      idx++; // To go to type character for array
    }
    
    // Write each variable (1 or for each one in array)
    for(int variableNum = 0; variableNum < numVariablesToWrite; variableNum++) {
      
      // floats promoted to double and chars and shorts promoted to uint32
      switch(packStr[idx]) {   // Type to expect.
        case 'i':
          if(arrPtr) {
            i = ((uint32_t*)arrPtr)[variableNum];
          } else {
            i = va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(i)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)WriteU32(temp, i);
          break;
          
        case 'f':
          if(arrPtr) {
            f = ((float*)arrPtr)[variableNum];
          } else {
            // Floats get promoted to doubles when packed into va_arg lists.
            f = (float) va_arg(arglist, double);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(f)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t *)WriteFloat(temp, f);
          break;
          
        case 'd':
          if(arrPtr) {
            d = ((double*)arrPtr)[variableNum];
          } else {
            d = va_arg(arglist, double);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(d)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t *)WriteDouble(temp, d);
          break;
          
        case 'h':
          if(arrPtr) {
            h = ((short*)arrPtr)[variableNum];
          } else {
            h = (short) va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(h)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t *)WriteU16(temp, h);
          break;
          
        case 'c':
          if(arrPtr) {
            c = ((uint8_t*)arrPtr)[variableNum];
          } else {
            c = (char) va_arg(arglist, int);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(c)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t *)WriteChar(temp, c);
          break;
          
        case 'l':
          if(arrPtr) {
            l = ((uint64_t*)arrPtr)[variableNum];
          } else {
            l = (uint64_t)va_arg(arglist,uint64_t);
          }
          
          // Make sure we have enough space in our buffer
          if ( ((temp - (uint8_t*)dst) + sizeof(l)) > dstBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t*)WriteU64(temp, l);
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
  
//  va_end(arglist);
  
  // If they requested the num bytes written, fill it in.
  if ( oNumBytesPacked ) {
    *oNumBytesPacked = (uint32_t)( temp - (uint8_t*)dst );
  }
  
  return error;
}

/**
 *  Wrapper for taking a variable argument list and passing it to our internal version which takes a va_list argument.
 **/
UtilMsgError SafeUtilMsgPack(void *dst, uint32_t dstBytesLen, uint32_t *oNumBytesPacked, const char *packStr, ...)
{
  va_list arglist;
  va_start(arglist, packStr);
  
  UtilMsgError error = _SafeUtilMsgPackIternal(dst, dstBytesLen, oNumBytesPacked, packStr, arglist);
  
  va_end(arglist);
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
static inline UtilMsgError _SafeUtilMsgUnpackInternal(const void *src, uint32_t srcBytesLen, uint32_t *oNumBytesUnpacked, const char *packStr, va_list arglist)
{
  uint64_t l;    //8 bytes
  uint64_t *pL = NULL;
  double d;      //8 bytes
  double *pD = NULL;
  uint32_t i;    //4 bytes
  unsigned int *pI = NULL;
  float f;       //4 bytes
  float *pF = NULL;
  uint16_t h;    //2 bytes
  uint16_t *pH = NULL;
  uint8_t c;     //1 byte
  uint8_t *pC = NULL;
  
  if ( oNumBytesUnpacked ) {
    *oNumBytesUnpacked = 0;
  }
  
  // Don't even try to pack any data if the supplied buffer isnt allocated.
  if ( ( src == NULL ) || ( srcBytesLen <= 0 ) )
  {
    return UTILMSG_ZEROBUFFER;
  }
  
  uint8_t *temp = (uint8_t *) src;
  
  UtilMsgError error = UTILMSG_OK;
  for(int idx = 0; (packStr[idx] != '\0') && (error == UTILMSG_OK); ++idx) {
    
    // If we're reading an array, the variable corresponding to the 'a'
    // character contains the number we will read, and the character at
    // the following index represents the type.
    unsigned int numVariablesToRead = 1; // By default, writing one variable at a time
    if(packStr[idx] == 'a') {
      // Make sure we don't read past the end of the buffer
      if ( ((temp - (uint8_t*)src) + sizeof(uint8_t)) > srcBytesLen ) {
        error = UTILMSG_BUFFEROVRN;
        break;
      }
      
      temp = (uint8_t *)ReadChar(&c, temp);   // Read the number of things to read
      pC =   (uint8_t *) va_arg(arglist, void *);
      *pC = c;    // Set this number in the struct. (Not really used for anything, but here for completeness.)
      numVariablesToRead = (unsigned int)c;
      if (numVariablesToRead == 0) {
        // Empty array, no variables will be assigned.
        // Get the next variable in the arg list
        pC = (uint8_t *) va_arg(arglist, void *);
      }
      idx++; // To go to type character for array
    }
    
    // Write each variable (1 or for each one in array)
    for(int variableNum = 0; variableNum < numVariablesToRead; variableNum++) {
      
      // floats promoted to double and chars and shorts promoted to int
      switch(packStr[idx]) {   // Type to expect.
        case 'i':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(i)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)ReadU32(&i, temp);
          if(variableNum == 0) {
            pI = (unsigned int *) va_arg(arglist, void *);
          } else {
            pI++;
          }
          *pI = i;
          break;
          
        case 'f':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(f)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)ReadFloat(&f, temp);
          if(variableNum == 0) {
            pF = (float *) va_arg(arglist, void *);
          } else {
            pF++;
          }
          *pF = f;
          break;
          
        case 'd':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(d)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)ReadDouble(&d, temp);
          if(variableNum == 0) {
            pD = (double *) va_arg(arglist, void *);
          } else {
            pD++;
          }
          *pD = d;
          break;
          
        case 'h':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(h)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)ReadU16(&h, temp);
          if(variableNum == 0) {
            pH = (unsigned short *) va_arg(arglist, void *);
          } else {
            pH++;
          }
          *pH = h;
          break;
          
        case 'c':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(c)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }

          temp = (uint8_t *)ReadChar(&c, temp);
          if(variableNum == 0) {
            pC = (uint8_t *) va_arg(arglist, void *);
          } else {
            pC++;
          }
          *pC = c;
          break;
          
        case 'l':
          // Make sure we don't read past the end of the buffer
          if ( ((temp - (uint8_t*)src) + sizeof(l)) > srcBytesLen ) {
            error = UTILMSG_BUFFEROVRN;
            break;
          }
          
          temp = (uint8_t *)ReadU64(&l, temp);
          if(variableNum == 0) {
            pL = (uint64_t *)va_arg(arglist, void *);
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
  
  // Write out the number of bytes we unpacked if it was requested.
  if ( oNumBytesUnpacked ) {
    *oNumBytesUnpacked = (uint32_t)( temp - (uint8_t*)src );
  }
  
  return error;
}

/**
 *  Wrapper for taking a variable argument list and passing it to our internal version which takes a va_list argument.
 *  This is just to maintain backwards compatibility with our unit tests until we can fully transition
 *  UtilMsgUnpack(...) to SafeUtilMsgUnpack(...).
 **/
UtilMsgError SafeUtilMsgUnpack(const void *src, unsigned int srcBytes, unsigned int *bytesUnpacked, const char *packStr, ...)
{
  va_list arglist;
  va_start(arglist, packStr);
  
  UtilMsgError error = _SafeUtilMsgUnpackInternal(src, srcBytes, bytesUnpacked, packStr, arglist);
  
  
  va_end(arglist);
  return error;
}

static uint32_t _SafeUtilGetMsgSizeInternal(const char *packStr, va_list arglist)
{
  uint32_t size = 0;

  for(int idx = 0; packStr[idx] != '\0'; ++idx) {

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

      size += sizeof(uint8_t); // size of the char that stores array size

      arrPtr = (void*)va_arg(arglist, void*);  // get pointer to the array
      idx++; // To go to type character for array
    }

    // Write each variable (1 or for each one in array)
    for(int variableNum = 0; variableNum < numVariablesToWrite; variableNum++) {

      // floats promoted to double and chars and shorts promoted to uint32
      switch(packStr[idx]) {   // Type to expect.
        case 'i':
          if(!arrPtr) {
            va_arg(arglist, int);
          }

          size += sizeof(uint32_t);
          break;

        case 'f':
          if(!arrPtr) {
            // Floats get promoted to doubles when packed into va_arg lists.
            va_arg(arglist, double);
          }

          size += sizeof(float);
          break;

        case 'd':
          if(!arrPtr) {
            va_arg(arglist, double);
          }

          size += sizeof(double);
          break;

        case 'h':
          if(!arrPtr) {
            va_arg(arglist, int);
          }

          size += sizeof(uint16_t);
          break;

        case 'c':
          if(!arrPtr) {
            va_arg(arglist, int);
          }

          size += sizeof(uint8_t);
          break;

        case 'l':
          if(!arrPtr) {
            va_arg(arglist,uint64_t);
          }

          size += sizeof(uint64_t);
          break;

        default:
          break;
      }
    }
  }

  return size;
}

// Gets the size required for a given packing/unpacking string
uint32_t SafeUtilGetMsgSize(const char *packStr, ...)
{
  va_list arglist;
  va_start(arglist, packStr);

  uint32_t size = _SafeUtilGetMsgSizeInternal(packStr, arglist);

  va_end(arglist);
  return size;
}
