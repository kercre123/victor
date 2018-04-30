/**
 * File: behaviorProceduralClock.cpp
 *
 * Author: Kevin M. Karol
 * Created: 1/31/18
 *
 * Description: Behavior which displays a procedural clock on the robot's face
 *
 * Copyright: Anki, Inc. 208
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"

#include "cannedAnimLib/proceduralFace/proceduralFace.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/aiComponent/timerUtility.h"
#include "engine/components/animationComponent.h"
#include "engine/components/spriteCacheComponent.h"
#include "engine/faceWorld.h"


#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kClockLayoutKey      = "clockLayout";
const char* kDigitMapKey         = "digitImageMap";
const char* kGetInTriggerKey     = "getInAnimTrigger";
const char* kGetOutTriggerKey    = "getOutAnimTrigger";
const char* kDisplayClockSKey    = "displayClockFor_s";
const char* kStaticElementsKey   = "staticElements";
const char* kShouldTurnToFaceKey = "shouldTurnToFace";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProceduralClock::BehaviorProceduralClock(const Json::Value& config)
: ICozmoBehavior(config)
{
  // load in digit to image map
  {
    int i = 0;
    for(auto& digit: config[kDigitMapKey]){
      _instanceParams.intsToImages[i] = Vision::SpriteNameFromString(digit.asString());
      i++;
    }
    ANKI_VERIFY(i == 10, 
                "BehaviorProceduralClock.Constructor.InvalidDigitMap",
                "Expected exactly 10 images in digit map, found %d",
                i);
  }

  const std::string kDebugStr = "BehaviorProceduralClock.ParsingIssue";

  _instanceParams.getInAnim = AnimationTriggerFromString(JsonTools::ParseString(config, kGetInTriggerKey, kDebugStr));
  _instanceParams.getOutAnim = AnimationTriggerFromString(JsonTools::ParseString(config, kGetOutTriggerKey, kDebugStr));
  _instanceParams.totalTimeDisplayClock = JsonTools::ParseUint8(config, kDisplayClockSKey, kDebugStr);
  JsonTools::GetValueOptional(config, kShouldTurnToFaceKey, _instanceParams.shouldTurnToFace);

  // add all static elements
  if(config.isMember(kStaticElementsKey)){
    for(auto& key: config[kStaticElementsKey].getMemberNames()){
      Vision::SpriteBoxName sbName = Vision::SpriteBoxNameFromString(key);
      const std::string spriteName =  config[kStaticElementsKey][key].asString();
      _instanceParams.staticElements[sbName] = Vision::SpriteNameFromString(spriteName);
    }
  }

  // load in the layout
  if(ANKI_VERIFY(config.isMember(kClockLayoutKey),kDebugStr.c_str(), "Missing layout key")){
    _instanceParams.layout = config[kClockLayoutKey];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kClockLayoutKey,
    kDigitMapKey,
    kGetInTriggerKey,
    kGetOutTriggerKey,
    kDisplayClockSKey,
    kStaticElementsKey,
    kShouldTurnToFaceKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::InitBehavior()
{
  // Setup the composite image
  auto& spriteCacheComp = GetBEI().GetComponentWrapper(BEIComponentID::SpriteCache).GetValue<SpriteCacheComponent>();
  Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
  _instanceParams.compImg = std::make_unique<Vision::CompositeImage>(spriteCacheComp.GetSpriteCache(),
                                                                     faceHueAndSaturation,
                                                                     _instanceParams.layout, 
                                                                     FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);

  if(_instanceParams.getDigitFunctions.size() == 0){
    // TODO: find a way to load this in from data - for now init the clock as one that counts up based on time
    // and allow behaviors to re-set functionality in code only
    auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
    
    std::map<Vision::SpriteBoxName, std::function<int()>> countUpFuncs;
    // Ten Mins Digit
    {
      auto tenMinsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplayMinutes(currentTime_s)/10;
      };
      countUpFuncs.emplace(std::make_pair(Vision::SpriteBoxName::TensLeftOfColon, tenMinsFunc));
    }
    // One Mins Digit
    {
      auto oneMinsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplayMinutes(currentTime_s) % 10;
      };
      countUpFuncs.emplace(std::make_pair(Vision::SpriteBoxName::OnesLeftOfColon, oneMinsFunc));
    }
    // Ten seconds digit
    {
      auto tenSecsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplaySeconds(currentTime_s)/10;
      };
      countUpFuncs.emplace(std::make_pair(Vision::SpriteBoxName::TensRightOfColon, tenSecsFunc));
    }
    // One seconds digit
    {
      auto oneSecsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplaySeconds(currentTime_s) % 10;
      };
      countUpFuncs.emplace(std::make_pair(Vision::SpriteBoxName::OnesRightOfColon, oneSecsFunc));
    }
    
    SetDigitFunctions(std::move(countUpFuncs));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::OnBehaviorActivated() 
{
  _lifetimeParams = LifetimeParams();

  if(_instanceParams.shouldTurnToFace && UpdateTargetFace().IsValid()){
    TransitionToTurnToFace();
  }else{
    TransitionToGetIn();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToTurnToFace()
{
  if(ANKI_VERIFY(_lifetimeParams.targetFaceID.IsValid(),
     "BehaviorProceduralClock.TransitionToTurnToFace.InvalidFace","")){
    DelegateIfInControl(new TurnTowardsFaceAction(_lifetimeParams.targetFaceID, M_PI_F, true),
                        &BehaviorProceduralClock::TransitionToGetIn);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToGetIn()
{
  _lifetimeParams.currentState = BehaviorState::GetIn;
  DelegateIfInControl(new TriggerAnimationAction(_instanceParams.getInAnim), 
                      &BehaviorProceduralClock::TransitionToShowClock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToShowClock()
{  
  if(_instanceParams.showClockCallback != nullptr){
    _instanceParams.showClockCallback();
  }

  _lifetimeParams.currentState = BehaviorState::ShowClock;
  auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
  _lifetimeParams.timeShowClockStarted = timerUtility.GetSystemTime_s();
  // displaying time on face handled in update loop
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToGetOut()
{
  _lifetimeParams.currentState = BehaviorState::GetOut;
  // Cancel the looping show clock body motion animation
  DelegateNow(new TriggerAnimationAction(_instanceParams.getOutAnim), 
                     [this](){ CancelSelf(); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_lifetimeParams.currentState == BehaviorState::ShowClock){
    auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
    const int currentTime_s = timerUtility.GetSystemTime_s();

    if(currentTime_s >= (_lifetimeParams.timeShowClockStarted + _instanceParams.totalTimeDisplayClock)){
      TransitionToGetOut();
    }else if(currentTime_s >= _lifetimeParams.nextUpdateTime_s){
      _lifetimeParams.nextUpdateTime_s = currentTime_s + 1;
      Vision::CompositeImageLayer::ImageMap empty;
      BuildAndDisplayProceduralClock(std::move(empty));      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BehaviorProceduralClock::BuildAndDisplayProceduralClock(Vision::CompositeImageLayer::ImageMap&& imageMap)
{
  auto& cacheComp = GetBEI().GetComponentWrapper(BEIComponentID::SpriteCache).GetValue<SpriteCacheComponent>();
  auto* spriteCache = cacheComp.GetSpriteCache();
  auto* seqContainer = cacheComp.GetSpriteSequenceContainer();
  using Entry = Vision::CompositeImageLayer::SpriteEntry;
  // set static quadrants
  for(auto& entry : _instanceParams.staticElements){
    auto mapEntry = Entry(spriteCache, seqContainer,  entry.second);
    imageMap.emplace(entry.first, std::move(mapEntry));
  }

  // set digits
  bool isLeadingZero = true;
  const std::vector<Vision::SpriteBoxName> orderedDigits = {Vision::SpriteBoxName::TensLeftOfColon,   
                                                            Vision::SpriteBoxName::OnesLeftOfColon, 
                                                            Vision::SpriteBoxName::TensRightOfColon,
                                                            Vision::SpriteBoxName::OnesRightOfColon};
  for(auto& spriteBoxName : orderedDigits){
    const int digitToDisplay = _instanceParams.getDigitFunctions[spriteBoxName]();
    isLeadingZero &= (digitToDisplay == 0);
    if(isLeadingZero){
      auto mapEntry = Entry(spriteCache, seqContainer, Vision::SpriteName::Clock_empty_grid);
      imageMap.emplace(spriteBoxName, std::move(mapEntry));
    }else{
      auto mapEntry = Entry(spriteCache, seqContainer, _instanceParams.intsToImages[digitToDisplay]);
      imageMap.emplace(spriteBoxName, std::move(mapEntry));
    }
  }
  
  
  using namespace Vision;

  CompositeImageLayer* digitLayer = _instanceParams.compImg->GetLayerByName(LayerName::Layer_4);

  if(ANKI_VERIFY(digitLayer != nullptr,"BehaviorProceduralClock.BuildAndDisplayProceduralClock.NoDigitLayer",
                 "Expected digit layout to be specified on Layer_4")){
    // Grab the image by name and
    digitLayer->SetImageMap(std::move(imageMap));
  }

  if(!_lifetimeParams.hasBaseImageBeenSent){
    // Send the base image over the wire
    GetBEI().GetAnimationComponent().DisplayFaceImage(*(_instanceParams.compImg.get()), 
                                                      ANIM_TIME_STEP_MS, 
                                                      0, 
                                                      true);
    _lifetimeParams.hasBaseImageBeenSent = true;
  }else{
    // Just update the composite image
    GetBEI().GetAnimationComponent().UpdateCompositeFaceImageAssets(LayerName::Layer_4, imageMap);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::SetDigitFunctions(std::map<Vision::SpriteBoxName, std::function<int()>>&& functions)
{
  _instanceParams.getDigitFunctions = functions;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID BehaviorProceduralClock::UpdateTargetFace()
{
  const bool considerTrackingOnlyFaces = false;
  std::set< Vision::FaceID_t > faces = GetBEI().GetFaceWorld().GetFaceIDs(considerTrackingOnlyFaces);
  
  std::set<SmartFaceID> smartFaces;
  for(auto& entry : faces){
    smartFaces.insert(GetBEI().GetFaceWorld().GetSmartFaceID(entry));
  }

  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  FaceSelectionComponent::FaceSelectionFactorMap criteriaMap;
  criteriaMap.insert(std::make_pair(FaceSelectionComponent::FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, 1));
  criteriaMap.insert(std::make_pair(FaceSelectionComponent::FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 3));
  _lifetimeParams.targetFaceID = faceSelection.GetBestFaceToUse(criteriaMap, smartFaces);

  return _lifetimeParams.targetFaceID;
}


} // namespace Cozmo
} // namespace Anki
