/**
 * File: behaviorPlaypenInitChecks.cpp
 *
 * Author: Al Chaussee
 * Created: 08/14/17
 *
 * Description: Checks that Cozmo can read the lift tool codes and that they are within expected bounds
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenReadToolCode.h"

#include "engine/actions/basicActions.h"
#include "engine/components/visionComponent.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPlaypenReadToolCode::BehaviorPlaypenReadToolCode(const Json::Value& config)
: IBehaviorPlaypen(config)
{

}

Result BehaviorPlaypenReadToolCode::OnBehaviorActivatedInternal()
{
  ReadToolCodeAction* action = new ReadToolCodeAction(false);
  DelegateIfInControl(action, [this](const ExternalInterface::RobotCompletedAction& rca) {
    TransitionToToolCodeRead(rca);
  });
  
  return RESULT_OK;
}

void BehaviorPlaypenReadToolCode::TransitionToToolCodeRead(const ExternalInterface::RobotCompletedAction& rca)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Save tool code images to robot (whether it succeeded to read code or not)
  std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetToolCodeImageJpegData();
  
  static const NVStorage::NVEntryTag toolCodeImageTags[PlaypenConfig::kNumToolCodes] =
    {NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft, NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight};
  
  
  // Verify number of images
  bool tooManyToolCodeImages = rawJpegData.size() > PlaypenConfig::kNumToolCodes;
  if(tooManyToolCodeImages)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenReadToolCode.TransitionToToolCodeRead.TooManyToolCodeImagesFound",
                        "%zu images found. Why?", rawJpegData.size());
    rawJpegData.resize(PlaypenConfig::kNumToolCodes);
  }
  
  // Write images to robot
  u32 imgIdx = 0;
  for(auto const& img : rawJpegData)
  {
    WriteToStorage(robot, toolCodeImageTags[imgIdx], img.data(), img.size(),
                   FactoryTestResultCode::TOOL_CODE_IMAGES_WRITE_FAILED);
    
    ++imgIdx;
    
    // Save calibration images to log on device
    PLAYPEN_TRY(GetLogger().AddFile("toolCodeImage_" + std::to_string(imgIdx) + ".jpg", img),
                FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  }
  
  if(tooManyToolCodeImages)
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::TOO_MANY_TOOL_CODE_IMAGES);
  }
  
  // Check result of tool code read
  if(rca.result != ActionResult::SUCCESS)
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::READ_TOOL_CODE_FAILED);
  }
  
  const ToolCodeInfo &info = rca.completionInfo.Get_readToolCodeCompleted().info;
  PRINT_NAMED_INFO("BehaviorPlaypenReadToolCode.TransitionToToolCodeRead.RecvdToolCodeInfo.Info",
                   "Code: %s, Expected L: (%f, %f), R: (%f, %f), Observed L: (%f, %f), R: (%f, %f)",
                   EnumToString(info.code),
                   info.expectedCalibDotLeft_x,  info.expectedCalibDotLeft_y,
                   info.expectedCalibDotRight_x, info.expectedCalibDotRight_y,
                   info.observedCalibDotLeft_x,  info.observedCalibDotLeft_y,
                   info.observedCalibDotRight_x, info.observedCalibDotRight_y);
  
  // Write tool code to log on device
  PLAYPEN_TRY(GetLogger().Append(info), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  // Store results to nvStorage
  u8 buf[info.Size()];
  size_t numBytes = info.Pack(buf, sizeof(buf));
  WriteToStorage(robot, NVStorage::NVEntryTag::NVEntry_ToolCodeInfo, buf, numBytes,
                 FactoryTestResultCode::TOOL_CODE_WRITE_FAILED);
  
  // Verify tool code data is in range
  f32 distL_x = std::fabsf(info.expectedCalibDotLeft_x - info.observedCalibDotLeft_x);
  f32 distL_y = std::fabsf(info.expectedCalibDotLeft_y - info.observedCalibDotLeft_y);
  f32 distR_x = std::fabsf(info.expectedCalibDotRight_x - info.observedCalibDotRight_x);
  f32 distR_y = std::fabsf(info.expectedCalibDotRight_y - info.observedCalibDotRight_y);
  
  if (distL_x > PlaypenConfig::kToolCodeDistThreshX_pix ||
      distL_y > PlaypenConfig::kToolCodeDistThreshY_pix ||
      distR_x > PlaypenConfig::kToolCodeDistThreshX_pix ||
      distR_y > PlaypenConfig::kToolCodeDistThreshY_pix)
  {
    PLAYPEN_SET_RESULT(FactoryTestResultCode::TOOL_CODE_POSITIONS_OOR);
  }

  PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS);
}

}
}


