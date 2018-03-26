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
#include "engine/components/cubes/cubeLightComponent.h"

#include "engine/activeCube.h"
#include "engine/activeObject.h"
#include "engine/activeObjectHelpers.h"
#include "engine/actions/actionInterface.h"
#include "engine/animations/animationContainers/cubeLightAnimationContainer.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/fileUtils/fileUtils.h"

#define DEBUG_TEST_ALL_ANIM_TRIGGERS 0
#define DEBUG_LIGHTS 0

// Scales colors by this factor when applying white balancing
static constexpr f32 kWhiteBalanceScale = 0.6f;

namespace Anki {
namespace Cozmo {

static constexpr int kNumCubeLeds = Util::EnumToUnderlying(CubeConstants::NUM_CUBE_LEDS);
  
static const ObjectLights kCubeLightsOff = {
  .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0,0}},
  .offPeriod_ms           = {{0,0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0,0}},
  .offset                 = {{0,0,0,0}},
  .rotate                 = false,
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
  this->rotate                 == other.rotate &&
  this->makeRelative           == other.makeRelative &&
  this->relativePoint          == other.relativePoint;
}
  

CubeLightComponent::CubeLightComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeLights)
{
  static_assert(AnimLayerEnum::User < AnimLayerEnum::Engine &&
                AnimLayerEnum::Engine < AnimLayerEnum::State,
                "CubeLightComponent.LayersInUnexpectedOrder");
}


void CubeLightComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  _cubeLightAnimations = std::make_unique<CubeLightAnimWrapper>(*(_robot->GetContext()->GetDataLoader()->GetCubeLightAnimations()));
  // Subscribe to messages
  if( _robot->HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _eventHandles);
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
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCubeSleep>();
  }
}


void CubeLightComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  UpdateInternal(true);
}


