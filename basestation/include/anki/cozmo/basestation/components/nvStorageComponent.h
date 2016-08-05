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

#include "anki/cozmo/basestation/robotDataBackupManager.h"
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
             const u8* data, size_t size,
             NVStorageWriteEraseCallback callback = {},
             bool broadcastResultToGame = false);
  
  bool Write(NVStorage::NVEntryTag tag,
             const std::vector<u8>* data,
             NVStorageWriteEraseCallback callback = {},
             bool broadcastResultToGame = false);
  
  // Erase the given entry from robot flash.
  // Returns true if request was successfully sent.
  // If broadcastToGame == true, a single MessageEngineToGame::NVStorageOpResult
  // will be broadcast when the operation is complete.
  bool Erase(NVStorage::NVEntryTag tag,
             NVStorageWriteEraseCallback callback = {},
             bool broadcastResultToGame = false);
  
  // Erases all data from robot flash.
  // IF includeFactory == true, THE FACTORY PARTITION IS ALSO ERASED
  // WHICH WIPES CAMERA CALIBRATION AMONG OTHER THINGS.
  // IF YOU'RE NOT COMPLETELY SURE YOU SHOULD DO IT, THEN YOU ABSOLUTELY SHOULDN'T!!!
  // NOTE: THIS FUNCTION FORCES A ROBOT REBOOT CAUSING DISCONNECT.
  bool WipeAll(bool includeFactory = false,
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

  // Returns true if this component is about to send a request or is currently waiting on a request
  bool HasPendingRequests();
  
  // Returns the word-aligned size that nvStorage automaticaly rounds up to on all saved content.
  // e.g. If you write 10 bytes to tag 2 and then read back tag 2, you will receive 12 bytes.
  static size_t MakeWordAligned(size_t size);

  // Event/Message handling
  template<typename T>
  void HandleMessage(const T& msg);
  
  // Kevin's sandbox function for testing
  // For dev only!
  void Test();
  
  RobotDataBackupManager& GetRobotDataBackupManager() { return _backupManager; }
 
private:
  
  // Info about a single write request
  struct WriteDataObject {
    WriteDataObject(NVStorage::NVEntryTag tag, std::vector<u8>* dataVec)
    : baseTag(tag)
    , nextTag(static_cast<u32>(tag))
    , sendIndex(0)
    , data(dataVec)
    { }
    
    ~WriteDataObject() {
      Util::SafeDelete(data);
    }

    NVStorage::NVEntryTag baseTag;
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> *data;
  };
  
  // Info on how to handle ACK'd writes/erases
  struct WriteDataAckInfo {
    WriteDataAckInfo()
    : numTagsLeftToAck(0)
    , callback({})
    , writeNotErase(true)
    , broadcastResultToGame(false)
    , timeoutTimeStamp(0)
    { }
    
    u32  numTagsLeftToAck;
    u32  numFailedWrites;
    
    NVStorageWriteEraseCallback callback;
    bool writeNotErase;
    bool broadcastResultToGame;
    TimeStamp_t timeoutTimeStamp;
  };
  
  // Info on how to handle read-requested data
  struct RecvDataObject {
    RecvDataObject()
    : data(nullptr)
    , callback({})
    , deleteVectorWhenDone(true)
    , broadcastResultToGame(false)
    , timeoutTimeStamp(0)    
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
    TimeStamp_t timeoutTimeStamp;
  };
  
  // Stores blobs of a multi-blob messgage.
  // When all blobs received remainingIndices
  struct PendingWriteData {
    PendingWriteData() { tag = NVStorage::NVEntryTag::NVEntry_Invalid; }
    NVStorage::NVEntryTag tag;
    std::vector<u8> data;
    std::unordered_set<u8> remainingIndices;
  };
  
  
  // Struct for holding any type of request to the robot's non-volatile storage
  struct NVStorageRequest {
    
    // Write request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageWriteEraseCallback callback, std::vector<u8>* data, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_WRITE)
    , tag(tag)
    , writeCallback(callback)
    , data(data)
    , wipeFactory(false)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    // Erase request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageWriteEraseCallback callback, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_ERASE)
    , tag(tag)
    , writeCallback(callback)
    , wipeFactory(false)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    // WipeAll request
    NVStorageRequest(bool includeFactory, NVStorageWriteEraseCallback callback, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_WIPEALL)
    , writeCallback(callback)
    , wipeFactory(includeFactory)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    // Read request
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageReadCallback callback, std::vector<u8>* data, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_READ)
    , tag(tag)
    , readCallback(callback)
    , data(data)
    , wipeFactory(false)
    , broadcastResultToGame(broadcastResultToGame)
    {}
    
    NVStorage::NVOperation op;
    NVStorage::NVEntryTag tag;
    NVStorageWriteEraseCallback writeCallback;
    NVStorageReadCallback readCallback;
    std::vector<u8>* data;
    bool wipeFactory;
    bool broadcastResultToGame;
  };
  
  Robot&       _robot;
  
  // ====== Message retry ========
  // Last write and read message sent
  NVStorage::NVStorageWrite _writeMsg;
  NVStorage::NVStorageRead  _readMsg;
  
  // Number of attempted sends of the last write or read message
  u8 _numSendAttempts;
  
  // Max num of attempts allowed for sending a write or read message
  const u8 _kMaxNumSendAttempts = 8;

  // Returns false if number of allowable retries exceeded
  bool ResendLastWrite();
  bool ResendLastRead();

  
  // ======= Robot event handlers ======
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  
  void SendRequest(NVStorageRequest req);
  
  // Queues blobs for a multi-blob message from game and sends them to robot when all blobs received.
  bool QueueWriteBlob(const NVStorage::NVEntryTag tag, const u8* data, u16 dataLength, u8 blobIndex, u8 numTotalBlobs);
  
  // Clear any data that was received from game (via NVStorageWriteEntry) for writing
  void ClearPendingWriteEntry();
  
  void BroadcastNVStorageOpResult(NVStorage::NVEntryTag tag, NVStorage::NVResult res, NVStorage::NVOperation op);
  
  std::vector<Signal::SmartHandle> _signalHandles;

  // Whether or not this tag is composed of (potentially) multiple blobs
  bool IsMultiBlobEntryTag(u32 tag) const;
  
  // Whether or not this is a legal entry tag
  bool IsValidEntryTag(u32 tag) const;
  
  // Whether or not this is a factory entry tag
  bool IsFactoryEntryTag(u32 tag) const;
  
  // Whether or not this tag is in the special 0xc000xxxx partition
  bool IsSpecialEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed base tag
  u32 GetBaseEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed end-of-range tag
  u32 GetTagRangeEnd(u32 startTag) const;
  
  // Queue of write/erase/read requests to be sent to robot
  std::queue<NVStorageRequest> _requestQueue;
  
  // Queue of data to be sent to robot for writing
  std::queue<WriteDataObject> _writeDataQueue;
  
  // Gating flag to make sure that the previous write was acked before another is sent
  bool _wasLastWriteAcked;
  
  // Map of NVEntryTag to ACK handling struct for write and erase requests
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
  
  // Ack timeout
  // If an operation is not acked within this timeout then give up waiting for it.
  // (Average write/read rate is ~10KB/s)
  static constexpr u32 _kAckTimeout_ms = 12800;
  
  // Manages the robot data backups
  // The backup manager lives here as it needs to update the backup everytime we write new things to the robot
  RobotDataBackupManager _backupManager;
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
