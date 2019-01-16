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


#include "engine/vision/visionModeDependencies.h"
#include "engine/vision/visionModesHelpers.h"
#include "util/container/symmetricMap.h"
#include "util/enums/stringToEnumMapper.hpp"

namespace Anki {
namespace Vector {
  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(VisionMode);

// To "register" a VisionMode with an associated neural network name,
// add it to this lookup table initialization. Note that multiple
// VisionModes _can_ refer to the same network name.
static const Util::SymmetricMap<VisionMode, std::string> sNetModeLUT{
  {VisionMode::DetectingPeople,            "person_detector"},    // Detects and localizes people
  {VisionMode::ClassifyingPeople,          "person_classifier"},  // Merely classifies whether people are present
  {VisionMode::DetectingHands,             "hand_detector"},
  {VisionMode::DetectingPets,              "mobilenet"}, // TODO: Update to real network
  {VisionMode::OffboardSceneDescription,   "offboard_scene_description"},
  {VisionMode::OffboardPersonDetection,    "offboard_person_detection"},
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
bool ShouldVisionModeRun(const VisionMode& visionMode,
                         const VisionProcessingResult& result,
                         const VisionModeSet& visionModesEnabled,
                         TimeStamp_t& t)
{
  static VisionModeDependencies sDependencies;
  return sDependencies.ShouldRun(visionMode, result, visionModesEnabled, t);
}
  
} // namespace Vector
} // namespace Anki

