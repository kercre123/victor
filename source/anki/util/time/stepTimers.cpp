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

#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/time/stepTimers.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <pthread.h>
#include <stack>
#include <vector>

#if ANKI_RUN_STEP_TIMERS

namespace Anki{ namespace Util {
namespace Time {

// internal linkage
namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Step
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Step
{
  inline Step(const std::string& n) : _name(n) { }
  
  inline void Start() {
    _chronoStart = std::chrono::high_resolution_clock::now();
    _procStart = std::clock();
  }
  inline void TakeSplit()  {
    _chronoSplit = std::chrono::high_resolution_clock::now();
    _procSplit = std::clock();
  }

  inline double GetSplitTime() const {
    const double ret = std::chrono::duration_cast<std::chrono::milliseconds>(_chronoSplit - _chronoStart).count();
    return ret;
  }

  inline double GetSplitProcessTime() const {
    return 1000.0 * (_procSplit - _procStart) / CLOCKS_PER_SEC;
  }
  
  inline const std::string& GetName() const { return _name; }
  
  Step* AddStep(const std::string& stepName)
  {
    _subs.push_back( Step(stepName) );
    _subs.back().Start();
    return &_subs.back();
  }
  
  double Print(const std::string& prefix, int levels) const
  {
    --levels;
    printf("%s + %-16s:[%.0f ms]\n", prefix.c_str(), _name.c_str(), GetSplitTime());
    
    if ( levels>=0 && !_subs.empty() )
    {
      double subTime = 0;
      for(auto& sIt:_subs) {
        subTime += sIt.Print("  "+prefix, levels);
      }

      // This prints how long the substeps took, and the time parent spent not assigned to any child
//      printf("%s - %-16s: substeps [%.0f ms](DF %.0f)\n",
//        prefix.c_str(), _name.c_str(),
//        subTime,
//        GetSplitTime()-subTime);
    }

    const double elapsed = GetSplitTime();
    return elapsed;
  }

  unsigned int PrintPercent(const std::string& prefix, double parentTime, unsigned int maxWidth, bool dryRun) const {

    // if dryRun is true, just go through everything and return the
    // string width. Otherwise, do the prints. maxWidth argument is
    // only meaningful if dryRun is false

    double time = GetSplitTime();
    double percent = 100.0;
    if( parentTime > 0.0 ) {
      percent = 100.0 * ( time / parentTime );
    }

    double childTime = 0.0;
    for(const auto& sIt:_subs) {
      childTime += sIt.GetSplitTime();
    }

    const double expandThreshold = 10.0;

    unsigned int width = numeric_cast<unsigned int>( prefix.length() + _name.length() + 3 );
    int spacingWidth = maxWidth - width + 1;
    if(spacingWidth <= 0) {
      spacingWidth = 1;
    }
    std::string spacing(spacingWidth, ' ');

    if( percent > expandThreshold ) {
      if(!dryRun) {
        PRINT_NAMED_INFO("StepTimer",
                         "%s + %s%s[%4.0f ms] {%4.0f proc} (%4.0f self)",
                         prefix.c_str(),
                         _name.c_str(),
                         spacing.c_str(),
                         time,
                         GetSplitProcessTime(),
                         time - childTime);
      }

      unsigned int returnWidth = width;

      for(const auto& sIt:_subs) {
        unsigned int childWidth = sIt.PrintPercent("  "+prefix, time, maxWidth, dryRun);
        if( childWidth > returnWidth ) {
          returnWidth = childWidth;
        }
      }

      return returnWidth;
    }
    else {
      if(!dryRun) {
        PRINT_NAMED_INFO("StepTimer", "%s - %s%s[%4.0f ms] {%4.0f proc} ...",
                         prefix.c_str(),
                         _name.c_str(),
                         spacing.c_str(),
                         time,
                         GetSplitProcessTime());

        // if this is the main update loop try block, it should never be less than 10% of the time
        if( _name == "BS::Update::try" && parentTime > 10.0) {
          PRINT_NAMED_WARNING("StepTimer.SlowTryBlock",
                              "Try block took %.2f milliseconds (%.2f proc), parent time was %.2f",
                              time,
                              GetSplitProcessTime(),
                              parentTime);
        }
      }
      return width;      
    }
  }


  void DeepSort() {
    // assume everything is already "split taken". Sort subs based on
    // their times, then iterate and sort those. Sort in ascending orser
    std::sort( _subs.begin(), _subs.end(),
               [](const Step& lhs, const Step& rhs) {
                 return lhs.GetSplitTime() > rhs.GetSplitTime();
               });

    for(auto& sIt:_subs) {
      sIt.DeepSort();
    }
  }    

  void Clear()
    {
      _subs.clear();
    }

private:
  std::string       _name;

  // wall time
  std::chrono::high_resolution_clock::time_point _chronoStart;
  std::chrono::high_resolution_clock::time_point _chronoSplit;

  // process time
  std::clock_t _procStart;
  std::clock_t _procSplit;

  std::vector<Step> _subs;
};

static Step sMainStep("ROOT");
static pthread_t sMainThreadId;
static std::deque<Step*> sStepStack;

}; // annon-namespace

void PushTimedStep(const char* name)
{
  if ( sStepStack.empty() ) {
    sMainStep.Start();
    sStepStack.push_back( &sMainStep );

    // store the current thread, so that calls will only work on this thread
    sMainThreadId = pthread_self();
  }

  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) )
    return;

