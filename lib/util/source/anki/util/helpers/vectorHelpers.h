/**
 * File: vectorHelpers.h
 *
 * Author:  Mark Wesley
 * Created: 12/03/15
 *
 * Description: Helper functions for working with std::vector
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Util_Helpers_VectorHelpers_H__
#define __Util_Helpers_VectorHelpers_H__


#include <vector>


// remove_swap - remove item at index, similar to erase(index, 1) _BUT_:
// Instead of moving all following items down it just swaps the items with the end.
// This is faster, but shouldn't be used if the order of items is important
template<typename T>
void remove_swap(std::vector<T>& inContainer, size_t index)
{
  const size_t vecSize = inContainer.size();
  assert(index < vecSize);
  
  if (index != (vecSize - 1))
  {
    inContainer[index] = std::move(inContainer.back());
  }
  
  inContainer.pop_back();
}


// remove_swap_item - remove all items in container that match inElement, uses remove_swap so:
// Instead of moving all following items down it just swaps the items with the end.
// This is faster, but shouldn't be used if the order of items is important
template<typename T>
void remove_swap_item(std::vector<T>& inContainer, const T& inElement)
{
  // Remove all matching items, done in reverse order to minimize swapping
  for (size_t i=inContainer.size(); i > 0; )
  {
    --i;
    if (inContainer[i] == inElement)
    {
      remove_swap(inContainer, i);
    }
  }
}


#endif // #ifndef __Util_Helpers_VectorHelpers_H__

