/**
 * File: BehaviorBoxDemoCountPeople.cpp
 *
 * Author: Andrew Stein
 * Created: 2019-01-14
 *
 * Description: Uses local neural net person classifier to trigger cloud object/person detection
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoCountPeople.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/imageSaver.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "opencv2/highgui.hpp" // Only needed for CV_FONT_NORMAL

#include <fstream>

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
 
namespace {
  CONSOLE_VAR(f32, kCountPeople_FontScale, "TheBox.Screen",  0.6f);
  CONSOLE_VAR_RANGED(f32, kTheBox_CountPeopleMotionThreshold, "TheBox.PersonDetection", 0.05f, 0.f, 1.f);
  CONSOLE_VAR_RANGED(f32, kTheBox_CountPeopleSalientPointOverlapThreshold, "TheBox.PersonDetection", 0.5f, 0.f, 1.f);
  
  // If set to Images, will load the saved image from the neural net and re-draw any detected salient points on it.
  // If set to JsonSalientPoints, will write the SalientPoints to Json and assume Web will render them.
  CONSOLE_VAR_ENUM(s32, kTheBox_CountPeopleSaveMode, "TheBox.PersonDetection", 0, "Off,Images,JsonSalientPoints");
  
  CONSOLE_VAR_RANGED(s32, kTheBox_CountPeopleSaveThumbnailSize, "TheBox.PersonDetection", 0.f, 0.f, 1.f);
  CONSOLE_VAR_RANGED(s32, kTheBox_CountPeopleDrawSalientPointThickness, "TheBox.PersonDetection", 3, 1, 5);
  CONSOLE_VAR_RANGED(s32, kTheBox_CountPeopleDrawSalientPointThumbnailThickness, "TheBox.PersonDetection", 2, 1, 5); 
  CONSOLE_VAR_RANGED(s32, kTheBox_CountPeopleJpegQuality, "TheBox.PersonDetection", 60, 0, 100);
 
  const char* const kSaveBaseName = "person_detection";
  const char* const kPersistentSaveSubDir = "photos";
}
  
namespace ConfigKeys
{
  const char* const kVisionTimeout         = "visionRequestTimeout_sec";
  const char* const kWaitTimeBetweenImages = "waitTimeBetweenImages_sec";
  const char* const kSaveQuality           = "saveQuality";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::BehaviorBoxDemoCountPeople(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.visionRequestTimeout_sec = JsonTools::ParseFloat(config, ConfigKeys::kVisionTimeout,
                                                            "BehaviorBoxDemoCountPeople");
  
  _iConfig.waitTimeBetweenImages_sec = JsonTools::ParseFloat(config, ConfigKeys::kWaitTimeBetweenImages,
                                                            "BehaviorBoxDemoCountPeople");
  
  _iConfig.saveQuality = JsonTools::ParseInt8(config, ConfigKeys::kSaveQuality, "BehaviorBoxDemoCountPeople");
  
  SubscribeToTags({EngineToGameTag::RobotObservedMotion});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::~BehaviorBoxDemoCountPeople()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoCountPeople::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.behaviorAlwaysDelegates = false;
  
  modifiers.visionModesForActiveScope->insert({VisionMode::DetectingMotion,              EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorMode,                   EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorModeCacheToWhiteboard,  EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    ConfigKeys::kVisionTimeout,
    ConfigKeys::kWaitTimeBetweenImages,
    ConfigKeys::kSaveQuality,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
 
  _dispImg.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  
  // Wait a few images on startup to clear out MirrorMode?
  WaitForImagesAction* waitAction = new WaitForImagesAction(10);
  DelegateIfInControl(waitAction, &BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::DrawText3Lines(const std::array<std::string,3>& strings, const ColorRGBA& color)
{
  _dispImg.FillWith(Vision::PixelRGB(0,0,0));
  
  const Vec2f textSize = _dispImg.GetTextSize(strings[0], kCountPeople_FontScale, 1);
  const std::array<f32,3> ypos = {{
    FACE_DISPLAY_HEIGHT/2 - 1.5f*textSize.y(),
    FACE_DISPLAY_HEIGHT/2,
    FACE_DISPLAY_HEIGHT/2 + 1.5f*textSize.y(),
  }};
  
  for(s32 i=0; i<3; ++i)
  {
    _dispImg.DrawTextCenteredHorizontally(strings[i], CV_FONT_NORMAL, kCountPeople_FontScale, 1,
                                     color, ypos[i], true);
  }
  
  const bool kInterruptRunning = true;
  GetBEI().GetAnimationComponent().DisplayFaceImage(_dispImg, ANIM_TIME_STEP_MS, kInterruptRunning);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForMotion", "");
  
  DrawText3Lines({{"Looking", "for", "Motion"}}, NamedColors::YELLOW);
  
  _dVars.startImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
  
  _dVars.isWaitingForMotion = true;
  _dVars.motionObserved = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::BehaviorUpdate()
{
  if(_dVars.isWaitingForMotion && _dVars.motionObserved)
  {
    _dVars.isWaitingForMotion = false;
    TransitionToWaitingForPeople();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForPeople()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForPeople", "");
  
  DrawText3Lines({{"Looking", "for", "People"}}, NamedColors::MAGENTA);
  
  WaitForImagesAction* waitForImage = new WaitForImagesAction(1, VisionMode::ClassifyingPeople, _dVars.startImageTime_ms);
  DelegateIfInControl(waitForImage, [this]() {
    
    std::list<Vision::SalientPoint> salientPoints;
    const auto& salientPointComp = GetAIComp<SalientPointsComponent>();
    salientPointComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::PersonPresent,
                                              _dVars.startImageTime_ms);
    
    LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForPeople.ClassifyingPeopleRan",
             "SalientPoints:%zu", salientPoints.size());
    
    if(!salientPoints.empty())
    {
      TransitionToWaitingForCloud();
    }
    else
    {
      TransitionToWaitingForMotion();
    }
    
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForCloud()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForCloud", "");
  
  DrawText3Lines({{"Waiting", "for", "Cloud"}}, NamedColors::CYAN);
  
  WaitForImagesAction* waitForImage = new WaitForImagesAction(1, VisionMode::OffboardPersonDetection);
  waitForImage->SetTimeoutInSeconds(_iConfig.visionRequestTimeout_sec);
  
  if(_iConfig.saveQuality != 0)
  {
    const std::string cachePath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->GetCachePath("cloud_vison");
    const bool kRemoveDistortion = false;
    ImageSaverParams saveParams(cachePath,
                                ImageSaverParams::Mode::SingleShot,
                                _iConfig.saveQuality,
                                kSaveBaseName,
                                Vision::ImageCacheSize::Full, // NOTE: this has no effect. Image is taken from NeuralNetRunner
                                kTheBox_CountPeopleSaveThumbnailSize,
                                1.f,
                                kRemoveDistortion);
    
    saveParams.saveConditions[VisionMode::OffboardPersonDetection] = ImageSaverParams::SaveConditionType::OnDetection;
    waitForImage->SetSaveParams(saveParams);
  }
  
  DelegateIfInControl(waitForImage, [this]() {
    
    std::list<Vision::SalientPoint> salientPoints;
    const auto& salientPointsComp = GetAIComp<SalientPointsComponent>();
    salientPointsComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::Person,
                                               _dVars.startImageTime_ms);
    
    LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForCloud.OffboardPersonDetectionRan",
             "SalientPoints:%zu", salientPoints.size());
    
    const bool drewSomething = DrawSalientPoints(salientPoints);
    if(drewSomething)
    {
      if(_iConfig.saveQuality != 0)
      {
        DrawSalientPointsOnSavedImage(salientPoints);
      }
      
      WaitAction* waitAction = new WaitAction(_iConfig.waitTimeBetweenImages_sec);
      DelegateIfInControl(waitAction, &BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion);
    }
    else
    {
      TransitionToWaitingForMotion();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoCountPeople::DrawSalientPoints(const std::list<Vision::SalientPoint>& salientPoints)
{
  if(salientPoints.empty())
  {
    return false;
  }
  
  const TimeStamp_t atTimestamp = salientPoints.front().timestamp;
  const bool gotImage = GetAIComp<AIWhiteboard>().GetMirrorModeImage(atTimestamp, _dispImg);
  if(!gotImage)
  {
    // Couldn't find a mirror mode image that matches the timestamp we want.
    // Fall back on just using the oldest so we still display _something_.
    // This may indicate that we've got more than normal lag to the Cloud, coupled with
    //  not a large enough cache of MirrorMode images in AIWhiteboard.
    LOG_WARNING("BehaviorBoxDemoCountPeople.DrawSalientPoints.NoImageFoundForTimestamp", "t:%u", atTimestamp);
    GetAIComp<AIWhiteboard>().GetOldestMirrorModeImage(_dispImg);
  }
  
  std::vector<Rectangle<f32>> alreadyDrawn;
  for(const auto& salientPoint : salientPoints)
  {
    Poly2f poly(salientPoint.shape);
    
    Rectangle<f32> rect(poly);
    bool keep = true;
    for(const auto& rectAlreadyDrawn : alreadyDrawn)
    {
      // Simple "non-maxima suppression". Don't draw this salient point if it overlaps one we've
      // already drawn too much
      if(rect.ComputeOverlapScore(rectAlreadyDrawn) > kTheBox_CountPeopleSalientPointOverlapThreshold)
      {
        keep = false;
        break;
      }
    }
    
    if(keep)
    {
      alreadyDrawn.emplace_back(rect);
      
      for(auto & pt : poly)
      {
        pt.x() *= FACE_DISPLAY_WIDTH;
        pt.x() = FACE_DISPLAY_WIDTH - pt.x(); // Mirror
        pt.y() *= FACE_DISPLAY_HEIGHT;
      }
      
      _dispImg.DrawPoly(poly, NamedColors::YELLOW, kTheBox_CountPeopleDrawSalientPointThickness);
    }
  }
  
  GetBEI().GetAnimationComponent().DisplayFaceImage(_dispImg, ANIM_TIME_STEP_MS);
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::DrawSalientPointsOnSavedImage(const std::list<Vision::SalientPoint>& salientPoints) const
{
  const std::string cachePath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->GetCachePath("cloud_vison");
  const std::string imgFilename = Util::FileUtils::FullFilePath({cachePath, std::string(kSaveBaseName) + ".jpg"});
  const std::string thumbnailFilename = Util::FileUtils::FullFilePath({cachePath,
                                                                       std::string(kSaveBaseName) + ".thm.jpg"});
  
  const TimeStamp_t timestamp = salientPoints.front().timestamp;
  const std::string persistentPath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->GetPersistentPath(kPersistentSaveSubDir);
  std::string saveFilename = Util::FileUtils::FullFilePath({persistentPath, (std::string(kSaveBaseName) + "_"
                                                                             + std::to_string(timestamp))});
  
  switch(kTheBox_CountPeopleSaveMode)
  {
    case 1:
    {
      std::async(std::launch::async, [salientPoints, imgFilename, thumbnailFilename, saveFilename]()
                 {
                   {
                     // Load up the saved image and draw the detections, then re-save for the web
                     Vision::ImageRGB imgForWeb;
                     const Result loadResult = imgForWeb.Load(imgFilename);
                     if(ANKI_VERIFY(loadResult == RESULT_OK,
                                    "BehaviorBoxDemoCountPeople.DrawSalientPointsOnSavedImage.LoadSavedImageFailed", ""))
                     {
                       for(const auto& salientPoint : salientPoints)
                       {
                         Poly2f poly(salientPoint.shape);
                         for(auto & pt : poly)
                         {
                           pt.x() *= imgForWeb.GetNumCols();
                           pt.y() *= imgForWeb.GetNumRows();
                         }
                         imgForWeb.DrawPoly(poly, NamedColors::YELLOW, kTheBox_CountPeopleDrawSalientPointThickness);
                       }

                       const Result saveResult = imgForWeb.Save(saveFilename + ".jpg", kTheBox_CountPeopleJpegQuality);
                       if(RESULT_OK != saveResult)
                       {
                         LOG_ERROR("BehaviorBoxDemoCountPeople.DrawSalientPointsOnSavedImage.SaveImageFailed", "%s",
                                   saveFilename.c_str());
                       }
                     }
                   }

                   {
                     // same as above, but for the thumbnail (if it exists)
                     Vision::ImageRGB imgForWeb;
                     const Result loadResult = imgForWeb.Load(thumbnailFilename);
                     if(loadResult == RESULT_OK )
                     {
                       for(const auto& salientPoint : salientPoints)
                       {
                         Poly2f poly(salientPoint.shape);
                         for(auto & pt : poly)
                         {
                           pt.x() *= imgForWeb.GetNumCols();
                           pt.y() *= imgForWeb.GetNumRows();
                         }
                         imgForWeb.DrawPoly(poly,
                                            NamedColors::YELLOW,
                                            kTheBox_CountPeopleDrawSalientPointThumbnailThickness);
                       }
                     
                       const Result saveResult = imgForWeb.Save(saveFilename + ".thm.jpg",
                                                                kTheBox_CountPeopleJpegQuality);
                       if(RESULT_OK != saveResult)
                       {
                         LOG_ERROR("BehaviorBoxDemoCountPeople.DrawSalientPointsOnSavedImage.SaveThumbnailFailed", "%s",
                                   saveFilename.c_str());
                       }
                     }

                   }
                 });
      break;
    }
      
    case 2:
    {
      // Move the saved image into place and write out a correspond Json file
      saveFilename += ".jpg";
      const bool success = Util::FileUtils::MoveFile(saveFilename + ".jpg", imgFilename);
      if(ANKI_VERIFY(success, "BehaviorBoxDemoCountPeople.DrawSalientPointsOnSavedImage.MoveFailed",
                     "%s->%s", imgFilename.c_str(), (saveFilename+".jpg").c_str()))
      {
        Json::Value salientPointsJson;
        for(const auto& salientPoint : salientPoints)
        {
          salientPointsJson.append(salientPoint.GetJSON());
        }
        Json::StyledWriter writer;
        std::ofstream file(saveFilename + ".json");
        if(ANKI_VERIFY(file.is_open(),
                       "BehaviorBoxDemoCountPeople.DrawSalientPointsOnSavedImage.OpenJsonFileFailed", "%s",
                       (saveFilename + ".json").c_str()))
        {
          file << writer.write(salientPointsJson);
          file.close();
        }
      }
      Util::FileUtils::MoveFile(saveFilename + ".thm.jpg", thumbnailFilename);
      
      break;
    }
      
    default:
      // Nothing to do
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::HandleWhileActivated(const EngineToGameEvent& event)
{
  if(event.GetData().GetTag() == EngineToGameTag::RobotObservedMotion)
  {
    const auto& msg = event.GetData().Get_RobotObservedMotion();
    if(_dVars.isWaitingForMotion && msg.img_area > kTheBox_CountPeopleMotionThreshold)
    {
      _dVars.motionObserved = true;
    }
  }
  else
  {
    LOG_ERROR("BehaviorBoxDemoCountPeople.HandeWhileActivated.UnexpectedEngineToGameEvent", "%s",
              MessageEngineToGameTagToString(event.GetData().GetTag()));
  }
}
  
  
}
}
