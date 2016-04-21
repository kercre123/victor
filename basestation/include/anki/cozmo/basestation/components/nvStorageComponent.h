/**
 * File: NVStorageComponent.h
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Interface for storing and retrieving data from robot's non-volatile storage
 *
 * Copyright: Anki, Inc. 2016
 **/


#ifndef __Cozmo_Basestation_NVStorageComponent_H__
#define __Cozmo_Basestation_NVStorageComponent_H__

#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"
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
  using NVStorageWriteEraseCallback = std::function<void(NVStorage::NVResult res)>;
  bool Write(NVStorage::NVEntryTag tag,
             u8* data, size_t size,
             NVStorageWriteEraseCallback callback = {},
             bool broadcastResultToGame = false);
  
  // Erase the given entry from robot flash.
  // Returns true if request was successfully sent.
  // If broadcastToGame == true, a single MessageEngineToGame::NVStorageOpResult
  // will be broadcast when the operation is complete.
  bool Erase(NVStorage::NVEntryTag tag,
             NVStorageWriteEraseCallback callback = {},
             bool broadcastResultToGame = false);
  
  // Request data stored on robot under the given tag.
  // Executes specified callback when data is received.
  //
  // Parameters
  // =====================
  // data: If null, a vector is allocated automatically internally to hold the
  //       data that is received from the robot.
  //       Otherwise, the specified vector is used to hold the received data.
  //
  // callback: When all the data is received, this function is called on it.
  //
  // broadcastResultToGame: If true, each data blob received from robot
  //                        is forwarded to game via MessageEngineToGame::NVDataStorage and when
  //                        all blobs are received it will also trigger MessageEngineToGame::NVStorageOpResult.
  //
  // Returns true if request was successfully sent.
  using NVStorageReadCallback = std::function<void(u8* data, size_t size, NVStorage::NVResult res)>;
  bool Read(NVStorage::NVEntryTag tag,
            NVStorageReadCallback callback = {},
            std::vector<u8>* data = nullptr,
            bool broadcastResultToGame = false);

  void Update();

  // Kevin's sandbox function for testing
  // For dev only!
  void Test();
 
private:
  
  // Info about a single write/erase request
  struct WriteDataObject {
    WriteDataObject()
    : baseTag(NVStorage::NVEntryTag::NVEntry_Invalid)
    , nextTag(0)
    , sendIndex(0)
    , writeNotErase(true)
    {}
    WriteDataObject(NVStorage::NVEntryTag tag, std::vector<u8>* dataVec, bool writeNotErase)
    : baseTag(tag)
    , nextTag(static_cast<u32>(tag))
    , sendIndex(0)
    , data(dataVec)
    , writeNotErase(writeNotErase)
    { }
    
    WriteDataObject(const WriteDataObject& other)
    : baseTag(other.baseTag)
    , nextTag(other.nextTag)
    , sendIndex(other.sendIndex)
    , data(other.data)
    , writeNotErase(other.writeNotErase)
    { }
    
    ~WriteDataObject() {
      Util::SafeDelete(data);
    }

    
    NVStorage::NVEntryTag baseTag;
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> *data;
    bool writeNotErase;
  };
  
  // Info on how to handle ACK'd writes/erases
  struct WriteDataAckInfo {
    WriteDataAckInfo()
    : numTagsLeftToAck(0)
    , callback({})
    , writeNotErase(true)
    , broadcastResultToGame(false)
    { }
    u32  numTagsLeftToAck;
    NVStorageWriteEraseCallback callback;
    bool writeNotErase;
    bool broadcastResultToGame;
  };
  
  // Info on how to handle read-requested data
  struct RecvDataObject {
    RecvDataObject()
    : data(nullptr)
    , callback({})
    , deleteVectorWhenDone(true)
    , broadcastResultToGame(false)
    { }
    
    ~RecvDataObject() {
      if (deleteVectorWhenDone) {
        Util::SafeDelete(data);
      }
    }
    
    std::vector<u8> *data;
    NVStorageReadCallback callback;
    bool deleteVectorWhenDone;
    bool broadcastResultToGame;
  };
  
  // Stores blobs of a multi-blob messgage.
  // When all blobs received remainingIndices
  struct PendingWriteData {
    PendingWriteData() { tag = NVStorage::NVEntryTag::NVEntry_Invalid; }
    NVStorage::NVEntryTag tag;
    std::vector<u8> data;
    std::unordered_set<u8> remainingIndices;
  };
  
  
  
  struct NVStorageRequest {
    
    // Write request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageWriteEraseCallback callback, std::vector<u8>* data, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_WRITE)
    , tag(tag)
    , writeCallback(callback)
    , data(data)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    // Erase request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageWriteEraseCallback callback, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_ERASE)
    , tag(tag)
    , writeCallback(callback)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    // Read request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageReadCallback callback, std::vector<u8>* data, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_READ)
    , tag(tag)
    , readCallback(callback)
    , data(data)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    NVStorage::NVOperation op;
    NVStorage::NVEntryTag tag;
    NVStorageWriteEraseCallback writeCallback;
    NVStorageReadCallback readCallback;
    std::vector<u8>* data;
    bool broadcastResultToGame;
  };
  
  Robot&       _robot;
  
  // Robot event handlers
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  // Game event handlers
  void HandleNVStorageWriteEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleNVStorageReadEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleNVStorageEraseEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleNVStorageClearPartialPendingWriteEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  void SendRequest(NVStorageRequest req);
  
  // Queues blobs for a multi-blob message from game and sends them to robot when all blobs received.
  bool QueueWriteBlob(const NVStorage::NVEntryTag tag, u8* data, u16 dataLength, u8 blobIndex, u8 numTotalBlobs);
  
  void BroadcastNVStorageOpResult(NVStorage::NVEntryTag tag, NVStorage::NVResult res, NVStorage::NVOperation op);
  
  std::vector<Signal::SmartHandle> _signalHandles;

  // Whether or not this tag is composed of (potentially) multiple blobs
  bool IsMultiBlobEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed base tag
  u32 GetBaseEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed end-of-range tag
  u32 GetTagRangeEnd(u32 startTag) const;
  
  // Queue of write/erase/read requests to be sent to robot
  std::queue<NVStorageRequest> _requestQueue;
  
  // Queue of data to be sent to robot for writing/erasing
  std::queue<WriteDataObject> _writeDataQueue;
  
  // Map of NVEntryTag to ACK handling struct.
  std::unordered_map<u32, WriteDataAckInfo > _writeDataAckMap;
  
  // Map of requested NVEntryTag to received data handling struct
  std::unordered_map<u32, RecvDataObject> _recvDataMap;

  // Storage for in-progress-of-receiving multi-blob data
  // via MessageGameToEngine::NVStorageWriteEntry
  PendingWriteData _pendingWriteData;
  
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