void CubeLightComponent::UpdateInternal(bool shouldPickNextAnim)
{
  const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  // We are going from delocalized to localized
  bool doRelocalizedUpdate = false;
  if(_robotDelocalized && _robot->IsLocalized())
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
        auto removeAndCallback = [&objectAnim, &objectInfo, &layer]()
        {
          auto callback = objectAnim.callback;
          const u32 actionTag = objectAnim.actionTagCallbackIsFrom;
          
          // Remove this animation from the stack
          // Note: objectAnim is now invalid. It can NOT be reassigned or accessed
          objectInfo.second.animationsOnLayer[layer].pop_back();
          
          // Call the callback since the animation is complete
          if(callback)
          {
            // Only call the callback if it isn't from an action or
            // it is from an action and that action's tag is still in use
            // If the action gets destroyed and the tag is no longer in use the callback would be
            // invalid
            if(actionTag == 0 ||
               IActionRunner::IsTagInUse(actionTag))
            {
              callback();
            }
          }
        };
        
        // Note: objectAnim invalidated by this call
        removeAndCallback();
      
        // Keep removing animations and calling callbacks while there are animations to stop
        // on this layer
        while(!objectInfo.second.animationsOnLayer[layer].empty() &&
              objectInfo.second.animationsOnLayer[layer].back().stopNow)
        {
          removeAndCallback();
        }
      
        // If there are no more animations being played on this layer
        if(objectInfo.second.animationsOnLayer[layer].empty())
        {
          // If all layers are enabled go back to default layer
          if(!objectInfo.second.isOnlyGameLayerEnabled)
          {
            const AnimLayerEnum prevLayer = layer;

            layer = AnimLayerEnum::State;
            
            PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.Update.NoAnimsLeftOnLayer",
                          "No more animations left on layer %s for object %u going to %s layer",
                          LayerToString(prevLayer),
                          objectInfo.first.GetValue(),
                          LayerToString(layer));
            
            // In some cases we don't want to PickNextAnimForDefaultLayer in order to prevent
            // lights from flickering when stopping and playing anims
            if(shouldPickNextAnim)
            {
              PickNextAnimForDefaultLayer(objectInfo.first);
            }
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
        if(objectAnim.curPattern->duration_ms > 0 ||
           objectAnim.durationModifier_ms != 0)
        {
          objectAnim.timeCurPatternEnds = curTime +
                                          objectAnim.curPattern->duration_ms +
                                          objectAnim.durationModifier_ms;
          
          DEV_ASSERT(objectAnim.timeCurPatternEnds > 0,
                     "CubeLightComponent.Update.TimeCurPatternEndLessThanZero");
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
                                          const ObjectLights& modifier,
                                          const s32 durationModifier_ms)
{
  return PlayLightAnim(objectID, animTrigger, AnimLayerEnum::Engine, callback, hasModifier, modifier, durationModifier_ms);
}

bool CubeLightComponent::PlayLightAnim(const ObjectID& objectID,
                                          const CubeAnimationTrigger& animTrigger,
                                          const AnimLayerEnum& layer,
                                          AnimCompletedCallback callback,
                                          bool hasModifier,
                                          const ObjectLights& modifier,
                                          const s32 durationModifier_ms)
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
    const std::string& animName = _robot->GetContext()->GetDataLoader()->GetCubeAnimationForTrigger(animTrigger);
    auto* anim = _cubeLightAnimations->_container.GetAnimation(animName);
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
    
    // Set this animation's duration modifier
    animation.durationModifier_ms = durationModifier_ms;
    
    animation.timeCurPatternEnds = 0;
    
    // If the first pattern in the animation has a non zero duration then update the time the pattern
    // should end
    if(animation.curPattern->duration_ms > 0 ||
       animation.durationModifier_ms != 0)
    {
      animation.timeCurPatternEnds = curTime +
                                     animation.curPattern->duration_ms +
                                     animation.durationModifier_ms;
      
      DEV_ASSERT(animation.timeCurPatternEnds > 0,
                 "CubeLightComponent.PlayLightAnim.TimeCurPatternEndLessThanZero");
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
  UpdateInternal(true);
}

bool CubeLightComponent::StopLightAnimAndResumePrevious(const CubeAnimationTrigger& animTrigger,
                                                        const ObjectID& objectID)
{
  return StopLightAnim(animTrigger, AnimLayerEnum::Engine, objectID);
}

bool CubeLightComponent::StopLightAnim(const CubeAnimationTrigger& animTrigger,
                                       const AnimLayerEnum& layer,
                                       const ObjectID& objectID,
                                       bool shouldPickNextAnim)
{  
  PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.StopLightAnim",
                "Stopping %s on object %d on layer %s.",
                EnumToString(animTrigger),
                objectID.GetValue(),
                LayerToString(layer));

  auto helper = [this](const CubeAnimationTrigger& animTrigger,
                       const ObjectID& objectID,
                       const AnimLayerEnum& layer,
                       bool& foundAnimWithTrigger) {
    auto& iter = _objectInfo[objectID];
    for(auto& anim : iter.animationsOnLayer[layer])
    {
      if(anim.trigger == animTrigger)
      {
        anim.stopNow = true;
        foundAnimWithTrigger = true;
      }
    }
  };

  bool foundAnimWithTrigger = false;
  if(objectID.IsUnknown())
  {
    for(auto& iter : _objectInfo)
    {
      helper(animTrigger, iter.first, layer, foundAnimWithTrigger);
    }
  }
  else
  {
    const auto& iter = _objectInfo.find(objectID);
    if(iter != _objectInfo.end())
    {
      helper(animTrigger, objectID, layer, foundAnimWithTrigger);
    }
  }
  
  // Manually update so the anims are immediately stopped
  UpdateInternal(shouldPickNextAnim);

  std::stringstream ss;
  for( const auto& objectInfoPair : _objectInfo ){
    if( objectID.IsSet() && objectID != objectInfoPair.first ) {
      continue;
    }
    
    ss << "object=" << objectInfoPair.first.GetValue() << " layer=" << LayerToString(layer)
       << " currLayer=" << LayerToString(objectInfoPair.second.curLayer) << " [";
    
    for(auto& anim : objectInfoPair.second.animationsOnLayer[layer]) {
      ss << anim.name << ":" << CubeAnimationTriggerToString( anim.trigger ) << "@" << anim.timeCurPatternEnds;
      if( anim.stopNow ) {
        ss << ":STOP_NOW";
      }
      ss << " ";
    }
    ss << "]";
  }

  PRINT_CH_DEBUG("CubeLightComponent", "CubeLightComponent.StopLignAnim.Result",
                 "%s anim '%s'. Current state: %s",
                 foundAnimWithTrigger ? "found" : "did not find",
                 EnumToString(animTrigger),
                 ss.str().c_str());
    
  return foundAnimWithTrigger;
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
    for(int i = 0; i < kNumCubeLeds; ++i)
    {
      pattern.lights.onColors[i] |= modifier.onColors[i];
      pattern.lights.offColors[i] |= modifier.offColors[i];
      pattern.lights.onPeriod_ms[i] += modifier.onPeriod_ms[i];
      pattern.lights.offPeriod_ms[i] += modifier.offPeriod_ms[i];
      pattern.lights.transitionOnPeriod_ms[i] += modifier.transitionOnPeriod_ms[i];
      pattern.lights.transitionOffPeriod_ms[i] += modifier.transitionOffPeriod_ms[i];
      pattern.lights.offset[i] += modifier.offset[i];
      pattern.lights.rotate |= modifier.rotate;
    }
    pattern.lights.makeRelative = modifier.makeRelative;
    pattern.lights.relativePoint = modifier.relativePoint;
  }
}

