/**
 * File: robotActionParams.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-01-10
 *
 * Description: Robot action params to hold motion profile constraints
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/planning/engine/robotActionParams.h"
#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

using namespace std;

namespace Anki {
namespace Planning {


RobotActionParams::RobotActionParams()
{
  Reset();
}

void RobotActionParams::Reset()
{
  // TODO:(bn) get this passed in somehow during a constructor somehweres. its in common config
  const f32 WHEEL_BASE_MM = 48.f;

  halfWheelBase_mm = WHEEL_BASE_MM * 0.5f;
  maxVelocity_mmps = 60.0;
  maxReverseVelocity_mmps = 25.0;
  oneOverMaxVelocity = 1.0 / maxVelocity_mmps; 
}

void RobotActionParams::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("halfWheelBase_mm", halfWheelBase_mm);
  writer.AddEntry("maxVelocity_mmps", maxVelocity_mmps);
  writer.AddEntry("maxReverseVelocity_mmps", maxReverseVelocity_mmps);
}

bool RobotActionParams::Import(const Json::Value& config)
{
  Reset();

  try {

    if( config.isNull() ) {
      PRINT_NAMED_ERROR("RobotActionParams.Import.Null", "config value is null");
      return false;
    }
  
    halfWheelBase_mm = config.get("halfWheelBase_mm", halfWheelBase_mm).asFloat();
    maxVelocity_mmps = config.get("maxVelocity_mmps", maxVelocity_mmps).asFloat();
    maxReverseVelocity_mmps = config.get("maxReverseVelocity_mmps", maxReverseVelocity_mmps).asFloat();

    if( NEAR_ZERO( maxVelocity_mmps ) ) {
      PRINT_NAMED_ERROR("RobotActionParams.Import.BadVelocity", "maxVelocity_mmps of %f too close to 0",
                        maxVelocity_mmps);
      oneOverMaxVelocity = 1.0f;
      return false;
    }

    oneOverMaxVelocity = 1.0 / maxVelocity_mmps;
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("RobotActionParams.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

}
}
