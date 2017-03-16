#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

#include "tasks.h"
#include "storage.h"

void Tasks::init() {
}

// Top 16 bytes of application space
const void* Tasks::aes_key() {
  return Storage::get(STORAGE_AES_KEY);
}

void Tasks::execute(const Task* task) {
}

void Tasks::manage(void) {
}
