/**
* File: visionModesHelpers.h
*
* Author: Lee Crippen
* Created: 06/12/16
*
* Description: Helper functions for dealing with CLAD generated visionModes
*
* Copyright: Anki, Inc. 2016
*
**/


#ifndef __Cozmo_Basestation_VisionModesHelpers_H__
#define __Cozmo_Basestation_VisionModesHelpers_H__

#include "clad/types/visionModes.h"
#include "coretech/common/shared/types.h"
#include "util/enums/enumOperators.h"

#include <set>

namespace Anki {
namespace Vector {

// Forward declaration
class VisionModeSet;
struct VisionProcessingResult;

DECLARE_ENUM_INCREMENT_OPERATORS(VisionMode);

// NeuralNet <-> VisionMode
//   To "register" a VisionMode with an associated neural network name,
//   add it to the lookup table initialization in the .cpp source file.
  
// Find the neural network names registered to a given vision mode (or vice versa). Returns true if there are any and
// populates networkNames. Returns false (and does not modify networkNames) otherwise.
bool GetNeuralNetsForVisionMode(const VisionMode mode, std::set<std::string>& networkNames);
bool GetVisionModesForNeuralNet(const std::string& networkName, std::set<VisionMode>& modes);
  
// Return the set of VisionModes that have a neural net registered.
const std::set<VisionMode>& GetVisionModesUsingNeuralNets();
  
// Checks if the given vision mode wants to run based on what's already in the given processing result
//   and what vision modes are enabled (Note that if visionMode is not in visionModesEnabled, this will return false!)
// If true is returned, the TimeStamp_t argument indicates the timestamp of the image that should be used
bool ShouldVisionModeRun(const VisionMode& visionMode,
                         const VisionProcessingResult& result,
                         const VisionModeSet& visionModesEnabled,
                         TimeStamp_t& t);
  
} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_VisionModesHelpers_H__

