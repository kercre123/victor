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
//  class MessageEngineToGame;
}
  
namespace RobotInterface {
  class RobotToEngine;
}
  
  
class NVStorageComponent : private Util::noncopyable
{
public: 

  NVStorageComponent(Robot& inRobot, const CozmoContext* context);
  virtual ~NVStorageComponent() {};
  
  // Save data to robot under the given tag.
  // Returns true if request was successfully sent.
  // If broadcastResultToGame == true, each write (there are potentially multiple)
  // is acked with MessageEngineToGame::NVStorageOpResult.
  bool Write(NVStorage::NVEntryTag tag, u8* data, size_t size, bool broadcastResultToGame = false);
  
  // Erase the given entry from robot flash.
  // Returns true if request was successfully sent.
  // If broadcastToGame == true, a single MessageEngineToGame::NVStorageOpResult
  // will be broadcast when the operation is complete.
  bool Erase(NVStorage::NVEntryTag tag, bool broadcastResultToGame = false);
  
  // Request data stored on robot under the given tag.
  // Executes specified callback when data is received.
  // Copies data directly into specified data vector if non-null.
  // Returns true if request was successfully sent.
  // If broadcastResultToGame == true, each data blob received from robot
  // is forwarded to game via MessageEngineToGame::NVDataStorage and when
  // all blobs are received it will also trigger MessageEngineToGame::NVStorageOpResult.
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
  
  // Info about a single write/erase request
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
  
  // Info on how to handle ACK'd writes/erases
  struct WriteDataAckInfo {
    u32  numTagsLeftToAck;
    bool broadcastResultToGame;
    bool writeNotErase;
  };
  
  // Info on how to handle read-requested data
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
  
  
  // Queue of data to be sent to robot for writing/erasing
  std::queue<WriteDataObject> _writeDataQueue;
  
  // Map of NVEntryTag to ACK handling struct.
  std::unordered_map<u32, WriteDataAckInfo > _writeDataAckMap;
  
  // Map of requested NVEntryTag to received data handling struct
  std::unordered_map<u32, RecvDataObject> _recvDataMap;
  
  // Maximum size of a single blob
  static constexpr u32 _kMaxNvStorageBlobSize        = 1024;
  
  // Maximum number of blobs allowed in a multi-blob entry
  static constexpr u32 _kMaxNumBlobsInMultiBlobEntry = 20;
  
  // Maximum size of a single multi-blob write entry
  static constexpr u32 _kMaxNvStorageEntrySize       = _kMaxNvStorageBlobSize * _kMaxNumBlobsInMultiBlobEntry;
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
