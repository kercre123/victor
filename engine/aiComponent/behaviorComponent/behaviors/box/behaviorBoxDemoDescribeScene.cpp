/**
 * File: BehaviorBoxDemoDescribeScene.cpp
 *
 * Author: Andrew Stein
 * Created: 2019-01-09
 *
 * Description: Shows text describing the scene when asked or when The Box is repositioned
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/image_impl.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoDescribeScene.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/visionComponent.h"

#include "opencv2/highgui.hpp"

namespace Anki {
namespace Vector {

namespace ConfigKeys
{
  const char* const kDelocTimeWindow = "delocTimeWindow_sec";
  const char* const kTextDisplayTime = "textDisplayTime_sec";
  const char* const kVisionTimeout   = "visionRequestTimeout_sec";
}
  
namespace
{
  const UserIntentTag kUserIntent = USER_INTENT(describe_scene);
  
  CONSOLE_VAR_RANGED(float, kTheBox_SceneDescriptionTextScale, "TheBox.Screen", 0.65f, 0.1, 1.f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::BehaviorBoxDemoDescribeScene(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.recentDelocTimeWindow_sec = JsonTools::ParseFloat(config, ConfigKeys::kDelocTimeWindow,
                                                             "BehaviorBoxDemoDescribeScene");
  
  _iConfig.textDisplayTime_sec = JsonTools::ParseFloat(config, ConfigKeys::kTextDisplayTime,
                                                       "BehaviorBoxDemoDescribeScene");
  
  _iConfig.visionRequestTimeout_sec = JsonTools::ParseFloat(config, ConfigKeys::kVisionTimeout,
                                                            "BehaviorBoxDemoDescribeScene");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoDescribeScene::~BehaviorBoxDemoDescribeScene()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoDescribeScene::WantsToBeActivatedBehavior() const
{

  const bool hasBehaviorEverRun = ( GetNumTimesBehaviorStarted() > 0 );
  auto& aiwb = GetAIComp<AIWhiteboard>();
  const float secSinceLastDeloc = aiwb.GetSecondsSinceLastDelocalization();
  const bool delocalizedRecently = (secSinceLastDeloc < _iConfig.recentDelocTimeWindow_sec);
  const bool isOnTreads = (GetBEI().GetRobotInfo().GetOffTreadsState() == OffTreadsState::OnTreads);
  if(hasBehaviorEverRun && delocalizedRecently && isOnTreads)
  {
    // when the box is put down, execute this behavior, but only after it's been started at least once
    // manually or with the command
    return true;
  }
  
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = uic.IsUserIntentPending(kUserIntent);
  if(pendingIntent)
  {
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::InitBehavior()
{
  //_iConfig.showTextBehavior = FindBehaviorByID(BehaviorID::ShowText);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    ConfigKeys::kDelocTimeWindow,
    ConfigKeys::kTextDisplayTime,
    ConfigKeys::kVisionTimeout,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::OnBehaviorActivated() 
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = uic.IsUserIntentPending(kUserIntent);
  if(pendingIntent)
  {
    SmartActivateUserIntent(kUserIntent);
  }
  
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // Put "Analyzing..." on the screen
  {
    const bool kInterruptRunning = true;
    Vision::ImageRGB dispImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    dispImg.FillWith(Vision::PixelRGB(0,0,0));
    dispImg.DrawTextCenteredHorizontally("Analyzing...", CV_FONT_NORMAL, 1.f, 1, NamedColors::YELLOW,
                                         FACE_DISPLAY_HEIGHT/2, true);
    GetBEI().GetAnimationComponent().DisplayFaceImage(dispImg, ANIM_TIME_STEP_MS, kInterruptRunning);
  }
  
  _dVars.lastImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
  
  WaitForImagesAction* waitAction = new WaitForImagesAction(1, VisionMode::OffboardSceneDescription,
                                                            _dVars.lastImageTime_ms);
  waitAction->SetTimeoutInSeconds(_iConfig.visionRequestTimeout_sec); // in case we never hear back from vision
  
  DelegateIfInControl(waitAction, [this](ActionResult result)
                      {
                        if(result == ActionResult::SUCCESS)
                        {
                          DisplayDescription();
                        }
                        else
                        {
                          using GE = AudioMetaData::GameEvent::GenericEvent;
                          using GO = AudioMetaData::GameObjectType;
                          GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Emote_Cant_Do_That_1,
                                                                   GO::Behavior);
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void BreakIntoLines(const std::string& description, const float imageWidth,
                           std::vector<std::string>& lines, f32& lineHeight)
{
  // Traverse the description
  std::istringstream descriptionStream(description);
  std::string nextLine;
  float nextLineLength = 0.f;
  lineHeight = 0.f;
  const float spaceLength = Vision::ImageRGB::GetTextSize(" ", kTheBox_SceneDescriptionTextScale, 1).x();
  do {
    // Grab next word from the description
    std::string word;
    descriptionStream >> word;
    
    const Vec2f wordSize = Vision::ImageRGB::GetTextSize(word, kTheBox_SceneDescriptionTextScale, 1);
    if(!word.empty() && (nextLineLength + wordSize.x() < imageWidth))
    {
      nextLine += word + " ";
      nextLineLength += wordSize.x() + spaceLength;
      lineHeight = std::max(lineHeight, wordSize.y());
    }
    else
    {
      lines.emplace_back(nextLine);
      nextLine = word + " ";
      nextLineLength = wordSize.x() + spaceLength;
    }
  } while(descriptionStream);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoDescribeScene::DisplayDescription()
{
  // TODO: is there a pretty way to reuse (delegate to) CheckForAndReactToSalientPoint here instead?
  const SalientPointsComponent& salientPointsComp = GetAIComp<SalientPointsComponent>();
  
  std::list<Vision::SalientPoint> salientPoints;
  salientPointsComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::SceneDescription,
                                             _dVars.lastImageTime_ms);
  if(!salientPoints.empty())
  {
    Vision::ImageRGB dispImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    dispImg.FillWith(Vision::PixelRGB(0,0,0));
    
    std::vector<std::string> lines;
    float lineHeight = 0.f;
    BreakIntoLines(salientPoints.begin()->description, dispImg.GetNumCols(), lines, lineHeight);
    
    // Draw all the lines of text
    s32 lineNum = 1;
    for(const auto& line : lines)
    {
      const bool kDropShadow = true;
      const s32  kThickness = 1;
      const f32  ypos = lineNum*(lineHeight+1.f);
      if(ypos >= dispImg.GetNumRows())
      {
        break;
      }
      dispImg.DrawText({1.f, ypos}, line, NamedColors::YELLOW,
                       kTheBox_SceneDescriptionTextScale, kDropShadow, kThickness);
      ++lineNum;
    }
    
    const u32 waitTime_ms = Util::SecToMilliSec(_iConfig.textDisplayTime_sec);
    GetBEI().GetAnimationComponent().DisplayFaceImage(dispImg, waitTime_ms);
    
    DelegateIfInControl(new WaitAction(_iConfig.textDisplayTime_sec));
  }
}

}
}
