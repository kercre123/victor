/**
 * File: dataPacking.cpp
 *
 * Author: raul
 * Created: 11/6/2013
 *
 * Description: Utility for bitwise data packing.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "dataPacking.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>
#include "util/logging/logging.h"

// if DEBUG_PACKING_PRINT : packing functions will print binary operations for visual debugging
#define DEBUG_PACKING_PRINT 0

namespace Anki {
namespace Util {
namespace DataPacking
{

void printBits(const void* p, int size, bool newLine)
{
  int i,j;
  for(i=0;i<size;i++) {
    for(j=7;j>=0;j--)
    printf("%u",(*((unsigned char*)p+i)&(1<<j))>>j);
  }
  if(newLine)
    puts("");
}

bool PackMaskedIds(const unsigned int* idArray, size_t idCount, unsigned char* outBuffer, size_t outBufferSize, const unsigned int bitsPerId, unsigned int& requiredBytesToSend)
{
  // supporting only byte access buffers
  const unsigned int bufferTypeInBits = 8;
  assert(sizeof(*outBuffer)*8==bufferTypeInBits);

  // calculate bytes required to store all ids
  requiredBytesToSend = (((unsigned int)idCount * bitsPerId) + bufferTypeInBits - 1) / bufferTypeInBits;

  // check destination buffer
  if ( outBufferSize < requiredBytesToSend )
  {
    PRINT_NAMED_ERROR("PackMaskedId", "outBuffer (%zd bytes) does not have enough bytes for the given id count %zd * %u = %zd bits = %u bytes. Programmer error",
                      outBufferSize, idCount, bitsPerId, idCount*bitsPerId, requiredBytesToSend);
    assert(false); // this is a programmer error, assert
    return false;
  }
  
  // check mask restriction
  const unsigned int maskSizeInBytes = sizeof(unsigned int); // we only need to know the size here
  const unsigned int maskSizeInBits  = maskSizeInBytes * 8;
  if ( bufferTypeInBits > (maskSizeInBits - bitsPerId) )
  {
    // We can only code Ids that start at indices [0, maskTypeInBits-bitsPerId]. If the bufferTypeInBits
    // is bigger than (maskTypeInBits - bitsPerId), some Ids require the mask to split between buffer units.
    // eg: 8 bits array, 8 bits mask, 3 bitsPerId. The fourth item would require a mask that can split 2 bits from 1 byte, and 1 bit from the next one
    // To solve this easily, change the mask to 16 bits.
    PRINT_NAMED_ERROR("PackMaskedId", "Mask of '%u' is too big for buffer and mask type. Programmer error.", bitsPerId);
    assert(false); // this is a programmer error, assert
    return false;
  }

  // since masks require bigger buffers than necessary, provide such support here
  unsigned char* outBufferPtr     = outBuffer;
  size_t         outBufferPtrSize = outBufferSize;
  // how many bytes does the mask require to write properly?
  size_t requiredBytesForMask = requiredBytesToSend + (maskSizeInBytes-1); // the mask writes N bytes beyond the actual required
  if ( requiredBytesForMask > outBufferSize )
  {
    // we need the buffer to be bigger, so create a temp one
    outBufferPtr     = new unsigned char[requiredBytesForMask];
    outBufferPtrSize = requiredBytesForMask;
  }
  
  // for programmer
  PRINT_NAMED_INFO("PackMaskedIds", "To pack %zu Ids with %u bits per Id:", idCount, bitsPerId );
  PRINT_NAMED_INFO("PackMaskedIds", "IdCount requires %u bytes to be sent", requiredBytesToSend );
  PRINT_NAMED_INFO("PackMaskedIds", "MaskSize requires %zu bytes from the buffer", requiredBytesForMask );
  PRINT_NAMED_INFO("PackMaskedIds", "%s (since outBuffer provided %zu)", outBufferSize != outBufferPtrSize ? "Temp buffer is required for mask:" : "Temp buffer is not required:", outBufferSize );
  
  // reset buffer
  memset(outBufferPtr, 0x00, outBufferPtrSize);
  
  const unsigned int firstBitOfMask = (maskSizeInBits - bitsPerId); // when shifted
  for( unsigned int idIdx=0; idIdx<idCount; ++idIdx )
  {
    unsigned int currentBit   = bitsPerId * idIdx;
    unsigned int charPosition = currentBit / bufferTypeInBits;
    unsigned int charOffset   = currentBit % bufferTypeInBits;
    
    unsigned int idHashedValue   = 0;
    unsigned int idValueAtBuffer = 0;
    assert( maskSizeInBytes == sizeof(idValueAtBuffer) );
    assert( maskSizeInBytes == sizeof(idHashedValue) );
    
    // otherwise we read/write beyond buffer
    assert( (charPosition*sizeof(unsigned char)+sizeof(idHashedValue))   <= outBufferPtrSize*sizeof(unsigned char) );
    assert( (charPosition*sizeof(unsigned char)+sizeof(idValueAtBuffer)) <= requiredBytesForMask*sizeof(unsigned char) );
    
    // hash this id
    unsigned int idValue = idArray[idIdx];
    if ( idValue > (1 << bitsPerId) )
    {
      // this value doesn't fit in the number of bits!
      PRINT_NAMED_ERROR("PackMaskedId", "Id '%u' is too big for the number of bits used to encode it '%u'. Programmer error.", idValue, bitsPerId);
      assert(false); // this is a programmer error, assert
      return false;
    }
    // idMaskedValue = ( idValue & idMask ); // not necessary, since the number is smaller than idMask
    idHashedValue = ( idValue << (firstBitOfMask - charOffset) );
    
    // htonl/ntohl is required or bits overlap with current algorithm (because of shifting applied)
#pragma GCC diagnostic push
#if __has_warning("-Wdeprecated-register")
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif
    idHashedValue = htonl(idHashedValue);
#pragma GCC diagnostic pop
    
    // first, read the current value in the buffer because we are encoding the current id with some others
    memcpy(&idValueAtBuffer, &outBufferPtr[charPosition], sizeof(idValueAtBuffer));
    
    // now build the result that would go into the buffer
    idValueAtBuffer = idValueAtBuffer | idHashedValue; // OR is fine because array was zeroed
    
    // and copy back into the buffer
    memcpy(&outBufferPtr[charPosition], &idValueAtBuffer, sizeof(idValueAtBuffer));
    
    #if DEBUG_PACKING_PRINT
    {
      printf("(id %2u)  '%u'\n", idIdx, idValue );
      printf("=idValue: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idValue, sizeof(idValue) );
      printf("idHashed: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idHashedValue, sizeof(idHashedValue) );
      printf("idBuffer: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idValueAtBuffer, sizeof(idValueAtBuffer) );
    }
    #endif
  }
  
  // if we used a temp buffer, we have to copy back to the original
  if ( outBufferPtrSize != outBufferSize )
  {
    #if DEBUG_PACKING_PRINT
    {
      printf("outTemp : ");
      printBits( outBufferPtr, outBufferPtrSize );
    }
    #endif
   
    // copy output
    memcpy(outBuffer, outBufferPtr, outBufferSize*sizeof(unsigned char));
    
    // delete temp memory
    delete [] outBufferPtr;
  }
  
  #if DEBUG_PACKING_PRINT
  {
    printf("packedBF: ");
    printBits( outBuffer, outBufferSize );
  }
  #endif
  
  
  return true;
}

bool UnpackMaskedIds(const unsigned char* inBuffer, size_t inBufferSize, unsigned int* outIdArray, size_t outIdCount, const unsigned int bitsPerId)
{
 
  // supporting only byte access buffers
  const unsigned int bufferTypeInBits = 8;
  assert(sizeof(*inBuffer)*8==bufferTypeInBits);
  
  // check mask restriction
  const unsigned int idMask = ((( 1 << bitsPerId ) - 1));     // mask to use
  const unsigned int maskSizeInBits   = sizeof(idMask) * 8;   // size of the mask
  if ( bufferTypeInBits > (maskSizeInBits - bitsPerId) )
  {
    // We can only code Ids that start at indices [0, maskTypeInBits-bitsPerId]. If the bufferTypeInBits
    // is bigger than (maskTypeInBits - bitsPerId), some Ids require the mask to split between buffer units.
    // eg: 8 bits array, 8 bits mask, 3 bitsPerId. The fourth item would require a mask that can split 2 bits from 1 byte, and 1 bit from the next one
    // To solve this easily, change the mask to 16 bits.
    PRINT_NAMED_ERROR("UnpackMaskedIds", "Mask of '%u' is too big for buffer and mask type. Programmer error.", bitsPerId);
    assert(false); // this is a programmer error, assert
    return false;
  }
  
  // calculate bytes required to read all ids
  const unsigned int requiredBytesToRead = (((unsigned int)outIdCount * bitsPerId) + bufferTypeInBits - 1) / bufferTypeInBits;
  if ( requiredBytesToRead > inBufferSize )
  {
    PRINT_NAMED_ERROR("UnpackMaskedIds", "inBuffer (%zu bytes) does not have enough bytes for the given id count %zu * %u = %zu bits = %u bytes. Programmer error",
                      inBufferSize, outIdCount, bitsPerId, outIdCount*bitsPerId, requiredBytesToRead);
    assert(false); // this is a programmer error, assert
    return false;
  }

  // since masks require bigger buffers than necessary, provide such support here
  const unsigned char* inBufferPtr = inBuffer;
  size_t         inBufferPtrSize   = inBufferSize;
  
  // how many bytes does the mask require to read properly?
  unsigned int requiredBytesForMask = requiredBytesToRead + (sizeof(idMask)-1); // the mask writes N bytes beyond the actual required
  if ( requiredBytesForMask > inBufferSize )
  {
    // we need the buffer to be bigger, so create a temp one
    unsigned char* newBufferPtr = new unsigned char[requiredBytesForMask];
    memset(newBufferPtr, 0, requiredBytesForMask*sizeof(unsigned char));
    
    // copy necessary bytes
    memcpy(newBufferPtr, inBuffer, requiredBytesToRead*sizeof(unsigned char));

    // set pointers
    inBufferPtr     = newBufferPtr;
    inBufferPtrSize = requiredBytesForMask;
  }
  
  // for programmer
  PRINT_NAMED_INFO("UnpackMaskedIds", "To unpack %zu Ids with %u bits per Id:", outIdCount, bitsPerId );
  PRINT_NAMED_INFO("UnpackMaskedIds", "IdCount requires %u bytes to be read", requiredBytesToRead );
  PRINT_NAMED_INFO("UnpackMaskedIds", "MaskSize requires %u bytes from the buffer", requiredBytesForMask );
  PRINT_NAMED_INFO("UnpackMaskedIds", "%s (since inBuffer provided %zu)", inBufferSize != inBufferPtrSize ? "Temp buffer is required for mask:" : "Temp buffer is not required:", inBufferSize );
  
  #if DEBUG_PACKING_PRINT
  {
    printf("unpackBF: ");
    printBits( inBuffer, inBufferSize );
    if ( inBufferPtrSize != inBufferSize )
    {
      printf("inTemp  : ");
      printBits( inBufferPtr, inBufferPtrSize );
    }
  }
  #endif
  
  // reset out buffer
  memset(outIdArray, 0, sizeof(unsigned int)*outIdCount);
  
  const unsigned int firstBitOfMask = (maskSizeInBits - bitsPerId); // when shifted
  for( unsigned int idIdx=0; idIdx<outIdCount; ++idIdx )
  {
    unsigned int currentBit   = bitsPerId * idIdx;
    unsigned int charPosition = currentBit / bufferTypeInBits;
    unsigned int charOffset   = currentBit % bufferTypeInBits;
    
    unsigned int idMaskedValue   = 0;
    unsigned int idValueAtBuffer = 0;
    assert( sizeof(idMask) == sizeof(idValueAtBuffer) );
    assert( sizeof(idMask) == sizeof(idMaskedValue) );
    
    // otherwise we read/write beyond buffer
    assert( (charPosition*sizeof(unsigned char)+sizeof(idValueAtBuffer)) <= inBufferPtrSize*sizeof(unsigned char) );
    assert( (charPosition*sizeof(unsigned char)+sizeof(idValueAtBuffer)) <= requiredBytesForMask*sizeof(unsigned char) );
    
    // read the value from the buffer
    memcpy(&idValueAtBuffer, &inBufferPtr[charPosition], sizeof(idValueAtBuffer));
    
    // grab the value (keep valueAtBuffer for debug printing)
    idMaskedValue = idValueAtBuffer;
    
    // htonl/ntohl required
#pragma GCC diagnostic push
#if __has_warning("-Wdeprecated-register")
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif
    idMaskedValue = ntohl(idMaskedValue);
#pragma GCC diagnostic pop
    
    // unmask the id
    idMaskedValue = ( idMaskedValue >> (firstBitOfMask - charOffset) );
    unsigned int idValue = ( idMaskedValue & idMask );
    
    // copy to the output
    outIdArray[idIdx] = idValue;
    
    #if DEBUG_PACKING_PRINT
    {
      printf("idBuffer: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idValueAtBuffer, sizeof(idValueAtBuffer) );
      printf("idMasked: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idMaskedValue, sizeof(idMaskedValue) );
      printf("idMask  : %*s", charPosition*bufferTypeInBits, "");
      printBits( &idMask, sizeof(idMask) );
      printf("=idValue: %*s", charPosition*bufferTypeInBits, "");
      printBits( &idValue, sizeof(idValue) );
      printf("=(id %2u)  '%u'\n", idIdx, idValue );
    }
    #endif
    
  }
  
  // if we used a temp buffer, we have to delete it
  if ( inBufferPtrSize != inBufferSize )
  {
    // delete temp memory
    delete [] inBufferPtr;
  }
  
  return true;
}

} // namespace
} // end namespace Util
} // end namespace Anki
