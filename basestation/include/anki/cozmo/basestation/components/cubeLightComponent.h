/**
 * File: cubeLightComponent.h
 *
 * Author: Al Chaussee
 * Created: 12/2/2016
 *
 * Description: Manages playing cube light animations on objects as well as backpack lights
 *              A cube light animation consists of multiple patterns, where each pattern has a duration and 
 *              when the pattern has been playing for that duration the next pattern gets played.
 *              An animation can be played on one of three layers (User/Game, Engine, Default), which
 *              function as a priority system with the User/Game layer having highest priority.
 *              Each layer can have multiple animations "playing" in a stack so once the top level animation
 *              finishes the next anim will resume playing assuming its total duration has not been reached. This
 *              is to support blending animations together (playing an animation on top of another)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CubeLightComponent_H__
#define __Anki_Cozmo_Basestation_Components_CubeLightComponent_H__

#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/types.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/ledTypes.h"
#include "clad/types/poseStructs.h"
#include "json/json.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <vector>
#include <map>

namespace Anki {
namespace Cozmo {

class ActiveObject;
class CozmoContext;
class CubeLightAnimationContainer;
class Robot;


using ObjectLEDArray = std::array<u32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS>;
struct ObjectLights {
  ObjectLEDArray onColors{};
  ObjectLEDArray offColors{};
  ObjectLEDArray onPeriod_ms{};
  ObjectLEDArray offPeriod_ms{};
  ObjectLEDArray transitionOnPeriod_ms{};
  ObjectLEDArray transitionOffPeriod_ms{};
  std::array<s32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS> offset{};
  u32 rotationPeriod_ms = 0;
  MakeRelativeMode makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
  Point2f relativePoint;
  
  bool operator==(const ObjectLights& other) const;
  bool operator!=(const ObjectLights& other) const { return !(*this == other);}
};
  


// A single light pattern (multiple light patterns make up an animation)
struct LightPattern
{
  // The name of this pattern (used for debugging)
  std::string name = "";
  
  // The object lights for this pattern
  ObjectLights lights;
  
  // How long this pattern should be played
  u32 duration_ms = 0;
  
  // Whether or not this pattern can be overridden by another pattern
  bool canBeOverridden = true;
  
  // Prints information about this pattern
  void Print() const;
};

class CubeLightComponent : private Util::noncopyable
{
public:
  CubeLightComponent(Robot& robot, const CozmoContext* context);
  
  void Update();
  
  using AnimCompletedCallback = std::function<void(void)>;
  
  // Takes whatever animation is pointed to by the animTrigger and plays it on the
  // specified object
  // Optionally takes a callback that will be called when the animation completes
  // as well as a modifier to be added to the animation
  // Plays the animation on the engine layer
  // Returns true if the animation was able to be played
  bool PlayLightAnim(const ObjectID& objectID,
                     const CubeAnimationTrigger& animTrigger,
                     AnimCompletedCallback callback = {},
                     bool hasModifier = false,
                     const ObjectLights& modifier = {});
  
  // Stops the specified animation pointed to by the animTrigger on an optional object
  // By default the specified animation is stopped on all objects
  void StopLightAnim(const CubeAnimationTrigger& animTrigger, const ObjectID& objectID = -1);
  
  // Stops all animations on all objects
  void StopAllAnims();
  
  // Returns how long an animation lasts in ms
  u32 GetAnimDuration(const CubeAnimationTrigger& trigger);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  // Plays the tap interaction animations on an object depending on its posestate
  void SetTapInteractionObject(const ObjectID& objectID);
  
  // Called whenever an object's posestate changes in order to update the lights
  void OnObjectPoseStateWillChange(const ObjectID& objectID,
                                   const PoseState oldPoseState,
                                   const PoseState newPoseState);
  
  Result SetObjectLights(const ObjectID& objectID, const ObjectLights& lights);
  Result SetObjectLights(const ObjectID& objectID,
                         const WhichCubeLEDs whichLEDs,
                         const u32 onColor,
                         const u32 offColor,
                         const u32 onPeriod_ms,
                         const u32 offPeriod_ms,
                         const u32 transitionOnPeriod_ms,
                         const u32 transitionOffPeriod_ms,
                         const bool turnOffUnspecifiedLEDs,
                         const MakeRelativeMode makeRelative,
                         const Point2f& relativeToPoint,
                         const u32 rotationPeriod_ms);
  
private:

  // Animations can be played on three different layers: User, Engine, or Default
  // User layer is for animations triggered by game
  // Engine layer is for animations triggered within engine
  // Default layer is for the constantly playing animations that change based on
  // object posestate (WakeUp, Connected, Visible, Carrying)
  enum AnimLayerEnum
  {
    User,   // Lights set from external sources like game
    Engine, // Lights set from within engine (interacting, doubleTap)
    State,  // Light patterns that are always playing and connected to the state of the cubes
            // (visible, connected, wakeUp)
    
    Count
  };
  
  // Multiple LightPatterns make up a LightAnim
  using LightAnim = std::list<LightPattern>;
  
  // Information about an actively playing LightAnim
  struct CurrentAnimInfo
  {
    // The name of this anim (used for debugging purposes)
    std::string name = "";
    
    // The animation trigger that caused this anim to play
    CubeAnimationTrigger trigger;
    
    // An iterator to the current light pattern within the LightAnim that is being played
    LightAnim::iterator curPattern;
    
    // An iterator to the last pattern in the current LightAnim
    LightAnim::iterator endOfPattern;
    
    // The basestation time that the curPattern ends and the next pattern in the LightAnim should
    // start playing
    TimeStamp_t timeCurPatternEnds = 0;
    
    // Whether or not this anim can be overridden by another anim
    bool canBeOverridden = true;
    
    // A callback to be called when the animation completes
    AnimCompletedCallback callback = {};
    
    // Whether or not this animation is a blend of two animations
    // Used to know when to remove animations out of the _blendedAnims map
    bool isBlended = false;
    
    // Whether or not this animation should be immediately stopped
    bool stopNow = false;
    
    // If this is != 0 then this animation was played from the TriggerCubeAnimationAction
    // This is to prevent calling a callback that is invalid due to the action being destroyed before the
    // animation completes
    u32 actionTagCallbackIsFrom = 0;
  };
  
  // Only TriggerCubeAnimationAction should call this PlayLightAnim function because
  // the action will play the animation on the user layer instead of the engine layer
  friend class TriggerCubeAnimationAction;
  bool PlayLightAnimFromAction(const ObjectID& objectID,
                               const CubeAnimationTrigger& animTrigger,
                               AnimCompletedCallback callback,
                               const u32& actionTag);
  
  // Plays an anim on an obect on the given layer. Optionally takes a callback and/or a modifier to be applied
  // to the animation
  // Returns true if the animation was able to be played
  bool PlayLightAnim(const ObjectID& objectID,
                     const CubeAnimationTrigger& animTrigger,
                     const AnimLayerEnum& layer,
                     AnimCompletedCallback callback = {},
                     bool hasModifier = false,
                     const ObjectLights& modifier = {});
  
  // Stop an animation on a specific layer for an object (or all objects)
  void StopLightAnim(const CubeAnimationTrigger& animTrigger,
                     const AnimLayerEnum& layer,
                     const ObjectID& objectID = -1);
  
  // Stops all animations on a given layer for an object (or all objects)
  void StopAllAnimsOnLayer(const AnimLayerEnum& layer, const ObjectID& objectID = -1);
  
  // Applies the modifier to the animation by augmenting the light values in the animation
  // Will apply the modifier to all patterns in the animation
  // Note: Will not modify the duration of the animation
  void ApplyAnimModifier(const LightAnim& anim,
                         const ObjectLights& modifier,
                         LightAnim& modifiedAnim);
  
  // Attempts to blend the anim with the currently playing animation on the object
  // If the anim has any on/offColors that are 0 they get replaced by the current on/offColor on the object
  // Returns true if the animation was successfully blended
  bool BlendAnimWithCurLights(const ObjectID& objectID,
                              const LightAnim& anim,
                              LightAnim& blendedAnim);
  
  // Picks the next default layer animation to play on the object
  void PickNextAnimForDefaultLayer(const ObjectID& objectID);
  
  // Debug function to play subsequent animation triggers on an object with each call to
  // the function
  void CycleThroughAnimTriggers(const ObjectID& objectID);
  
  // Send a message to game to let them know the object's current light values
  // Will only send if _sendTransitionMessages is true
  void SendTransitionMessage(const ObjectID& objectID, const ObjectLights& values);
  
  // Enables or disables the playing of only game layer animations
  void EnableGameLayerOnly(const ObjectID& objectID, bool enable);
  
  const char* LayerToString(const AnimLayerEnum& layer) const;
  
  ActiveObject* GetActiveObjectInAnyFrame(const ObjectID& objectID);
  
  Result SetLights(const ActiveObject* object, const u32 rotationPeriod_ms);
  
  // Applies white balancing to a color
  ColorRGBA WhiteBalanceColor(const ColorRGBA& color) const;

  Robot& _robot;
  
  // Reference to the container of cube light animations
  CubeLightAnimationContainer& _cubeLightAnimations;
  
  // An array of all the animations that are playing on each layer (indexed by layer)
  // Multiple animations can be "playing" on a given layer (hence the list of PatternPlayInfo)
  // This is so that you can blend animations and return to the previously playing animation
  using AnimLayer = std::array<std::list<CurrentAnimInfo>, AnimLayerEnum::Count>;
  
  // Contains information about all animations on all layers for an object
  struct ObjectInfo
  {
    // The current layer that an animation is playing on
    AnimLayerEnum curLayer = AnimLayerEnum::State;
    
    // Whether or not only the user/game layer is enabled
    bool isOnlyGameLayerEnabled = false;
    
    // Array (indexed by layer) of lists of PatternPlayInfo (actively playing animations)
    AnimLayer animationsOnLayer;
    
    // Array (indexed by layer) of dynamically created animations (blended anims and modified anims)
    std::array<LightAnim, AnimLayerEnum::Count> blendedAnims;
  };

  // Maps objectIDs to ObjectInfo to keep track of the state of each object
  std::map<ObjectID, ObjectInfo> _objectInfo;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  // Whether or not we should send CubeLightsStateTransition messages to game when object lights
  // change
  bool _sendTransitionMessages = false;
  
  // Whether or not the robot is delocalized
  // Used to update the lights when we relocalize
  bool _robotDelocalized = false;
  
  bool _onlyGameLayerEnabledForAll = false;
  
};

}
}

#endif
