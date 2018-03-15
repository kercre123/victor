/**
 * File: xythetaStates.h
 *
 * Author: Michael Willett
 * Created: 2018-03-13
 *
 * Description: basic state definitions for xytheta planner
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __PLANNING_BASESTATION_XYTHETAPLANNER_DEFINITIONS_H__
#define __PLANNING_BASESTATION_XYTHETAPLANNER_DEFINITIONS_H__


#include "coretech/common/shared/types.h"
#include "json/json-forwards.h"
#include "util/math/math.h"
#include <string>


namespace Anki
{

namespace Util {
class JsonWriter;
}

namespace Planning
{

#define THETA_BITS 4
#define MAX_XY_BITS 14

using GraphTheta = uint8_t;
using GraphXY    = int16_t;

union StateID;

// descritized state for quick lookup in state graph
class GraphState
{
  friend std::ostream& operator<<(std::ostream& out, const GraphState& state);
public:

  GraphState() : x(0), y(0), theta(0) {};
  GraphState(StateID sid);
  GraphState(GraphXY x, GraphXY y, GraphXY theta) : x(x), y(y), theta(theta) {};

  // returns true if successful
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  bool operator==(const GraphState& other) const;
  bool operator!=(const GraphState& other) const;

  StateID GetStateID() const;

  GraphXY x;
  GraphXY y;
  GraphTheta theta;
};

// bit field representation that packs into an int useful for checking the open and closed sets for expanded states
// NOTE: this is separated from State in case state needs to contain more information than the 32-bit form can hold,
//       and to reduce overhead of unpacking non-byte aligned data 
union StateID
{
  StateID() : v(0) {}
  StateID(const StateID& other) : v(other.v) {}
  StateID(const GraphState& state) : s(state) {}

  // Allow for conversion freely to/from int
  StateID(u32 val) : v(val) {}
  operator u32()       { return v; }
  operator u32() const { return v; }

  struct S
  {
    S() : theta(0), x(0), y(0) {}
    explicit S(const GraphState& state) : S( state.GetStateID().s ) {}

    unsigned int theta : THETA_BITS;
    signed int x : MAX_XY_BITS;
    signed int y : MAX_XY_BITS;
  };

  S   s;
  u32 v;
};

static_assert(sizeof(StateID::S) == sizeof(u32), "StateID must pack into 32 bits");

// continuous states are set up with the exact state being at the
// bottom left of the cell 
// TODO:(bn) think more about that, should probably add half a cell width
class State_c
{
  friend std::ostream& operator<<(std::ostream& out, const State_c& state);
public:
  State_c() : x_mm(0), y_mm(0), theta(0) {};
  State_c(float x, float y, float theta) : x_mm(x), y_mm(y), theta(theta) {};

  // returns true if successful
  bool Import(const Json::Value& config);
  void Dump(Util::JsonWriter& writer) const;

  float x_mm;
  float y_mm;
  float theta;
};

}
}

#endif
