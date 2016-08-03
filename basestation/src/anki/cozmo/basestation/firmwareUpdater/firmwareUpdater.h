/**
 * File: firmwareUpdater
 *
 * Author: Mark Wesley
 * Created: 03/25/16
 *
 * Description: Handles firmware updating (up or downgrading) of >=1 Cozmo robot(s)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_FirmwareUpdater_FirmwareUpdater_H__
#define __Cozmo_Basestation_FirmwareUpdater_FirmwareUpdater_H__


#include "anki/common/types.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/types/firmwareTypes.h"
#include "util/signals/simpleSignal_fwd.h"
#include <json/json-forwards.h>
#include <assert.h>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>


namespace Anki {
namespace Cozmo {
  

class CozmoContext;
class Robot;

namespace RobotInterface
{
  class EngineToRobot;
  namespace OTA {
    struct Ack;
  }
}
  

class AsyncLoaderData
{
public:
  
  AsyncLoaderData() : _isComplete(false) { }
  
  void Init(const std::string& filename)
  {
    _loadedBytes.clear();
    _filename = filename;
    _isComplete = false;
  }

  const std::vector<uint8_t>& GetBytes() const { return _loadedBytes; }
        std::vector<uint8_t>& GetBytes()       { return _loadedBytes; }
  
  const std::string& GetFilename() const { return _filename; }
  
  void SetComplete(bool newVal = true) { _isComplete = newVal; }
  const bool IsComplete() const { return _isComplete; }
  
private:
  
  std::vector<uint8_t> _loadedBytes;
  std::string          _filename;
  bool                 _isComplete;
};
  

class FirmwareUpdater
{
public:
  static const char* const kFirmwareVersionKey; // = "version";
  static const char* const kFirmwareTimeKey; // = "time";
  
  explicit FirmwareUpdater(const CozmoContext* context);
  ~FirmwareUpdater();
  
  using RobotMap = RobotManager::RobotMap;
  
  bool InitUpdate(const RobotMap& robots, int version);
  bool Update(const RobotMap& robots);

  using JsonCallback = std::function<void(const Json::Value&)>;
  void LoadHeader(const JsonCallback& callback);
  
  void HandleFlashWriteAck(RobotID_t robotId, const RobotInterface::OTA::Ack& flashWriteAck);
  
private:
  
  class RobotUpgradeInfo
  {
  public:
    
    explicit RobotUpgradeInfo(RobotID_t id) : _id(id), _hasAckPending(false), _hasDisconnected(false), _isConnected(true) {}
    
    RobotID_t GetId() const { return _id; }
    bool      HasAckPending() const { return _hasAckPending; }
    bool      IsConnected() const { return _isConnected; }
    bool      HasRebooted() const { return _hasDisconnected && _isConnected; }
    
    void RequestAck() { _hasAckPending = true; }
    void ConfirmAck() { _hasAckPending = false; }
    void SetIsConnected(bool newVal) { _isConnected = newVal; _hasDisconnected = (_hasDisconnected || !_isConnected); }
    
  private:
    
    RobotID_t _id;
    bool      _hasAckPending;
    bool      _hasDisconnected;
    bool      _isConnected;
  };
  
  bool IsComplete() const;
  
  bool SendToAllRobots(const RobotMap& robots, const RobotInterface::EngineToRobot& msg);
  bool HaveAllRobotsAcked() const;
  bool AreAllRobotsDisconnected() const;
  
  bool ShouldRobotsBeRebooting() const;
  bool HaveAllRobotsRebooted() const;

  void WaitForLoadingThreadToExit();
  
  void CleanupOnExit();
  
  void VerifyActiveRobots(const RobotMap& robots);

  void SetSubState(const RobotMap& robots, FirmwareUpdateSubStage newState);
  void GotoFailedState(const RobotMap& robots, FirmwareUpdateResult updateResult);
  void AdvanceState(const RobotMap& robots);
  
  void UpdateSubState(const RobotMap& robots);
  
  void SendProgressToGame(const RobotMap& robots, float ratioComplete = 0.0f);
  void SendCompleteResultToGame(const RobotMap& robots, FirmwareUpdateResult updateResult);
  
  std::string& GetFwSignature(FirmwareUpdateStage stage)
  {
    const size_t idx = (size_t)stage;
    assert(idx < kFwSignatureCount);
    return _fwSignatures[idx];
  }
  
  static void LoadHeaderData(AsyncLoaderData& fileLoaderData, const JsonCallback& callback);
  bool SendWriteMessages(const RobotMap& robots);
  
  // ==================== Member Data ====================
  
  AsyncLoaderData _fileLoaderData;
  std::thread     _loadingThread;
  
  const CozmoContext* _context;
  
  std::vector<RobotUpgradeInfo> _robotsToUpgrade;
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  static constexpr size_t kFwSignatureCount = (size_t)FirmwareUpdateStage::Done;
  std::string     _fwSignatures[kFwSignatureCount];
  
  uint32_t        _numFramesInSubState;
  uint32_t        _numBytesWritten;
  uint32_t        _numBytesProcessed = 0;
  
  int32_t         _version;
  
  FirmwareUpdateStage     _state;
  FirmwareUpdateSubStage  _subState;
  
  int16_t _currentPacketNumber = 0;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_FirmwareUpdater_FirmwareUpdater_H__

