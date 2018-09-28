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

#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"
#include "clad/types/nvStorageTypes.h"
#include "coretech/common/shared/types.h"

#include <vector>
#include <map>
#include <unordered_map>

namespace Anki {
namespace Vector {

class Robot;
class CozmoContext;  
  
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
  
  // TODO: For COZMO_V2 consider making Write(), Read(), and Erase() just return NVResult
  //       and and not take a callback since they should return immediately.
  
  // Save data to robot under the given tag.
  // Returns true if request was successfully sent.
  using NVStorageWriteEraseCallback = std::function<void(NVStorage::NVResult res)>;
  bool Write(NVStorage::NVEntryTag tag,
             const u8* data, size_t size,
             NVStorageWriteEraseCallback callback = {});
  
  bool Write(NVStorage::NVEntryTag tag,
             const std::vector<u8>* data,
             NVStorageWriteEraseCallback callback = {});
  
  // Erase the given entry from robot flash.
  // Returns true if request was successfully sent.
  bool Erase(NVStorage::NVEntryTag tag,
             NVStorageWriteEraseCallback callback = {});
  
  // Erases all data from robot flash.
  bool WipeAll(NVStorageWriteEraseCallback callback = {});
  
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
  // Returns true if request was successfully sent.
  using NVStorageReadCallback = std::function<void(u8* data, size_t size, NVStorage::NVResult res)>;
  bool Read(NVStorage::NVEntryTag tag,
            NVStorageReadCallback callback = {},
            std::vector<u8>* data = nullptr);
  
  // Returns the word-aligned size that nvStorage automaticaly rounds up to on all saved content.
  // e.g. If you write 10 bytes to tag 2 and then read back tag 2, you will receive 12 bytes.
  static size_t MakeWordAligned(size_t size);

private:
  
  Robot*       _robot = nullptr;
  
  // Map of all stored data
  using TagDataMap = std::unordered_map<NVStorage::NVEntryTag, std::vector<u8> >;
  TagDataMap _tagDataMap;
  
  // Data file read/write methods
  void LoadDataFromFiles();
  bool WriteEntryToFile(NVStorage::NVEntryTag tag);
  
  // Path of NVStorage data folder
  std::string _kStoragePath;

# ifdef SIMULATOR
  void LoadSimData();
# endif
  
  // Whether or not this is a legal entry tag
  bool IsValidEntryTag(NVStorage::NVEntryTag tag) const;
  
  // Whether or not this is a factory entry tag
  bool IsFactoryEntryTag(NVStorage::NVEntryTag tag) const;
  
  bool Erase(NVStorage::NVEntryTag tag,
             NVStorageWriteEraseCallback callback,
             u32 eraseSize);  
  
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
