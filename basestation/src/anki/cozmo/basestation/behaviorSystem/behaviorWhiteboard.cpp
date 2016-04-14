/**
 * File: behaviorWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorWhiteboard.h"

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

namespace Anki {
namespace Cozmo {

// all coordinates have to be this close from their counterpart to be considered the same observation (and thus override it)
CONSOLE_VAR(float, kBW_PossibleMarkerClose_mm, "BehaviorWhiteboard", 50.0f);
CONSOLE_VAR(float, kBW_PossibleMarkerClose_rad, "BehaviorWhiteboard", PI_F); // current markers flip due to distance, consider 360 since we don't care
// limit to how many pending possible markers we have stored
CONSOLE_VAR(uint32_t, kBW_MaxPossibleMarkers, "BehaviorWhiteboard", 10);
// debug render
CONSOLE_VAR(bool, kBW_DebugRenderPossibleMarkers, "BehaviorWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderPossibleMarkersZ, "BehaviorWhiteboard", 35.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWhiteboard::BehaviorWhiteboard(Robot& robot)
: _robot(robot)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::Init()
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
  }
  else {
    PRINT_NAMED_WARNING("BehaviorWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::OnRobotDelocalized()
{
  // at the moment the whiteboard won't try to update origins, so just flush all poses
  _possibleMarkers.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::DisableCliffReaction(void* id)
{
  _disableCliffIds.insert(id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::RequestEnableCliffReaction(void* id)
{
  size_t numErased = _disableCliffIds.erase(id);

  if( numErased == 0 ){
    PRINT_NAMED_WARNING("BehaviorWhiteboard.RequestEnableCliffReaction.InvalidId",
                        "tried to request enabling cliff reaction with id %p, but no id found. %zu in set",
                        id,
                        _disableCliffIds.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorWhiteboard::IsCliffReactionEnabled() const
{
  return _disableCliffIds.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::RemovePossibleMarkersMatching(ObjectType objectType, const Pose3d& pose)
{
  // iterate all current possible markers
  auto possibleMarkerIt = _possibleMarkers.begin();
  while ( possibleMarkerIt != _possibleMarkers.end() )
  {
    // compare type
    if ( possibleMarkerIt->type == objectType )
    {
      // compare locations
      const Vec3f& newLocation = pose.GetTranslation();
      const Vec3f& entryLocation = possibleMarkerIt->pose.GetTranslation();
      const bool closePossibleMarker = IsNearlyEqual( newLocation, entryLocation, kBW_PossibleMarkerClose_mm );
      if ( closePossibleMarker )
      {
        // they are close, remove old entry
        possibleMarkerIt = _possibleMarkers.erase( possibleMarkerIt );
        // jump up to not increment iterator after erase
        continue;
      }
    }

    ++possibleMarkerIt;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BehaviorWhiteboard::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg;
  
  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  ASSERT_NAMED( _robot.GetID() == possibleObject.robotID, "BehaviorWhiteboard.HandleMessage.RobotObservedObject.UnexpectedRobotID");
  
  // calculate pose for object
  const UnitQuaternion<float> obsQuat(possibleObject.quaternion_w, possibleObject.quaternion_x, possibleObject.quaternion_y, possibleObject.quaternion_z);
  const Rotation3d obsRot(obsQuat);
  const Pose3d obsPose( obsRot, {possibleObject.world_x, possibleObject.world_y, possibleObject.world_z}, _robot.GetWorldOrigin());
  
  // iterate markers we previously had and remove them if we think they belong to this object
  RemovePossibleMarkersMatching(possibleObject.objectType, obsPose);

  // update render
  UpdatePossibleMarkerRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BehaviorWhiteboard::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg.possibleObject;

  // this is for the future. In the future, should a white board of one robot get messages from another robot? Should
  // it just ignore them? This assert will fire when this whiteboard receives a message from another robot. Make a
  // decision then
  ASSERT_NAMED( _robot.GetID() == possibleObject.robotID, "BehaviorWhiteboard.HandleMessage.RobotObservedPossibleObject.UnexpectedRobotID");
  
  // calculate pose for new object
  const UnitQuaternion<float> obsQuat(possibleObject.quaternion_w, possibleObject.quaternion_x, possibleObject.quaternion_y, possibleObject.quaternion_z);
  const Rotation3d obsRot(obsQuat);
  const Pose3d obsPose( obsRot, {possibleObject.world_x, possibleObject.world_y, possibleObject.world_z}, _robot.GetWorldOrigin());

  // remove any markers that are similar to this
  RemovePossibleMarkersMatching(possibleObject.objectType, obsPose);
  
  // check with the world if this a marker for an object we have already recognized, because in that case we
  // are not interested in storing it again
  Vec3f maxLocDist(kBW_PossibleMarkerClose_mm);
  Radians maxLocAngle(kBW_PossibleMarkerClose_rad);
  ObservableObject* prevObservedObject =
    _robot.GetBlockWorld().FindClosestMatchingObject( possibleObject.objectType, obsPose, maxLocDist, maxLocAngle);
  
  // found object nearby, no need
  if ( nullptr != prevObservedObject && prevObservedObject->IsExistenceConfirmed() )
  {
    return;
  }

  // if we are at the limit of stored markers, remove the front one
  if ( _possibleMarkers.size() >= kBW_MaxPossibleMarkers )
  {
    // we reached the limit, remove oldest entry
    ASSERT_NAMED(!_possibleMarkers.empty(), "BehaviorWhiteboard.HandleMessage.ReachedLimitEmpty");
    _possibleMarkers.pop_front();
    // PRINT_NAMED_INFO("BehaviorWhiteboard.HandleMessage.BeyondMarkerLimit", "Reached limit of pending markers, removing oldest one");
  }
  
  // always add new entry
  _possibleMarkers.emplace_back( obsPose, possibleObject.objectType );
  
  // update render
  UpdatePossibleMarkerRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::UpdatePossibleMarkerRender()
{
  if ( kBW_DebugRenderPossibleMarkers )
  {
    _robot.GetContext()->GetVizManager()->EraseSegments("BehaviorWhiteboard.PossibleMarkers");
    for ( auto& possibleMarkerIt : _possibleMarkers )
    {
      // this offset should not be applied pose
      const Vec3f& zRenderOffset = (Z_AXIS_3D() * kBW_DebugRenderPossibleMarkersZ);
      
      const Pose3d& thisPose = possibleMarkerIt.pose;
    
      const float kLen = kBW_PossibleMarkerClose_mm;
      Quad3f quad3({ kLen,  kLen, 0},
                   {-kLen,  kLen, 0},
                   { kLen, -kLen, 0},
                   {-kLen, -kLen, 0});
      thisPose.ApplyTo(quad3, quad3);
      quad3 += zRenderOffset;
      _robot.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorWhiteboard.PossibleMarkers",
          quad3, NamedColors::ORANGE, false);

      Vec3f directionEndPoint = X_AXIS_3D() * kLen*0.5f;
      directionEndPoint = thisPose * directionEndPoint;
      _robot.GetContext()->GetVizManager()->DrawSegment("BehaviorWhiteboard.PossibleMarkers",
          thisPose.GetTranslation() + zRenderOffset, directionEndPoint + zRenderOffset, NamedColors::YELLOW, false);
    }
  }
}


} // namespace Cozmo
} // namespace Anki
