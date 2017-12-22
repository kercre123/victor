/**
 * File: multiRobotComponent.cpp
 *
 * Author: Kevin Yoon
 * Created: 2017-12-15
 *
 * Description: Manages communications with other robots on the same network
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/multiRobotComponent.h"
#include "engine/components/interRobotComms.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/interRobotMessages.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"
#include "util/time/universalTime.h"

#include "json/json.h"

#include <sstream>

namespace Anki {
namespace Cozmo {


namespace {
  const static char* kLogChannel = "MultiRobotComponent";

  const static u32 kIRMVersion = 1;
  const static int kMaxMsgSize = sizeof(InterRobotMessage);

  const static int kRequestTimeout_ms = 5000;

  CONSOLE_VAR(u32, kPoseWrtLandmarkUpdatePeriod_ms, "MultiRobotComponent", 1000);
}

void PrintBuf(const char* name, const u8* data, int size)
{
  std::stringstream ss;
  for(int i=0; i < size; ++i)
  {
    ss << std::hex << (int)data[i] << " ";
  }
  PRINT_NAMED_WARNING("MRC.PrintBuf", "%s: %s", name, ss.str().c_str());
}
  
MultiRobotComponent::MultiRobotComponent(Robot& robot, const CozmoContext* context)
: _robot(robot)
, _context(context)
//, _comms(std::make_unique<InterRobotComms>())
, _comms(new InterRobotComms())
, _robotIDCounter(robot.GetID() + 1)
, _requestedSessionID(INVALID_SESSION_ID)
, _requestedRobotID(0)
, _requestTimeoutTime_ms(0)
, _currInteraction(RobotInteraction::Invalid)
, _currSessionID(INVALID_SESSION_ID)
, _isSessionMaster(false)
, _landmark(ObjectType::InvalidObject)
, _lastPoseWrtLandmarkBroadcast_ms(0)
{
  // Initialize sessionID counter with some random number so  that there's
  // unlikely to be collisions with sessionIDs from other robots
  //_sessionIDCounter = _context->GetRandom()->RandInt(std::numeric_limits<int>::max());
  _sessionIDCounter = static_cast<SessionID_t>(Util::Time::UniversalTime::GetCurrentTimeInNanoseconds());

  // Get unique mfgID
  RobotID_t robotID = robot.GetID();
  #ifdef SIMULATOR
  // In sim, the mfgID can just be the robotID
  _mfgID = robotID;
  #else
  // Do a system call to get unique ID to use as mfgID
  const char* cmd = "getprop ro.serialno";
  const int kBufSize = 32;
  std::array<char, kBufSize> buffer;
  std::string serialno;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    PRINT_NAMED_ERROR("MultiRobotComponent.Ctor.GetPropFailed", "");
    return;
  }
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), kBufSize, pipe.get()) != nullptr) {
      serialno += buffer.data();
    }
  }
  
  PRINT_NAMED_WARNING("MultiRobotComponent.Ctor.SerialNo", "%s", serialno.c_str());
  _mfgID = stoi(serialno, nullptr, 16);
  #endif

  _mfgToRobotIDMap[_mfgID]  = robotID;
  _robotToMfgIDMap[robotID] = _mfgID;

}

void MultiRobotComponent::OnRobotDelocalized()
{
  TerminateSession();
  _robotInfoByLandmarkMap.clear();
}
  
void MultiRobotComponent::Update()
{
  ProcessMessages();

  UpdatePoseWrtLandmark();

  // Check if pending requests should timeout
  if (_requestTimeoutTime_ms > 0 && _requestTimeoutTime_ms < BaseStationTimer::getInstance()->GetCurrentTimeStamp()) {
    PRINT_NAMED_WARNING("MultiRobtoComponent.Update.RequestTimedOut", "");
    ResolvePendingRequest(false);
  }
}

void MultiRobotComponent::TerminateSession()
{
  if (_currSessionID == INVALID_SESSION_ID) {
    // No session in progress
    return;
  }
  
  _robotInfoByLandmarkMap[_landmark].erase(_requestedRobotID);
  
  IRMPacketHeader header;
  if (!GetPacketHeader(_requestedRobotID, header)) {
    PRINT_NAMED_WARNING("MultiRobotComponent.TerminateSession.InvalidRobotID", "RobotID: %d", _requestedRobotID);
    return;
  }
  
  InterRobotMessage msg;
  msg.Set_endSession(EndSession(header, _currSessionID));
  SendMessage(msg);
  PRINT_NAMED_WARNING("MultiRobotComponent.TerminateSession", "sessionID: %x", _currSessionID) ;

  _currSessionID = INVALID_SESSION_ID;
}
  
void MultiRobotComponent::SetLandmark(ObjectType objectType)
{
  if (_landmark != objectType) {
     PRINT_CH_INFO(kLogChannel, 
                   "MultiRobotComponent.SetLandmark.NewLandmark", 
                   "%s (was %s)", 
                   EnumToString(objectType), EnumToString(_landmark));
    _landmark = objectType;
  }

}

MultiRobotComponent::MRC_RobotList MultiRobotComponent::GetRobotsLocatedToLandmark() const
{
  MRC_RobotList lst;
  const auto it = _robotInfoByLandmarkMap.find(_landmark);
  if (it != _robotInfoByLandmarkMap.end()) {
    for (auto robot_it = it->second.begin(); robot_it != it->second.end(); ++robot_it) {
      lst.push_back(robot_it->first);
    }
  }
  return lst;
}


void MultiRobotComponent::UpdatePoseWrtLandmark()
{
  
  // If landmark object has been observed, advertise the robot's location wrt it.
  // NB: Some major assumptions made here
  //     1) There is only one instance of the landmark cube in the world
  //     2) The cube never moves
  auto now_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if (_lastPoseWrtLandmarkBroadcast_ms + kPoseWrtLandmarkUpdatePeriod_ms < now_ms) {
    
    Pose3d poseWrtLandmark;
    bool   poseWrtLandmarkSet = false;
    
    // If landmark object was previously already set then just look for it again by ID
    if (_landmarkObjectID.IsSet()) {
      ObservableObject* landmarkObj = _robot.GetBlockWorld().GetLocatedObjectByID(_landmarkObjectID);
      if (landmarkObj) {
        if (!_robot.GetPose().GetWithRespectTo(landmarkObj->GetPose(), poseWrtLandmark)) {
          PRINT_NAMED_WARNING("MultiRobotComponent.UpdatePoseWrtLandmark.MismatchedOrigins-1", "ObjectID: %d", landmarkObj->GetID().GetValue());
        } else {
          poseWrtLandmarkSet = true;
        }
      } else {
        PRINT_NAMED_WARNING("MultiRobotComponent.UpdatePoseWrtLandmark.LandmarkObjectNotFound", "ObjectID: %d", _landmarkObjectID.GetValue());
        _landmarkObjectID.UnSet();
      }
    }
    
    BlockWorldFilter filter;
    filter.SetAllowedTypes({_landmark});
    filter.AddFilterFcn([this, &poseWrtLandmark, &poseWrtLandmarkSet](const ObservableObject* blockPtr) {
      if (poseWrtLandmarkSet) {
        return false;
      }
      
      if (!_robot.GetPose().GetWithRespectTo(blockPtr->GetPose(), poseWrtLandmark)) {
        PRINT_NAMED_WARNING("MultiRobotComponent.UpdatePoseWrtLandmark.MismatchedOrigins", "ObjectID: %d", blockPtr->GetID().GetValue());
        return false; // Continue on to next object
      }
      
      PRINT_NAMED_WARNING("MultiRobotComponent.UpdatePoseWrtLandmark.ObjectFound", "ObjectID: %d", blockPtr->GetID().GetValue());
      poseWrtLandmarkSet = true;
      _landmarkObjectID = blockPtr->GetID();
      return false;
    });
    
    _robot.GetBlockWorld().FilterLocatedObjects(filter);

    if (!poseWrtLandmarkSet) {
      // No landmark object found
      return;
    }
    
    // Set parent as origin so that it doesn't flatten to robot's pose wrt origin
    poseWrtLandmark.SetParent(_robot.GetWorldOrigin());
    
    // Broadcast message
    IRMPacketHeader header;
    GetBroadcastPacketHeader(header);
    
    InterRobotMessage msg;
    msg.Set_poseWrtLandmark(PoseWrtLandmark(header, _landmark, poseWrtLandmark.ToPoseStruct3d(_robot.GetPoseOriginList())));
    SendMessage(msg);
    
//    PRINT_NAMED_WARNING("MRC.UpdatePoseWrtLandmark.Pose", "%d: %f %f %f",
//                        _robot.GetID(),
//                        msg.Get_poseWrtLandmark().poseWrtLandmark.x,
//                        msg.Get_poseWrtLandmark().poseWrtLandmark.y,
//                        msg.Get_poseWrtLandmark().poseWrtLandmark.z);
    
    _lastPoseWrtLandmarkBroadcast_ms = now_ms;
  }  
  
}

void MultiRobotComponent::ProcessMessages() 
{
// Check for messages received
  u8 recvBuf[kMaxMsgSize];
  ssize_t recvSize;
  while( (recvSize = _comms->Recv((char*)recvBuf, kMaxMsgSize)) > 0 ) {
    InterRobotMessage msg;
    msg.Unpack(recvBuf, (size_t)recvSize);

    ProcessMessage(msg);
  }
}

  
void MultiRobotComponent::HandleMessage(const RobotID_t& senderID, const PoseWrtLandmark& msg)
{
  PRINT_NAMED_WARNING("MultiRobotComponent.HandlePoseWrtLandmark",
                      "From: %x, To: %x, Landmark: %s (RobotID: %u)",
                      msg.header.src, msg.header.dest, EnumToString(msg.landmark), _robot.GetID() );
 
  if (msg.landmark != _landmark) {
    return;
  }

  ObservableObject* landmarkObj = _robot.GetBlockWorld().GetLocatedObjectByID(_landmarkObjectID);
  if (landmarkObj == nullptr) {
    PRINT_NAMED_WARNING("MultiRobotComponent.HandlePoseWrtLandmark.LandmarkNotFoundYet", "");
    return;
  }
  
  // Get pose of other robot wrt landmark
  PoseStruct3d poseStruct = msg.poseWrtLandmark;
  poseStruct.originID = _robot.GetWorldOriginID();
  Pose3d poseWrtLandmark(poseStruct, _robot.GetPoseOriginList());
  
  // Set pose's parent to landmark object
  poseWrtLandmark.SetParent(landmarkObj->GetPose());
  
  
  RobotInfo& rInfo = _robotInfoByLandmarkMap[msg.landmark][senderID];
  rInfo.lastUpdateTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  rInfo.poseWrtLandmark = poseWrtLandmark;
  
  //  ========= For debug =========
  // Flatten pose
  Pose3d otherRobotPose = poseWrtLandmark.GetWithRespectToRoot();
  
//  PRINT_NAMED_WARNING("MRC.HandlePoseWrtLandmark.poseWrtLandmark", "%d: %f %f %f, %f %f %f, %f %f %f",
//                      _robot.GetID(),
//                      msg.poseWrtLandmark.x, msg.poseWrtLandmark.y, msg.poseWrtLandmark.z,
//                      poseWrtLandmark.GetTranslation().x(),
//                      poseWrtLandmark.GetTranslation().y(),
//                      poseWrtLandmark.GetTranslation().z(),
//                      otherRobotPose.GetTranslation().x(),
//                      otherRobotPose.GetTranslation().y(),
//                      otherRobotPose.GetTranslation().z());
  


  auto viz = _context->GetVizManager();
  Quad2f robotFootprint = _robot.GetBoundingQuadXY(otherRobotPose);
  viz->DrawPoseMarker(0, robotFootprint, ::Anki::NamedColors::DARKGRAY);
}

void MultiRobotComponent::HandleMessage(const RobotID_t& senderID, const InteractionRequest& msg)
{
  PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionRequest", 
                      "From: %x, To: %x, Interaction: %s, SessionID: %x (RobotID: %u)",
                      msg.header.src, msg.header.dest, EnumToString(msg.interaction), msg.sessionID, _robot.GetID() );

  // If already in a session then reject
  bool acceptSession = _currSessionID == INVALID_SESSION_ID;
  
  
  // Do we have a request pending?
  if (acceptSession && _requestedSessionID != INVALID_SESSION_ID) {
    // Is the requested session from the same robot?
    if (_requestedRobotID == senderID) {
      // If request came from same robot that we already sent a request to,
      // accept it only if the sessionID is greater than the requested sessionID.
      acceptSession = msg.sessionID > _requestedSessionID;
    } else {
      // Request came from a robot other than the one that we requested
      // so just reject it
      acceptSession = false;
    }
  }

  IRMPacketHeader header;
  GetPacketHeader(senderID, header);
  InterRobotMessage response;
  
  if (acceptSession) {
    _currInteraction = msg.interaction;
    _currSessionID = msg.sessionID;
    _requestedSessionID = INVALID_SESSION_ID;
    _requestTimeoutTime_ms = 0;
    _requestedRobotID = senderID;
    _isSessionMaster = false;
    
    response.Set_interactionResponse(InteractionResponse(header, msg.sessionID, true));
    
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::MultiRobotSessionStarted(_requestedRobotID, _currSessionID, _currInteraction, _isSessionMaster)));
    
    PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionRequest.SessionAccepted",
                        "SessionID: %x (Robots %d and %d)", msg.sessionID, _robot.GetID(), senderID );
  } else {
    response.Set_interactionResponse(InteractionResponse(header, msg.sessionID, false));
    
    PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionRequest.SessionRejected",
                        "SessionID: %x (Robots %d and %d)", msg.sessionID, _robot.GetID(), senderID );

  }
  SendMessage(response);
  
}

void MultiRobotComponent::HandleMessage(const RobotID_t& senderID, const InteractionResponse& msg)
{
  PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionResponse", 
                      "From: %x, To: %x, SessionID: %x (RobotID: %u)",
                      msg.header.src, msg.header.dest, msg.sessionID, _robot.GetID() );

  if (msg.sessionID != _requestedSessionID) {
    if (_currSessionID != INVALID_SESSION_ID) {
      if (msg.accepted) {
        PRINT_NAMED_ERROR("MultiRobotComponent.HandleInteractionResponse.RequestAcceptedButAlreadyInSession",
                          "Received sessionID: %x, current sessionID: %x", msg.sessionID, _currSessionID);
      } else {
        PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionResponse.AlreadyInSession",
                            "Probably accepted session requested from the same robot. Received sessionID: %x, current sessionID: %x", msg.sessionID, _currSessionID);
      }
    } else {
      PRINT_NAMED_ERROR("MultiRobotComponent.HandleInteractionResponse.SessionMismatch",
                        "Requested %x, received %x", _requestedSessionID, msg.sessionID);
    }
    return;
  }

  if (senderID != _requestedRobotID) {
    PRINT_NAMED_ERROR("MultiRobotComponent.HandleInteractionResponse.RobotMismatch",
                      "Requested %x, sender %x", _requestedRobotID, senderID);
    return;
  }
  
  ResolvePendingRequest(msg.accepted);
}

void MultiRobotComponent::ResolvePendingRequest(bool requestAccepted)
{ 
  if (requestAccepted) {
    
    _currSessionID = _requestedSessionID;
    _isSessionMaster = true;

    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::MultiRobotSessionStarted(_requestedRobotID, _currSessionID, _currInteraction, _isSessionMaster)));
    
    PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionResponse.SessionAccepted",
                        "SessionID: %x (Robots %d and %d)", _currSessionID, _robot.GetID(), _requestedRobotID);
  } else {
    PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionResponse.SessionRejected",
                        "SessionID: %x (Robots %d and %d)", _requestedSessionID, _robot.GetID(), _requestedRobotID);
  }

  _requestedSessionID = INVALID_SESSION_ID;
  _requestTimeoutTime_ms = 0;


  if (_requestCallback) {
    _requestCallback(requestAccepted);
  }
}


void MultiRobotComponent::HandleMessage(const RobotID_t& senderID, const InteractionStateTransition& msg)
{  
  PRINT_NAMED_WARNING("MultiRobotComponent.HandleInteractionStateTransition.NewState",
                      "SessionID: %x, newState: %d", _currSessionID, msg.newState );

  _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::MultiRobotInteractionStateTransition(_currSessionID, _currInteraction, msg.newState)));
}
  
void MultiRobotComponent::HandleMessage(const RobotID_t& senderID, const EndSession& msg)
{  
  PRINT_NAMED_WARNING("MultiRobotComponent.HandleEndSession",
                      "SessionID: %x (%x)", msg.sessionID, _currSessionID );

  _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::MultiRobotSessionEnded(senderID, _currSessionID)));
}
  
void MultiRobotComponent::ProcessMessage(const InterRobotMessage& msg)
{
  #define HANDLE_MSG(msg) {\
    if (msg.header.dest != -1 && msg.header.dest != _mfgID) {\
     break;\
    }\
    RobotID_t senderID;\
    GetRobotIDFromMfgID(msg.header.src, senderID, true);\
    HandleMessage(senderID, msg);\
  }\
  
  const auto& tag = msg.GetTag();
  switch(tag) {
    case InterRobotMessageTag::poseWrtLandmark:
    {
      HANDLE_MSG(msg.Get_poseWrtLandmark());
      break;
    }
    case InterRobotMessageTag::interactionRequest:
    {
      HANDLE_MSG(msg.Get_interactionRequest());
      break;
    }
    case InterRobotMessageTag::interactionResponse:
    {
      HANDLE_MSG(msg.Get_interactionResponse());
      break;
    }
    case InterRobotMessageTag::interactionStateTransition:
    {
      HANDLE_MSG(msg.Get_interactionStateTransition());
      break;
    }
    case InterRobotMessageTag::endSession:
    {
      HANDLE_MSG(msg.Get_endSession());
      break;
    }
    default:
    {
      PRINT_NAMED_WARNING("MultiRobotComponent.ProcessMessage.InvalidTag", "%hhu", tag);
      break;
    }
  }
}


bool MultiRobotComponent::GetRobotIDFromMfgID(const MfgID_t mfgID, RobotID_t& robotID, bool createIDIfNotFound)
{
  auto it = _mfgToRobotIDMap.find(mfgID);
  if (it != _mfgToRobotIDMap.end()) {
    robotID = it->second;
    return true;
  } else if(createIDIfNotFound) {
    robotID = _robotIDCounter++;
    _mfgToRobotIDMap[mfgID] = robotID;
    _robotToMfgIDMap[robotID] = mfgID;
    return true;
  }

  return false;
}

bool MultiRobotComponent::GetMfgIDFromRobotID(const RobotID_t& robotID, MfgID_t& mfgID) const
{
  auto it = _robotToMfgIDMap.find(robotID);
  if (it != _robotToMfgIDMap.end()) {
    mfgID = it->second;
    return true;
  } 

  return false;
}


bool MultiRobotComponent::GetPacketHeader(const RobotID_t robotID, IRMPacketHeader& header) const
{
  header.version = kIRMVersion;
  header.src = _mfgID;

  if (!GetMfgIDFromRobotID(robotID, header.dest)) {
    return false;
  }

  return true;
}  

bool MultiRobotComponent::GetBroadcastPacketHeader(IRMPacketHeader& header) const
{
  header.version = kIRMVersion;
  header.src = _mfgID;
  header.dest = -1;
  return true;
}

  

Result MultiRobotComponent::RequestInteraction(RobotID_t robotID, RobotInteraction interaction, RequestInteractionCallback cb)
{
  if (_requestedSessionID != INVALID_SESSION_ID) {
    PRINT_NAMED_WARNING("MultiRobotComponent.RequestInteraction.RequestPending", "Ignoring this request since pending");
    return RESULT_FAIL;
  }
  
  _requestedSessionID = _sessionIDCounter++;
  _requestedRobotID   = robotID;
  _currInteraction    = interaction;
  _requestCallback    = cb;

  IRMPacketHeader header;
  if (!GetPacketHeader(robotID, header)) {
    PRINT_NAMED_WARNING("MultiRobotComponent.RequestInteraction.InvalidRobotID", "RobotID: %d", robotID);
    return RESULT_FAIL;
  }

  InterRobotMessage msg;
  msg.Set_interactionRequest(InteractionRequest(header, interaction, _requestedSessionID));
  SendMessage(msg);
  PRINT_NAMED_WARNING("MRC.Request", "Interaction: %s, sessionID: %x", EnumToString(interaction), _requestedSessionID) ;
  

  _requestTimeoutTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() + kRequestTimeout_ms;

  return RESULT_OK;
}


Result MultiRobotComponent::SendInteractionStateTransition(int state) const
{
  if (_currSessionID == INVALID_SESSION_ID) {
    PRINT_NAMED_WARNING("MRC.SendInteractionStateTransition.NotInSession", "");
    return RESULT_FAIL;
  }

  IRMPacketHeader header;
  GetPacketHeader(_requestedRobotID, header);

  InterRobotMessage msg;
  msg.Set_interactionStateTransition(InteractionStateTransition(header, _currSessionID, state));
  SendMessage(msg);
  //PRINT_NAMED_WARNING("MRC.SendInteractionStateTransition.SentState", "%d: State: %d", _robot.GetID(), state);
  return RESULT_OK;
}

Result MultiRobotComponent::GetSessionPartnerPose(Pose3d& p) const
{
  if (_currSessionID == INVALID_SESSION_ID) {
    PRINT_NAMED_WARNING("MultiRobotComponent.GetSessionPartnerPose.NotInSession","");
    return RESULT_FAIL;
  }

  auto landmark_it = _robotInfoByLandmarkMap.find(_landmark);
  if (landmark_it != _robotInfoByLandmarkMap.end()) {
    auto robot_it = landmark_it->second.find(_requestedRobotID);
    if (robot_it != landmark_it->second.end()) {
      p = robot_it->second.poseWrtLandmark.GetWithRespectToRoot();   // Since we set it's parent when this pose was received, this is the actual pose of the partner

      return RESULT_OK;
    }
  }

  return RESULT_FAIL;
}

void MultiRobotComponent::SendMessage(const InterRobotMessage& msg) const
{
  u8 data[kMaxMsgSize];
  msg.Pack(data, msg.Size());
  ssize_t sentBytes = _comms->Send((char*)data, (int)msg.Size());
  if (sentBytes < msg.Size()) {
    PRINT_NAMED_WARNING("MultiRobotComponent.SendMessage.SendFailed", "Sent %zd of %zu bytes", sentBytes, msg.Size());
  }
}

} // namespace Cozmo
} // namespace Anki
