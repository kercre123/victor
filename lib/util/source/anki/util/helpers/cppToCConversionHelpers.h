/**
 * File: cppToCConversionHelpers
 *
 * Author: Mark Wesley
 * Created: 10/10/2014
 *
 * Description: Helper functions for more easily copying C++/STL structures into C-style arrays etc.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Util_Helpers_CppToCConversionHelpers_H__
#define __Util_Helpers_CppToCConversionHelpers_H__


#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/helpers/cConversionHelpers.h"
#include <iterator>

namespace Anki{ namespace Util
{

  
// STL containers into C-arrays


template <typename CT, typename BT, typename F>
size_t FillBufferFromContainer(const CT& inContainer, BT* outBuffer, size_t outBufferCapacity, F fromItFunc)
{
  const size_t numItemsInContainer = inContainer.size();
  
  assert(outBufferCapacity >= numItemsInContainer);
  const size_t numItemsToCopy = Min(numItemsInContainer, outBufferCapacity);
  
  size_t i = 0;
  const typename CT::const_iterator itEnd = inContainer.end();
  for (typename CT::const_iterator it = inContainer.begin(); (it != itEnd) && (i < numItemsToCopy); ++it)
  {
    outBuffer[i++] = fromItFunc(it);
  }
  assert(i == numItemsToCopy);
  
  return numItemsToCopy;
}


template <typename CT, typename BT>
inline const BT& ExtractFirstFromIterator(const typename CT::const_iterator& it)
{
  return it->first;
}


template <typename CT, typename BT>
inline const BT& ExtractSecondFromIterator(const typename CT::const_iterator& it)
{
  return it->second;
}


template <typename CT, typename BT>
inline const BT& ExtractValueFromIterator(const typename CT::const_iterator& it)
{
  return *it;
}

  
template <typename CT, typename BT>
inline size_t FillBufferFromMapKeys(const CT& inContainer, BT* outBuffer, size_t outBufferCapacity)
{
  return FillBufferFromContainer(inContainer, outBuffer, outBufferCapacity, ExtractFirstFromIterator<CT, BT>);
}


template <typename CT, typename BT>
inline size_t FillBufferFromMapValues(const CT& inContainer, BT* outBuffer, size_t outBufferCapacity)
{
  return FillBufferFromContainer(inContainer, outBuffer, outBufferCapacity, ExtractSecondFromIterator<CT, BT>);
}


template <typename CT, typename BT>
inline size_t FillBufferFromContainerValues(const CT& inContainer, BT* outBuffer, size_t outBufferCapacity)
{
  return FillBufferFromContainer(inContainer, outBuffer, outBufferCapacity, ExtractValueFromIterator<CT, BT>);
}

template <typename CT, typename BT>
  inline void FillContainerFromBufferValues(CT& inContainer, const BT* inBuffer, size_t inBufferLength)
{
  auto back_it = back_inserter(inContainer);
  while(inBufferLength-- > 0) {
    back_it = *inBuffer++;
  }
}


// Strings

  
inline size_t CopyStringIntoOutBuffer(const char* inString, char* outBuffer, size_t outBufferLen)
{
  return numeric_cast<size_t>( AnkiCopyStringIntoOutBuffer(inString, outBuffer, numeric_cast<uint32_t>(outBufferLen)) );
}



} // namespace Util
} // namespace Anki


#endif // #ifndef __Util_Helpers_CppToCConversionHelpers_H__

