/**
 * File: parseSalientPointsFromJson.h
 *
 * Author: Andrew Stein
 * Date:   3/7/2019
 *
 * Description: Helpers to parse salient points, e.g. from Json delivered from "offboard" neural nets.
 *
 * Copyright: Anki, Inc. 2019
 **/

#ifndef __Anki_Vision_ParseSalientPointsFromJson_H__
#define __Anki_Vision_ParseSalientPointsFromJson_H__

#include "clad/types/offboardVision.h"
#include "coretech/common/shared/types.h"
#include "json/json.h"

#include <list>

namespace Anki {
namespace Vision {
  
// Forward declaration
struct SalientPoint;
  
namespace JsonKeys {
  extern const char* const SalientPoints;
}

Result ParseSalientPointsFromJson(const Json::Value& jsonSalientPoints,
                                  const int imageRows, const int imageCols, const TimeStamp_t timestamp,
                                  std::list<SalientPoint>& salientPoints);

}
}

#endif /* __Anki_Vision_ParseSalientPointsFromJson_H__ */
