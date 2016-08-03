/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/rotation.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
//#include "util/math/numericCast.h"
//#include "util/random/randomGenerator.h"

#define DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS 0

namespace Anki {
namespace Cozmo {

// all coordinates have to be this close from their counterpart to be considered the same observation (and thus override it)
CONSOLE_VAR(float, kBW_PossibleObjectClose_mm, "AIWhiteboard", 50.0f);
CONSOLE_VAR(float, kBW_PossibleObjectClose_rad, "AIWhiteboard", PI_F); // current objects flip due to distance, consider 360 since we don't care
// limit to how many pending possible objects we have stored
CONSOLE_VAR(uint32_t, kBW_MaxPossibleObjects, "AIWhiteboard", 10);
CONSOLE_VAR(float, kFlatPosisbleObjectTol_deg, "AIWhiteboard", 10.0f);
CONSOLE_VAR(float, kBW_MaxHeightForPossibleObject_mm, "AIWhiteboard", 30.0f);
// debug render
CONSOLE_VAR(bool, kBW_DebugRenderPossibleObjects, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderPossibleObjectsZ, "AIWhiteboard", 35.0f);
CONSOLE_VAR(bool, kBW_DebugRenderBeacons, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderBeaconZ, "AIWhiteboard", 35.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::AIWhiteboard(Robot& robot)
: _robot(robot)
, _gotOffChargerAtTime_sec(-1.0f)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::Init()
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotMarkedObjectPoseUnknown>();
  }
  else {
    PRINT_NAMED_WARNING("AIWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotDelocalized()
{
  RemovePossibleObjectsFromZombieMaps();

  UpdatePossibleObjectRender();
  
  // TODO rsam we probably want to rematch beacons when robot relocalizes.
  _beacons.clear();
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ProcessClearQuad(const Quad2f& quad)
{
  const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
  // remove all objects inside clear quads
  auto isObjectInsideQuad = [&quad, worldOriginPtr](const PossibleObject& possibleObj)
  {
    Pose3d relPose;
    if ( possibleObj.pose.GetWithRespectTo(*worldOriginPtr, relPose) ) {
      return quad.Contains( relPose.GetTranslation() );
    } else {
      return false;
    }
  };
  const size_t countBefore = _possibleObjects.size();
  _possibleObjects.remove_if( isObjectInsideQuad );
  const size_t countAfter = _possibleObjects.size();
  
  // update render if we removed something
  if ( countBefore != countAfter ) {
    UpdatePossibleObjectRender();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::FinishedSearchForPossibleCubeAtPose(ObjectType objectType, const Pose3d& pose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.FinishedSearch",
                      "finished search, so removing possible object");
  }
  RemovePossibleObjectsMatching(objectType, pose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetHasStackToAdmire(ObjectID topBlockID, ObjectID bottomBlockID)
{
  _topOfStackToAdmire = topBlockID;
  _bottomOfStackToAdmire = bottomBlockID;
  PRINT_CH_INFO("AIWhiteboard", "StackToAdmire", "admiring stack [%d, %d] (bottom, top)",
                    _bottomOfStackToAdmire.GetValue(),
                    _topOfStackToAdmire.GetValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const
{
  possibleObjects.clear();
  
  // iterate all possible objects
  const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
  for( const auto& possibleObject : _possibleObjects )
  {
    // if we can obtain a pose with respect to the current origin, store that output (relative pose and type)
    Pose3d poseWRTOrigin;
    if ( possibleObject.pose.GetWithRespectTo(*worldOriginPtr, poseWRTOrigin) )
    {
      possibleObjects.emplace_back( poseWRTOrigin, possibleObject.type );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::AddBeacon( const Pose3d& beaconPos )
{
  _beacons.emplace_back( beaconPos );

  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AIBeacon* AIWhiteboard::GetActiveBeacon() const
{
  if ( _beacons.empty() ) {
    return nullptr;
  }
  
  return &_beacons[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsMatching(ObjectType objectType, const Pose3d& pose)
{
  // iterate all current possible objects
  auto possibleObjectIt = _possibleObjects.begin();
  while ( possibleObjectIt != _possibleObjects.end() )
  {
    // compare type (there shouldn't be many of the same type, but can happen if we see more than one possible object
    // for the same cube)
    if ( possibleObjectIt->type == objectType )
    {
      Pose3d relPose;
      if ( possibleObjectIt->pose.GetWithRespectTo(pose, relPose) )
      {
        // compare locations
        const float distSQ = kBW_PossibleObjectClose_mm*kBW_PossibleObjectClose_mm;
        const bool isClosePossibleObject = FLT_LE(relPose.GetTranslation().LengthSq(), distSQ );
        if ( isClosePossibleObject )
        {
          if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
            PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Remove",
                              "removing possible object that was at (%f, %f, %f), matching object type %d",
                              relPose.GetTranslation().x(),
                              relPose.GetTranslation().y(),
                              relPose.GetTranslation().z(),
                              objectType);
          }

          // they are close, remove old entry
          possibleObjectIt = _possibleObjects.erase( possibleObjectIt );
          // jump up to not increment iterator after erase
          continue;
        }
      }
    }

    // not erased, increment iter
    ++possibleObjectIt;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsFromZombieMaps()
{
  PossibleObjectList::iterator iter = _possibleObjects.begin();
  while( iter != _possibleObjects.end() )
  {
    const Pose3d* objOrigin = &(iter->pose.FindOrigin());
    const bool isZombie = _robot.GetBlockWorld().IsZombiePoseOrigin(objOrigin);
    if ( isZombie ) {
      if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
        PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "Deleted possible object because it was zombie");
      }
     iter = _possibleObjects.erase(iter);
    } else {
      ++iter;
    }
  }
  if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "%zu possible objects not zombie", _possibleObjects.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg;
  
  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  ASSERT_NAMED( _robot.GetID() == possibleObject.robotID, "AIWhiteboard.HandleMessage.RobotObservedObject.UnexpectedRobotID");
  
  Pose3d obsPose( msg.pose, _robot.GetPoseOriginList() );
  
  // iterate objects we previously had and remove them if we think they belong to this object
  RemovePossibleObjectsMatching(possibleObject.objectType, obsPose);

  // update render
  UpdatePossibleObjectRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg.possibleObject;

  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  ASSERT_NAMED( _robot.GetID() == possibleObject.robotID, "AIWhiteboard.HandleMessage.RobotObservedPossibleObject.UnexpectedRobotID");
  
  Pose3d obsPose( msg.possibleObject.pose, _robot.GetPoseOriginList() );
  
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "ObservedPossible", "robot observed a possible object");
  }

  ConsiderNewPossibleObject(possibleObject.objectType, obsPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotMarkedObjectPoseUnknown& msg)
{
  const ObservableObject* obj = _robot.GetBlockWorld().GetObjectByID(msg.objectID);
  if( nullptr == obj ) {
    PRINT_NAMED_WARNING("AIWhiteboard.HandleMessage.RobotMarkedObjectPoseUnknown.NoBlock",
                        "couldnt get object with id %d",
                        msg.objectID);
    return;
  }

  PRINT_CH_INFO("AIWhiteboard", "MarkedUnknown",
                    "marked %d unknown, adding to possible objects",
                    msg.objectID);

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PoseMarkedUnknown", "considering old pose from object %d as possible object",
                      msg.objectID);
  }
  
  // its position has become Unknown. We probably were at the location where we expected this object to be,
  // and it's no there. So we don't want to keep its markers anymore
  RemovePossibleObjectsMatching(obj->GetType(), obj->GetPose());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ConsiderNewPossibleObject(ObjectType objectType, const Pose3d& obsPose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Consider",
                      "considering pose (%f, %f, %f)",
                      obsPose.GetTranslation().x(),
                      obsPose.GetTranslation().y(),
                      obsPose.GetTranslation().z());
  }
  
  // only add relatively flat objects
  const RotationMatrix3d Rmat = obsPose.GetRotationMatrix();
  const bool isFlat = (Rmat.GetAngularDeviationFromParentAxis<'Z'>() < DEG_TO_RAD(kFlatPosisbleObjectTol_deg));

  if( ! isFlat ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NotFlat",
                        "pose isn't flat, so not adding to list (angle %fdeg)",
                        Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees());
    }

    return;
  }

  Pose3d relPose;
  if( ! obsPose.GetWithRespectTo(_robot.GetPose(), relPose) ) {
    PRINT_NAMED_WARNING("AIWhiteboard.PossibleObject.NoRelPose",
                        "Couldnt get pose WRT to robot");
    return;
  }

  // can't really use size since this is just a pose
  // TODO:(bn) get the size based on obectType so this can be relative to the bottom of the object?
  if( relPose.GetTranslation().z() > kBW_MaxHeightForPossibleObject_mm ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.TooHigh",
                        "pose is too high, not considering. relativeZ = %f",
                        relPose.GetTranslation().z());
    }

    return;
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.RemovingOldObjects",
                      "removing any old possible objects that are nearby");
  }

  // remove any objects that are similar to this
  RemovePossibleObjectsMatching(objectType, obsPose);
  
  // check with the world if this a possible marker for an object we have already recognized, because in that case we
  // are not interested in storing it again.
  
  Vec3f maxLocDist(kBW_PossibleObjectClose_mm);
  Radians maxLocAngle(kBW_PossibleObjectClose_rad);
  ObservableObject* prevObservedObject =
    _robot.GetBlockWorld().FindClosestMatchingObject( objectType, obsPose, maxLocDist, maxLocAngle);
  
  // found object nearby, no need
  if ( nullptr != prevObservedObject )
  {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NearbyObject",
                        "Already had a real(known) object nearby, not adding a new one");
    }

    return;
  }

  // if we are at the limit of stored objects, remove the front one
  if ( _possibleObjects.size() >= kBW_MaxPossibleObjects )
  {
    // we reached the limit, remove oldest entry
    ASSERT_NAMED(!_possibleObjects.empty(), "AIWhiteboard.HandleMessage.ReachedLimitEmpty");
    _possibleObjects.pop_front();
    // PRINT_NAMED_INFO("AIWhiteboard.HandleMessage.BeyondObjectLimit", "Reached limit of pending objects, removing oldest one");
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Add",
                      "added possible object. rot=%fdeg, z=%f",
                      Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees(),
                      relPose.GetTranslation().z());
  }

  
  // always add new entry
  _possibleObjects.emplace_back( obsPose, objectType );
  
