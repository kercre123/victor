#include <stdint.h>
#include <stddef.h>
#include <string.h>

extern "C" {
  #include "nrf.h"
}

#include "storage.h"

// upper 16bits are a revision
static const uint32_t MAGICKEY_VALUE = 0x01005a43;
static const int STORAGE_SIZE = 0x400;
static const uint32_t STORAGE_ADDRESS = (0x1F000 - STORAGE_SIZE);
static const int TOTAL_WORDS = (STORAGE_SIZE - sizeof(MAGICKEY_VALUE)) / sizeof(uint32_t);
static const uint32_t UNUSED_ENTRY = ~0;

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

struct StorageEntry {
  const void* ptr;
  int length;
};

static StorageData* const STORAGE = (StorageData*) STORAGE_ADDRESS;

static StorageEntry toc[STORAGE_TOTAL_KEYS];
static int words_left;
static int end_of_data;
static int end_of_toc;

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

  // Address was not erased
  if (~0 != *address) {
    return STORAGE_NOT_ERASED;
  }

  // Do not write an empty word
  if (word == ~0) {
    return STORAGE_OK;
  }
  
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  *address = word;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

  return STORAGE_OK;
}

static void setup_storage(void) {
  // Initialize our in memory FAT
  memset(toc, 0, sizeof(toc));
  end_of_data = STORAGE_SIZE;

  for (end_of_toc = 0; STORAGE->toc[end_of_toc].raw != UNUSED_ENTRY; end_of_toc++) {
    StorageTOC* entry = &STORAGE->toc[end_of_toc];
    
    toc[entry->key].ptr = (const void*)(STORAGE_ADDRESS + entry->address);
    toc[entry->key].length = entry->length;
    
    end_of_data = entry->address;
  }

  // Since we sometimes level more compact, we have to make sure we only write on word boundaries
  end_of_data &= ~3;
  
  words_left = (end_of_data - sizeof(MAGICKEY_VALUE)) / sizeof(uint32_t) - (end_of_toc + 1);
}

static int write_words(int bytes) {  
  return (bytes + sizeof(uint32_t) - 1) / sizeof(uint32_t);
}

static void storage_level(void) {
  StorageData memory;

  // Format in memory copy
  memset(&memory, 0xFF, sizeof(memory));
  memory.magickey = MAGICKEY_VALUE;

  // Empty memory points
  int data_write_ptr = TOTAL_WORDS;
  int toc_write_ptr = 0;

  for (int i = 0; i < STORAGE_TOTAL_KEYS; i++) {
    // Null entry
    if (!toc[i].length) {
      continue ;
    }

    // Copy in our data (could pack tighter)
    data_write_ptr -= write_words(toc[i].length);
    memcpy(&memory.raw[data_write_ptr], toc[i].ptr, toc[i].length);

    // Setup our toc
    StorageTOC* entry = &memory.toc[toc_write_ptr++];
    entry->length = toc[i].length;
    entry->key = i;
    entry->address = data_write_ptr * sizeof(uint32_t) + sizeof(MAGICKEY_VALUE);
  }

  // Do not need to level memory
  if (!memcmp(&memory, STORAGE, sizeof(memory))) {
    return ;
  }
  
  // Write back to memory
  uint32_t* read = (uint32_t*) &memory;
  uint32_t* write = (uint32_t*) STORAGE_ADDRESS;
  
  storage_erase();
  for (int i = 0; i < sizeof(memory) / sizeof(uint32_t); i++) {
    storage_write_word(&write[i], read[i]);
  }

  // Reconfigure out flash space
  setup_storage();
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

  setup_storage();

  // Level storage on boot to make sure that we don't do this expensive operation unless we have to.
  storage_level();
}

static bool store_key(StorageKey ident, const void* data, int length) {
  int word_length = write_words(length);

  // Do we have enough space for the entire entry plus the TOC entry
  if (words_left < word_length + 1) {
    return false;
  }

  words_left -= word_length + 1;

  // Write and allocate data space
  end_of_data -= word_length * sizeof(uint32_t);
  const uint8_t* source = (const uint8_t*)data;
  uint32_t* target = (uint32_t*)(STORAGE_ADDRESS + end_of_data);
  
  for (int i = 0; i < word_length; i++) {
    uint32_t data;
    memcpy(&data, &source[i*sizeof(data)], sizeof(data));
    storage_write_word(&target[i], data);
  }
  
  // Write out our table of contents
  StorageTOC toc;

  toc.key = ident;
  toc.length = length;
  toc.address = end_of_data;
  storage_write_word(&STORAGE->toc[end_of_toc++].raw, toc.raw);

  return true;
}

StorageError Storage::set(StorageKey ident, const void* data, int length) {
  StorageEntry* entry = &toc[ident];

  // Check if this key needs to be updated
  if (entry->length == length && !memcmp(data, entry->ptr, length)) {
    return STORAGE_OK;
  }

  if (store_key(ident, data, length)) {
    return STORAGE_OK;
  }
  
  storage_level();

  if (store_key(ident, data, length)) {
    return STORAGE_OK;
  }

  return STORAGE_OUT_OF_SPACE;
}

const void* Storage::get(StorageKey ident, int& length) {
  length = toc[ident].length;
  return toc[ident].ptr;
}

const void* Storage::get(StorageKey ident) {
  return toc[ident].ptr;
}
