/**
 * File: stepTimers
 *
 * Author: raul
 * Created: 07/15/14
 *
 * Description: Set of functions and structs to add 'Steps' whose time will be measured and can be printed 
 * for performance evaluations.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Util_StepTimers_H__
#define __Util_StepTimers_H__

#include "util/helpers/noncopyable.h"
#include "util/global/globalDefinitions.h"
#include <string>

#if defined(ANKI_PROFILING_ENABLED) && ANKI_PROFILING_ENABLED
#  define ANKI_RUN_STEP_TIMERS 1
#else
#  define ANKI_RUN_STEP_TIMERS 0 // ANKI_DEVELOPER_CODE
#endif

namespace Anki{ namespace Util {
namespace Time {
#if ANKI_RUN_STEP_TIMERS

// adds a step whose time starts counting immediately
void PushTimedStep(const char* name);

// pops all timed steps up to the given one (the deepest one that matches the name)
void PopTimedStep(const char* name);

// pops last step
void PopTimedStep();

// return the time the current step has taken, in milliseconds
double GetTimeMillis();

// Print all current steps
void PrintTimedSteps(int levels);

// Print as deep as it goes, but only expand nodes that took more than
// 10% of their parents time
void PrintTimedSteps();

// Clear everything, removing timers and memory. Resets time on ROOT as well
void ClearSteps();

// This step removes itself automatically when it goes out of scope
struct ScopedStep : Anki::Util::noncopyable
{
  // constructor
  inline ScopedStep(const char* name) : _name(name) { PushTimedStep(name); }
  inline virtual ~ScopedStep() { PopTimedStep(_name.c_str()); }
  
private:
  std::string _name;
};

#else // ANKI_RUN_STEP_TIMERS

inline void PushTimedStep(const char*) {}
inline void PopTimedStep(const char*) {}
inline void PopTimedStep() {}
inline double GetTimeMillis() {return -1.0;}
inline void PrintTimedSteps(int) {}
inline void PrintTimedSteps() {}
inline void ClearSteps() {}

struct ScopedStep : Anki::Util::noncopyable
{
  // constructor
  inline ScopedStep(const char*) {}
  inline virtual ~ScopedStep() {}
};

#endif // ANKI_RUN_STEP_TIMERS

};
};
} // namespace

#endif // __Util_StepTimers_H__
