/**
 * File: vectorToArray
 *
 * Author: damjan
 * Created: 6/4/14
 * 
 * Description: 
 * 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
 
#ifndef __Util_Helpers_VectorToArray_H__
#define __Util_Helpers_VectorToArray_H__

#include <vector>
#include <stdint.h>
//#include <type_traits>

namespace Anki
{
namespace Util
{

template<class T>
uint32_t VectorPtrToArray(const std::vector<const T*> &vectorOfPointers, void** buffer, uint32_t size)
{
  size_t count = 0;
  for (; count < size && count < vectorOfPointers.size(); ++count) {
    buffer[count] = const_cast<void *>(static_cast<const void *>(vectorOfPointers[count]));
    //buffer[count] = static_cast<void *>(vectorOfPointers[count]);
  }
  return (uint32_t)count;
}

// T and Y must be compatible/convertible
template<class T, class Y>
uint32_t VectorValueToArray(const std::vector<T> &vectorOfValues, Y* buffer, uint32_t size)
{
  size_t count = 0;
  for (; count < size && count < vectorOfValues.size(); ++count) {
    buffer[count] = vectorOfValues[count];
  }
  return (uint32_t)count;
}


} // end namespace Util
} // end namespace Anki
#endif //__Util_Helpers_VectorToArray_H__