bool CubeLightComponent::BlendAnimWithCurLights(const ObjectID& objectID,
                                                const LightAnim& anim,
                                                LightAnim& blendedAnim)
{
  ActiveObject* activeObject = _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID);
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
  // with whatever the corresponding color that is currently being displayed on the object
  for(auto& pattern : blendedAnim)
  {
    for(int i = 0; i < kNumCubeLeds; ++i)
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

bool CubeLightComponent::StopAndPlayLightAnim(const ObjectID& objectID,
                                              const CubeAnimationTrigger& animTriggerToStop,
                                              const CubeAnimationTrigger& animTriggerToPlay,
                                              AnimCompletedCallback callback,
                                              bool hasModifier,
                                              const ObjectLights& modifier)
{
  DEV_ASSERT(!_objectInfo[objectID].animationsOnLayer[AnimLayerEnum::Engine].empty(),
             "CubeLightComponent.StopAndPlayLightAnim.NoAnimsCurrentlyOnEngineLayer");
  // Stop the anim and prevent the update call in StopLightAnim from picking a next default anim
  // This will prevent the lights from briefly flickering between the calls to stop and play
  StopLightAnim(animTriggerToStop, AnimLayerEnum::Engine, objectID, false);
  return PlayLightAnim(objectID, animTriggerToPlay, callback, hasModifier, modifier);
}

void CubeLightComponent::PickNextAnimForDefaultLayer(const ObjectID& objectID)
{
  if(DEBUG_TEST_ALL_ANIM_TRIGGERS)
  {
    CycleThroughAnimTriggers(objectID);
    return;
  }
  
  // If CubeSleep is enabled then play the sleep animation
  if(_enableCubeSleep)
  {
    PlayLightAnim(objectID,
                  (_skipSleepAnim ? CubeAnimationTrigger::SleepNoFade : CubeAnimationTrigger::Sleep),
                  AnimLayerEnum::State);
    return;
  }
  
  CubeAnimationTrigger anim = CubeAnimationTrigger::Connected;
  
  const ObservableObject* object = _robot->GetBlockWorld().GetLocatedObjectByID(objectID);
  if(object != nullptr)
  {
    if(object->IsPoseStateKnown())
    {
      anim = CubeAnimationTrigger::Visible;
    }
  }
  
  if(_robot->GetCarryingComponent().GetCarryingObject() == objectID)
  {
    anim = CubeAnimationTrigger::Carrying;
  }
  
  PlayLightAnim(objectID, anim, AnimLayerEnum::State);
}

u32 CubeLightComponent::GetAnimDuration(const CubeAnimationTrigger& trigger)
{
  const std::string& animName = _robot->GetContext()->GetDataLoader()->GetCubeAnimationForTrigger(trigger);
  const auto* anim = _cubeLightAnimations->_container.GetAnimation(animName);
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


void CubeLightComponent::OnActiveObjectPoseStateChanged(const ObjectID& objectID,
                                                        const PoseState oldPoseState,
                                                        const PoseState newPoseState)
{
  if(oldPoseState == newPoseState)
  {
    return;
  }
  
  if(newPoseState != PoseState::Known) // TODO Change to use function
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
    if( _onlyGameLayerEnabledForAll != enable ) {
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
          if( pair.second.animationsOnLayer[AnimLayerEnum::Engine].empty() ) {
            pair.second.curLayer = AnimLayerEnum::State;
          }
          else {
            pair.second.curLayer = AnimLayerEnum::Engine;
          }
          PickNextAnimForDefaultLayer(pair.first);
        }
      }
      _onlyGameLayerEnabledForAll = enable;
    }
  }
  else
  {
    auto iter = _objectInfo.find(objectID);
    if(iter != _objectInfo.end())
    {
      if( enable != iter->second.isOnlyGameLayerEnabled ) {
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
          if( iter->second.animationsOnLayer[AnimLayerEnum::Engine].empty() ) {
            iter->second.curLayer = AnimLayerEnum::State;
          }
          else {
            iter->second.curLayer = AnimLayerEnum::Engine;
          }
          iter->second.isOnlyGameLayerEnabled = false;
          PickNextAnimForDefaultLayer(objectID);
        }
      }
    }
  }
}

