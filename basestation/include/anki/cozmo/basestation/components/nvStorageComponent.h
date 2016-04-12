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
#include <unordered_map>
#include <unordered_set>

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
  bool Write(NVStorage::NVEntryTag tag, u8* data, int size);
  
  // Erase the given entry from robot flash
  void Erase(NVStorage::NVEntryTag tag);
  
  // Request data stored on robot under the given tag.
  // Executes specified callback when data is received.
  // Copies data into specified data vector if non-null.
  // Returns true if request was successfully sent.
  // TODO: Some other message gets sent when the data is ready.
  using NVStorageReadCallback = std::function<void(u8* data, size_t size)>;
  bool Read(NVStorage::NVEntryTag tag, NVStorageReadCallback callback = {}, std::vector<u8>* data = nullptr);

  void Update();

 
private:
  
  Robot&       _robot;
  
  // Handlers
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  std::vector<Signal::SmartHandle> _signalHandles;

  // Whether or not this tag is composed of (potentially) multiple blobs
  bool IsMultiBlobEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed base tag
  u32 GetBaseEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed end-of-range tag
  u32 GetTagRangeEnd(u32 startTag) const;
  
  struct SendDataObject {
    SendDataObject(u32 tag, std::vector<u8>&& dataVec) {
      baseTag = tag;
      nextTag = tag;
      sendIndex = 0;
      data = dataVec;
    }
    u32 baseTag;
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> data;
  };
  
  struct RecvDataObject {
    ~RecvDataObject() {
      if (deleteVectorWhenDone)
        delete data;
    }
    
    std::vector<u8> *data;
    NVStorageReadCallback callback;
    bool deleteVectorWhenDone;
  };
  
  
  // Queue of pairs of (nextSendIndex, data) to be sent to robot for saving
  std::queue<SendDataObject> _dataToSendQueue;
  
  // Map of all tags sent for writing the specified NVEntryTag
  std::unordered_map<u32, std::unordered_set<u32> > _sentWriteTags;
  
  // Map of requested tag from robot to vector where the data should be stored
  std::unordered_map<u32, RecvDataObject> _recvDataMap;

  static constexpr u32 _kMaxNvStorageBlobSize        = 1024;
  static constexpr u32 _kMaxNumBlobsInMultiBlobEntry = 40;
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
