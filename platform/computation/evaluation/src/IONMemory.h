#ifndef __EVALUATOR_IONMEMORY_H__
#define __EVALUATOR_IONMEMORY_H__

#include <stdexcept>

#include "linux/ion.h"

namespace Anki
{

class IONException : public std::runtime_error {
public:
  IONException(const std::string& what) : std::runtime_error(what) {}
};

class IONBuffer {
public:
  IONBuffer();
  IONBuffer(size_t size);
  ~IONBuffer();

  void resize(size_t size);

  int fd() { return _buffer_fd; }
  void* data() { return _data; }
  size_t size() { return _size; }

private:
  friend class IONAllocator;

  int _buffer_fd;
  ion_user_handle_t _handle;
  size_t _size;
  void * _data;
};

class IONAllocator {
public:
  static IONAllocator& instance();

  void allocate(size_t size, IONBuffer& buffer);
  void deallocate(IONBuffer& buffer);
private:
  IONAllocator();
  ~IONAllocator();
  void init();
  void deinit();

  int _device_fd;
};

} /* namespace Anki */

#endif /* __EVALUATOR_IONMEMORY_H__ */