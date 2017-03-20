/**
 * File: pointerHandle.h
 *
 * Author: baustin
 * Created: 01/12/2016
 *
 * Description: Implements a smart handle to wrap externally owned pointers
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef UTIL_POINTER_HANDLE_H_
#define UTIL_POINTER_HANDLE_H_

#include <memory>
#include <type_traits>

namespace Anki {
namespace Util {

template <typename T, typename B = T>
class ExternalPointerHandle {
public:

  template <typename Derived>
  using SubHandle = ExternalPointerHandle<Derived, B>;

  class Owner {
  public:
    explicit Owner(T* ptr) : masterPtr(ptr) {}

    using base_type = B;
    using ptr_type = T;

    Owner(const Owner& other) = delete;
    Owner& operator=(const Owner&) = delete;
    Owner(Owner&& other) noexcept = default;
    Owner& operator=(Owner&&) noexcept = default;

    T* get() const {
      return static_cast<T*>(masterPtr.get());
    }

    T* operator->() const {
      return get();
    }

    T& operator*() const {
      return *get();
    }

    explicit operator bool() const {
      return (bool)masterPtr;
    }
    
    void reset() {
      masterPtr.reset();
    }
    
    void reset(T* ptr) {
      masterPtr.reset(ptr);
    }

    std::weak_ptr<B> getHandle() const { return std::weak_ptr<B>(masterPtr); }
  private:
    std::shared_ptr<B> masterPtr;
  };

  explicit ExternalPointerHandle(const Owner& ptr) : handle(ptr.getHandle()) {}
  explicit ExternalPointerHandle(std::nullptr_t) {}
  explicit ExternalPointerHandle() {}

  // can be constructed from a more-derived handle that shares our base type
  // (casting from a more-base handle requires explicit usage of cast() below)
  template <typename X, typename = typename std::enable_if<std::is_base_of<T, X>::value && !std::is_same<T, X>::value>::type>
  ExternalPointerHandle(const ExternalPointerHandle<X, B>& other) : handle(other.handle) {}

  // can construct from a more-derived owner
  template <typename TOwner, typename = typename std::enable_if<
        std::is_base_of<T, typename TOwner::ptr_type>::value
    &&  std::is_base_of<B, typename TOwner::base_type>::value
    && (!std::is_same<T, typename TOwner::ptr_type>::value
        || !std::is_same<B, typename TOwner::base_type>::value)
    &&  std::is_same<TOwner, typename ExternalPointerHandle<typename TOwner::ptr_type, typename TOwner::base_type>::Owner>::value
  >::type>
  ExternalPointerHandle(const TOwner& other) : handle(other.getHandle()) {}

private:
  explicit ExternalPointerHandle(const std::weak_ptr<B>& weakPtr) : handle(weakPtr) {}
public:

  ExternalPointerHandle& operator=(std::nullptr_t) {
    handle.reset();
    return *this;
  }

  T* get() const {
    return static_cast<T*>(handle.lock().get());
  }

  T* operator->() const {
    return get();
  }

  T& operator*() const {
    return *get();
  }

  explicit operator bool() const {
    return !handle.expired();
  }

  template <typename Derived>
  ExternalPointerHandle<Derived, B> cast() {
    return ExternalPointerHandle<Derived, B>(handle);
  }

private:
  std::weak_ptr<B> handle;

  template <typename T1, typename T2>
  friend class ::Anki::Util::ExternalPointerHandle;
};

}; // namespace
}; // namespace

#endif
