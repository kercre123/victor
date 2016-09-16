/**
 * File: sim_nvStorage.h
 *
 * Author: Al Chaussee
 * Created: 4/21/2016
 *
 * Description:
 *    Simulated NVStorage for simulating reading and writing data to robot flash
 *    Is able to save and load data from file simulating how the NVStorage is constant
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef SIMULATOR
#error This file (sim_nvStorage.h) should not be used without SIMULATOR defined.
#endif

#include "anki/common/types.h"
#include "clad/types/nvStorage.h"
#include <array>


namespace Anki {
  namespace Cozmo {
    class SimNVStorage
    {
    public:
      SimNVStorage();
      ~SimNVStorage();
      
      NVStorage::NVResult Write(u32 address, u32* data, u32 length);
      NVStorage::NVResult Read (u32 address, u32* data, u32 length);
      NVStorage::NVResult Erase(u32 address);
      
    private:
      
      bool IsRequestInRange(u32 address, u32 length);
    
      // Maximum size of nvStorage in bytes
      static const u32 MAX_FLASH_SIZE = NVStorage::NVConst_MAX_ADDRESS - NVStorage::NVConst_MIN_ADDRESS;
      
      // Stores blobs mapped by tags
      std::array<uint8_t, MAX_FLASH_SIZE> nvStorage_;
    };
    
  }
}
