
#if 1
#include <stdint.h>
#include <stdio.h>

#undef FACTORY_TEST
#define FACTORY_TEST 1
#include "anki/cozmo/shared/factory/emrHelper.h"

extern "C" {
  int emr_set(uint8_t index, uint32_t value) {


    uint32_t* emr = (uint32_t*)Anki::Cozmo::Factory::GetEMR();
    if (emr) {
      emr[index] = value;
      return 0;
    }
    return 3;
  }

  int emr_get(uint8_t index, uint32_t* value) {
    uint32_t* emr = (uint32_t*)Anki::Cozmo::Factory::GetEMR();
    if (emr) {
      *value = emr[index];
      return 0;
    }
    return 3;
  }

}


  #endif
