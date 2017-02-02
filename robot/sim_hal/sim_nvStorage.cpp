/**
 * File: sim_nvStorage.cpp
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
#error This file (sim_nvStorage.cpp) should not be used without SIMULATOR defined.
#endif

#include "sim_nvStorage.h"
#include "anki/cozmo/robot/hal.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

// Whether or not to save to/load from file
#define LOAD_FROM_FILE 0

namespace Anki {
  namespace Cozmo {
    
    SimNVStorage::SimNVStorage()
    {
      // Initialize with 0xff
      nvStorage_.fill(0xff);
      
      
      if(LOAD_FROM_FILE)
      {
        // Reads from nvstorage.txt
        if(false == Util::FileUtils::CreateDirectory("nvStorage", false, true))
        {
          printf("Sim_Robot.SimNVStorage: Failed to create nvStorage directory\n");
          return;
        }
        std::vector<u8> file = Util::FileUtils::ReadFileAsBinary("nvStorage/nvstorage.bin");
        if(file.size() == 0)
        {
          return;
        }

        std::copy(file.data(), file.data() + file.size(), nvStorage_.begin() );
      }
    }
    
    SimNVStorage::~SimNVStorage()
    {
      if(LOAD_FROM_FILE)
      {
        std::vector<uint8_t> contents;
        contents.assign(nvStorage_.begin(), nvStorage_.end());
        Util::FileUtils::WriteFile("nvStorage/nvstorage.bin", contents);
      }
    }

    
    bool SimNVStorage::IsRequestInRange(u32 address, u32 length) {
      if ((address < NVStorage::NVConst_MIN_ADDRESS) || (address + length > NVStorage::NVConst_MAX_ADDRESS) ) {
        printf("Sim_Robot.SimNVStorage.InvalidRange: address 0x%x, length %u\n", address, length);
        return false;
      }
      return true;
    }
    
    NVStorage::NVResult SimNVStorage::Write(u32 address, u32* data, u32 length)
    {
      if (IsRequestInRange(address, length)) {
        u8* src = reinterpret_cast<u8*>(data);
        std::copy(src, src + length, nvStorage_.begin() + address - NVStorage::NVConst_MIN_ADDRESS);
        
        return NVStorage::NV_OKAY;
      }
      return NVStorage::NV_BAD_ARGS;
    }
    
    NVStorage::NVResult SimNVStorage::Read (u32 address, u32* data, u32 length)
    {
      if (IsRequestInRange(address, length)) {
        std::copy(nvStorage_.begin() + address - NVStorage::NVConst_MIN_ADDRESS,
                  nvStorage_.begin() + address - NVStorage::NVConst_MIN_ADDRESS + length,
                  reinterpret_cast<u8*>(data));
        return NVStorage::NV_OKAY;
      }
      return NVStorage::NV_BAD_ARGS;
    }
    
    NVStorage::NVResult SimNVStorage::Erase(u32 address)
    {
      if (IsRequestInRange(address , 0)) {
        // Address must be on 4K boundary
        if (address % 0x1000 == 0) {
          // Erase 4K bytes
          memset(nvStorage_.begin() + address - NVStorage::NVConst_MIN_ADDRESS, 0xff, 0x1000);
          return NVStorage::NV_OKAY;
        } else {
          printf("Sim_Robot.SimNVStorage.InvalidEraseAddr: address 0x%x\n", address);
        }
      }
      return NVStorage::NV_BAD_ARGS;
    }
    

    namespace HAL
    {
      namespace
      {
        SimNVStorage simNVStorage;
      }
      
      void FlashInit()
      {
        // Nothing to do here
      }
      
      NVStorage::NVResult FlashWrite(u32 address, u32* data, u32 length)
      {
        return simNVStorage.Write(address, data, length);
      }
      
      NVStorage::NVResult FlashRead (u32 address, u32* data, u32 length)
      {
        return simNVStorage.Read(address, data, length);
      }
      
      NVStorage::NVResult FlashErase(u32 address)
      {
        return simNVStorage.Erase(address);
      }

      bool FlashWriteOkay(u32 address, u32 length)
      {
        return simNVStorage.IsRequestInRange(address, length);
      }
      
    }
  }
}
