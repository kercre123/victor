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
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/nvStorage.h"
#include <map>
#include <queue>

namespace Anki {
  namespace Cozmo {
    class SimNVStorage
    {
    public:
      SimNVStorage();
      ~SimNVStorage();
      
      // Handles sending op/read result messages to engine with a delay mimicking that of the robot
      void NVUpdate();
      
      // Writes the data in msg to nvStorage
      void NVWrite(Anki::Cozmo::NVStorage::NVStorageWrite const& msg);
      
      // Sends the data in NVStorage at the requested tags in msg to engine
      void NVRead(Anki::Cozmo::NVStorage::NVStorageRead const& msg);

      // Wipes all data, optionally including factory, as long as the key is correct
      void NVWipeAll(Anki::Cozmo::NVStorage::NVWipeAll const& msg);
      
    private:
    
      // Returns true if the tag exists in nvStorage
      bool TagExists(uint32_t tag);
      
      // Returns true if this is a valid tag range
      bool IsValidRange(uint32_t start, uint32_t end);
      
      // Makes the data in the give blob word aligned (word = 4 bytes) by adding padding of zeros
      NVStorage::NVStorageBlob MakeWordAligned(NVStorage::NVStorageBlob blob);
    
      // Maximum size of nvStorage in bytes
      const f32 maxStorageSize_bytes = 128000;
      
      // Delay between sending op/read result messages back to engine
      const f32 delayBetweenMessages_ms = 200;
      
      // Time the last op/read result was sent
      f32 lastSendTime_ = 0;
      
      // Current size of nvStorage in bytes
      f32 curStorageSize_bytes_ = 0;
      
      // Stores blobs mapped by tags
      std::map<uint32_t, NVStorage::NVStorageBlob> nvStorage_;
      
      // Queues for holding op/read result messages to be sent to engine
      std::queue<RobotInterface::NVOpResultToEngine> nvOpResultMsgQueue_;
      std::queue<RobotInterface::NVReadResultToEngine> nvReadResultMsgQueue_;
    };
    
    namespace SimNVStorageSpace
    {
      void Update();
      void Write(Anki::Cozmo::NVStorage::NVStorageWrite const& msg);
      void Read(Anki::Cozmo::NVStorage::NVStorageRead const& msg);
      void WipeAll(Anki::Cozmo::NVStorage::NVWipeAll const& msg);
    }
  }
}
