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

#ifndef COZMO_V2
#include "anki/cozmo/basestation/robotDataBackupManager.h"
#endif
#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"
#include "clad/types/nvStorage.h"
#include "anki/common/types.h"

#include <vector>
#include <queue>
#include <map>
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
  
  // Get the maximum number of bytes that can be saved for the given tag
  u32 GetMaxSizeForEntryTag(NVStorage::NVEntryTag tag);
  
  // TODO: For COZMO_V2 consider making Write(), Read(), and Erase() just return NVResult
  //       and and not take a callback since they should return immediately.
  
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
  bool WipeAll(NVStorageWriteEraseCallback callback = {},
               bool broadcastResultToGame = false);
  
  bool WipeFactory(NVStorageWriteEraseCallback callback = {});
  
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
  
# ifndef COZMO_V2
  RobotDataBackupManager& GetRobotDataBackupManager() { return _backupManager; }
# endif
 
private:
  
  Robot&       _robot;
  
# pragma mark --- Start of Cozmo 2.0 only members ---  
# ifdef COZMO_V2
  
  // Map of all stored data
  using TagDataMap = std::unordered_map<u32, std::vector<u8> >;
  TagDataMap _tagDataMap;
  
  // Data file read/write methods
  void LoadDataFromFiles();
  void WriteEntryToFile(u32 tag);
  
  // Path of NVStorage data folder
  const std::string _kStoragePath;

# ifdef SIMULATOR
  void LoadSimData();
# endif
  
# pragma mark --- End of Cozmo 2.0 only members ---
# else
# pragma mark --- Start of Cozmo 1.x only members ---
  
  static constexpr u32 _kHeaderMagic = 0x435a4d4f; // "CZMO" in hex
  struct NVStorageHeader {
    u32 magic = _kHeaderMagic;
    u32 version = 0;
    u32 dataSize = 0;
    u16 dataVersion = 0;
  };
  
  // Info about a single write request
  struct WriteDataObject {
    WriteDataObject()
    : baseTag(NVStorage::NVEntryTag::NVEntry_NEXT_SLOT)
    , nextTag(0)
    , sendIndex(0)
    , data(nullptr)
    , sending(false)
    { }
    
    WriteDataObject(NVStorage::NVEntryTag tag, std::vector<u8>* dataVec)
    : baseTag(tag)
    , nextTag(static_cast<u32>(tag))
    , sendIndex(0)
    , data(dataVec)
    , sending(false)
    { }
    
    void ClearData() {
      Util::SafeDelete(data);
    }
    
    ~WriteDataObject() {
      ClearData();
    }
    
    NVStorage::NVEntryTag baseTag;
    u32 nextTag;
    u32 sendIndex;
    std::vector<u8> *data;
    bool sending;
  };
  
  // Info on how to handle ACK'd writes/erases
  struct WriteDataAckInfo {
    WriteDataAckInfo()
    : tag(NVStorage::NVEntryTag::NVEntry_NEXT_SLOT)
    , callback({})
    , broadcastResultToGame(false)
    , timeoutTimeStamp(0)
    , pending(false)
    { }

    NVStorage::NVEntryTag tag;
    NVStorageWriteEraseCallback callback;
    bool broadcastResultToGame;
    TimeStamp_t timeoutTimeStamp;
    bool pending;
  };
  
  // Info on how to handle read-requested data
  struct RecvDataObject {
    RecvDataObject()
    : tag(NVStorage::NVEntryTag::NVEntry_NEXT_SLOT)
    , data(nullptr)
    , callback({})
    , deleteVectorWhenDone(true)
    , broadcastResultToGame(false)
    , timeoutTimeStamp(0)
    , pending(false)
    , dataSizeKnown(false)
    { }
    
    void ClearData() {
      if (deleteVectorWhenDone) {
        Util::SafeDelete(data);
      }
    }
    
    ~RecvDataObject() {
      ClearData();
    }
    
    NVStorage::NVEntryTag tag;
    std::vector<u8> *data;
    NVStorageReadCallback callback;
    bool deleteVectorWhenDone;
    bool broadcastResultToGame;
    TimeStamp_t timeoutTimeStamp;
    bool pending;
    bool dataSizeKnown;
  };
  
  enum class NVSCState {
    IDLE,
    SENDING_WRITE_DATA,
    RECEIVING_DATA,
  };
  
  NVSCState _state;
  void SetState(NVSCState s);

  
  // Data currently being sent to robot for writing
  WriteDataObject _writeDataObject;
  
  // ACK handling struct for write and erase requests
  WriteDataAckInfo _writeDataAckInfo;
  
  // Received data handling struct
  RecvDataObject _recvDataInfo;
  
  // Manages the robot data backups
  // The backup manager lives here as it needs to update the backup everytime we write new things to the robot
  RobotDataBackupManager _backupManager;
  
  
  // ======= Robot event handlers ======
  void HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message);

  
  // ====== Message retry ========
  NVStorage::NVCommand _lastCommandSent;
  
  void SendErase(u32 tag);
  
  // Number of attempted sends of the last write or read message
  u8 _numSendAttempts;
  
  // Max num of attempts allowed for sending a write or read message
  const u8 _kMaxNumSendAttempts = 8;
  
  // Returns false if number of allowable retries exceeded
  bool ResendLastCommand();
  
  // Size of header that should be at the start of all entries, except factory entries
  static constexpr u32 _kEntryHeaderSize      = sizeof(NVStorageHeader);
  static_assert(_kEntryHeaderSize % 4 == 0, "NVStorageHeader must have a 4-byte aligned size");  // Using sizeof should ensure this is true, but checking just in case.
  
  // Ack timeout
  // If an operation is not acked within this timeout then give up waiting for it.
  // (Average write/read rate is ~10KB/s)
  static constexpr u32 _kAckTimeout_ms        = 5000;
  

  // Struct for holding any type of request to the robot's non-volatile storage
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
    NVStorageRequest(NVStorage::NVEntryTag tag, NVStorageWriteEraseCallback callback, bool broadcastResultToGame, u32 eraseSize = 0)
    : op(NVStorage::NVOperation::NVOP_ERASE)
    , tag(tag)
    , writeCallback(callback)
    , broadcastResultToGame(broadcastResultToGame)
    , eraseSize(eraseSize)
    {}
    
    // WipeAll request
    NVStorageRequest(NVStorageWriteEraseCallback callback, bool broadcastResultToGame)
    : op(NVStorage::NVOperation::NVOP_WIPEALL)
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
    u32 eraseSize;
  };
  
  // Queue of write/erase/read requests to be sent to robot
  std::queue<NVStorageRequest> _requestQueue;
  
  void ProcessRequest();
  
