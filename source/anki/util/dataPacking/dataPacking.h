/**
 * File: dataPacking.c
 *
 * Author: raul
 * Created: 11/6/2013
 *
 * Description: Utility for bitwise data packing.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef UTIL_DATAPACKING_H_
#define UTIL_DATAPACKING_H_

#include <cstdlib> // size_t

namespace Anki {
namespace Util {
namespace DataPacking
{

// prints binary value of the given pointer
// note it does not fix endianness, so you'll see the layout the bytes are actually stored in
// eg: big-endian    32bit '1' prints 00000000 00000000 00000000 00000001
// eg: little-endian 32bit '1' prints 00000001 00000000 00000000 00000000
void printBits(const void* p, int ptrsize, bool newLine=true);

/**
 * Packs the given array of ids into a destination buffer:
 * Implemementation will check for errors (value too big to fit in the mask, etc, and inform about errors)
 *
 * idArray             : array of ids (unsigned ints) to pack
 * idCount             : number ids in idArray (number of items, not size in bytes)
 * destBuffer          : destination buffer to write to. Size has to be at least bitsPerId*idCount+sizeof(uint)
 * destBufferSize      : size of destBuffer(size in bytes)
 * bitsPerId           : number of bits you want to code the Ids to. Note this limits the maximum Id you can code
 * requiredBytesToSend : regardless of destBufferSize, this is the actual number of bytes that have Ids encoded in it
 *                       this field depends on idCount, bitsPerId, and byte padding.
 *
 * @return : true on success, false if any error happens. If false is returned, the state of the 
 *           outputs is undefined, and should not be used.
 */
bool PackMaskedIds(const unsigned int* idArray, size_t idCount, unsigned char* destBuffer, size_t destBufferSize, const unsigned int bitsPerId, unsigned int& requiredBytesToSend);
  
/**
 * Unpacks an array of Ids from the given buffer.
 * Implemementation will check for errors (buffer to small, etc., and inform about these errors)
 *
 * buffer              : input buffer to read from. Size has to be bitsPerId*idCount+sizeof(uint)
 * bufferSize          : size of destBuffer(size in bytes)
 * outIdArray          : array of ids (unsigned ints) to unpack to
 * outIdCount          : number ids to unpack (number of items, not size in bytes)
 * bitsPerId           : number of bits that were used to pack the Ids
 *
 * @return : true on success, false if any error happens. If false is returned, the state of the 
 *           outputs is undefined, and should not be used.
 */
bool UnpackMaskedIds(const unsigned char* buffer, size_t bufferSize, unsigned int* outIdArray, size_t outIdCount, const unsigned int bitsPerId);

} // namespace
} // end namespace Util
} // end namespace Anki

#endif  //