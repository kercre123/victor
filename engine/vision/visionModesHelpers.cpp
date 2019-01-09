/**
* File: visionModesHelpers.cpp
*
* Author: Lee Crippen
* Created: 06/12/16
*
* Description: Helper functions for dealing with CLAD generated visionModes
*
* Copyright: Anki, Inc. 2016
*
**/


#include "engine/vision/visionModesHelpers.h"
#include "engine/vision/visionModeSet.h"
#include "engine/vision/visionProcessingResult.h"
#include "util/container/symmetricMap.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Vector {

IMPLEMENT_ENUM_INCREMENT_OPERATORS(VisionMode);

// To "register" a VisionMode with an associated neural network name,
// add it to this lookup table initialization. Note that multiple
// VisionModes _can_ refer to the same network name.
static const Util::SymmetricMap<VisionMode, std::string> sNetModeLUT{
  {VisionMode::DetectingPeople, "person_detector"},
  {VisionMode::DetectingHands,  "hand_detector"},
  {VisionMode::DetectingPets,   "mobilenet"}, // TODO: Update to real network
  {VisionMode::OffboardSceneDescription,  "test_offboard_model"},
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GetNeuralNetsForVisionMode(const VisionMode mode, std::set<std::string>& networkNames)
{
  sNetModeLUT.Find(mode, networkNames);
  return (!networkNames.empty());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool GetVisionModesForNeuralNet(const std::string& networkName, std::set<VisionMode>& modes)
{
  sNetModeLUT.Find(networkName, modes);
  return (!modes.empty());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::set<VisionMode>& GetVisionModesUsingNeuralNets()
{
  static std::set<VisionMode> sModes;
  if(sModes.empty())
  {
    // Fill on first use. No need to refill each call because sNetModeLUT is
    // static const.
    sNetModeLUT.GetKeys(sModes);
  }
  return sModes;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ShouldVisionModeRun(const VisionMode& visionMode, const VisionProcessingResult& result, TimeStamp_t& usingTimestamp)
{
  usingTimestamp = 0;
  
  // TODO: Unify with SaveConditionType from ImageSaver
  // NOTE: In order for an ALL condition to be true, all the detections must have the same timestamp
  enum ConditionType { ANY, ALL };
  struct RunCondition {
    ConditionType type;
    VisionModeSet modes;
  };
 
  // Set of any modes that must be present in order for a given mode to run
  // TODO: Get this from Json config
  const std::map<VisionMode, RunCondition> kRequiredModesToRun{
    //{VisionMode::OffboardFaceRecognition, RunCondition{ANY, {VisionMode::DetectingFaces}}},
  };
  
  auto iter = kRequiredModesToRun.find(visionMode);
  if(iter == kRequiredModesToRun.end())
  {
    // No entry in the table means there are no required modes, so always run
    usingTimestamp = (TimeStamp_t)result.timestamp;
    return true;
  }
  
  const TimeStamp_t kAnyTimestamp = 0;
  switch(iter->second.type)
  {
    case ANY:
    {
      for(VisionMode requiredMode : iter->second.modes)
      {
        usingTimestamp = result.ContainsDetectionsForMode(requiredMode, kAnyTimestamp);
        if(usingTimestamp > 0)
        {
          return true;
        }
      }
      return false;
    }
      
    case ALL:
    {
      for(VisionMode requiredMode : iter->second.modes)
      {
        usingTimestamp = result.ContainsDetectionsForMode(requiredMode, kAnyTimestamp);
        if(usingTimestamp == 0)
        {
          return false;
        }
      }
      return true;
    }
  } // switch(type)
}
  
} // namespace Vector
} // namespace Anki

