/**
 * File: visionScheduleMediator_fwd
 *
 * Author: Sam Russell
 * Created: 2018-01-22
 *
 * Description: Forward declarations for VisionScheduleMediator
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_visionScheduleMediator_fwd_H__
#define __Engine_Components_visionScheduleMediator_fwd_H__

#include "clad/types/visionModes.h"

namespace Anki {
namespace Vector {

  enum class EVisionUpdateFrequency
  {
    Low,
    Med,
    High,
    Standard
  };

  struct VisionModeRequest
  {
    VisionMode mode;
    EVisionUpdateFrequency frequency;

    // Define comparison to enable storage in std::set
    bool operator<(const VisionModeRequest& rhs) const { return this->mode < rhs.mode; }
  };
      
} // namespace Vector
} // namespace Anki

#endif // __Engine_Components_visionScheduleMediator_fwd_H__