  // update render
  UpdatePossibleObjectRender();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdatePossibleObjectRender()
{
  if ( kBW_DebugRenderPossibleObjects )
  {
    const Pose3d* worldOriginPtr = _robot.GetWorldOrigin();
    _robot.GetContext()->GetVizManager()->EraseSegments("AIWhiteboard.PossibleObjects");
    for ( auto& possibleObjectIt : _possibleObjects )
    {
      // this offset should not be applied pose
      const Vec3f& zRenderOffset = (Z_AXIS_3D() * kBW_DebugRenderPossibleObjectsZ);
      
      Pose3d thisPose;
      if ( possibleObjectIt.pose.GetWithRespectTo(*worldOriginPtr, thisPose))
      {
        const float kLen = kBW_PossibleObjectClose_mm;
        Quad3f quad3({ kLen,  kLen, 0},
                     {-kLen,  kLen, 0},
                     { kLen, -kLen, 0},
                     {-kLen, -kLen, 0});
        thisPose.ApplyTo(quad3, quad3);
        quad3 += zRenderOffset;
        _robot.GetContext()->GetVizManager()->DrawQuadAsSegments("AIWhiteboard.PossibleObjects",
            quad3, NamedColors::ORANGE, false);

        Vec3f directionEndPoint = X_AXIS_3D() * kLen*0.5f;
        directionEndPoint = thisPose * directionEndPoint;
        _robot.GetContext()->GetVizManager()->DrawSegment("AIWhiteboard.PossibleObjects",
            thisPose.GetTranslation() + zRenderOffset, directionEndPoint + zRenderOffset, NamedColors::YELLOW, false);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdateBeaconRender()
{
  // re-draw all beacons since they all use the same id
  if ( kBW_DebugRenderBeacons )
  {
    const std::string renderId("AIWhiteboard.UpdateBeaconRender");
    _robot.GetContext()->GetVizManager()->EraseSegments(renderId);
  
    // iterate all beacons and render
    for( const auto& beacon : _beacons )
    {
      // currently we don't support beacons from older origins (rsam: I will soon)
      ASSERT_NAMED( (&beacon.GetPose().FindOrigin()) == _robot.GetWorldOrigin(),
      "AIWhiteboard.UpdateBeaconRender.BeaconFromOldOrigin");
      
      Vec3f center = beacon.GetPose().GetTranslation();
      center.z() += kBW_DebugRenderBeaconZ;
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius(), NamedColors::GREEN, false);
    }
  }
}

} // namespace Cozmo
} // namespace Anki