  Step* const newStep = sStepStack.back()->AddStep( name );
  sStepStack.push_back(newStep);
}

void PopTimedStep(const char* name)
{
  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ){
    return;
  }

  while( sStepStack.size() > 1 ) // 1 because we want ROOT to stay there  if it's there
  {
    // take split now that we are popping it
    Step* const curStep = sStepStack.back();
    curStep->TakeSplit();
    
    // pop from running ones
    sStepStack.pop_back();
    
    // and if it's the one we wanted, then stop popping more
    const bool done = ( curStep->GetName() == name );
    if ( done ) {
      break;
    }
  }
}

// pops last step
void PopTimedStep()
{
  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ) {
    return;
  }

  if ( sStepStack.size() > 1 ) // 1 because we want ROOT to stay there if it's there
  {
    // take split now that we are popping it
    Step* const curStep = sStepStack.back();
    curStep->TakeSplit();
    
    // pop from running ones
    sStepStack.pop_back();
  }
}

double GetTimeMillis()
{
  if ( sStepStack.empty() ) {
    return -1.0;
  }

  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ) {
    return -1.0;
  }

  sStepStack.back()->TakeSplit();
  return sStepStack.back()->GetSplitTime();
}


// Print all current steps
void PrintTimedSteps(int levels)
{
  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ) {
    return;
  }

  // Take a split on all running timers
  for( auto& stepPtrIt : sStepStack )
  {
    stepPtrIt->TakeSplit();
  }

  if ( !sStepStack.empty() )
  {
    sStepStack.back()->Print("", levels);
  }
}


// Print as deep as it goes, but only expand nodes that took more than
// 10% of their parents time
void PrintTimedSteps()
{
  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ) {
    return;
  }

  // Take a split on all running timers
  for( auto& stepPtrIt : sStepStack )
  {
    stepPtrIt->TakeSplit();
  }

  sStepStack.back()->DeepSort();

  if ( !sStepStack.empty() )
  {
    unsigned int width = sStepStack.back()->PrintPercent("", -1.0, 0, true);

    sStepStack.back()->PrintPercent("", -1.0, width, false);
  }
}


void ClearSteps()
{
  // only run if we are on the "main" timing thread
  if( !pthread_equal( sMainThreadId, pthread_self() ) ) {
    return;
  }

  // clear everything, including ROOT (it will be re-created later)
  if( sStepStack.size() > 1 ) {
    printf("WARNING: clearing stepTimers when stack is not empty!\n");
  }

  sStepStack.clear();  
  sMainStep.Clear();
}


};
};
} // namespace

#endif // ANKI_RUN_STEP_TIMERS

