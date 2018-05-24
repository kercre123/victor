
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#undef FACTORY_TEST
#define FACTORY_TEST 1
#include "anki/cozmo/shared/factory/emrHelper.h"

extern "C" {
  bool emr_is_locked(void) {
    const uint8_t index_PACKED_OUT_FLAG = 6;
    const Anki::Cozmo::Factory::EMR* emr;
    emr = Anki::Cozmo::Factory::GetEMR();
    return !emr || emr->data[index_PACKED_OUT_FLAG] > 0;
  }
  
  int emr_set(uint8_t index, uint32_t value) {
    const Anki::Cozmo::Factory::EMR* emr;
    emr = Anki::Cozmo::Factory::GetEMR();
    if (emr) {
      if( emr_is_locked() )
        return 4; //ERR_LOCKED
      uint32_t *item = (uint32_t*)&(emr->data[index]);
      *item = value;
      return 0;
    }
    return 3; //ERR_SYSTEM
  }

  int emr_get(uint8_t index, uint32_t* value) {
    const Anki::Cozmo::Factory::EMR* emr;
    emr = Anki::Cozmo::Factory::GetEMR();
    if (emr) {
      *value = emr->data[index];
      return 0;
    }
    return 3; //ERR_SYSTEM
  }

}
