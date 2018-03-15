/**
 * File: xythetaStates.cpp
 *
 * Author: Michael Willett
 * Created: 2018-03-13
 *
 * Description: contains basic definitions for planner stuff
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "xythetaStates.h"
#include "xythetaEnvironment.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/jsonWriter/jsonWriter.h"

#include "coretech/common/engine/jsonTools.h"
#include "json/json.h"

namespace Anki {
namespace Planning {

GraphState::GraphState(StateID sid)
  :
  x(xythetaEnvironment::GetXFromStateID(sid)),
  y(xythetaEnvironment::GetYFromStateID(sid)),
  theta(xythetaEnvironment::GetThetaFromStateID(sid))
{
}

bool GraphState::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("GraphState.Import.Null", "config value is null");
      return false;
    }

    if(!JsonTools::GetValueOptional(config, "x", x) ||
       !JsonTools::GetValueOptional(config, "y", y) ||
       !JsonTools::GetValueOptional(config, "theta", theta)) {
      PRINT_NAMED_ERROR("GraphState.Import.Invalid", "could not parse state, dump follows");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("GraphState.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void GraphState::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("x", x);
  writer.AddEntry("y", y);
  writer.AddEntry("theta", theta);
}

StateID GraphState::GetStateID() const
{
  //return x | y<<MAX_XY_BITS | theta<<(2*MAX_XY_BITS);
  StateID sid;
  sid.s.x = x;
  sid.s.y = y;
  sid.s.theta = theta;
  return sid;
}

bool GraphState::operator==(const GraphState& other) const
{
  // TODO:(bn) use union?
  return x==other.x && y==other.y && theta==other.theta;
}

bool GraphState::operator!=(const GraphState& other) const
{
  // TODO:(bn) use union?
  return x!=other.x || y!=other.y || theta!=other.theta;
}


std::ostream& operator<<(std::ostream& out, const GraphState& state)
{
  return out<<'['<<(int)state.x<<','<<(int)state.y<<','<<(int)state.theta<<']';
}

std::ostream& operator<<(std::ostream& out, const State_c& state)
{
  return out<<'('<<state.x_mm<<','<<state.y_mm<<','<<state.theta<<')';
}

bool State_c::Import(const Json::Value& config)
{
  try {
    if( config.isNull() ) {
      PRINT_NAMED_ERROR("State_c.Import.Null", "config value is null");
      return false;
    }
  
    if(!JsonTools::GetValueOptional(config, "x_mm", x_mm) ||
       !JsonTools::GetValueOptional(config, "y_mm", y_mm) ||
       !JsonTools::GetValueOptional(config, "theta_rads", theta)) {
      PRINT_NAMED_ERROR("State_c.Import.Invalid", "could not parse State_c, dump follows");
      JsonTools::PrintJsonCout(config, 1);
      return false;
    }
  }
  catch( const std::exception&  e ) {
    PRINT_NAMED_ERROR("State_c.Import.Exception",
                      "json exception: %s",
                      e.what());
    return false;
  }

  return true;
}

void State_c::Dump(Util::JsonWriter& writer) const
{
  writer.AddEntry("x_mm", x_mm);
  writer.AddEntry("y_mm", y_mm);
  writer.AddEntry("theta_rads", theta);
}

} // namespace Planning
} // namespace Anki