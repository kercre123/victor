/**
 * File: stackCycleMonitor.cpp
 *
 * Author: ross
 * Created: 2018-06 26
 *
 * Description: Monitors the stack for cycles, which usually indicate that the robot is looking stupid.
 *              For now, this is only looking for _rapid_ cycles, which usually indicates a bug, but eventually
 *              we may consider longer loops.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/stackMonitors/stackCycleMonitor.h"

#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/container/circularBuffer.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

namespace {
  constexpr unsigned int kCapacity = 5; // see comment in NotifyOfChange
  constexpr unsigned int kMaxTicks = 2; // ditto
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StackCycleMonitor::StackCycleMonitor()
{
  _lastTick = 0;
  _recentBehaviors.Reset( kCapacity );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackCycleMonitor::NotifyOfChange( BehaviorExternalInterface& bei,
                                        const std::vector<IBehavior*>& stack,
                                        const BehaviorStack* stackComponent )
{
  size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  
  // We could be clever here in detecting cycles or common subsequences. For now, this is simple.
  // Clear the history whenever a behavior last for a few ticks, and if not, search for AAAAA, ABABA,
  // or variations of these in the history. The pattern AAAAA implies that behavior A delegates to B,
  // but behavior B cancels self or doesn't delegate (and behaviorAlwaysDelegates).
  // If kMaxTicks>1, patterns like AABAABA match as well
  if( currTick - _lastTick <= kMaxTicks ) {
    if( CheckForCycle() ) {
      std::string behaviorA = (_recentBehaviors[0] != nullptr) ? _recentBehaviors[0]->GetDebugLabel() : "null";
      std::string behaviorB = (_recentBehaviors[1] != nullptr) ? _recentBehaviors[1]->GetDebugLabel() : "null";
      // intentional spam. deal with it
      if( behaviorA == behaviorB ) {
        PRINT_NAMED_WARNING( "StackCycleMonitor.NotifyOfChange.CycleDetected",
                             "A cycle was detected. %s is repeatedly delegating to something that ends immediately",
                             behaviorA.c_str() );
      } else {
        PRINT_NAMED_WARNING( "StackCycleMonitor.NotifyOfChange.CycleDetected",
                             "A cycle was detected between %s and %s",
                             behaviorA.c_str(), behaviorB.c_str() );
      }
      // todo: now might be a good time to switch to a reliable behavior. for now, we just need an easy-to-find
      // statement in the log to help identify another possible cause of a "frozen robot"
    }
  } else {
    // a behavior lasted more than kMaxTicks, so start over
    _recentBehaviors.clear();
  }
  
  // add top of stack to the circ buffer
  const IBehavior* topOfStack = stackComponent->GetTopOfStack();
  if( topOfStack != nullptr ) {
    _recentBehaviors.push_back( topOfStack );
  } else {
    // also look for cycles between A0A0A0
    if( (_recentBehaviors.size() > 0) && (_recentBehaviors.back() != nullptr) ) {
      _recentBehaviors.push_back( nullptr );
    }
  }
  
  _lastTick = currTick;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StackCycleMonitor::CheckForCycle() const
{
  bool patternMatches = false;
  if( _recentBehaviors.size() == _recentBehaviors.capacity() ) {
    static_assert( kCapacity & 1, "Expecting odd capacity" );
    static_assert( kCapacity > 2, "Expecting at least 3" );
    const IBehavior* behaviorA = _recentBehaviors[0];
    const IBehavior* behaviorB = _recentBehaviors[1];
    patternMatches = true;
    for( int i=2; patternMatches && (i<_recentBehaviors.size()-1); i+=2 ) {
      patternMatches &= (_recentBehaviors[i] == behaviorA) &&
                        (_recentBehaviors[i+1] == behaviorB);
    }
    patternMatches &= (behaviorA == _recentBehaviors.back());
  }
  return patternMatches;
}

}
}
