/**
 * File: visionProcessingResult.cpp
 *
 * Author: Andrew Stein
 * Date:   12/10/2018
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/vision/visionProcessingResult.h"

#include "clad/types/salientPointTypes.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline bool TimeStampsMatch(const TimeStamp_t t_ref, const TimeStamp_t t)
{
  const bool anyTimeStamp = (t_ref == 0);
  return anyTimeStamp || (t_ref == t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static TimeStamp_t SalientPointDetectionPresent(const std::list<Vision::SalientPoint>& salientPoints,
                                                const Vision::SalientPointType salientType,
                                                const TimeStamp_t atTimestamp)
{
  for(const auto& salientPoint : salientPoints)
  {
    if( TimeStampsMatch(atTimestamp, salientPoint.timestamp) && (salientType == salientPoint.salientType) )
    {
      return salientPoint.timestamp;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t VisionProcessingResult::ContainsDetectionsForMode(const VisionMode mode, const TimeStamp_t atTimestamp) const
{
  switch(mode)
  {
    case VisionMode::DetectingMarkers:
    {
      for(const auto& marker: observedMarkers)
      {
        if(TimeStampsMatch(atTimestamp, marker.GetTimeStamp()))
        {
          return marker.GetTimeStamp();
        }
      }
      return 0;
    }
      
    case VisionMode::DetectingFaces:
    {
      for(const auto& face : faces)
      {
        if(TimeStampsMatch(atTimestamp, face.GetTimeStamp()))
        {
          return face.GetTimeStamp();
        }
      }
      return 0;
    }
      
    case VisionMode::DetectingHands:
      return SalientPointDetectionPresent(salientPoints, Vision::SalientPointType::Hand, atTimestamp);
      
    case VisionMode::DetectingPeople:
      return SalientPointDetectionPresent(salientPoints, Vision::SalientPointType::Person, atTimestamp);
      
    case VisionMode::ClassifyingPeople:
      return SalientPointDetectionPresent(salientPoints, Vision::SalientPointType::PersonPresent, atTimestamp);
      
    case VisionMode::DetectingMotion:
    {
      if(!observedMotions.empty())
      {
        return observedMotions.begin()->timestamp;
      }
      return 0;
    }
    default:
      LOG_ERROR("VisionProcessingResult.ContainsDetectionsForMode.ModeNotSupported",
                "VisionMode:%s", EnumToString(mode));
      return 0;
  }
}
  
} // namespace Vector
} // namespace Anki
