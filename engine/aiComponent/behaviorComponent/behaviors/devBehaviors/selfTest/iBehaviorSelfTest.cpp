/**
 * File: iBehaviorSelfTest.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Base class for all self test related behaviors
 *              All SelfTest behaviors should be written to be able to continue even after
 *              receiving unexpected things (basically conditional branches should only contain code
 *              that calls SET_RESULT)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTest.h"
#include "engine/components/animationComponent.h"
#include "engine/robot.h"

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/vision/engine/image_impl.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "opencv2/highgui.hpp"

namespace Anki {
namespace Vector {

namespace {

// Set of messages that will immediately cause any selftest behaivor to end
// Unless they have explicitly subscribed to the message themselves and are expecting to see it
static const std::set<ExternalInterface::MessageEngineToGameTag> kFailureTags = {
  ExternalInterface::MessageEngineToGameTag::CliffEvent,
  ExternalInterface::MessageEngineToGameTag::RobotStopped,
  ExternalInterface::MessageEngineToGameTag::ChargerEvent,
  ExternalInterface::MessageEngineToGameTag::MotorCalibration,
  ExternalInterface::MessageEngineToGameTag::MotorAutoEnabled,
  ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged,
  ExternalInterface::MessageEngineToGameTag::UnexpectedMovement
};

// Static (shared) across all selftest behaviors
// Maps a behavior name/idStr to a vector of results it has failed/completed with
static std::map<std::string, std::vector<SelfTestResultCode>> results;

// Hacky way of giving any selftest behavior easy access to check something that an individual behavior
// sets. This way the behavior that is checking doesn't have to dynamic cast to the individual behavior.
// Only behaviorSelfTestSoundCheck should be setting this bool
static bool receivedFFTResult = false;
}
 
IBehaviorSelfTest::IBehaviorSelfTest(const Json::Value& config, SelfTestResultCode timeoutCode)
: ICozmoBehavior(config)
, _timeoutCode(timeoutCode)
{
  // Don't want to invailidate kFailureTags by std::move-ing it so make a copy and move that
  auto failureTagCopy = kFailureTags;
  ICozmoBehavior::SubscribeToTags(std::move(failureTagCopy));
}

bool IBehaviorSelfTest::WantsToBeActivatedBehavior() const
{
  return true;
}

void IBehaviorSelfTest::OnBehaviorActivated()
{
  // Add a timer to force the behavior to end if it runs too long
  AddTimer(SelfTestConfig::kDefaultTimeout_ms, [this](){
    PRINT_NAMED_WARNING("IBehaviorSelfTest.Timeout",
                        "Behavior %s has timed out and we are %signoring failures",
                        GetDebugLabel().c_str(),
                        (ShouldIgnoreFailures() ? "" : "NOT "));
    
    if(!ShouldIgnoreFailures())
    {
      SetResult(_timeoutCode);
    }
    else
    {
      SetResult(SelfTestResultCode::SUCCESS);
    }
  });
  
  OnBehaviorActivatedInternal();
}
  
void IBehaviorSelfTest::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }

  // Immediately stop the behavior if the result is not success
  if(_result != SelfTestResultCode::UNKNOWN &&
     _result != SelfTestResultCode::SUCCESS)
  {
    CancelSelf();
    return;
  }

  // Update the timers
  for(auto& timer : _timers)
  {
    timer.Tick();
  }

  // Keep updating behavior until it completes
  if(_lastStatus != SelfTestStatus::Complete)
  {
    _lastStatus = SelfTestUpdateInternal();
  }
  
  if(_lastStatus != SelfTestStatus::Running)
  {
    CancelSelf();
  }
}
  
void IBehaviorSelfTest::HandleWhileActivated(const EngineToGameEvent& event)
{
  EngineToGameTag tag = event.GetData().GetTag();
  
  // If a subclass subscribed to the tag then let them handle it otherwise it is one we (the baseclass) subscribed
  // to, one of the kFailureTags
  if(_tagsSubclassSubscribeTo.count(tag) > 0)
  {
    HandleWhileActivatedInternal(event);
  }
  else
  {
    switch(tag)
    {
      case EngineToGameTag::CliffEvent:
      {
        SELFTEST_SET_RESULT(SelfTestResultCode::CLIFF_UNEXPECTED);
        break;
      }
      case EngineToGameTag::RobotStopped:
      {
        SELFTEST_SET_RESULT(SelfTestResultCode::CLIFF_UNEXPECTED);
        break;
      }
      case EngineToGameTag::ChargerEvent:
      {
        if(event.GetData().Get_ChargerEvent().onCharger)
        {
          SELFTEST_SET_RESULT(SelfTestResultCode::UNEXPECTED_ON_CHARGER);
        }
        break;
      }
      case EngineToGameTag::MotorCalibration:
      {
        const auto& payload = event.GetData().Get_MotorCalibration();
        if(payload.motorID == MotorID::MOTOR_HEAD)
        {
          SELFTEST_SET_RESULT(SelfTestResultCode::HEAD_MOTOR_CALIB_UNEXPECTED);
        }
        else if(payload.motorID == MotorID::MOTOR_LIFT)
        {
          SELFTEST_SET_RESULT(SelfTestResultCode::LIFT_MOTOR_CALIB_UNEXPECTED);
        }
        else
        {
          SELFTEST_SET_RESULT(SelfTestResultCode::MOTOR_CALIB_UNEXPECTED);
        }
        break;
      }
      case EngineToGameTag::MotorAutoEnabled:
      {
        const auto& payload = event.GetData().Get_MotorAutoEnabled();
        if(!payload.enabled)
        {
          if(payload.motorID == MotorID::MOTOR_HEAD)
          {
            SELFTEST_SET_RESULT(SelfTestResultCode::HEAD_MOTOR_DISABLED);
          }
          else if(payload.motorID == MotorID::MOTOR_LIFT)
          {
            SELFTEST_SET_RESULT(SelfTestResultCode::LIFT_MOTOR_DISABLED);
          }
          else
          {
            SELFTEST_SET_RESULT(SelfTestResultCode::MOTOR_DISABLED);
          }
        }
        break;
      }
      case EngineToGameTag::RobotOffTreadsStateChanged:
      {
        if(event.GetData().Get_RobotOffTreadsStateChanged().treadsState != OffTreadsState::OnTreads)
        {
          SELFTEST_SET_RESULT(SelfTestResultCode::ROBOT_PICKUP);
        }
        break;
      }
      case EngineToGameTag::UnexpectedMovement:
      {
        SELFTEST_SET_RESULT(SelfTestResultCode::UNEXPECTED_MOVEMENT_DETECTED);
        break;
      }
      default:
      {
        return;
      }
    }
  }
}

void IBehaviorSelfTest::HandleWhileActivated(const RobotToEngineEvent& event)
{
  HandleWhileActivatedInternal(event);
}

void IBehaviorSelfTest::SubscribeToTags(std::set<EngineToGameTag>&& tags)
{
  _tagsSubclassSubscribeTo.insert(tags.begin(), tags.end());
  
  // If the caller wants to subscribe to one of the failure type tags then don't let them subscribe
  // since we already are subscribed to it and it will already call HandleWhileActivatedInternal()
  for(auto iter = tags.begin(); iter != tags.end();)
  {
    if(kFailureTags.count(*iter) > 0)
    {
      iter = tags.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  
  ICozmoBehavior::SubscribeToTags(std::move(tags));
}

void IBehaviorSelfTest::SubscribeToTags(std::set<GameToEngineTag>&& tags)
{
  ICozmoBehavior::SubscribeToTags(std::move(tags));
}

bool IBehaviorSelfTest::DelegateIfInControl(IActionRunner* action, SimpleCallback callback)
{
  // Caller is not checking the action result so fail if the action fails
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      SELFTEST_SET_RESULT(SelfTestResultCode::ACTION_FAILED);
    }

    callback();
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

bool IBehaviorSelfTest::DelegateIfInControl(IActionRunner* action, ActionResultCallback callback)
{
  // Caller is checking the action result so just print a warning on failure and let the
  // caller deal with the action result
  auto callbackWrapper = [this, callback](ActionResult result){
    if(result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorSelfTest.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetDebugLabel().c_str(),
                          EnumToString(result));
    }
    
    callback(result);
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

bool IBehaviorSelfTest::DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback)
{
  // Caller is checking the rca so just print a warning on failure and let the
  // caller deal with the rca
  auto callbackWrapper = [this, callback](const ExternalInterface::RobotCompletedAction& rca){
    if(rca.result != ActionResult::SUCCESS)
    {
      PRINT_NAMED_WARNING("IBehaviorSelfTest.ActionFailed", "Behavior %s had an action fail with result %s",
                          GetDebugLabel().c_str(),
                          EnumToString(rca.result));
    }
    
    callback(rca);
  };
  return ICozmoBehavior::DelegateIfInControl(action, callbackWrapper);
}

bool IBehaviorSelfTest::ShouldIgnoreFailures() const
{
  return SelfTestConfig::kIgnoreFailures;
}

void IBehaviorSelfTest::SetResult(SelfTestResultCode result)
{
  _result = result;
  ClearTimers();
  AddToResultList(result);
}

void IBehaviorSelfTest::AddToResultList(SelfTestResultCode result)
{
  results[GetDebugLabel()].push_back(result);
}

const std::map<std::string, std::vector<SelfTestResultCode>>& IBehaviorSelfTest::GetAllSelfTestResults()
{
  return results;
}

void IBehaviorSelfTest::ResetAllSelfTestResults()
{
  results.clear();
}

const std::set<ExternalInterface::MessageEngineToGameTag>& IBehaviorSelfTest::GetFailureTags()
{
  return kFailureTags;
}

void IBehaviorSelfTest::ReceivedFFTResult()
{
  receivedFFTResult = true;
}

bool IBehaviorSelfTest::DidReceiveFFTResult()
{
  bool b = receivedFFTResult;
  receivedFFTResult = false;
  return b;
}

void IBehaviorSelfTest::RemoveTimer(const std::string& name)
{
  for(auto iter = _timers.begin(); iter != _timers.end(); ++iter)
  {
    if(iter->GetName() == name)
    {
      _timers.erase(iter);
      return;
    }
  }
}

void IBehaviorSelfTest::RemoveTimers(const std::string& name)
{
  auto iter = _timers.begin();
  while(iter != _timers.end())
  {
    if(iter->GetName() == name)
    {
      iter = _timers.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

void IBehaviorSelfTest::ClearTimers()
{
  // Remove all timers except for the ones marked as "DontDelete"
  for(auto iter = _timers.begin(); iter < _timers.end();)
  {
    if(iter->GetName() != "DontDelete")
    {
      iter = _timers.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

void IBehaviorSelfTest::IncreaseTimeoutTimer(TimeStamp_t time_ms)
{
  auto iter = _timers.begin();
  while(iter != _timers.end())
  {
    if(iter->GetName() == "")
    {
      iter->AddTime(time_ms);
    }

    ++iter;
  }
}

void IBehaviorSelfTest::DrawTextOnScreen(Robot& robot,
                                         const std::vector<std::string>& text,
                                         ColorRGBA textColor,
                                         ColorRGBA bg,
                                         float rotate_deg)
{
  auto& anim = robot.GetAnimationComponent();
  Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH, {bg.r(), bg.g(), bg.b()});

  const int thickness = 1;
  const int font = CV_FONT_NORMAL;

  cv::Size minSize(std::numeric_limits<int>::max(),
                   std::numeric_limits<int>::max());
  float minScale = 0;

  // Figure out the minimum text size we need such that all lines of text will fit
  // on the display
  for(const auto& t : text)
  {
    cv::Size size;
    float scale;

    Vision::Image::MakeTextFillImageWidth(t,
                                          font,
                                          thickness,
                                          img.GetNumCols(),
                                          size,
                                          scale);

    if(size.height < minSize.height)
    {
      minSize = size;
      minScale = scale;
    }
  }

  // Figure out the y offset starting point in order to draw
  // all lines of text centered vertically
  int offset = 0;
  if(text.size() == 1)
  {
    offset = img.GetNumRows()/2 + minSize.height/2;
  }
  else
  {
    offset = img.GetNumRows()/2 - (std::ceil(text.size()/2.f) * minSize.height/2);
  }

  // Start drawing text to image
  for(auto iter = text.begin(); iter != text.end(); ++iter)
  {
    float scale = minScale;
    int textThickness = thickness;

    // If the text after this line is an empty string
    // treat it special. This current text will be scaled
    // and drawn such that it takes up both lines
    auto next = iter + 1;
    if(next != text.end() &&
       *next == "")
    {
      textThickness = 2;
      scale *= 2.f;
      offset += minSize.height;
    }
    
    img.DrawTextCenteredHorizontally(*iter,
                                     font,
                                     scale,
                                     textThickness,
                                     textColor,
                                     offset,
                                     false);

    // If we scaled this text due to the next text being ""
    // then we need to skip ""
    if(scale != minScale)
    {
      iter++;
    }

    // Increment line spacing
    offset += minSize.height + 2;
  }

  // Rotate the image by rotate_deg
  if(!Util::IsNearZero(rotate_deg))
  {
    const auto M = cv::getRotationMatrix2D({(float)img.GetNumCols()/2, (float)img.GetNumRows()/2},
                                            rotate_deg,
                                            1);
    cv::warpAffine(img.get_CvMat_(), img.get_CvMat_(), M, {img.GetNumCols(), img.GetNumRows()});
  }

  anim.DisplayFaceImage(img, 0, true); 
}

}
}
