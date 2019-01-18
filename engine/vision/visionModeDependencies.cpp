/**
 * File: visionModeDependencies.cpp
 *
 * Author: Andrew Stein
 * Created: 01/14/2019
 *
 * Description: [See header file]
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/vision/visionModeDependencies.h"
#include "engine/vision/visionModeSet.h"
#include "engine/vision/visionProcessingResult.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "VisionSystem"

namespace Anki {
namespace Vector {

namespace {
  
  // Fraction of the image needed to be seen as moving to enable person detection when OffboardPersonDetection is enabled
  CONSOLE_VAR_RANGED(float, kTheBox_ObservedMotionThreshold, "TheBox", 0.1f, 0.f, 1.f);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionModeDependencies::VisionModeDependencies()
{
//  RegisterShouldRunFunction(VisionMode::OffboardPersonDetection,
//                            &VisionModeDependencies::ShouldRunOffboardPersonDetection);
//  
//  RegisterShouldRunFunction(VisionMode::ClassifyingPeople,
//                            &VisionModeDependencies::ShouldRunPersonClassification);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionModeDependencies::RegisterShouldRunFunction(const VisionMode& forMode, const ShouldRunFcn& shouldRunFcn)
{
  _shouldRunMap.emplace(forMode, shouldRunFcn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeDependencies::ShouldRun(const VisionMode& visionMode,
                                       const VisionProcessingResult& result,
                                       const VisionModeSet& visionModesEnabled,
                                       TimeStamp_t& usingTimestamp) const
{
  // Don't run the mode if it's not even enabled
  if(!visionModesEnabled.Contains(visionMode))
  {
    return false;
  }
  
  // "Normal" usage is to use the image with the timestamp corresponding to the result's timestamp
  // If we should use a different one (e.g. an older image processed by a NeuralNetRunner), then that
  // will be set case by case below.
  usingTimestamp = (TimeStamp_t)result.timestamp;
  
  auto iter = _shouldRunMap.find(visionMode);
  if(iter == _shouldRunMap.end())
  {
    // No entry in the table means there are no additional dependencies/requirements, so always run
    return true;
  }
  
  const ShouldRunFcn& shouldRunFcn = iter->second;
  return shouldRunFcn(result, visionModesEnabled, usingTimestamp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeDependencies::ShouldRunOffboardPersonDetection(const VisionProcessingResult& result,
                                                              const VisionModeSet& visionModesEnabled,
                                                              TimeStamp_t& usingTimestamp)
{
  const TimeStamp_t kAnyTimestamp = 0;
  usingTimestamp = result.ContainsDetectionsForMode(VisionMode::ClassifyingPeople, kAnyTimestamp);
  if(usingTimestamp > 0)
  {
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeDependencies::ShouldRunPersonClassification(const VisionProcessingResult& result,
                                                           const VisionModeSet& visionModesEnabled,
                                                           TimeStamp_t& usingTimestamp)
{
  if(!visionModesEnabled.Contains(VisionMode::OffboardPersonDetection))
  {
    // Offboard person detection is a special case (handled below).
    // If it's not enabled, then ClassifyingPeople should run whenever enabled.
    return true;
  }
  
  if(!visionModesEnabled.Contains(VisionMode::DetectingMotion))
  {
    LOG_WARNING("VisionModeDependencies.ShouldRunPersonClassification.MotionNotEnabled", "");
    return false;
  }
  
  // When offboard person detection is enabled, we want to run person classification iff motion is detected,
  // so that offboard person detection only runs when "enough" motion is detected AND then the person classifier sees
  // people as well.
  for(const auto& observedMotion : result.observedMotions)
  {
    if(Util::IsFltGE(observedMotion.img_area, kTheBox_ObservedMotionThreshold))
    {
      DEV_ASSERT(usingTimestamp == result.timestamp,
                 "VisionModeDependencies.ShouldRunPersonClassification.UsingTimestampNotSet");
      LOG_INFO("VisionModeDependencies.ShouldRunPersonClassification.FoundMotion",
               "Area:%.3f t:%ums", observedMotion.img_area, usingTimestamp);
      return true;
    }
  }
  
  LOG_PERIODIC_INFO(20, "VisionModeDependencies.ShouldRunPersonClassification.NoMotion",
                    "Area:%.3f", (result.observedMotions.empty() ? 0.f : result.observedMotions.front().img_area));
  return false;
}
  
} // namespace Vector
} // namespace Anki
