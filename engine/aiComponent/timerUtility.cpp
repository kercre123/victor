/**
* File: timerUtility.h
*
* Author: Kevin M. Karol
* Created: 2/5/18
*
* Description: Keep track of information relating to the user facing
* timer utility
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/timerUtility.h"
#include "engine/aiComponent/timerUtilityDevFunctions.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include "util/console/consoleInterface.h"

#include <ctime>
#include <ratio>
#include <chrono>

namespace Anki {
namespace Vector {
  
namespace{
Anki::Vector::TimerUtility* sTimerUtility = nullptr;
  
  int GetUTCOffset() {
    time_t t = time(NULL);
    struct tm lt = {0};
    
    localtime_r(&t, &lt);
    
    return lt.tm_gmtoff;
  }
}

#if ANKI_DEV_CHEATS
CONSOLE_VAR(u32, kAdvanceTimerSeconds,   "TimerUtility.AdvanceTimerSeconds", 60);
CONSOLE_VAR(u32, kAdvanceTimerAndAnticSeconds,   "TimerUtility.AdvanceTimerAndAnticSeconds", 60);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimer(ConsoleFunctionContextRef context)
{ 
  AdvanceTimerBySeconds(kAdvanceTimerSeconds);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimerAndAntic(ConsoleFunctionContextRef context)
{  
  AdvanceTimerBySeconds(kAdvanceTimerAndAnticSeconds);
  AdvanceAnticBySeconds(kAdvanceTimerAndAnticSeconds);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimerBySeconds(int seconds)
{  
  if(sTimerUtility != nullptr){
    sTimerUtility->AdvanceTimeBySeconds(seconds);
  }
}



CONSOLE_FUNC(AdvanceTimer, "TimerUtility.AdvanceTimer");
CONSOLE_FUNC(AdvanceTimerAndAntic, "TimerUtility.AdvanceTimerAndAntic");
#endif // ANKI_DEV_CHEATS


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimerHandle::GetSteadyTime_s()
{
  using namespace std::chrono;
  auto time_s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
  return static_cast<int>(time_s);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TimerUtility
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::TimerUtility(const Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::TimerUtility){
  if( sTimerUtility != nullptr ) {
    PRINT_NAMED_WARNING("TimerUtility.Constructor.MultipleInstances","TimerUtility instance exists already");
  }
  sTimerUtility = this;
  
  _robot = &robot;

  // first in list is vector timer
  _allTimers.push_front({});
  
  // subscribe to alexa timers
  auto* messageHandler = robot.GetContext()->GetRobotManager()->GetMsgHandler();
  
  _handles.push_front(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::alexaAlerts,
                                            std::bind(&TimerUtility::OnAlexaAlerts, this, std::placeholders::_1)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::~TimerUtility()
{
  sTimerUtility = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::SharedHandle TimerUtility::StartTimer(int timerLength_s)
{
  auto& vectorTimer = *_allTimers.begin();
  ANKI_VERIFY(vectorTimer == nullptr,
              "TimerUtility.StartTimer.TimerAlreadySet", 
              "Current design says we don't overwrite timers - remove this verify if that changes");
  vectorTimer.reset( new TimerHandle(timerLength_s) );
  return vectorTimer;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::ClearVectorTimer()
{
  _allTimers.begin()->reset();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::ClearRingingTimers()
{
  PRINT_NAMED_WARNING("WHATNOW","Clearing ringing timers");
  ANKI_VERIFY( !_allTimers.empty(), "WHATNOW", "" );
  auto it = _allTimers.begin();
  if( (*it) != nullptr && (*it)->GetTimeRemaining_s() == 0 ) {
    it->reset(); // reset vector timer;
  }
  ++it;
  PRINT_NAMED_WARNING("WHATNOW","Clearing ringing alexa timers");
  std::vector<int> clearedAlexaTimers;
  int cnt=1;
  for( ; it != _allTimers.end(); ) {
    ANKI_VERIFY( (*it) != nullptr, "WHATNOW", "idx %d is null (siz=%zu", cnt,_allTimers.size());
    ++cnt;
    if( (*it) != nullptr && (*it)->GetTimeRemaining_s() == 0 ) {
      clearedAlexaTimers.push_back( (*it)->GetAlexaID() );
      it = _allTimers.erase(it);
    } else {
      ++it;
    }
  }
  if( !clearedAlexaTimers.empty() ) {
    NotifyAlexaOfClearedTimers( clearedAlexaTimers );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::ClearAllTimers()
{
  auto it = _allTimers.begin();
  it->reset(); // reset vector timer;
  ++it;
  std::vector<int> clearedAlexaTimers;
  for( ; it != _allTimers.end(); ) {
    clearedAlexaTimers.push_back( (*it)->GetAlexaID() );
    it = _allTimers.erase(it);
  }
  NotifyAlexaOfClearedTimers( clearedAlexaTimers );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::NotifyAlexaOfClearedTimers(const std::vector<int>& timers)
{
  RobotInterface::AlexaAlertsCancelled msg;
  msg.alertIDs.fill(0);
  for( int i=0; i<std::min(msg.alertIDs.size(), timers.size()); ++i ) {
    msg.alertIDs[i] = timers[i];
  }
  PRINT_NAMED_WARNING("WHATNOW", "engine cancelling %zu timers", timers.size());
  _robot->SendRobotMessage<RobotInterface::AlexaAlertsCancelled>( msg );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::SharedHandle TimerUtility::GetSoonestTimer() const
{
  auto it = _allTimers.begin();
  SharedHandle soonest = *it;
  int soonestTime = (soonest != nullptr) ? soonest->GetTimeRemaining_s() : std::numeric_limits<int>::max();
  for( ; it != _allTimers.end(); ++it ) {
    if( (*it) != nullptr && (*it)->GetTimeRemaining_s() < soonestTime ) {
      soonestTime = (*it)->GetTimeRemaining_s();
      soonest = (*it);
    }
  }
  return soonest;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimerUtility::GetSteadyTime_s() const
{
  using namespace std::chrono;
  auto time_s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
  return static_cast<int>(time_s);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::OnAlexaAlerts( const AnkiEvent<RobotInterface::RobotToEngine>& msg )
{
  // todo: if the timer behavior is running and an alert is removed, stop the behavior
  // (usually the timer is stopped from engine instead of from alexa)
  auto& alerts = msg.GetData().Get_alexaAlerts();
  
  // to find what alerts were cancelled
  std::set<int> oldAlerts;
  std::set<int> newAlerts;
  std::map<int,int> oldTimes;
  for( auto& alert : _allTimers ) {
    if( alert ) {
      oldAlerts.insert(alert->GetAlexaID());
      oldTimes[alert->GetAlexaID()] = alert->GetTimeRemaining_s();
    }
  }
  
  
  {
    auto tmp = _allTimers.begin();
    ++tmp;
    _allTimers.erase(tmp, _allTimers.end());
  }
  
  std::map<int,int> newTimes;
  static_assert(sizeof(alerts.alertIDs) / sizeof(alerts.alertIDs[0]) == 8, "");
  for( int i=0; i<8; ++i ) {
    // "null terminated"
    if( alerts.alertIDs[i] == 0 ) {
      break;
    }
    const int alertID = alerts.alertIDs[i];
    const int time_s = alerts.alertTimes[i] + GetUTCOffset();
    const bool isAlarm = alerts.isAlarm[i];
    newTimes[alertID] = time_s;
    PRINT_NAMED_WARNING("WHATNOW", "engine received alert id=%d, t=%d, isAlarm=%d (std=%d, sys=%d)", alertID, time_s, isAlarm, GetSteadyTime_s(), GetEpochTime_s());
    int duration_s = std::max(time_s - GetEpochTime_s(), 0);
    auto newHandle = std::make_shared<TimerHandle>(duration_s);
    //newHandle->SetTimeSinceEpoch( time_s );
    newHandle->SetAlexaID(alertID);
    newHandle->SetIsAlarm(isAlarm);
    _allTimers.push_back( newHandle );
    
    newAlerts.insert(alertID);
  }
  
  std::vector<int> removedAlerts;
  std::vector<int> addedAlerts;
  std::set_difference(oldAlerts.begin(), oldAlerts.end(), newAlerts.begin(), newAlerts.end(),
                      std::inserter(removedAlerts, removedAlerts.begin()));
  std::set_difference(newAlerts.begin(), newAlerts.end(), oldAlerts.begin(), oldAlerts.end(),
                      std::inserter(addedAlerts, addedAlerts.begin()));
  for( int old : oldAlerts ) {
    PRINT_NAMED_WARNING("WHATNOW", "old=%d", old);
  }
  for( int old : newAlerts ) {
    PRINT_NAMED_WARNING("WHATNOW", "new=%d", old);
  }
  for( int old : removedAlerts ) {
    PRINT_NAMED_WARNING("WHATNOW", "removed=%d", old);
  }
  
  // find the earliest
  _cancelTime = 0;
  int minTime = std::numeric_limits<int>::max();
  bool hasOneToCancel = false;
  for( int idx : removedAlerts ) {
    if( oldTimes[idx] <= minTime ) {
      hasOneToCancel = true;
      minTime = oldTimes[idx];
    }
  }
  if( hasOneToCancel ) {
    PRINT_NAMED_WARNING("WHATNOW", "Cancelling at time %d", minTime);
    _cancelTime = minTime;
  }
  
  // find the earliest set time
  _setTime = 0;
  minTime = std::numeric_limits<int>::max();
  bool hasOneToSet = false;
  for( int idx : addedAlerts ) {
    if( newTimes[idx] <= minTime ) {
      hasOneToSet = true;
      minTime = newTimes[idx];
    }
  }
  if( hasOneToSet ) {
    PRINT_NAMED_WARNING("WHATNOW", "Setting at time %d", minTime);
    _setTime = minTime;
  }
  
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimerUtility::GetEpochTime_s() const
{
  using namespace std::chrono;
  auto time_s = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
  return static_cast<int>(time_s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerUtility::HasPendingCancel(int* remaining_s) const
{
  if( _cancelTime>0 ) {
    if( remaining_s != nullptr) {
      *remaining_s = _cancelTime;
    }
    return true;
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerUtility::HasPendingSet(int* remaining_s) const
{
  if( _setTime>0 ) {
    if( remaining_s != nullptr) {
      *remaining_s = _setTime;
    }
    return true;
  } else {
    return false;
  }
}


} // namespace Vector
} // namespace Anki