void CubeLightComponent::SendTransitionMessage(const ObjectID& objectID, const ObjectLights& values)
{
  // Convert MS to LED FRAMES
  #define MS_TO_LED_FRAMES(ms)  ((ms) == std::numeric_limits<u32>::max() ?      \
                                  std::numeric_limits<u8>::max() :              \
                                  Util::numeric_cast<u8>(((ms)+CUBE_LED_FRAME_LENGTH_MS-1)/CUBE_LED_FRAME_LENGTH_MS))
  
  if(_sendTransitionMessages)
  {
    ObservableObject* obj = _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID);
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
    for(u8 i = 0; i < kNumCubeLeds; ++i)
    {
      msg.lights[i].onColor = values.onColors[i];
      msg.lights[i].offColor = values.offColors[i];
      msg.lights[i].transitionOnFrames = MS_TO_LED_FRAMES(values.transitionOnPeriod_ms[i]);
      msg.lights[i].transitionOffFrames = MS_TO_LED_FRAMES(values.transitionOffPeriod_ms[i]);
      msg.lights[i].onFrames = MS_TO_LED_FRAMES(values.onPeriod_ms[i]);
      msg.lights[i].offFrames = MS_TO_LED_FRAMES(values.offPeriod_ms[i]);
      msg.lights[i].offset = MS_TO_LED_FRAMES(values.offset[i]);
    }
    
    msg.lightRotation = values.rotate;
    
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
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
void CubeLightComponent::HandleMessage(const ExternalInterface::ObjectConnectionState& msg)
{
  if(msg.connected && IsValidLightCube(msg.object_type, false))
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
  ObjectID objectID = msg.objectID;
  if(objectID < 0) {
    objectID = _robot->GetBlockWorld().GetSelectedObject();
  }
  
  PlayLightAnim(objectID, msg.trigger, AnimLayerEnum::User);
}

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::StopCubeAnim& msg)
{
  ObjectID objectID = msg.objectID;
  if(objectID < 0) {
    objectID = _robot->GetBlockWorld().GetSelectedObject();
  }
  
  StopLightAnim(msg.trigger, AnimLayerEnum::User, objectID);
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
    .rotate                 = msg.rotate,
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
                  msg.rotate);
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
    const bool isCarryingCube = _robot->GetCarryingComponent().IsCarryingObject(pair.first);
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

template<>
void CubeLightComponent::HandleMessage(const ExternalInterface::EnableCubeSleep& msg)
{
  PRINT_CH_INFO("CubeLightComponent", "CubeLightComponent.EnableCubeSleep",
                "%s cube sleep", (msg.enable ? "Enabling" : "Disabling"));
  
  _enableCubeSleep = msg.enable;
  _skipSleepAnim = msg.skipAnimation;
  
  static const ObjectID kAllObjects{};
  
  // If we are disabling cube sleep then stop the sleep animation
  if(!_enableCubeSleep)
  {
    StopLightAnim(CubeAnimationTrigger::Sleep, AnimLayerEnum::State, kAllObjects);
    StopLightAnim(CubeAnimationTrigger::SleepNoFade, AnimLayerEnum::State, kAllObjects);
  }
  
  // Force game layer to be disabled, this will stop any game layer animations
  // and force a default layer anim to be played. In the case where _enableCubeSleep is
  // true the sleep anim will be picked otherwise an appropriate anim will be picked
  EnableGameLayerOnly(kAllObjects, false);
}

