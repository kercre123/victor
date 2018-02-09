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

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/components/animationComponent.h"
#include "engine/aiComponent/templatedImageCache.h"
#include "engine/aiComponent/timerUtility.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kClockTemplateKey  = "clockTemplate";
const char* kDigitMapKey       = "digitImageMap";
const char* kGetInTriggerKey   = "getInAnimTrigger";
const char* kGetOutTriggerKey  = "getOutAnimTrigger";
const char* kDisplayClockSKey  = "displayClockFor_s";
const char* kStaticElementsKey = "staticElements";
const char* kTimerElementsKey = "timerElementsMap";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProceduralClock::BehaviorProceduralClock(const Json::Value& config)
: ICozmoBehavior(config)
{
  // load in digit to image map
  {
    int i = 0;
    for(auto& digit: config[kDigitMapKey]){
      _instanceParams.intsToImages[i] = digit.asString();
      i++;
    }
    ANKI_VERIFY(i == 10, 
                "BehaviorProceduralClock.Constructor.InvalidDigitMap",
                "Expected exactly 10 images in digit map, found %d",
                i);
  }

  {
    for(auto& key : config[kTimerElementsKey].getMemberNames()){
      _instanceParams.digitToTemplateMap[StringToDigitID(key)] = config[kTimerElementsKey][key].asString();
    }
    ANKI_VERIFY(_instanceParams.digitToTemplateMap.size() == Util::EnumToUnderlying(DigitID::Count),
                "BehaviorProceduralClock.Constructor.MissingElements",
                "Expected digit map to have %zu elements, but it has %d",
                _instanceParams.digitToTemplateMap.size(),
                Util::EnumToUnderlying(DigitID::Count));
  }

  const std::string kDebugStr = "BehaviorProceduralClock.ParsingIssue";

  _instanceParams.templateJSON = config[kClockTemplateKey];
  _instanceParams.getInAnim = AnimationTriggerFromString(JsonTools::ParseString(config, kGetInTriggerKey, kDebugStr));
  _instanceParams.getOutAnim = AnimationTriggerFromString(JsonTools::ParseString(config, kGetOutTriggerKey, kDebugStr));
  _instanceParams.totalTimeDisplayClock = JsonTools::ParseUint8(config, kDisplayClockSKey, kDebugStr);


  // add all static elements
  if(config.isMember(kStaticElementsKey)){
    for(auto& key: config[kStaticElementsKey].getMemberNames()){
      _instanceParams.staticElements[key] = config[kStaticElementsKey][key].asString();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::InitBehavior()
{
  if(_instanceParams.getDigitFunctions.size() == 0){
    // TODO: find a way to load this in from data - for now init the clock as one that counts up based on time
    // and allow behaviors to re-set functionality in code only
    auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
    
    std::map<DigitID, std::function<int()>> countUpFuncs;
    // Ten Mins Digit
    {
      auto tenMinsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplayMinutes(currentTime_s)/10;
      };
      countUpFuncs.emplace(std::make_pair(DigitID::DigitOne, tenMinsFunc));
    }
    // One Mins Digit
    {
      auto oneMinsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplayMinutes(currentTime_s) % 10;
      };
      countUpFuncs.emplace(std::make_pair(DigitID::DigitTwo, oneMinsFunc));
    }
    // Ten seconds digit
    {
      auto tenSecsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplaySeconds(currentTime_s)/10;
      };
      countUpFuncs.emplace(std::make_pair(DigitID::DigitThree, tenSecsFunc));
    }
    // One seconds digit
    {
      auto oneSecsFunc = [&timerUtility](){
        const int currentTime_s = timerUtility.GetSystemTime_s();
        return TimerHandle::SecondsToDisplaySeconds(currentTime_s) % 10;
      };
      countUpFuncs.emplace(std::make_pair(DigitID::DigitFour, oneSecsFunc));
    }
    
    SetDigitFunctions(std::move(countUpFuncs));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::OnBehaviorActivated() 
{
  _lifetimeParams = LifetimeParams();
  TransitionToGetIn();
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
  _lifetimeParams.currentState = BehaviorState::ShowClock;
  
  auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
  _lifetimeParams.timeShowClockStarted = timerUtility.GetSystemTime_s();
  // displaying time on face handled in update loop
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::TransitionToGetOut()
{
  _lifetimeParams.currentState = BehaviorState::GetOut;
  DelegateIfInControl(new TriggerAnimationAction(_instanceParams.getOutAnim), 
                     [this](){ CancelSelf(); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if(_lifetimeParams.currentState == BehaviorState::ShowClock){
    auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
    const int currentTime_s = timerUtility.GetSystemTime_s();

    if(currentTime_s >= (_lifetimeParams.timeShowClockStarted + _instanceParams.totalTimeDisplayClock)){
      TransitionToGetOut();
    }else if(currentTime_s >= _lifetimeParams.nextUpdateTime_s){
      _lifetimeParams.nextUpdateTime_s = currentTime_s + 1;
      std::map<std::string, std::string> quadrantMap;
      BuildAndDisplayTimer(quadrantMap);      
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
auto BehaviorProceduralClock::StringToDigitID(const std::string& str) -> DigitID
{
  if(str == "DigitOne"){ return DigitID::DigitOne;}
  else if(str == "DigitTwo"){ return DigitID::DigitTwo;}
  else if(str == "DigitThree"){ return DigitID::DigitThree;}
  else if(str == "DigitFour"){ return DigitID::DigitFour;}
  else { 
    PRINT_NAMED_WARNING("BehaviorProceduralClock.StringToDigitID.NoMatchFound",
                        "Digit %s not found in enum", str.c_str());
    return DigitID::Count;
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::BuildAndDisplayTimer(std::map<std::string, std::string>& quadrantMap)
{
  // set static quadrants
  for(auto& entry : _instanceParams.staticElements){
    quadrantMap[entry.first] = entry.second;
  }

  // set digits
  for(int enumIdx = 0; enumIdx < Util::EnumToUnderlying(DigitID::Count); enumIdx++){
    const DigitID enumVal = DigitID(enumIdx);
    const int digitToDisplay = _instanceParams.getDigitFunctions[enumVal]();
    quadrantMap[_instanceParams.digitToTemplateMap[enumVal]] = _instanceParams.intsToImages[digitToDisplay];
  }

  // build the full image
  auto& imageCache = GetBEI().GetAIComponent().GetComponent<TemplatedImageCache>(AIComponentID::TemplatedImageCache);
  const auto& image = imageCache.BuildImage(GetDebugLabel(), 
                                            FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT, 
                                            _instanceParams.templateJSON, quadrantMap);
  // draw it to the face
  GetBEI().GetAnimationComponent().DisplayFaceImage(image, 1000.0f, true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProceduralClock::SetDigitFunctions(std::map<DigitID, std::function<int()>>&& functions)
{
  _instanceParams.getDigitFunctions = functions;
  if(ANKI_DEV_CHEATS){
    std::set<DigitID> digits;
    for(auto& entry: _instanceParams.getDigitFunctions){
      digits.insert(entry.first);
    }
    ANKI_VERIFY(Util::EnumToUnderlying(DigitID::Count) == digits.size(),
                "BehaviorProceduralClock.SetDigitFunctions.ImproperNumberOfFunctions",
                "Expected %d functions, received %zu",
                Util::EnumToUnderlying(DigitID::Count), digits.size());
  }
}


} // namespace Cozmo
} // namespace Anki
