/**
 * File: multiRobotComponent.h
 *
 * Author: Kevin Yoon
 * Created: 2017-12-15
 *
 * Description: Manages communications with other robots on the same network
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_MultiRobotComponent_H__
#define __Cozmo_Basestation_MultiRobotComponent_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/types/objectTypes.h"
#include "clad/types/interRobotMessages.h"

#include "util/helpers/noncopyable.h"

#include <unordered_map>


namespace Anki {
namespace Cozmo {



// forward declarations
class CozmoContext;
class Robot;
class InterRobotComms;

// - IsRobotLocatedToCommonLandmark()
// - SetCommonLandmark(ObjectType)
// - GetLandmark()
// - RequestInteraction(InteractionType, timestamp) // timestamp in crase request cross in 

// - HandleRequestInteraction()


class MultiRobotComponent
{
public:

  using SessionID_t = s32;
  static const int INVALID_SESSION_ID = -1;
  
  MultiRobotComponent(Robot& robot, const CozmoContext* context);
  ~MultiRobotComponent() = default;

  void OnRobotDelocalized();
  
  void Update();

  void SetLandmark(ObjectType objectType);
  
  bool IsInSession() const { return _currSessionID != INVALID_SESSION_ID; }
  
  void TerminateSession();
  
  using MRC_RobotList = std::vector<RobotID_t>;
  MRC_RobotList GetRobotsLocatedToLandmark() const;
  
  // Should result in MultiRobotSession
  // TODO: Should do timeout or something in case request is rejected
  using RequestInteractionCallback = std::function<void(bool accepted)>;
  Result RequestInteraction(RobotID_t robotID, RobotInteraction interaction, RequestInteractionCallback cb = {});


  // === These methods only work if you're in a sesion ===

  // For state machine syncronization of the interaction behvaior
  // across both robots
  Result SendInteractionStateTransition(int state) const;

  // Returns RESULT_OK if the pose of the partner robot is known
  Result GetSessionPartnerPose(Pose3d& p) const;


  
private:
  using MfgID_t = s32;

  bool GetRobotIDFromMfgID(const MfgID_t mfgID, RobotID_t& robotID, bool createIDIfNotFound = false);
  bool GetMfgIDFromRobotID(const RobotID_t& robotID, MfgID_t& mfgID) const;
  bool GetPacketHeader(RobotID_t robotID, IRMPacketHeader& header) const;
  bool GetBroadcastPacketHeader(IRMPacketHeader& header) const;
  
  void UpdatePoseWrtLandmark();
  
  void ResolvePendingRequest(bool requestAccepted);

  void SendMessage(const InterRobotMessage& msg) const;
  
  // Message handlers
  void HandleMessage(const RobotID_t& senderID, const PoseWrtLandmark& msg);
  void HandleMessage(const RobotID_t& senderID, const InteractionRequest& msg);
  void HandleMessage(const RobotID_t& senderID, const InteractionResponse& msg);
  void HandleMessage(const RobotID_t& senderID, const InteractionStateTransition& msg);
  void HandleMessage(const RobotID_t& senderID, const EndSession& msg);

  void ProcessMessages();
  void ProcessMessage(const InterRobotMessage& msg);

  Robot&           _robot;
  const CozmoContext* _context;
  InterRobotComms* _comms;
  
  
  MfgID_t          _mfgID;

  RobotID_t        _robotIDCounter;     // Counter for deciding which RobotID_t a given MfgID_t should map to
  
  // Session vars
  SessionID_t      _sessionIDCounter;   // Counter for deciding which ID to request next
  SessionID_t      _requestedSessionID; // Session ID that is currently requested and pending
  RobotID_t        _requestedRobotID;
  RequestInteractionCallback _requestCallback;
  TimeStamp_t      _requestTimeoutTime_ms;
  RobotInteraction _currInteraction;
  SessionID_t      _currSessionID;      // Session ID for current confirmed session
  bool             _isSessionMaster;    // true if this robot requested the session and it was accepted
  
  ObjectType       _landmark;
  ObjectID         _landmarkObjectID;

  // Last time this robot's pose wrt the landmark was broadcast
  TimeStamp_t      _lastPoseWrtLandmarkBroadcast_ms;


  typedef struct {    
    TimeStamp_t lastUpdateTime_ms;
    Pose3d poseWrtLandmark;
  } RobotInfo;
  
  using RobotInfoMap_t = std::unordered_map<RobotID_t, RobotInfo>;
  using RobotInfoByLandmarkMap_t = std::unordered_map<ObjectType, RobotInfoMap_t>;
  RobotInfoByLandmarkMap_t _robotInfoByLandmarkMap;
  
  // TODO: get rid of these usings
  using MfgToRobotIDMap_t = std::unordered_map<MfgID_t, RobotID_t>;
  MfgToRobotIDMap_t _mfgToRobotIDMap;
  
  using RobotToMfgIDMap_t = std::unordered_map<RobotID_t, MfgID_t>;
  RobotToMfgIDMap_t _robotToMfgIDMap;
  
};


} // namespace Cozmo
} // namespace Anki

#endif //__Cozmo_Basestation_MultiRobotComponent_H__
