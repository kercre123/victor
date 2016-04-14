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
  bool Write(NVStorage::NVEntryTag tag, u8* data, size_t size, bool broadcastResultToGame = false);
  
  // Erase the given entry from robot flash
  void Erase(NVStorage::NVEntryTag tag, bool broadcastResultToGame = false);
  
  // Request data stored on robot under the given tag.
  // Executes specified callback when data is received.
  // Copies data into specified data vector if non-null.
  // Returns true if request was successfully sent.
  // TODO: Some other message gets sent when the data is ready.
  using NVStorageReadCallback = std::function<void(u8* data, size_t size)>;
  bool Read(NVStorage::NVEntryTag tag,
            NVStorageReadCallback callback = {},
            std::vector<u8>* data = nullptr,
            bool broadcastResultToGame = false);

  void Update();

 
private:
  
  Robot&       _robot;
  
  // Robot event handlers
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  // Game event handlers
  void HandleNVStorageWriteEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleNVStorageReadEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleNVStorageEraseEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  
  std::vector<Signal::SmartHandle> _signalHandles;

  // Whether or not this tag is composed of (potentially) multiple blobs
  bool IsMultiBlobEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed base tag
  u32 GetBaseEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed end-of-range tag
  u32 GetTagRangeEnd(u32 startTag) const;
  
  struct WriteDataObject {
    WriteDataObject(NVStorage::NVEntryTag tag, std::vector<u8>&& dataVec, bool write) {
      baseTag = tag;
      nextTag = (u32)tag;
      sendIndex = 0;
      data = dataVec;
      writeNotErase = write;
    }
    NVStorage::NVEntryTag baseTag;
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> data;
    bool writeNotErase;
  };
  
  struct RecvDataObject {
    ~RecvDataObject() {
      if (deleteVectorWhenDone)
        delete data;
    }
    
    std::vector<u8> *data;
    NVStorageReadCallback callback;
    bool deleteVectorWhenDone;
    bool broadcastResultToGame;
  };
  
  
  // Queue of data to be sent to robot for saving
  std::queue<WriteDataObject> _dataToSendQueue;
  
  // Map of NVEntryTag to the number of tags expected to be confirmed written.
  std::unordered_map<u32, u32 > _numWriteTagsToConfirm;
  
  // Map of requested tag from robot to vector where the data should be stored
  std::unordered_map<u32, RecvDataObject> _recvDataMap;

  static constexpr u32 _kMaxNvStorageEntrySize       = 20000;
  static constexpr u32 _kMaxNvStorageBlobSize        = 1024;
  static constexpr u32 _kMaxNumBlobsInMultiBlobEntry = 40;
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
