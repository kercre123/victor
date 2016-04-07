#include <stdint.h>
#include <stddef.h>
#include <string.h>

extern "C" {
  #include "nrf.h"
}

#include "storage.h"

static const uint32_t STORAGE_ADDRESS = (0x18000 + 0x6C00);
static const int STORAGE_SIZE = 0x400;
static const int TOTAL_WORDS = (STORAGE_SIZE - 4) / sizeof(uint32_t);

// upper 16bits are a revision
static const uint32_t MAGICKEY_VALUE = 0x01005a43;

struct StorageTOC {
  union {
    struct {
      uint8_t  key;
      uint8_t  length;
      uint16_t address;
    };
    uint32_t raw;
  };
};

struct StorageData {
  uint32_t  magickey;
  union {
    StorageTOC toc[1];
    uint32_t raw[TOTAL_WORDS];
  };
};

static StorageData* const STORAGE = (StorageData*) STORAGE_ADDRESS;

static StorageTOC* end_of_toc;
static uint32_t* start_of_data;

// These are our raw access calls
static void storage_erase() {
  int address = STORAGE_ADDRESS;
  
  while (address < STORAGE_ADDRESS + STORAGE_SIZE) {
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->ERASEPAGE = address;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    
    address += NRF_FICR->CODEPAGESIZE;
  }
}

static StorageError storage_write_word(uint32_t* address, uint32_t word) {
  if ((int)address < STORAGE_ADDRESS || (int)address >= STORAGE_ADDRESS + STORAGE_SIZE) {
    return STORAGE_OUT_OF_BOUNDS;
  }

  if (~0 != *address) {
    return STORAGE_NOT_ERASED;
  }

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  *address = word;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

  return STORAGE_OK;
}

static void storage_level(void) {
  StorageData memory;

  // Format in memory copy
  memset(&memory, 0xFF, sizeof(memory));
  memory.magickey = MAGICKEY_VALUE;

  // Start creating a limited TOC
  StorageTOC *last_toc = &memory.toc[0];
  StorageTOC *cursor = &STORAGE->toc[0];
  bool collapse = false;
  
  // Reduce repeat keys
  while (cursor < end_of_toc) {
    StorageTOC *locate = &memory.toc[0];
    
    // See if this is a redundant key
    while (locate < last_toc) {
      if (locate->key == cursor->key) {
        collapse = true;
        break ;
      }
      locate++;
    }
    
    memcpy(locate, cursor++, sizeof(StorageTOC));
    
    // New key, collapse
    if (locate == last_toc) { last_toc++; }
  }
  
  // No duplicate keys, do not flaten
  if (!collapse) return ;

  // Flatten data
  cursor = &memory.toc[0];
  uint32_t* start_of_memory = (uint32_t*)&memory;
  uint32_t* end_of_memory = (uint32_t*)((int)&memory + STORAGE_SIZE);
  
  while (cursor < last_toc) {
    int words = (cursor->length + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    
    end_of_memory -= words;
    memcpy(end_of_memory, (void*)(STORAGE_ADDRESS + cursor->address), cursor->length);
    cursor->address = (int)end_of_memory - (int)start_of_memory;
    cursor++;
  }

  // Write memory copy back to disk
  storage_erase();
  for (int i = 0; i < STORAGE_SIZE / sizeof(uint32_t); i++) {
    uint32_t word = *(start_of_memory++);
    
    if (word == ~0) { continue ; }
    storage_write_word((uint32_t*)(STORAGE_ADDRESS + i), word);
  }
}

// These are our KVS functions
static inline void storage_format() {
  storage_erase();
  storage_write_word((uint32_t*)STORAGE_ADDRESS, MAGICKEY_VALUE);
}

void Storage::init() {
  if (STORAGE->magickey != MAGICKEY_VALUE)
  {
    storage_format();
  }

  // This is the next available TOC area
  end_of_toc = &STORAGE->toc[0];
  
  // Find an unallocated sector
  while (end_of_toc->raw != ~0) {
    end_of_toc++;
  }

  // This is the last byte written
  if (end_of_toc != STORAGE->toc) {
    start_of_data = (uint32_t*) (STORAGE_ADDRESS + end_of_toc[-1].address);
  } else {
    start_of_data = (uint32_t*)(STORAGE_ADDRESS + STORAGE_SIZE);
  }

  // Level storage on boot to make sure that we don't do this expensive operation unless we have to.
  storage_level();
}

static StorageTOC* find_key(StorageKey ident) {
  StorageTOC* end = &end_of_toc[-1];
  
  while (end >= STORAGE->toc) {
    if (end->key == ident) {
      return end;
    }
    end--;
  }
  
  return NULL;
}

static bool store_key(StorageKey ident, const void* data, int length) {
  // Find 32-bit write address
  int round_length = (length + sizeof(uint32_t) - 1) / sizeof(uint32_t);

  uint32_t* write_location = start_of_data - round_length;
  
  // Out of space error
  if ((void*)(write_location - 1) <= (void*)end_of_toc) {
    return false;
  }

  start_of_data = write_location;

  // Stage our structure
  StorageTOC toc;
  toc.key = ident;
  toc.length = length;
  toc.address = (int)write_location - STORAGE_ADDRESS;
  
  // Write to memory
  const uint32_t* raw_data = (const uint32_t*) data;

  storage_write_word((uint32_t*)(end_of_toc++), toc.raw);
  for (int i = 0; i < length; i += 4) {
    storage_write_word(write_location++, *(raw_data++));
  }

  return true;
}

StorageError Storage::get(StorageKey ident, void* data, int& length) {
  StorageTOC* toc = find_key(ident);
  
  if (toc == NULL) {
    return STORAGE_NOT_FOUND;
  }
  
  memcpy(data, (uint32_t*) (STORAGE_ADDRESS + toc->address), toc->length);
  length = toc->length;
  
  return STORAGE_OK;
}

StorageError Storage::set(StorageKey ident, const void* data, int length) {
  if (store_key(ident, data, length)) {
    return STORAGE_OK;
  }
  
  storage_level();

  if (!store_key(ident, data, length)) {
    return STORAGE_OUT_OF_SPACE;
  }

  return STORAGE_OK;
}

const void* Storage::get_lazy(StorageKey ident) {
  StorageTOC* toc = find_key(ident);
  
  if (toc == NULL) {
    return NULL;
  }
  
  return (const void*) (STORAGE_ADDRESS + toc->address);
}
