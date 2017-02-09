/**
 * File: cubeLightComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 12/2/2016
 *
 * Description: Manages playing cube light animations on objects
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
#include "anki/cozmo/basestation/components/cubeLightComponent.h"

#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/animationContainers/cubeLightAnimationContainer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ledEncoding.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/fileUtils/fileUtils.h"

#define DEBUG_TEST_ALL_ANIM_TRIGGERS 0
#define DEBUG_LIGHTS 0

// Scales colors by this factor when applying white balancing
static constexpr f32 kWhiteBalanceScale = 0.6f;

namespace Anki {
namespace Cozmo {

static const ObjectLights kCubeLightsOff = {
  .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0,0}},
  .offPeriod_ms           = {{0,0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0,0}},
  .offset                 = {{0,0,0,0}},
  .rotationPeriod_ms      = 0,
  .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
  .relativePoint          = {0,0}
};
  
  
bool ObjectLights::operator==(const ObjectLights& other) const
{
  return this->onColors        == other.onColors &&
  this->offColors              == other.offColors &&
  this->onPeriod_ms            == other.onPeriod_ms &&
  this->offPeriod_ms           == other.offPeriod_ms &&
  this->transitionOnPeriod_ms  == other.transitionOnPeriod_ms &&
  this->transitionOffPeriod_ms == other.transitionOffPeriod_ms &&
  this->offset                 == other.offset &&
  this->rotationPeriod_ms      == other.rotationPeriod_ms &&
  this->makeRelative           == other.makeRelative &&
  this->relativePoint          == other.relativePoint;
}
  

CubeLightComponent::CubeLightComponent(Robot& robot, const CozmoContext* context)
: _robot(robot)
, _cubeLightAnimations(context->GetRobotManager()->GetCubeLightAnimations())
{
  // Subscribe to messages
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectConnectionState>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotDelocalized>();
    
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCubeLightsStateTransitionMessages>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::FlashCurrentLightsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::PlayCubeAnim>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::StopCubeAnim>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetAllActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableLightStates>();
  }
  
  static_assert(AnimLayerEnum::User < AnimLayerEnum::Engine &&
                AnimLayerEnum::Engine < AnimLayerEnum::State,
                "CubeLightComponent.LayersInUnexpectedOrder");
}

void CubeLightComponent::Update()
{
  const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  // We are going from delocalized to localized
  bool doRelocalizedUpdate = false;
  if(_robotDelocalized && _robot.IsLocalized())
  {
    doRelocalizedUpdate = true;
    _robotDelocalized = false;
  }
  
  for(auto& objectInfo : _objectInfo)
  {
    // Pick the correct animation to play when relocalizing
    if(doRelocalizedUpdate)
    {
      PickNextAnimForDefaultLayer(objectInfo.first);
      continue;
    }
  
    AnimLayerEnum& layer = objectInfo.second.curLayer;
    
    // If there are no animations on this layer don't do anything
    if(objectInfo.second.animationsOnLayer[layer].empty())
    {
      continue;
    }
    
    auto& objectAnim = objectInfo.second.animationsOnLayer[layer].back();
    
    // If the current animation never ends and it isn't being stopped don't do anything
    if(objectAnim.timeCurPatternEnds == 0 && !objectAnim.stopNow)
    {
      continue;
    }
    // Otherwise if the current pattern in this animation should end or the whole animation is being stopped
    else if(curTime >= objectAnim.timeCurPatternEnds || objectAnim.stopNow)
    {
      // The current pattern is ending so try to go to the next pattern in the animation
      ++objectAnim.curPattern;
      
      // If there are no more patterns in this animation or the animation is being stopped
      if(objectAnim.curPattern == objectAnim.endOfPattern || objectAnim.stopNow)
      {
        // Call the callback since the animation is complete
        if(objectAnim.callback)
        {
          // Only call the callback if it isn't from an action or
          // it is from an action and that action's tag is still in use
          // If the action gets destroyed and the tag is no longer in use the callback would be
          // invalid
          if(objectAnim.actionTagCallbackIsFrom == 0 ||
             IActionRunner::IsTagInUse(objectAnim.actionTagCallbackIsFrom))
          {
            objectAnim.callback();
          }
        }
        
        // Remove this animation from the stack
        // Note: objectAnim is now invalid. It can NOT be reassigned or accessed
        objectInfo.second.animationsOnLayer[layer].pop_back();
      
        // If there are no more animations being played on this layer
        if(objectInfo.second.animationsOnLayer[layer].empty())
        {
          // If all layers are enabled go back to default layer
          if(!objectInfo.second.isOnlyGameLayerEnabled)
          {
            const AnimLayerEnum prevLayer = layer;

            layer = AnimLayerEnum::State;
            
            PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.Update.NoAnimsLeftOnLayer",
                          "No more animations left on layer %s going to %s layer",
                          LayerToString(prevLayer),
                          LayerToString(layer));
            
            PickNextAnimForDefaultLayer(objectInfo.first);
          }
          // Otherwise only game layer is enable so just turn the lights off
          else
          {
            Result res = SetObjectLights(objectInfo.first, kCubeLightsOff);
            if(res != RESULT_OK)
            {
              PRINT_NAMED_WARNING("CubeLightComponent.Update.SetLightsFailed", "");
            }
          }
          continue;
        }

        // There are still animations being played on this layer so switch to the next anim
        // Note: A new variable objectAnim2 is created instead of reassigning objectAnim
        // This is because objectAnim has become invalid by the call to pop_back() which means
        // trying to assign to it is not valid
        //
        // std::list<std::string> l; l.push_back("a"); l.push_back("b");
        // std::string& x = l.back(); // reference to "b"
        // l.pop_back(); // removes "b" and invalidates x
        // x = l.back(); // this looks like it should make x reference "a" however x is
        // invalid from the pop_back and cannot be reassigned
        const auto& prevObjectAnim = objectInfo.second.animationsOnLayer[layer].back();
        PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.Update.PlayingPrevious",
                      "Have previous anim %s on layer %s id %u",
                      prevObjectAnim.name.c_str(),
                      LayerToString(layer),
                      objectInfo.first.GetValue());
        
        SendTransitionMessage(objectInfo.first, prevObjectAnim.curPattern->lights);
        Result res = SetObjectLights(objectInfo.first, prevObjectAnim.curPattern->lights);
        if(res != RESULT_OK)
        {
          PRINT_NAMED_WARNING("CubeLightComponent.Update.SetLightsFailed", "");
        }
        continue;
      }
      // Otherwise, we are going to the next pattern in the animation so update the time the pattern will end
      else
      {
        objectAnim.timeCurPatternEnds = 0;
        if(objectAnim.curPattern->duration_ms > 0)
        {
          objectAnim.timeCurPatternEnds = curTime + objectAnim.curPattern->duration_ms;
        }
      }
      
      // Finally set the lights with the current pattern of the animation
      SendTransitionMessage(objectInfo.first, objectAnim.curPattern->lights);
      Result res = SetObjectLights(objectInfo.first, objectAnim.curPattern->lights);
      if(res != RESULT_OK)
      {
        PRINT_NAMED_WARNING("CubeLightComponent.Update.SetLightsFailed", "");
      }
    }
  }
}

bool CubeLightComponent::PlayLightAnimFromAction(const ObjectID& objectID,
                                                 const CubeAnimationTrigger& animTrigger,
                                                 AnimCompletedCallback callback,
                                                 const u32& actionTag)
{
  const AnimLayerEnum layer = AnimLayerEnum::User;
  const bool res = PlayLightAnim(objectID, animTrigger, layer, callback);
  if(res)
  {
    // iter is guaranteed to be valid otherwise PlayLightAnim will have returned false
    // animationOnLayer[layer] is guaranteed to not be empty because PlayLightAnim will return true
    // when an animation is added to the layer
    auto iter = _objectInfo.find(objectID);
    
    DEV_ASSERT(iter != _objectInfo.end(), "CubeLightComponent.PlayLightAnimFromAction.IterNotValid");
    
    // Update the action tag the callback is from, the animation was just added to the end
    // of animationsOnLayer[layer] by the call to PlayLightAnim
    iter->second.animationsOnLayer[layer].back().actionTagCallbackIsFrom = actionTag;
  }
  return res;
}

bool CubeLightComponent::PlayLightAnim(const ObjectID& objectID,
                                          const CubeAnimationTrigger& animTrigger,
                                          AnimCompletedCallback callback,
                                          bool hasModifier,
                                          const ObjectLights& modifier)
{
  return PlayLightAnim(objectID, animTrigger, AnimLayerEnum::Engine, callback, hasModifier, modifier);
}

bool CubeLightComponent::PlayLightAnim(const ObjectID& objectID,
                                          const CubeAnimationTrigger& animTrigger,
                                          const AnimLayerEnum& layer,
                                          AnimCompletedCallback callback,
                                          bool hasModifier,
                                          const ObjectLights& modifier)
{
  auto iter = _objectInfo.find(objectID);
  if(iter != _objectInfo.end())
  {
    auto& objectInfo = iter->second;
    auto& animationsOnLayer = objectInfo.animationsOnLayer[layer];
    
    const bool isPlayingAnim = !animationsOnLayer.empty();
    const bool isCurAnimBlended = (isPlayingAnim && animationsOnLayer.back().isBlended);
    
    // If we are currently playing an animation and it can not be overridden do nothing
    if(isPlayingAnim &&
       !animationsOnLayer.back().canBeOverridden)
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.CantBeOverridden",
                    "Current anim %s can not be overridden on object %u layer %s",
                    animationsOnLayer.back().name.c_str(),
                    objectID.GetValue(),
                    LayerToString(layer));
      return false;
    }
    
    // Get the name of the animation to play for this trigger
    const std::string& animName = _robot.GetContext()->GetRobotManager()->GetCubeAnimationForTrigger(animTrigger);
    auto* anim = _cubeLightAnimations.GetAnimation(animName);
    if(anim == nullptr)
    {
      PRINT_NAMED_WARNING("CubeLightComponent.NoAnimForTrigger",
                          "No animation found for trigger %s", animName.c_str());
      return false;
    }
    
    // If only the game layer is enabled and we are trying to play this animation on a different layer
    if(objectInfo.isOnlyGameLayerEnabled &&
       layer != AnimLayerEnum::User)
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.OnlyGameLayerEnabled",
                    "Only game layer is enabled, refusing to play anim %s on object %u layer %s",
                    animName.c_str(),
                    objectID.GetValue(),
                    LayerToString(layer));
      return false;
    }
    
    // If the game/user layer is not enabled and an animation is trying to be played on the game/user layer
    // don't play it
    if(!objectInfo.isOnlyGameLayerEnabled &&
       layer == AnimLayerEnum::User)
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.NotPlayingUserAnim",
                    "Refusing to play anim on %s layer when it is not enabled",
                    LayerToString(layer));
      return false;
    }
    
    // If we have a modifier to apply to the animation apply it
    LightAnim modifiedAnim;
    if(hasModifier)
    {
      ApplyAnimModifier(*anim, modifier, modifiedAnim);
    }
    
    // Attempt to blend the animation with the current light pattern. If the animation can not be blended
    // with the current animation then blendedAnim will be the same as either modifiedAnim or animIter->second
    LightAnim blendedAnim;
    const bool didBlend = BlendAnimWithCurLights(objectID,
                                                 (hasModifier ? modifiedAnim : *anim),
                                                 blendedAnim);
    
    // If the current animation is already a blended animation and the animation to play was able
    // to be blended with the current animation then do nothing
    // TODO (Al): Maybe support this ability in the future. Animation duration of blended animations gets
    // complicated
    if(isCurAnimBlended && didBlend)
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.AlreadyPlayingBlendedAnim",
                    "Can't play a blended anim over already playing blended anim");
      return false;
    }
    
    CurrentAnimInfo info;
    info.callback = callback;
    // Treat modified animations the same as blended animations (ie they live in the blendedAnims array)
    if(!isCurAnimBlended &&
       (didBlend || hasModifier))
    {
      // Store the blendedAnim in the blendedAnims array so we can grab iterators to it
      objectInfo.blendedAnims[layer] = blendedAnim;
      
      info.name = (isPlayingAnim ? animationsOnLayer.back().name+"+" : "") + animName;
      info.trigger = animTrigger;
      info.curPattern = objectInfo.blendedAnims[layer].begin();
      info.endOfPattern = objectInfo.blendedAnims[layer].end();
      info.isBlended = true;
    }
    else
    {
      info.name = animName;
      info.trigger = animTrigger;
      info.curPattern = anim->begin();
      info.endOfPattern = anim->end();
      
      // If this animation has no duration then clear the existing animations on the layer
      if(info.curPattern->duration_ms == 0)
      {
        animationsOnLayer.clear();
      }
    }
    
    // Add the new animation to the list of animations playing on this layer
    animationsOnLayer.push_back(info);
    
    auto& animation = animationsOnLayer.back();
    
    const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    animation.timeCurPatternEnds = 0;
    
    // If the first pattern in the animation has a non zero duration then update the time the pattern
    // should end
    if(animation.curPattern->duration_ms > 0)
    {
      animation.timeCurPatternEnds = curTime + animation.curPattern->duration_ms;
    }
    
    // Whether or not this animation canBeOverridden is defined by whether or not the last pattern in the
    // animation definition can be overridden
    animation.canBeOverridden = anim->back().canBeOverridden;

    // If this layer has higher priority then actually set the object lights
    // User > Engine > Default (User layer takes precedence over all layers)
    if(layer <= objectInfo.curLayer)
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.SettingLights",
                    "Playing trigger %s on layer %s for object %u",
                    EnumToString(animTrigger),
                    LayerToString(layer),
                    objectID.GetValue());
      
      // Update the current layer for this object
      objectInfo.curLayer = layer;
      
      SendTransitionMessage(objectID, animation.curPattern->lights);
      Result res = SetObjectLights(objectID, animation.curPattern->lights);
      if(res != RESULT_OK)
      {
        // Since no light pattern was set, setting the end time of the pattern to zero will make it so that
        // everytime it is updated nothing will happen (previous pattern will probably be playing)
        animation.timeCurPatternEnds = 0;
      }
    }
    // Otherwise the animation was still added to the layer but a higher priority layer is currently
    // playing an animation so we didn't actually set object lights
    else
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.LightsNotSet",
                    "Lights not set on object %u layer %s because current layer is %u",
                    objectID.GetValue(), LayerToString(layer), objectInfo.curLayer);
    }
    return true;
  }
  else
  {
    PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.PlayLightAnim.InvalidObjectID",
                  "No object with id %u exists in _objectInfo map can't play animation",
                  objectID.GetValue());
    return false;
  }
}

void CubeLightComponent::StopAllAnims()
{
  StopAllAnimsOnLayer(AnimLayerEnum::Engine);
}

void CubeLightComponent::StopAllAnimsOnLayer(const AnimLayerEnum& layer, const ObjectID& objectID)
{
  // If objectID is unknown (-1) then stop all animations for all objects on the given layer
  if(objectID.IsUnknown())
  {
    PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.StopAllAnimsOnLayer.AllObjects",
                  "Stopping all anims for all objects on layer %s",
                  LayerToString(layer));
    for(auto& pair : _objectInfo)
    {
      for(auto& anim : pair.second.animationsOnLayer[layer])
      {
        anim.stopNow = true;
      }
    }
  }
  else
  {
    auto iter = _objectInfo.find(objectID);
    if(iter == _objectInfo.end())
    {
      PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.StopAllAnimsOnLayer.BadObject",
                    "ObjectId %u does not exist in _objectInfo map unable to stop anims",
                    objectID.GetValue());
      return;
    }

    for(auto& anim : iter->second.animationsOnLayer[layer])
    {
      anim.stopNow = true;
    }
  }
  
  // Manually update so the anims are immediately stopped
  Update();
}

void CubeLightComponent::StopLightAnim(const CubeAnimationTrigger& animTrigger,
                                          const ObjectID& objectID)
{
  StopLightAnim(animTrigger, AnimLayerEnum::Engine, objectID);
}

void CubeLightComponent::StopLightAnim(const CubeAnimationTrigger& animTrigger,
                                          const AnimLayerEnum& layer,
                                          const ObjectID& objectID)
{
  PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.StopLightAnim",
                "Stopping %s on object %u on layer %s",
                EnumToString(animTrigger),
                objectID.GetValue(),
                LayerToString(layer));
  
  auto helper = [this](const CubeAnimationTrigger& animTrigger,
                       const ObjectID& objectID,
                       const AnimLayerEnum& layer) {
    auto& iter = _objectInfo[objectID];
    for(auto& anim : iter.animationsOnLayer[layer])
    {
      if(anim.trigger == animTrigger)
      {
        anim.stopNow = true;
      }
    }
  };

  if(objectID.IsUnknown())
  {
    for(auto& iter : _objectInfo)
    {
      helper(animTrigger, iter.first, layer);
    }
  }
  else
  {
    const auto& iter = _objectInfo.find(objectID);
    if(iter != _objectInfo.end())
    {
      helper(animTrigger, objectID, layer);
    }
  }
  
  // Manually update so the anims are immediately stopped
  Update();
}

void CubeLightComponent::ApplyAnimModifier(const LightAnim& anim,
                                           const ObjectLights& modifier,
                                           LightAnim& modifiedAnim)
{
  modifiedAnim.clear();
  modifiedAnim.insert(modifiedAnim.begin(), anim.begin(), anim.end());
  
  // For every pattern in the animation apply the modifier
  for(auto& pattern : modifiedAnim)
  {
    for(int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
    {
      pattern.lights.onColors[i] |= modifier.onColors[i];
      pattern.lights.offColors[i] |= modifier.offColors[i];
      pattern.lights.onPeriod_ms[i] += modifier.onPeriod_ms[i];
      pattern.lights.offPeriod_ms[i] += modifier.offPeriod_ms[i];
      pattern.lights.transitionOnPeriod_ms[i] += modifier.transitionOnPeriod_ms[i];
      pattern.lights.transitionOffPeriod_ms[i] += modifier.transitionOffPeriod_ms[i];
      pattern.lights.offset[i] += modifier.offset[i];
      pattern.lights.rotationPeriod_ms += modifier.rotationPeriod_ms;
    }
    pattern.lights.makeRelative = modifier.makeRelative;
    pattern.lights.relativePoint = modifier.relativePoint;
  }
}

bool CubeLightComponent::BlendAnimWithCurLights(const ObjectID& objectID,
                                                const LightAnim& anim,
                                                LightAnim& blendedAnim)
{
  ActiveObject* activeObject = _robot.GetBlockWorld().GetConnectedActiveObjectByID(objectID);
  if(activeObject == nullptr)
  {
    PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.BlendAnimWithCurLights.NullActiveObject",
                  "No active object with id %u found in block world unable to blend anims",
                  objectID.GetValue());
    return false;
  }
  
  blendedAnim.clear();
  blendedAnim.insert(blendedAnim.begin(), anim.begin(), anim.end());
  
  bool res = false;
  
  // For every pattern in the anim replace the on/offColors that are completely zero
  // (completely zero meaning alpha is 0 as well as r,g,b are 0, normally alpha should be 1)
  // with what ever the corresponding color that is currently being displayed on the object
  for(auto& pattern : blendedAnim)
  {
    for(int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
    {
      const ActiveObject::LEDstate& ledState = activeObject->GetLEDState(i);
      if(pattern.lights.onColors[i] == 0)
      {
        pattern.lights.onColors[i] = ledState.onColor.AsRGBA();
        res = true;
      }
      if(pattern.lights.offColors[i] == 0)
      {
        pattern.lights.offColors[i] = ledState.offColor.AsRGBA();
        res = true;
      }
    }
  }

  return res;
}

void CubeLightComponent::PickNextAnimForDefaultLayer(const ObjectID& objectID)
{
  if(DEBUG_TEST_ALL_ANIM_TRIGGERS)
  {
    CycleThroughAnimTriggers(objectID);
    return;
  }
  
  CubeAnimationTrigger anim = CubeAnimationTrigger::Connected;
  
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(objectID);
  if(object != nullptr)
  {
    if(object->IsPoseStateKnown())
    {
      anim = CubeAnimationTrigger::Visible;
    }
  }
  
  if(_robot.GetCarryingObject() == objectID)
  {
    anim = CubeAnimationTrigger::Carrying;
  }
  
  PlayLightAnim(objectID, anim, AnimLayerEnum::State);
}

u32 CubeLightComponent::GetAnimDuration(const CubeAnimationTrigger& trigger)
{
  const std::string& animName = _robot.GetContext()->GetRobotManager()->GetCubeAnimationForTrigger(trigger);
  const auto* anim = _cubeLightAnimations.GetAnimation(animName);
  if(anim == nullptr)
  {
    PRINT_NAMED_WARNING("CubeLightComponent.NoAnimForTrigger",
                        "No animation found for trigger %s", animName.c_str());
    return 0;
  }
  
  u32 duration_ms = 0;
  for(const auto& pattern : *anim)
  {
    duration_ms += pattern.duration_ms;
  }
  return duration_ms;
}

void CubeLightComponent::SetTapInteractionObject(const ObjectID& objectID)
{
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(objectID);
  CubeAnimationTrigger anim = CubeAnimationTrigger::DoubleTappedUnsure;
  if((object != nullptr) && object->IsPoseStateKnown())
  {
    // we know about this cube in this origin
    anim = CubeAnimationTrigger::DoubleTappedKnown;
  }
  PlayLightAnim(objectID, anim);
}

void CubeLightComponent::OnObjectPoseStateWillChange(const ObjectID& objectID,
                                                        const PoseState oldPoseState,
                                                        const PoseState newPoseState)
{
  if(oldPoseState == newPoseState)
  {
    return;
  }
  
  if(newPoseState != PoseState::Known)
  {
    PlayLightAnim(objectID, CubeAnimationTrigger::Connected, AnimLayerEnum::State);
  }
  else
  {
    // If going to Known change to Visible
    PlayLightAnim(objectID, CubeAnimationTrigger::Visible, AnimLayerEnum::State);
  }
}

void CubeLightComponent::EnableGameLayerOnly(const ObjectID& objectID, bool enable)
{
  PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.EnableGameLayerOnly",
                "%s game layer only for %s",
                (enable ? "Enabling" : "Disabling"),
                (objectID.IsUnknown() ? "all objects" :
                 ("object " + std::to_string(objectID.GetValue())).c_str()));
  
  if(objectID.IsUnknown())
  {
    if(enable)
    {
      SetObjectLights(objectID, kCubeLightsOff);
      for(auto& pair : _objectInfo)
      {
        pair.second.isOnlyGameLayerEnabled = true;
      }
      StopAllAnimsOnLayer(AnimLayerEnum::Engine);
      StopAllAnimsOnLayer(AnimLayerEnum::State);
    }
    else
    {
      StopAllAnimsOnLayer(AnimLayerEnum::User);
      
      for(auto& pair : _objectInfo)
      {
        pair.second.isOnlyGameLayerEnabled = false;
        pair.second.curLayer = AnimLayerEnum::State;
        PickNextAnimForDefaultLayer(pair.first);
      }
    }
    _onlyGameLayerEnabledForAll = enable;
  }
  else
  {
    auto iter = _objectInfo.find(objectID);
    if(iter != _objectInfo.end())
    {
      if(enable)
      {
        SetObjectLights(objectID, kCubeLightsOff);
        StopAllAnimsOnLayer(AnimLayerEnum::Engine, objectID);
        StopAllAnimsOnLayer(AnimLayerEnum::State, objectID);
        iter->second.isOnlyGameLayerEnabled = true;
      }
      else
      {
        StopAllAnimsOnLayer(AnimLayerEnum::User, objectID);
        StopAllAnimsOnLayer(AnimLayerEnum::State, objectID);
        iter->second.curLayer = AnimLayerEnum::State;
        iter->second.isOnlyGameLayerEnabled = false;
        PickNextAnimForDefaultLayer(objectID);
      }
    }
  }
}

void CubeLightComponent::SendTransitionMessage(const ObjectID& objectID, const ObjectLights& values)
{
  if(_sendTransitionMessages)
  {
    ObservableObject* obj = _robot.GetBlockWorld().GetConnectedActiveObjectByID(objectID);
    if(obj == nullptr)
    {
      PRINT_NAMED_WARNING("CubeLightComponent.SendTransitionMessage.NullObject",
                          "Got null object using id %d",
                          objectID.GetValue());
      return;
    }
    
    ExternalInterface::CubeLightsStateTransition msg;
    msg.objectID = objectID;
    msg.factoryID = obj->GetFactoryID();
    msg.objectType = obj->GetType();
    
    LightState lights;
    for(u8 i = 0; i < (u8)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
    {
      msg.lights[i].onColor = ENCODED_COLOR(values.onColors[i]);
      msg.lights[i].offColor = ENCODED_COLOR(values.offColors[i]);
      msg.lights[i].transitionOnFrames = MS_TO_LED_FRAMES(values.transitionOnPeriod_ms[i]);
      msg.lights[i].transitionOffFrames = MS_TO_LED_FRAMES(values.transitionOffPeriod_ms[i]);
      msg.lights[i].onFrames = MS_TO_LED_FRAMES(values.onPeriod_ms[i]);
      msg.lights[i].offFrames = MS_TO_LED_FRAMES(values.offPeriod_ms[i]);
      msg.lights[i].offset = MS_TO_LED_FRAMES(values.offset[i]);
    }
    
    msg.lightRotation_ms = values.rotationPeriod_ms;
    
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
  }
}

void LightPattern::Print() const
{
  for(int i = 0; i < 4; i++)
  {
    PRINT_CH_DEBUG("CubeLightComponent", "LightPattern.Print",
                   "%s LED %u, onColor %u, offColor %u, onFrames %u, "
                   "offFrames %u, transOnFrames %u, transOffFrames %u, offset %u",
                   name.c_str(),
                   i, lights.onColors[i],
                   lights.offColors[i],
                   lights.onPeriod_ms[i],
                   lights.offPeriod_ms[i],
                   lights.transitionOnPeriod_ms[i],
                   lights.transitionOffPeriod_ms[i],
                   lights.offset[i]);
  }
}

const char* CubeLightComponent::LayerToString(const AnimLayerEnum& layer) const
{
  switch(layer)
  {
    case AnimLayerEnum::State:
      return "State";
    case AnimLayerEnum::Engine:
      return "Engine";
    case AnimLayerEnum::User:
      return "User/Game";
    case AnimLayerEnum::Count:
      return "Invalid";
  }
}

// =============Message handlers for cube lights==================

template<>
void CubeLightComponent::HandleMessage(const ObjectConnectionState& msg)
{
  if(msg.connected &&
     (msg.device_type == ActiveObjectType::OBJECT_CUBE1 ||
      msg.device_type == ActiveObjectType::OBJECT_CUBE2 ||
      msg.device_type == ActiveObjectType::OBJECT_CUBE3))
  {
    // Add the objectID to the _objectInfo map and play the wake up animation
    ObjectInfo info = {};
    info.isOnlyGameLayerEnabled = _onlyGameLayerEnabledForAll;
    _objectInfo.emplace(msg.objectID, info);
    
    PlayLightAnim(msg.objectID, CubeAnimationTrigger::WakeUp, AnimLayerEnum::State);
  }
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::PlayCubeAnim& msg)
{
  PlayLightAnim(msg.objectID, msg.trigger, AnimLayerEnum::User);
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::StopCubeAnim& msg)
{
  StopLightAnim(msg.trigger, AnimLayerEnum::User, msg.objectID);
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::FlashCurrentLightsState& msg)
{
  PlayLightAnim(msg.objectID, CubeAnimationTrigger::Flash, AnimLayerEnum::Engine);
}

void CubeLightComponent::CycleThroughAnimTriggers(const ObjectID& objectID)
{
  // Each time the function is called play the next CubeAnimationTrigger
  static int t = 0;
  PlayLightAnim(objectID, static_cast<CubeAnimationTrigger>(t));
  t++;
  if(t >= static_cast<int>(CubeAnimationTrigger::Count))
  {
    t = 0;
  }
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::SetAllActiveObjectLEDs& msg)
{
  const ObjectLights lights {
    .onColors               = msg.onColor,
    .offColors              = msg.offColor,
    .onPeriod_ms            = msg.onPeriod_ms,
    .offPeriod_ms           = msg.offPeriod_ms,
    .transitionOnPeriod_ms  = msg.transitionOnPeriod_ms,
    .transitionOffPeriod_ms = msg.transitionOffPeriod_ms,
    .offset                 = msg.offset,
    .rotationPeriod_ms      = msg.rotationPeriod_ms,
    .makeRelative           = msg.makeRelative,
    .relativePoint          = {msg.relativeToX,msg.relativeToY}
  };
  
  SetObjectLights(msg.objectID, lights);
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::SetActiveObjectLEDs& msg)
{
  SetObjectLights(msg.objectID,
                  msg.whichLEDs,
                  msg.onColor,
                  msg.offColor,
                  msg.onPeriod_ms,
                  msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms,
                  msg.transitionOffPeriod_ms,
                  msg.turnOffUnspecifiedLEDs,
                  msg.makeRelative,
                  {msg.relativeToX, msg.relativeToY},
                  msg.rotationPeriod_ms);
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::EnableCubeLightsStateTransitionMessages& msg)
{
  _sendTransitionMessages = msg.enable;
  
  // Send messages for all objects so that game knows their current lights
  for(const auto& pair : _objectInfo)
  {
    const auto& lightPatterns = pair.second.animationsOnLayer[pair.second.curLayer];
    ObjectLights lights;
    if(lightPatterns.empty())
    {
      lights = kCubeLightsOff;
    }
    else
    {
      lights = lightPatterns.back().curPattern->lights;
    }
    SendTransitionMessage(pair.first, lights);
  }
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::RobotDelocalized& msg)
{
  _robotDelocalized = true;
  
  // Set light states back to connected since Cozmo has lost track of all objects
  // (except for carried objects, because we still know where those are)
  for(auto& pair: _objectInfo)
  {
    const bool isCarryingCube = _robot.IsCarryingObject(pair.first);
    if(!isCarryingCube)
    {
      PlayLightAnim(pair.first, CubeAnimationTrigger::Connected, AnimLayerEnum::State);
    }
  }
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::EnableLightStates& msg)
{
  // When enable == true in the EnableLightStates message all layers should be enabled
  // so we don't want to only enable the game layer hence the negation of msg.enable
  // This is so the EnableLightStates message did not need to change
  EnableGameLayerOnly(msg.objectID, !msg.enable);
}

ActiveObject* CubeLightComponent::GetActiveObjectInAnyFrame(const ObjectID& objectID)
{
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
  filter.AddAllowedID(objectID);
  ActiveObject* activeObject = dynamic_cast<ActiveObject*>(_robot.GetBlockWorld().FindLocatedMatchingObject(filter));
  
  return activeObject;
}

Result CubeLightComponent::SetObjectLights(const ObjectID& objectID, const ObjectLights& values)
{
  ActiveCube* activeObject = nullptr;
  if ( values.makeRelative == MakeRelativeMode::RELATIVE_LED_MODE_OFF )
  {
    // note this could be a checked_cast
    activeObject = dynamic_cast<ActiveCube*>( _robot.GetBlockWorld().GetConnectedActiveObjectByID(objectID) );
  }
  else
  {
    // note this could be a checked_cast
    activeObject = dynamic_cast<ActiveCube*>( _robot.GetBlockWorld().GetLocatedObjectByID(objectID) );
  }
  
  if(activeObject == nullptr)
  {
    PRINT_CH_INFO("CubeLightController",
                  "CubeLightController.SetObjectLights.NullActiveObject",
                  "Null active object pointer");
    return RESULT_FAIL_INVALID_OBJECT;
  }
  
  activeObject->SetLEDs(values.onColors,
                        values.offColors,
                        values.onPeriod_ms,
                        values.offPeriod_ms,
                        values.transitionOnPeriod_ms,
                        values.transitionOffPeriod_ms,
                        values.offset);
  
  ActiveCube* activeCube = dynamic_cast<ActiveCube*>(activeObject);
  if(activeCube == nullptr)
  {
    PRINT_CH_INFO("CubeLightController",
                  "CubeLightController.SetObjectLights.NullActiveCube",
                  "Null active cube pointer");
    return RESULT_FAIL_INVALID_OBJECT;
  }
  
  // NOTE: if make relative mode is "off", this call doesn't do anything:
  activeCube->MakeStateRelativeToXY(values.relativePoint, values.makeRelative);
  
  return SetLights(activeObject, values.rotationPeriod_ms);
}

Result CubeLightComponent::SetObjectLights(const ObjectID& objectID,
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
                                            const u32 rotationPeriod_ms)
{
  ActiveCube* activeCube = nullptr;
  if ( makeRelative == MakeRelativeMode::RELATIVE_LED_MODE_OFF )
  {
    // note this could be a checked_cast
    activeCube = dynamic_cast<ActiveCube*>( _robot.GetBlockWorld().GetConnectedActiveObjectByID(objectID) );
  }
  else
  {
    // note this could be a checked_cast
    activeCube = dynamic_cast<ActiveCube*>( _robot.GetBlockWorld().GetLocatedObjectByID(objectID) );
  }
  
  // trying to do relative mode in an object that is not located in the current origin, will fail, since its
  // pose doesn't mean anything for relative purposes
  if(activeCube == nullptr)
  {
    PRINT_CH_INFO("CubeLightComponent",
                  "CubeLightComponent.SetObjectLights.NullActiveCube",
                  "Null active cube pointer (was it null or not a cube?)");
    return RESULT_FAIL_INVALID_OBJECT;
  }
  
  WhichCubeLEDs rotatedWhichLEDs = whichLEDs;
  // NOTE: if make relative mode is "off", this call doesn't do anything:
  rotatedWhichLEDs = activeCube->MakeWhichLEDsRelativeToXY(whichLEDs, relativeToPoint, makeRelative);
  
  activeCube->SetLEDs(rotatedWhichLEDs, onColor, offColor, onPeriod_ms, offPeriod_ms,
                      transitionOnPeriod_ms, transitionOffPeriod_ms, 0,
                      turnOffUnspecifiedLEDs);
  
  return SetLights(activeCube, rotationPeriod_ms);
}


Result CubeLightComponent::SetLights(const ActiveObject* object, const u32 rotationPeriod_ms)
{
  std::array<Anki::Cozmo::LightState, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS> lights;
  for(int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
  {
    // Apply white balancing and encode colors
    const ActiveObject::LEDstate ledState = object->GetLEDState(i);
    lights[i].onColor  = ENCODED_COLOR(WhiteBalanceColor(ledState.onColor));
    lights[i].offColor = ENCODED_COLOR(WhiteBalanceColor(ledState.offColor));
    lights[i].onFrames  = MS_TO_LED_FRAMES(ledState.onPeriod_ms);
    lights[i].offFrames = MS_TO_LED_FRAMES(ledState.offPeriod_ms);
    lights[i].transitionOnFrames  = MS_TO_LED_FRAMES(ledState.transitionOnPeriod_ms);
    lights[i].transitionOffFrames = MS_TO_LED_FRAMES(ledState.transitionOffPeriod_ms);
    lights[i].offset = MS_TO_LED_FRAMES(ledState.offset);
    
    if(DEBUG_LIGHTS)
    {
      PRINT_CH_DEBUG("CubeLightComponent", "CubeLightComponent.SetLights",
                     "LED %u, onColor 0x%x (0x%x), offColor 0x%x (0x%x), onFrames 0x%x (%ums), "
                     "offFrames 0x%x (%ums), transOnFrames 0x%x (%ums), transOffFrames 0x%x (%ums), offset 0x%x (%ums)",
                     i, lights[i].onColor, ledState.onColor.AsRGBA(),
                     lights[i].offColor, ledState.offColor.AsRGBA(),
                     lights[i].onFrames, ledState.onPeriod_ms,
                     lights[i].offFrames, ledState.offPeriod_ms,
                     lights[i].transitionOnFrames, ledState.transitionOnPeriod_ms,
                     lights[i].transitionOffFrames, ledState.transitionOffPeriod_ms,
                     lights[i].offset, ledState.offset);
    }
  }
  
  if(DEBUG_LIGHTS)
  {
    PRINT_CH_DEBUG("CubeLightComponent", "CubeLightComponent.SetLights",
                   "Setting lights for object %d (activeID %d)",
                   object->GetID().GetValue(), object->GetActiveID());
  }
  
  _robot.SendMessage(RobotInterface::EngineToRobot(SetCubeGamma(object->GetLEDGamma())));
  _robot.SendMessage(RobotInterface::EngineToRobot(CubeID((uint32_t)object->GetActiveID(),
                                                          MS_TO_LED_FRAMES(rotationPeriod_ms))));
  return _robot.SendMessage(RobotInterface::EngineToRobot(CubeLights(lights)));
}

// TEMP (Kevin): WhiteBalancing is eventually to be done in body so just doing something simple here to get us by.
//               Basically if there is any red at all, then blue and green channels are scaled down to 60%.
ColorRGBA CubeLightComponent::WhiteBalanceColor(const ColorRGBA& origColor) const
{
  ColorRGBA color = origColor;
  if(color.GetR() > 0) {
    color.SetB( kWhiteBalanceScale * color.GetB());
    color.SetG( kWhiteBalanceScale * color.GetG());
  }
  return color;
}

}
}
