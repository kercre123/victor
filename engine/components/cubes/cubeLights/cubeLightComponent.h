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

#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/shared/types.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/cubeAnimationTrigger.h"
#include "clad/types/ledTypes.h"
#include "clad/types/poseStructs.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h" // some useful typedefs
#include "engine/components/cubes/cubeLights/cubeLightAnimation.h"
#include "engine/components/cubes/cubeLights/cubeLightAnimationHelpers.h"
#include "engine/engineTimeStamp.h"
#include "engine/robotComponents_fwd.h"
#include "json/json.h"

#include "util/cladHelpers/cladEnumToStringMap.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <vector>
#include <map>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

class ActiveObject;
class CozmoContext;
class CubeAnimation;
class CubeLightAnimationContainer;
class IGatewayInterface;
class Robot;

class CubeLightComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  using AnimationHandle = u32;

  CubeLightComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::CubeComms);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  #if ANKI_DEV_CHEATS
    void SaveLightsToDisk(const std::string& fileName,
                          const CubeLightAnimation::Animation& anim);
  #endif

  using AnimCompletedCallback = std::function<void(void)>;

  CubeLightAnimation::Animation* GetAnimation(const CubeAnimationTrigger& animTrigger);

  // Takes whatever animation is pointed to by the animTrigger and plays it on the
  // specified object
  // Optional Arguments
  // - A callback that will be called when the animation completes
  // - A light pattern modifier to be added to all patterns in the animation
  // - A duration modifier to increase the duration of all patterns in the animation
  // Plays the animation on the engine layer
  // Returns true if the animation was able to be played
  bool PlayLightAnimByTrigger(const ObjectID& objectID,
                              const CubeAnimationTrigger& animTrigger,
                              AnimCompletedCallback callback = {},
                              bool hasModifier = false,
                              const CubeLightAnimation::ObjectLights& modifier = {},
                              const s32 durationModifier_ms = 0);
  
  bool PlayLightAnim(const ObjectID& objectID,
                     CubeLightAnimation::Animation& animation,
                     AnimCompletedCallback callback,
                     const std::string& debugName,
                     AnimationHandle& outHandle);


  // Stops the specified animation pointed to by the animTrigger on an optional object
  // and resumes whatever animation was previously playing
  // By default the specified animation is stopped on all objects
  // Returns true if the animation was stopped
  bool StopLightAnimAndResumePrevious(const CubeAnimationTrigger& animTrigger,
                                      const ObjectID& objectID = -1);

  bool StopLightAnimAndResumePrevious(const AnimationHandle& handle,
                                      const ObjectID& objectID = -1);
  
  // Stops the specified animation and immediately plays the animTriggerToPlay without allowing
  // whatever animation was previously playing from resuming
  // Returns true if the animation was able to be played
  bool StopAndPlayLightAnim(const ObjectID& objectID,
                            const CubeAnimationTrigger& animTriggerToStop,
                            const CubeAnimationTrigger& animTriggerToPlay,
                            AnimCompletedCallback callback = {},
                            bool hasModifier = false,
                            const CubeLightAnimation::ObjectLights& modifier = {});
  // handle will be used to stop an animation, and then set with the handle for the new animToPlay
  bool StopAndPlayLightAnim(const ObjectID& objectID,
                            AnimationHandle& handle,
                            CubeLightAnimation::Animation& animToPlay,
                            const std::string& debugName,
                            AnimCompletedCallback callback = {},
                            bool hasModifier = false,
                            const CubeLightAnimation::ObjectLights& modifier = {});
  
  // Stops all animations on all objects
  void StopAllAnims();

  // Connection/Status light controls
  // NOTE: objectID's throughout this system will likely be unnecessary on victor since he has only 
  // got one cube (just let the cubeCommsComponent worry about it) but that's a thought for later...
  bool PlayConnectionLights(const ObjectID& objectID, AnimCompletedCallback callback = {});
  bool PlayDisconnectionLights(const ObjectID& objectID, AnimCompletedCallback callback = {});
  bool CancelDisconnectionLights(const ObjectID& objectID);
  bool PlayTapResponseLights(const ObjectID& objectID, AnimCompletedCallback callback = {});
  bool EnableStatusAnims(const bool enable);

  template<typename T>
  void HandleMessage(const T& msg);
  
  // Called whenever an active object's posestate changes in order to update the lights
  void OnActiveObjectPoseStateChanged(const ObjectID& objectID,
                                      const PoseState oldPoseState,
                                      const PoseState newPoseState);
  
  // We don't want to expose Animation layers outside of the cubeLightComponent
  // but there are scenarios where game takes over control of lights and clears
  // out engine lights, or new cubes connect while game is in control of lights
  // In these scenarios engine may need to check whether it can set lights or not
  // to ensure that a StopAndPlay was not cancelled before another one was called
  // which will hit an assert
  bool CanEngineSetLightsOnCube(const ObjectID& objectID);
  
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

  // Information about an actively playing Animation
  struct CurrentAnimInfo
  {
    // The name of this anim (used for debugging purposes)
    std::string debugName = "";
    
    // Animation handle used internally/externally to reference playing animations
    AnimationHandle animationHandle;
    
    // An iterator to the current light pattern within the CubeLightAnimation that is being played
    CubeLightAnimation::Animation::iterator curPattern;
    
    // An iterator to the last pattern in the current CubeLightAnimation
    CubeLightAnimation::Animation::iterator endOfPattern;
    
    // The basestation time that the curPattern ends and the next pattern in the CubeLightAnimation should
    // start playing
    EngineTimeStamp_t timeCurPatternEnds = 0;
    
    // Whether or not this anim can be overridden by another anim
    bool canBeOverridden = true;
    
    // A callback to be called when the animation completes
    AnimCompletedCallback callback = {};
    
    // Whether or not this animation is a blend of two animations
    // Used to know when to remove animations out of the _blendedAnims map
    bool isBlended = false;
    
    // Whether or not this animation should be immediately stopped
    bool stopNow = false;
    
    // Time to add to the duration of all patterns in this animation
    s32 durationModifier_ms = 0;
    
    // If this is != 0 then this animation was played from the TriggerCubeAnimationAction
    // This is to prevent calling a callback that is invalid due to the action being destroyed before the
    // animation completes
    u32 actionTagCallbackIsFrom = 0;
  };
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
    std::array<CubeLightAnimation::Animation, AnimLayerEnum::Count> blendedAnims;
  };

  Robot* _robot = nullptr;

  std::unique_ptr<CubeLightAnimationContainer> _cubeLightAnimations;
  Util::CladEnumToStringMap<CubeAnimationTrigger>* _triggerToAnimMap;

  // Maps objectIDs to ObjectInfo to keep track of the state of each object
  std::map<ObjectID, ObjectInfo> _objectInfo;
  
  std::list<Signal::SmartHandle> _eventHandles;

  IGatewayInterface* _gi = nullptr;
  std::set<AppToEngineTag> _appToEngineTags;
  
  // Whether or not we should send CubeLightsStateTransition messages to game when object lights
  // change
  bool _sendTransitionMessages = false;
  
  // Whether or not the robot is delocalized
  // Used to update the lights when we relocalize
  bool _robotDelocalized = false;
  
  bool _onlyGameLayerEnabledForAll = false;
  
  // Whether or not cube sleep is enabled
  bool _enableCubeSleep = false;
  
  // Whether or not to play the fade out cube sleep animation
  bool _skipSleepAnim = false;

  // Whether or not to play connection/status lights
  // Default to false so that status lights are driven from the CubeConnectionCoordinator
  bool _enableStatusAnims = false;
  
  // The last cube light gamma that was sent to the robot (global across all cubes)
  u32 _prevGamma = 0;

  AnimationHandle _nextAnimationHandle = 0;
  std::unordered_map<CubeAnimationTrigger, AnimationHandle> _triggerToHandleMap;
  
  void UpdateInternal(bool shouldPickNextAnim);

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
  bool PlayLightAnimByTrigger(const ObjectID& objectID,
                              const CubeAnimationTrigger& animTrigger,
                              const AnimLayerEnum& layer,
                              AnimCompletedCallback callback = {},
                              bool hasModifier = false,
                              const CubeLightAnimation::ObjectLights& modifier = {},
                              const s32 durationModifier_ms = 0);

  bool PlayLightAnimInternal(const ObjectID& objectID,
                             CubeLightAnimation::Animation& animation,
                             const AnimLayerEnum& layer,
                             AnimCompletedCallback callback,
                             bool hasModifier,
                             const CubeLightAnimation::ObjectLights& modifier,
                             const s32 durationModifier_ms,
                             const std::string& debugName,
                             AnimationHandle& outHandle);
  
  bool StopLightAnimByTrigger(const CubeAnimationTrigger& animTrigger,
                              const AnimLayerEnum& layer,
                              const ObjectID& objectID = -1,
                              bool shouldPickNextAnim = true);

  // Stop an animation on a specific layer for an object (or all objects)
  // shouldPickNextAnimOnStop determines if we can immediately pick a default anim to
  // start playing on the State layer should there be no Engine anims left
  // Returns true if the animation was stopped
  bool StopLightAnimInternal(const AnimationHandle& animationHandle,
                             const AnimLayerEnum& layer,
                             const ObjectID& objectID = -1,
                             bool shouldPickNextAnim = true);
  
  // Stops all animations on a given layer for an object (or all objects)
  void StopAllAnimsOnLayer(const AnimLayerEnum& layer, const ObjectID& objectID = -1);
  
  Result SetCubeLightAnimation(const ObjectID& objectID, const CubeLightAnimation::ObjectLights& lights);
  Result SetCubeLightAnimation(const ObjectID& objectID,
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
                               const bool rotate);

  // Applies the modifier to the animation by augmenting the light values in the animation
  // Will apply the modifier to all patterns in the animation
  // Note: Will not modify the duration of the animation
  void ApplyAnimModifier(const CubeLightAnimation::Animation& anim,
                         const CubeLightAnimation::ObjectLights& modifier,
                         CubeLightAnimation::Animation& modifiedAnim);
  
  // Attempts to blend the anim with the currently playing animation on the object
  // If the anim has any on/offColors that are 0 they get replaced by the current on/offColor on the object
  // Returns true if the animation was successfully blended
  bool BlendAnimWithCurLights(const ObjectID& objectID,
                              const CubeLightAnimation::Animation& anim,
                              CubeLightAnimation::Animation& blendedAnim);
  
  // Picks the next default layer animation to play on the object
  void PickNextAnimForDefaultLayer(const ObjectID& objectID);
  
  // Debug function to play subsequent animation triggers on an object with each call to
  // the function
  void CycleThroughAnimTriggers(const ObjectID& objectID);
  
  // Send a message to game to let them know the object's current light values
  // Will only send if _sendTransitionMessages is true
  void SendTransitionMessage(const ObjectID& objectID, const CubeLightAnimation::ObjectLights& values);
  
  // Enables or disables the playing of only game layer animations
  void EnableGameLayerOnly(const ObjectID& objectID, bool enable);
  
  const char* LayerToString(const AnimLayerEnum& layer) const;
  
  Result SetLights(const ActiveObject* object, const bool rotate);
  
  // Applies white balancing to a color
  ColorRGBA WhiteBalanceColor(const ColorRGBA& color) const;
  
  void HandleAppRequest(const AppToEngineEvent& event);
};

}
}

#endif