# endif // #ifdef COZMO_V2
# pragma mark --- End of Cozmo 1.x only members ---
  
  // Stores blobs of a multi-blob messgage.
  // When the first blob of a new tag is received, remainingIndices is populated with
  // all the indices it expects to receive. Indices are removed as blobs are received.
  // When all blobs are received remainingIndices should be empty.
  struct PendingWriteData {
    NVStorage::NVEntryTag tag;
    std::vector<u8> data;
    std::unordered_set<u8> remainingIndices;
    bool pending = false;
  };
  
  // Map of the max number of bytes that may be written at each entry tag
  static std::map<NVStorage::NVEntryTag, u32> _maxSizeTable;
  static std::map<NVStorage::NVEntryTag, u32> _maxFactoryEntrySizeTable;
  
  // Initialize _maxSizeTable
  void InitSizeTable();
  
  // Queues blobs for a multi-blob message from game and sends them to robot when all blobs received.
  bool QueueWriteBlob(const NVStorage::NVEntryTag tag, const u8* data, u16 dataLength, u8 blobIndex, u8 numTotalBlobs);
  
  // Clear any data that was received from game (via NVStorageWriteEntry) for writing
  void ClearPendingWriteEntry();
  
  void BroadcastNVStorageOpResult(NVStorage::NVEntryTag tag,
                                  NVStorage::NVResult res,
                                  NVStorage::NVOperation op,
                                  u8 index = 0,
                                  const u8* data = nullptr,
                                  size_t data_length = 0);
  
  void BroadcastNVStorageReadResults(NVStorage::NVEntryTag tag,
                                     u8* dataPtr = nullptr,
                                     size_t dataSize = 0);
  
  std::vector<Signal::SmartHandle> _signalHandles;

  // Whether or not this tag is composed of (potentially) multiple blobs.
  // Only applies to factory entries.
  bool IsMultiBlobEntryTag(NVStorage::NVEntryTag tag) const;
  
  // Whether or not this is a legal entry tag
  bool IsValidEntryTag(NVStorage::NVEntryTag tag) const;
  
  // Whether or not this is a factory entry tag
  bool IsFactoryEntryTag(NVStorage::NVEntryTag tag) const;
  bool IsPotentialFactoryEntryTag(u32 tag) const;
  bool IsTagInFactoryBlock(u32 tag) const;
  
  // Whether or not this tag is in the special 0xc000xxxx partition
  bool IsSpecialEntryTag(u32 tag) const;
  
  // Given any tag, returns the assumed base tag
  NVStorage::NVEntryTag GetBaseEntryTag(u32 tag) const;
  
  bool Erase(NVStorage::NVEntryTag tag,
             NVStorageWriteEraseCallback callback,
             bool broadcastResultToGame,
             u32 eraseSize);

  // Storage for in-progress-of-receiving multi-blob data
  // via MessageGameToEngine::NVStorageWriteEntry
  PendingWriteData _pendingWriteData;
  
  
  // Whether or not we should be writing headers
  bool _writingFactory = false;
  
  // Only BehaviorFactoryTest should ever call this function since it is special and is writing factory data
  friend class BehaviorFactoryTest;
  void EnableWritingFactory(bool enable) { _writingFactory = enable; }
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
