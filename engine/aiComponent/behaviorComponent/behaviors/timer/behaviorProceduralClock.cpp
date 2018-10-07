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
#include "engine/components/dataAccessorComponent.h"
#include "engine/faceWorld.h"


#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {

namespace{
const char* kClockLayoutKey         = "clockLayout";
const char* kDigitMapKey            = "digitImageMap";
const char* kGetInTriggerKey        = "getInAnimTrigger";
const char* kGetOutTriggerKey       = "getOutAnimTrigger";
const char* kDisplayClockSKey       = "displayClockFor_s";
const char* kStaticElementsKey      = "staticElements";
const char* kShouldTurnToFaceKey    = "shouldTurnToFace";
const char* kShouldPlayAudioKey     = "shouldPlayAudioOnClockUpdates";

}
const std::vector<Vision::SpriteBoxName> BehaviorProceduralClock::DigitDisplayList = 
{
  Vision::SpriteBoxName::TensLeftOfColon,
  Vision::SpriteBoxName::OnesLeftOfColon,
  Vision::SpriteBoxName::TensRightOfColon,
  Vision::SpriteBoxName::OnesRightOfColon
};

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
  _instanceParams.totalTimeDisplayClock_sec = JsonTools::ParseUint8(config, kDisplayClockSKey, kDebugStr);
  JsonTools::GetValueOptional(config, kShouldTurnToFaceKey, _instanceParams.shouldTurnToFace);
  JsonTools::GetValueOptional(config, kShouldPlayAudioKey, _instanceParams.shouldPlayAudioOnClockUpdates);

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
    kShouldTurnToFaceKey,
    kShouldPlayAudioKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  GetBehaviorJsonKeysInternal(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::InitBehavior()
{
  auto& accessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  auto* spriteCache = accessorComp.GetSpriteCache();
  auto* seqContainer = accessorComp.GetSpriteSequenceContainer();
  using Entry = Vision::CompositeImageLayer::SpriteEntry;
  // set static quadrants
  for(auto& entry : _instanceParams.staticElements){
    auto mapEntry = Entry(spriteCache, seqContainer,  entry.second);
    _instanceParams.staticImageMap.emplace(entry.first, std::move(mapEntry));
  }

  // Setup the composite image
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
  _instanceParams.compImg = std::make_unique<Vision::CompositeImage>(dataAccessorComp.GetSpriteCache(),
                                                                     faceHueAndSaturation,
                                                                     _instanceParams.layout, 
                                                                     FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);

  auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
  if(_instanceParams.getDigitFunction == nullptr){
    // TODO: find a way to load this in from data - for now init the clock as one that counts up based on time
    // and allow behaviors to re-set functionality in code only
    
    GetDigitsFunction countUpFunction = [&timerUtility](const int offset){
      std::map<Vision::SpriteBoxName, int> outMap;
      const int currentTime_s = timerUtility.GetSteadyTime_s() + offset;
      // Ten Mins Digit
      {          
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensLeftOfColon, 
                                      TimerHandle::SecondsToDisplayMinutes(currentTime_s)/10));
      }
      // One Mins Digit
      {
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesLeftOfColon, 
                                            TimerHandle::SecondsToDisplayMinutes(currentTime_s) % 10));
      }
      // Ten seconds digit
      {
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensRightOfColon, 
                                      TimerHandle::SecondsToDisplaySeconds(currentTime_s)/10));
      }
      // One seconds digit
      {
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesRightOfColon, 
                       TimerHandle::SecondsToDisplaySeconds(currentTime_s) % 10));
      }
      return outMap;
    };

    SetGetDigitFunction(std::move(countUpFunction));
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
  _lifetimeParams.timeShowClockStarted = timerUtility.GetSteadyTime_s();

  TransitionToShowClockInternal();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToShowClockInternal()
{
  for(int i = 0; i < _instanceParams.totalTimeDisplayClock_sec; i++){
    BuildAndDisplayProceduralClock(i, i*1000);   
  }
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
    const int currentTime_s = timerUtility.GetSteadyTime_s();

    if(currentTime_s >= (_lifetimeParams.timeShowClockStarted + _instanceParams.totalTimeDisplayClock_sec)){
      TransitionToGetOut();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::BuildAndDisplayProceduralClock(const int clockOffset_s, const int displayOffset_ms)
{
  // Animation streamer can only apply images/sounds on animation keyframes so ensure displayOffset is aligned to a frame
  auto displayOffset_aligned_ms = displayOffset_ms;
  displayOffset_aligned_ms -= (displayOffset_aligned_ms % ANIM_TIME_STEP_MS);

  auto& accessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  auto* spriteCache = accessorComp.GetSpriteCache();
  auto* seqContainer = accessorComp.GetSpriteSequenceContainer();
  using Entry = Vision::CompositeImageLayer::SpriteEntry;

  // set digits
  bool isLeadingZero = ShouldDimLeadingZeros();

  const std::map<Vision::SpriteBoxName, int> digitMap = _instanceParams.getDigitFunction(clockOffset_s);

  Vision::CompositeImageLayer::ImageMap imageMap = _instanceParams.staticImageMap;
  for(auto& pair : digitMap){
    isLeadingZero &= (pair.second == 0);
    if(isLeadingZero){
      auto mapEntry = Entry(spriteCache, seqContainer, Vision::SpriteName::Clock_Empty_Grid);
      imageMap.emplace(pair.first, std::move(mapEntry));
    }else{
      auto mapEntry = Entry(spriteCache, seqContainer, _instanceParams.intsToImages[pair.second]);
      imageMap.emplace(pair.first, std::move(mapEntry));
    }
  }
  
  
  using namespace Vision;

  CompositeImageLayer* digitLayer = _instanceParams.compImg->GetLayerByName(LayerName::Clock_Display);

  if(ANKI_VERIFY(digitLayer != nullptr,
                 "BehaviorProceduralClock.BuildAndDisplayProceduralClock.NoDigitLayer",
                 "Expected digit layout to be specified on Clock_Display")){
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
    Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
    CompositeImage compImg(accessorComp.GetSpriteCache(), faceHueAndSaturation);
    auto intentionalCopy = *digitLayer;
    compImg.AddLayer(std::move(intentionalCopy));
    // Just update the numbers in the image
    GetBEI().GetAnimationComponent().UpdateCompositeImage(compImg, displayOffset_aligned_ms);
  }

  if(_instanceParams.shouldPlayAudioOnClockUpdates){
    // Have the animation process send ticks at the appropriate time stamp 
    AudioEngine::Multiplexer::PostAudioEvent audioMessage;
    audioMessage.gameObject = Anki::AudioMetaData::GameObjectType::Animation;
    audioMessage.audioEvent = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Timer_Countdown;

    RobotInterface::EngineToRobot wrapper(std::move(audioMessage));
    GetBEI().GetAnimationComponent().AlterStreamingAnimationAtTime(std::move(wrapper), displayOffset_aligned_ms);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::SetGetDigitFunction(GetDigitsFunction&& function)
{
  _instanceParams.getDigitFunction = function;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID BehaviorProceduralClock::UpdateTargetFace()
{
  auto smartFaces = GetBEI().GetFaceWorld().GetSmartFaceIDs(0);

  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  FaceSelectionComponent::FaceSelectionFactorMap criteriaMap;
  criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, 1));
  criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 3));
  _lifetimeParams.targetFaceID = faceSelection.GetBestFaceToUse(criteriaMap, smartFaces);

  return _lifetimeParams.targetFaceID;
}


} // namespace Vector
} // namespace Anki
