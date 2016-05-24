/**
 * File: noncopyable.h
 *
 * Author: raul
 * Created: 05/02/2014
 *
 * Description: Base class for objects that are not meant to be copied.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_NONCOPYABLE_H_
#define UTIL_NONCOPYABLE_H_

namespace Anki{ namespace Util {

// delete copy constructor and assignment operator
class noncopyable
{
 protected:
  noncopyable() = default;
  ~noncopyable() = default;
  noncopyable( const noncopyable& ) = delete;
  noncopyable& operator=( const noncopyable& ) = delete;
};

}; // namespace
}; // namespace

#endif