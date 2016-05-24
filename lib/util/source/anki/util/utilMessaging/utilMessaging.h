/**
 * File: utilMessaging.h
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
 * Copyright: Anki, Inc. 2011
 *
 **/

#ifndef UTIL_UTILMESSAGING_H_
#define UTIL_UTILMESSAGING_H_

#ifdef LINUX
// TODO:(bn) ok to always include this?
#include <string.h>
#endif

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#undef __STDC_LIMIT_MACROS

#ifndef USING_SAFE_UTILMSGPACK
#define USING_SAFE_UTILMSGPACK 1
#endif // USING_SAFE_UTILMSGPACK

#if defined(UINT32_MAX)
#define MSGPACK_SIZE_MAX UINT32_MAX
#else
#define MSGPACK_SIZE_MAX 4294967295U
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
  UTILMSG_OK,         // Everything is a-ok
  UTILMSG_ZEROBUFFER, // Allocated input buffer was not supplied
  UTILMSG_BUFFEROVRN, // We would have read/written past the end of the input buffer
  UTILMSG_INVALIDARG, // An invalid pack string was detected
} UtilMsgError;

// Writes a uint64 to the dest buffer. Does not convert to network byte order.
// Returns the pointer incremented by the number of bytes written.
void *WriteU64(void* dest, uint64_t src);

// Writes a uint32 to the dest buffer after converting to network byte order.
// Returns the pointer incremented by the number of bytes written.
//void * WriteNetU32(void *dest, unsigned int src);

// Writes a uint32 to the dest buffer. Does not convert to network byte order.
// Returns the pointer incremented by the number of bytes written.
void * WriteU32(void *dest, uint32_t src);

// Writes a uint16 to the dest buffer after converting to network byte order.
// Returns the pointer incremented by the number of bytes written.
//void * WriteNetU16(void *dest, unsigned short src);

// Writes a uint16 to the dest buffer. Does not convert to network byte order.
// Returns the pointer incremented by the number of bytes written.
void * WriteU16(void *dest, uint16_t src);

// Writes a char to the dest buffer. Returns the pointer incremented
// by the number of bytes written.
void * WriteChar(void *dest, uint8_t src);

// Do not marshal the float values into htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * WriteFloat(void *dest, float src);

// Do not marshal the float values into htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * WriteDouble(void *dest, double src);

// Reading: works similarly to write. But it returns the SRC pointer
// incremented by the number of bytes read.

// Reads a uint64 to the dest buffer. Does not convert to network byte order.
// Returns the src pointer incremented by the number of bytes written.
void * ReadU64(uint64_t *dest, void *src);

// Reads a uint32 to the dest buffer after converting to network byte order.
// Returns the src pointer incremented by the number of bytes written.
//void * ReadNetU32(unsigned int *dest, void *src);

// Reads a uint32 to the dest buffer. Does not convert to network byte order.
// Returns the src pointer incremented by the number of bytes written.
void * ReadU32(uint32_t *dest, void *src);

// Reads a uint16 to the dest buffer after converting to network byte order.
// Returns the src pointer incremented by the number of bytes written.
//void * ReadNetU16(unsigned short *dest, void *src);

// Reads a uint16 to the dest buffer. Does not convert to network byte order.
// Returns the src pointer incremented by the number of bytes written.
void * ReadU16(uint16_t *dest, void *src);

// Reads a char to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
void * ReadChar(uint8_t *dest, void *src);

// Reads a 4 byte float to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
// Do not unmarshall the float values through htonl. Floats are defined by IEEE
// standard and have the same byte order regardless of endianness of machine
void * ReadFloat(float *dest, void *src);

// Reads an 8 byte float to the dest buffer. Returns the src pointer incremented
// by the number of bytes written.
void * ReadDouble(double *dest, void *src);

// Packs a list of variables into the 'dst' byte array. You must
// pass a 'packStr' to inform the function what the type of the
// variables are coming the variable arg list. The possible types
// are 'i' for int (4 bytes), 'f' for float (4 bytes), 'd' for double
// (8 bytes), 'h' for short (2 bytes), and 'c' for char (1 byte). For array, use
// 'a' followed by the type of the array (ex: 'ad') and use the variable
// corresponding to the 'a' to use an integer to specify the number of elements.
// For example, to pack two chars, a float, an array of shorts, and two ints use
// "ccfahii" as the packStr.
// 'i' for int (4 bytes)
// 'f' for float (4 bytes)
// 'd' for double (8 bytes)
// 'h' for short (2 bytes)
// 'c' for char (1 byte)
UtilMsgError SafeUtilMsgPack(void *dst, uint32_t dstBytesLen, uint32_t *oNumBytesPacked, const char *packStr, ...);

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
// 'i' for int (4 bytes)
// 'f' for float (4 bytes)
// 'd' for double (8 bytes)
// 'h' for short (2 bytes)
// 'c' for char (1 byte)
UtilMsgError SafeUtilMsgUnpack(const void *src, uint32_t srcBytesLen, uint32_t *oNumBytesUnpacked, const char *packStr, ...);

// Gets the size required for a given packing/unpacking string
uint32_t SafeUtilGetMsgSize(const char *packStr, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // #ifndef UTIL_UTILMESSAGING_H_




