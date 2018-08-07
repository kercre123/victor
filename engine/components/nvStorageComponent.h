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


#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"


#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"
#include "clad/types/nvStorageTypes.h"
#include "coretech/common/shared/types.h"

#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace Anki {
namespace Vector {

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
  
  
class NVStorageComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public: 
  NVStorageComponent();
  virtual ~NVStorageComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
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

private:
  
  Robot*       _robot = nullptr;
  
  // Map of all stored data
  using TagDataMap = std::unordered_map<u32, std::vector<u8> >;
  TagDataMap _tagDataMap;
  
  // Data file read/write methods
  void LoadDataFromFiles();
  bool WriteEntryToFile(u32 tag);
  
  // Path of NVStorage data folder
  std::string _kStoragePath;

# ifdef SIMULATOR
  void LoadSimData();
# endif
  
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
  friend class BehaviorPlaypenTest;
  void EnableWritingFactory(bool enable) { _writingFactory = enable; }

  // Opens the file (block device) which contains camera calibration
  // File seek position will be set to the beginning of camera calibration
  int OpenCameraCalibFile(int openFlags);

  // Reads the camera calibration and populates the corresponding entry in
  // _tagDataMap
  void ReadCameraCalibFile();

  // Writes the camera calibration to file (block device)
  // iter should point to the camera calibration entry in _tagDataMap
  bool WriteCameraCalibFile(const TagDataMap::iterator& iter);
};

} // namespace Vector
} // namespace Anki

#endif /* defined(__Cozmo_Basestation_NVStorageComponent_H__) */
