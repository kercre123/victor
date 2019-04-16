/**
* File: sharedCircularBuffer.h
*
* Author: Ron Barry
* Created: 3/26/2019
*
* Description: Provides a way to share a circular buffer of data with other processes
*              via a block of shared memory. This has the advantage of being much
*              faster than a domain socket, as well as being readable my multiple
*              consumers, but at the cost of needing to allocate the entire buffe in
*              advance.
*
* Copyright: Anki, Inc. 2019
*
*/

#ifndef __AnimProcess_CozmoAnim_SharedCircularBuffer_H_
#define __AnimProcess_CozmoAnim_SharedCircularBuffer_H_

#include <dirent.h>
#include <fcntl.h>
#include "stdio.h"
#include <string>
#include "string.h"
#include "sys/errno.h"
#include "sys/ioctl.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <sys/mman.h>
#include <unistd.h>

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

namespace Anki{
namespace Util{

/* TODO
 * The startup process needs to remove the run/SharedCircularBuffer directory
 */ 

static const uint64_t header_magic_num = 0x08675309deadbeef;

typedef enum {
  GET_STATE_OKAY = 0,
  GET_STATE_BEHIND,
  GET_STATE_PLEASE_WAIT,
} GetState;

template<class T, uint32_t N>
class SharedCircularBuffer {
public:
  SharedCircularBuffer(const std::string name, bool owner) :
      _name(name),
      _initted(false),
      _owner(owner),
      _offset(-1)
  {
    Init();
  }

  ~SharedCircularBuffer() {
    if ((not _owner) && _initted) {
      --_buffer->reader_count;
    }
  }

  T* GetWritePtr() {
    return &_buffer->objects[_buffer->queued_count % N];
  }

  void Advance() {
    if (_owner) {
      ++_buffer->queued_count;
    }
  }

  GetState GetNext(T** object) {
    if (not _initted && not Init()) {
      // The writer hasn't started providing data yet.
      return GET_STATE_PLEASE_WAIT;
    }

    ++_offset;

    if (not _buffer->queued_count || _offset >= _buffer->queued_count) {
      --_offset;
      return GET_STATE_PLEASE_WAIT;
    }

    GetState get_state = GET_STATE_OKAY;
    if (_offset < _buffer->queued_count - (3*N/4)) {
      _offset = _buffer->queued_count - 1;
      get_state = GET_STATE_BEHIND;
    }

    *object = _buffer->objects[_offset%N];

    return get_state;
  }

  uint64_t GetNumReaders() {
    LOG_WARNING("GetNumReaders()", "_buffer: %p", _buffer);
    LOG_WARNING("GetNumReaders()", "GetNumReaders: %llu", _buffer->reader_count);

    return _buffer->reader_count;
  }


private:
  struct MemoryMap {
    uint64_t          header_magic_num; // This is a pointer because the value is stored in the memory map.
    uint64_t          queued_count;     // This is a pointer because the value is stored in the memory map.
    uint64_t          reader_count;     // Current number of buffer subscribers. NOT CURRENTLY SEMAPHORE-GUARDED
    T                 objects[N];
  };

  std::string        _name;             // The name of the file in /run that is mapped to this buffer.
  bool               _initted;          // Have we successfully set up for our role (owner | !owner)
  bool               _owner;            // Whether we're the writer.
  struct MemoryMap*  _buffer;
  uint64_t           _offset;           // For non-owners, the last object ID they retrieved.

  bool Init() {
    if(_initted) {
      return true;
    }

    uint32_t buffer_size = sizeof(struct MemoryMap);

    std::string fd_share_filename("/run/sharedCircularBuffer/");
    fd_share_filename += _name;

    int fd;
    if(_owner) {
      //TODO: Other applications may require a security review to lock down this file.
      fd = open(fd_share_filename.c_str(), O_RDWR | O_CREAT, 0777);  
      ftruncate(fd, buffer_size);
    } else {
      fd = open(fd_share_filename.c_str(), O_RDWR);
      if(fd < 0) {
        return false;
      }
    }
    _buffer = static_cast<struct MemoryMap*>(mmap(NULL, buffer_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
    if(_buffer == MAP_FAILED) {
      LOG_WARNING("SharedCircularBuffer", "Init() mmap call failed: %s", strerror(errno));
    }

    if(_owner) {
      _buffer->queued_count = 0;
      _buffer->reader_count = 0;
      _buffer->header_magic_num = header_magic_num;
    } else {
      ++_buffer->reader_count;
    }

    _initted = true;
    return true;
  }
};

} //namespace Util
} //namespace Anki

#endif //__AnimProcess_CozmoAnim_SharedCircularBuffer_H_