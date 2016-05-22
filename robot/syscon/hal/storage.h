#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>

enum StorageKey {
  STORAGE_AES_KEY
};

enum StorageError {
  STORAGE_OK,
  STORAGE_OUT_OF_SPACE,
  STORAGE_NOT_FOUND,
  STORAGE_NOT_ERASED,
  STORAGE_OUT_OF_BOUNDS
};

namespace Storage {
  // Verify the storage on boot
  void init();
  const void* get_lazy(StorageKey ident);
  StorageError get(StorageKey ident, void* data, int& length);
  StorageError set(StorageKey ident, const void* data, int length);
}

#endif