Result CubeLightComponent::SetObjectLights(const ObjectID& objectID, const ObjectLights& values)
{
  ActiveCube* activeObject = nullptr;
  if ( values.makeRelative == MakeRelativeMode::RELATIVE_LED_MODE_OFF )
  {
    // note this could be a checked_cast
    activeObject = dynamic_cast<ActiveCube*>( _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID) );
  }
  else
  {
    // note this could be a checked_cast
    activeObject = dynamic_cast<ActiveCube*>( _robot->GetBlockWorld().GetLocatedObjectByID(objectID) );
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
  
  return SetLights(activeObject, values.rotate);
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
                                            const bool rotate)
{
  ActiveCube* activeCube = nullptr;
  if ( makeRelative == MakeRelativeMode::RELATIVE_LED_MODE_OFF )
  {
    // note this could be a checked_cast
    activeCube = dynamic_cast<ActiveCube*>( _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID) );
  }
  else
  {
    // note this could be a checked_cast
    activeCube = dynamic_cast<ActiveCube*>( _robot->GetBlockWorld().GetLocatedObjectByID(objectID) );
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
  
  return SetLights(activeCube, rotate);
}

  
bool CubeLightComponent::CanEngineSetLightsOnCube(const ObjectID& objectID)
{
  const auto& info = _objectInfo.find(objectID);
  if(info != _objectInfo.end()){
    return !info->second.isOnlyGameLayerEnabled;
  }
  return false;
}


Result CubeLightComponent::SetLights(const ActiveObject* object, const bool rotate)
{
  CubeLights cubeLights;
  cubeLights.rotate = rotate;
  for(int i = 0; i < kNumCubeLeds; ++i)
  {
    // Apply white balancing and encode colors
    const ActiveObject::LEDstate ledState = object->GetLEDState(i);
    cubeLights.lights[i].onColor  = WhiteBalanceColor(ledState.onColor);
    cubeLights.lights[i].offColor = WhiteBalanceColor(ledState.offColor);
    cubeLights.lights[i].onFrames  = MS_TO_LED_FRAMES(ledState.onPeriod_ms);
    cubeLights.lights[i].offFrames = MS_TO_LED_FRAMES(ledState.offPeriod_ms);
    cubeLights.lights[i].transitionOnFrames  = MS_TO_LED_FRAMES(ledState.transitionOnPeriod_ms);
    cubeLights.lights[i].transitionOffFrames = MS_TO_LED_FRAMES(ledState.transitionOffPeriod_ms);
    cubeLights.lights[i].offset = MS_TO_LED_FRAMES(ledState.offset);
    
    if(DEBUG_LIGHTS)
    {
      PRINT_CH_DEBUG("CubeLightComponent", "CubeLightComponent.SetLights",
                     "LED %u, onColor 0x%x (0x%x), offColor 0x%x (0x%x), onFrames 0x%x (%ums), "
                     "offFrames 0x%x (%ums), transOnFrames 0x%x (%ums), transOffFrames 0x%x (%ums), offset 0x%x (%ums)",
                     i, cubeLights.lights[i].onColor, ledState.onColor.AsRGBA(),
                     cubeLights.lights[i].offColor, ledState.offColor.AsRGBA(),
                     cubeLights.lights[i].onFrames, ledState.onPeriod_ms,
                     cubeLights.lights[i].offFrames, ledState.offPeriod_ms,
                     cubeLights.lights[i].transitionOnFrames, ledState.transitionOnPeriod_ms,
                     cubeLights.lights[i].transitionOffFrames, ledState.transitionOffPeriod_ms,
                     cubeLights.lights[i].offset, ledState.offset);
    }
  }
  
  if(DEBUG_LIGHTS)
  {
    PRINT_CH_DEBUG("CubeLightComponent", "CubeLightComponent.SetLights",
                   "Setting lights for object %d (activeID %d)",
                   object->GetID().GetValue(), object->GetActiveID());
  }
  
  // Only send gamma when it changes since it is global across all cubes
  // (Currently never changes)
  const u32 gamma = object->GetLEDGamma();
  if(gamma != _prevGamma)
  {
    // TODO: do we need this?
    //_robot->SendMessage(RobotInterface::EngineToRobot(SetCubeGamma(gamma)));
    _prevGamma = gamma;
  }

  const bool result = _robot->GetCubeCommsComponent().SendCubeLights((uint32_t)object->GetActiveID(),
                                                                     cubeLights);

  return result ? RESULT_OK : RESULT_FAIL;
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
