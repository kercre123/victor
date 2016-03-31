/**
 * File: NVStorageComponent.h
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Interface for storing and retrieving data from robot flash
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Cozmo_Basestation_NVStorageComponent_H__
#define __Cozmo_Basestation_NVStorageComponent_H__

#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"
#include "clad/types/nvStorage.h"
#include "anki/common/types.h"

#include <vector>
#include <queue>
#include <map>

namespace Anki {
namespace Cozmo {

class IExternalInterface;
template <typename Type>
class AnkiEvent;
class Robot;
class CozmoContext;

namespace ExternalInterface {
  class MessageGameToEngine;
  class MessageEngineToGame;
}
  
namespace RobotInterface {
//  class MessageHandler;
//  class EngineToRobot;
  class RobotToEngine;
//  enum class EngineToRobotTag : uint8_t;
//  enum class RobotToEngineTag : uint8_t;
} // end namespace RobotInterface
  
  
class NVStorageComponent : private Util::noncopyable
{
public: 

  NVStorageComponent(Robot& inRobot, const CozmoContext* context);
  virtual ~NVStorageComponent() {};
  
  // Save data to robot under the given tag.
  // Returns true if data passes checks to be sent to robot.
  // Caller should expect a MessageEngineToGame indicating success/failure of write.
  bool Save(NVStorage::NVEntryTag tag, u8* data, int size);
  
  // Request data stored on robot under the given tag to be written to data.
  // Returns true if request was successfully sent.
  // TODO: Some other message gets sent when the data is ready.
  using NVStorageBlobReceiveCallback = std::function<void(u8* data, size_t size)>;
  bool Request(NVStorage::NVEntryTag tag, std::vector<u8>& data, NVStorageBlobReceiveCallback callback = {});

  void Update();

 
private:
  
  Robot&       _robot;
  
  // Handlers
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  bool IsMultiBlobEntryTag(u32 tag) const;
  u32 GetBaseEntryTag(u32 tag) const;
  
  struct SendDataObject {
    SendDataObject(u32 tag, std::vector<u8>&& dataVec) {
      nextTag = tag;
      sendIndex = 0;
      data = dataVec;
    }
    
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> data;
  };
  
  struct RecvDataObject {
    std::vector<u8> *data;
    NVStorageBlobReceiveCallback callback;
  };
  
  // Queue of pairs of (nextSendIndex, data) to be sent to robot for saving
  std::queue<SendDataObject> _dataToSendQueue;
  
  // Map of requested tag from robot to vector where the data should be stored
//  std::map<NVStorage::NVEntryTag, std::vector<u8>* > _recvDataMap;
    std::map<NVStorage::NVEntryTag, RecvDataObject > _recvDataMap;

  static constexpr u32 _kMaxNvStorageBlobSize        = 1024;
  static constexpr u32 _kMaxNumBlobsInMultiBlobEntry = 40;
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
