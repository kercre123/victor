/**
 * File: MinMaxQueue
 *
 * Author: Matt Michini
 * Created: 09/18/2018
 *
 * Description: A queue data structure that efficiently keeps track of the min and max elements within it at all times.
 *              Complexity (where n is number of elements in the container):
 *                - Pushing an element: Worst case O(n), amortized O(1)
 *                - Popping an element: O(1)
 *                - Retrieving min/max value: O(1)
 *
 *              Adapted from https://people.cs.uct.ac.za/~ksmith/articles/sliding_window_minimum.html#sliding-window-minimum-algorithm.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Util_Container_MinMaxQueue_H__
#define __Util_Container_MinMaxQueue_H__

#include <assert.h>
#include <deque>

namespace Anki {
namespace Util {

template <typename T>
class MinMaxQueue
{
public:
  
  void push(const T& val) {
    // Update max-tracking deque
    while(!_maxTracker.empty() && _maxTracker.back() < val) {
      _maxTracker.pop_back();
    }
    _maxTracker.push_back(val);
    
    // Update min-tracking deque
    while(!_minTracker.empty() && _minTracker.back() > val) {
      _minTracker.pop_back();
    }
    _minTracker.push_back(val);
    
    // Finally, add the value to the underlying container
    _data.push_back(val);
  }
  
  void pop() {
    assert(!_data.empty());
    // Update max-tracking deque
    assert(_maxTracker.front() >= _data.front());
    if (_data.front() == _maxTracker.front()) {
      _maxTracker.pop_front();
    }
    
    // Update min-tracking deque
    assert(_minTracker.front() <= _data.front());
    if (_data.front() == _minTracker.front()) {
      _minTracker.pop_front();
    }
    
    // Finally, pop the element from the underlying container
    _data.pop_front();
  }
  
  const T& front() const {
    return _data.front();
  }
  
  T& front() {
    return _data.front();
  }
  
  const T& back() const {
    return _data.back();
  }
  
  T& back() {
    return _data.back();
  }
  
  // Returns the minimum element in the container
  T min() const {
    assert(!_data.empty() && !_minTracker.empty());
    return _minTracker.front();
  }
  
  // Returns the maximum element in the container
  T max() const {
    assert(!_data.empty() && !_maxTracker.empty());
    return _maxTracker.front();
  }
  
  size_t size()  const    { return _data.size(); }
  bool   empty() const    { return _data.empty(); }
  
  void clear() {
    _data.clear();
    _minTracker.clear();
    _maxTracker.clear();
  }
  
private:
  
  // The underlying container (we use a std::deque here instead of a std::queue since deque has a clear() method)
  std::deque<T> _data;
  
  // Helper deques to keep track of the min/max elements
  // See https://people.cs.uct.ac.za/~ksmith/articles/sliding_window_minimum.html#sliding-window-minimum-algorithm
  // or https://www.nayuki.io/page/sliding-window-minimum-maximum-algorithm
  std::deque<T> _minTracker;
  std::deque<T> _maxTracker;
};

} // end namespace Util
} // end namespace Anki

#endif // __Util_Container_MinMaxQueue_H__
