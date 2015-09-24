/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#ifndef __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHostImpl_H__
#define __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHostImpl_H__

#include "anki/cozmo/basestation/engineImpl/cozmoEngineImpl.h"
#include "clad/types/imageSendMode.h"
#include "clad/types/cameraSettings.h"

namespace Anki {
namespace Cozmo {

template <typename Type>
class AnkiEvent;

namespace SpeechRecognition {
class KeyWordRecognizer;
}
namespace ExternalInterface {
class MessageGameToEngine;
}

class CozmoEngineHostImpl : public CozmoEngineImpl
{
public:
  CozmoEngineHostImpl(IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform);
  ~CozmoEngineHostImpl();
  Result StartBasestation();

  void ForceAddRobot(AdvertisingRobot robotID,
                     const char*      robotIP,
                     bool             robotIsSimulated);

  void ListenForRobotConnections(bool listen);

  Robot* GetFirstRobot();
  int    GetNumRobots() const;
  Robot* GetRobotByID(const RobotID_t robotID); // returns nullptr for invalid ID
  std::vector<RobotID_t> const& GetRobotIDList() const;

  // TODO: Remove once we don't have to specially handle forced adds
  virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;

  // TODO: Remove these in favor of it being handled via messages instead of direct API polling
  bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);

  void SetImageSendMode(RobotID_t robotID, ImageSendMode newMode);
  void SetRobotImageSendMode(RobotID_t robotID, ImageSendMode newMode, CameraResolutionClad resolution);

  void ReadAnimationsFromDisk() override {
    Robot* robot = _robotMgr.GetFirstRobot();
    if (robot != nullptr) {
      PRINT_NAMED_INFO("CozmoEngineHostImpl.ReloadAnimations", "ReadAnimationDir");
      robot->ReadAnimationDir();
    }
  };
protected:

  virtual Result InitInternal() override;
  virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void InitPlaybackAndRecording();

  Result AddRobot(RobotID_t robotID);

  bool                         _isListeningForRobots;
  Comms::AdvertisementService  _robotAdvertisementService;
  RobotManager                 _robotMgr;
  RobotMessageHandler          _robotMsgHandler;
  SpeechRecognition::KeyWordRecognizer* _keywordRecognizer;

  std::map<AdvertisingRobot, bool> _forceAddedRobots;
  BaseStationTime_t _lastAnimationFolderScan;
  
#ifdef COZMO_RECORDING_PLAYBACK
  // TODO: Make use of these for playback/recording
  IRecordingPlaybackModule *recordingPlaybackModule_;
  IRecordingPlaybackModule *uiRecordingPlaybackModule_;
#endif

}; // class CozmoEngineHostImpl



} // namespace Cozmo
} // namespace Anki


#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHostImpl_H__
