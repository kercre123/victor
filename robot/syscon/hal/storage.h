#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>

enum StorageKey {
  STORAGE_AES_KEY,
  STORAGE_CRASH_LOG_NRF,
  STORAGE_CRASH_LOG_K02,
  STORAGE_TOTAL_KEYS
};

enum StorageError {
  STORAGE_OK,
  STORAGE_OUT_OF_SPACE,
  STORAGE_NOT_ERASED,
  STORAGE_OUT_OF_BOUNDS
};

namespace Storage {
  // Verify the storage on boot
  void init();
  const void* get(StorageKey ident);
  const void* get(StorageKey ident, int& length);
  StorageError set(StorageKey ident, const void* data, int length);
}

#endif
