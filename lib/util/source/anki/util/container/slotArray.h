/**
 * File: slotArray
 *
 * Author: baustin
 * Created: 3/25/15
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_SlotArray_H__
#define __Util_SlotArray_H__

#include <assert.h>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>

namespace Anki {
namespace Util {

template <class T>
class SlotArray
{
public:
  class iterator;
  class const_iterator;

  // constructor
  explicit SlotArray(size_t size)
  : _capacity(size)
  , _num(0)
  , _array(new T*[size]())
  {
  }

  // destructor
  ~SlotArray() {
    clear();
    delete[] _array;
    _array = nullptr;
  }

  // copy construct
  SlotArray(const SlotArray& other)
  : _capacity(other._capacity)
  , _num(other._num)
  , _array(new T*[other._capacity])
  {
    for (auto i = 0; i < _capacity; i++) {
      _array[i] = other._array[i];
      if (_array[i] != nullptr) {
        // deep copy non-null pointers
        _array[i] = new T(*_array[i]);
      }
    }
  }

  // move construct
  SlotArray(SlotArray&& other)
  : _capacity(other._capacity)
  , _num(other._num)
  , _array(other._array)
  {
    other._array = nullptr;
    other._num = 0;
    other._capacity = 0;
  }

  // copy assignment
  SlotArray& operator=(const SlotArray& other) {
    SlotArray temp(other);
    *this = std::move(temp);
    return *this;
  }

  // move assignment
  SlotArray& operator=(SlotArray&& other) {
    std::swap(_array, other._array);
    std::swap(_num, other._num);
    std::swap(_capacity, other._capacity);
    return *this;
  }

  // iterators
  iterator begin() {
    return iterator(*this);
  }
  const_iterator begin() const {
    return const_iterator(*this);
  }
  const_iterator cbegin() const {
    return begin();
  }
  iterator end() {
    return iterator(*this, _capacity);
  }
  const_iterator end() const {
    return const_iterator(*this, _capacity);
  }
  const_iterator cend() const {
    return end();
  }

  // element access
  T* operator[](size_t index) {
    assert(index >= 0 && index < _capacity);
    return _array[index];
  }
  const T* operator[](size_t index) const {
    assert(index >= 0 && index < _capacity);
    return _array[index];
  }

  // element insertion
  template<class ...Args>
  T& emplace_at(size_t index, Args&&... args) {
    assert(index >= 0 && index < _capacity);
    erase(index);
    create_object(index, std::forward<Args>(args)...);
    return *(_array[index]);
  }
  template<class ...Args>
  T* emplace_first(Args&&... args) {
    size_t index = get_first_empty();
    if (index >= _capacity) {
      return nullptr;
    }
    create_object(index, std::forward<Args>(args)...);
    return _array[index];
  }

  // element destruction
  void erase(size_t index) {
    if (index < _capacity && _array[index] != nullptr) {
      destroy_object(index);
    }
  }
  iterator& erase(iterator& it) {
    if (it != end()) {
      size_t index = it._index;
      it++;
      erase(index);
    }
    return it;
  }
  void clear() {
    for (size_t i = 0; i < _capacity; i++) {
      erase(i);
    }
  }

  // copying in elements
  void copy_at(size_t index, const T& object) {
    if (index < _capacity) {
      erase(index);
      create_object(index, object);
    }
  }
  T* copy_first(const T& object) {
    size_t index = get_first_empty();
    if (index < _capacity) {
      create_object(index, object);
      return _array[index];
    }
    return nullptr;
  }

  // container information
  size_t size() const {
    return _num;
  }
  size_t capacity() const {
    return _capacity;
  }
  bool empty() const {
    return _num == 0;
  }

  // element information
  size_t get_index(const T& object) const {
    if (_capacity > 0) {
      T** const endPtr = _array + _capacity;
      T** const ptr = std::find(_array, endPtr, &object);
      if (ptr != endPtr) {
        return ptr - &_array[0];
      }
    }
    // not from this list
    return std::numeric_limits<size_t>::max();
  }

  // iterator definitions: long, clusterfucky, involves templates, you've been warned
private:
  template <typename Base, typename Container>
  class iterator_base
  {
  public:
    typedef std::forward_iterator_tag iterator_category;

    bool operator==(const iterator_base& other) const {
      return _container == other._container && _index == other._index;
    };
    bool operator!=(const iterator_base& other) const {
      return _container != other._container || _index != other._index;
    };

    iterator_base& operator++() {
      if (_index < _container->capacity()) {
        _index++;
        while (_index < _container->capacity() && (*_container)[_index] == nullptr) {
          _index++;
        }
      }
      return *this;
    }
    inline iterator_base& operator++(int) {
      return ++(*this);
    }

    Base& operator*() const {
      return *(*_container)[_index];
    }
    Base* operator->() const {
      return (*_container)[_index];
    }

  protected:
    iterator_base(Container& container)
    : _container(&container)
    , _index(0)
    {
      if (_container->capacity() > 0 && (*_container)[_index] == nullptr) {
        (*this)++;
      }
    }

    friend class SlotArray;

  private:
    Container* _container;
  protected:
    size_t _index;
  };

public:
  class iterator : public iterator_base<T, SlotArray>
  {
  protected:
    iterator(SlotArray& container)
    : iterator_base<T, SlotArray>(container)
    {}

    iterator(SlotArray& container, size_t startIndex)
    : iterator_base<T, SlotArray>(container)
    {
      this->_index = startIndex;
    }

    friend class SlotArray;
  };

  class const_iterator : public iterator_base<const T, const SlotArray>
  {
  protected:
    const_iterator(const SlotArray& container)
    : iterator_base<const T, const SlotArray>(container)
    {}

    const_iterator(const SlotArray& container, size_t startIndex)
    : iterator_base<const T, const SlotArray>(container)
    {
      this->_index = startIndex;
    }

    friend class SlotArray;
  };

private:
  size_t get_first_empty() const {
    size_t i;
    for (i = 0; i < _capacity; i++) {
      if (_array[i] == nullptr) {
        break;
      }
    }
    return i;
  }

  // private functions to create/destroy, even if simple, to make sure we don't screw up _num counting
  template <class ...Args>
  void create_object(size_t index, Args&&... args) {
    T*& ptr = _array[index];
    assert(ptr == nullptr); // this slot had better be cleared out before this is called
    ptr = new T(std::forward<Args>(args)...);
    _num++;
  }

  void destroy_object(size_t index) {
    T*& ptr = _array[index];
    assert(ptr != nullptr); // this should never be called for null pointers
    delete ptr;
    ptr = nullptr;
    _num--;
  }

  size_t _capacity;
  size_t _num;
  T** _array;
};

} // namespace Util
} // namespace Anki

#endif // __Util_SlotArray_H__
