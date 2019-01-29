#include "IONMemory.h"

#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <iostream>

#define ION_IOMMU_HEAP_ID 25

namespace Anki {

IONBuffer::IONBuffer()
  : IONBuffer(0)
{
}

IONBuffer::IONBuffer(size_t size)
  : _buffer_fd(0)
  , _handle(0)
  , _size(0)
  , _data(0)
{
  resize(size);
}

IONBuffer::~IONBuffer()
{
  IONAllocator::instance().deallocate(*this);
}

void IONBuffer::resize(size_t size)
{
  if (size != _size){
    IONAllocator::instance().deallocate(*this);
  }
  if (size > 0){
    IONAllocator::instance().allocate(size, *this);
  }
}

IONAllocator::IONAllocator()
{
  init();
}

IONAllocator::~IONAllocator()
{
  deinit();
}

IONAllocator& IONAllocator::instance()
{
  static IONAllocator gAllocator;
  return gAllocator;
}

void IONAllocator::allocate(size_t num_bytes, IONBuffer& buffer)
{
  int rc = 0;
  struct ion_handle_data handle_data;
  struct ion_allocation_data alloc;
  struct ion_fd_data ion_info_fd;
  void *data = NULL;

  //=====================
  // Allocate the buffer 

  memset(&alloc, 0, sizeof(alloc));
  alloc.len = num_bytes;
  /* to make it page size aligned */
  alloc.len = (alloc.len + 4095U) & (~4095U);
  alloc.align = 4096;
  alloc.flags = ION_FLAG_CACHED;   // use in conjunction with CL_MEM_HOST_WRITEBACK_QCOM
  // alloc.flags = 0;                 // use in conjunction with CL_MEM_HOST_UNCACHED_QCOM
  alloc.heap_id_mask = 0x1 << ION_IOMMU_HEAP_ID;
  rc = ioctl(_device_fd, ION_IOC_ALLOC, &alloc);
  if (rc < 0) {
    throw IONException(std::string(__FUNCTION__) + ": ION allocation failed: " + std::string(strerror(errno)));
  }

  //=====================
  // Share the buffer 
  memset(&ion_info_fd, 0, sizeof(ion_info_fd));
  ion_info_fd.handle = alloc.handle;
  rc = ioctl(_device_fd, ION_IOC_SHARE, &ion_info_fd);
  if (rc < 0) {
    throw IONException(std::string(__FUNCTION__) + ": ION map failed: " + std::string(strerror(errno)));
  }

  data = mmap(NULL,
    alloc.len,
    PROT_READ  | PROT_WRITE,
    MAP_SHARED,
    ion_info_fd.fd,
    0);

  if (data == MAP_FAILED) {
    throw IONException(std::string(__FUNCTION__) + ": ION mmap failed: " + std::string(strerror(errno)));
  }

  buffer._data = data;
  buffer._size = alloc.len;
  buffer._handle = ion_info_fd.handle;
  buffer._buffer_fd = ion_info_fd.fd;
}

void IONAllocator::deallocate(IONBuffer& buffer)
{
  if (buffer._size == 0 || buffer._data == nullptr)
    return;

  struct ion_handle_data handle_data;
  int rc = 0;

  if ((buffer._data != NULL) && (buffer._buffer_fd > 0) && (buffer._handle > 0)) {
    rc = munmap(buffer._data, buffer._size);
  }

  if (rc == -1) {
    throw IONException(std::string(__FUNCTION__) + ": ION unmap failed: " + std::string(strerror(errno)));
  }

  if (buffer._buffer_fd > 0) {
    close(buffer._buffer_fd);
    buffer._buffer_fd = 0;
  }

  memset(&handle_data, 0, sizeof(handle_data));
  handle_data.handle = buffer._handle;
  rc = ioctl(_device_fd, ION_IOC_FREE, &handle_data);
  if (rc != 0) {
    throw IONException(std::string(__FUNCTION__) + ": ION free failed: " + std::string(strerror(errno)));
  }

  buffer._data = nullptr;
  buffer._size = 0;
  buffer._handle = 0;
  buffer._buffer_fd = 0;
}

void IONAllocator::init()
{
  _device_fd = open("/dev/ion", O_RDONLY);
  if (_device_fd <= 0) {
    throw IONException(std::string(__FUNCTION__) + ": ION dev open failed: " + std::string(strerror(errno)));
  }
}

void IONAllocator::deinit()
{
  if (close(_device_fd) < 0){
    throw IONException(std::string(__FUNCTION__) + ": ION dev close failed: " + std::string(strerror(errno)));
  }
}

} /* namespace Anki */