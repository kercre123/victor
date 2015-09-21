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
#include "clad/types/imageTypes.h"

namespace Anki {
namespace Cozmo {

template <typename Type>
class AnkiEvent;

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
  void SetRobotImageSendMode(RobotID_t robotID, ImageSendMode newMode, ImageResolution resolution);

  void StartAnimationTool() override { _animationReloadActive = true; };
protected:

  virtual Result InitInternal() override;
  virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void InitPlaybackAndRecording();

  void ReloadAnimations(const BaseStationTime_t currTime_ns);

  Result AddRobot(RobotID_t robotID);

  bool                         _isListeningForRobots;
  Comms::AdvertisementService  _robotAdvertisementService;
  RobotManager                 _robotMgr;
  RobotInterface::MessageHandler& _robotMsgHandler;

  std::map<AdvertisingRobot, bool> _forceAddedRobots;
  BaseStationTime_t _lastAnimationFolderScan;
  bool _animationReloadActive;
  
#ifdef COZMO_RECORDING_PLAYBACK
  // TODO: Make use of these for playback/recording
  IRecordingPlaybackModule *recordingPlaybackModule_;
  IRecordingPlaybackModule *uiRecordingPlaybackModule_;
#endif

}; // class CozmoEngineHostImpl



} // namespace Cozmo
} // namespace Anki


#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineHostImpl_H__
