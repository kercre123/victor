/**
 * File: AIInformationAnalyzer
 *
 * Author: Raul
 * Created: 09/13/16
 *
 * Description: Module for the AI that processes information from sensors, memory map, or whatever source of information
 * and stores the results to be used by behaviors.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"

// processes
#include "engine/aiComponent/aiInformationAnalysis/informationAnalysisProcesses/aiInfoAnalysisProcCalculateInterestingRegions.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/navMap/iNavMap.h"
#include "engine/robot.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#include <type_traits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIInformationAnalyzer::AIInformationAnalyzer()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::InformationAnalyzer)
{
  // register the callbacks
  _processes[EProcess::CalculateInterestingRegions]._callback = &AIInfoAnalysisProcCalculateInterestingRegions;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::InitDependent(Cozmo::Robot* robot, const AICompMap& dependentComps)
{
  _robot = robot;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::UpdateDependent(const AICompMap& dependentComps)
{
  // iterate every process and run the ones that need to
  for( const auto& infoPair : _processes )
  {
    const ProcessInfo& info = infoPair.second;
    const bool shouldRun = ShouldRun(info);
    if ( shouldRun )
    {
      // run the process
      if ( info._callback ) {
        info._callback(*this, *_robot);
      } else {
        PRINT_NAMED_ERROR("AIInformationAnalyzer.Update","Process wants to run, but no code provided");
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::AddEnableRequest(EProcess process, const std::string& requesterID)
{
  // add to the proper map
  const bool addedOk = AddRequest( _processes[process]._enableRequests, requesterID );
  
  // warn if it was duplicated
  if ( !addedOk ) {
    PRINT_NAMED_WARNING("AIInformationAnalyzer.AddEnableRequest.DuplicatedID",
      "%s had already requested enabling %d",
      requesterID.c_str(),
      std::underlying_type<EProcess>::type(process));
  }
  else
  {
    PRINT_CH_INFO("AIInfoAnalysis", "AIInformationAnalyzer.AddEnableRequest",
      "'%s' requested to enable process '%s'", requesterID.c_str(), AIInformationAnalysis::StringFromEProcess(process));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::RemoveEnableRequest(EProcess process, const std::string& requesterID)
{
  // remove from the proper map (I expect an entry to be there, so calling std::map[] should be fine)
  const bool removed = RemoveRequest( _processes[process]._enableRequests, requesterID );
  
  // warn if not removed
  if ( !removed ) {
    PRINT_NAMED_WARNING("AIInformationAnalyzer.AddEnableRequest.IDNotFound",
      "%s had not requested enabling %d before or had already removed the request.",
      requesterID.c_str(),
      std::underlying_type<EProcess>::type(process));
  }
  else
  {
    PRINT_CH_INFO("AIInfoAnalysis", "AIInformationAnalyzer.RemoveEnableRequest",
      "'%s' removed the request to enable process '%s'", requesterID.c_str(), AIInformationAnalysis::StringFromEProcess(process));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::AddDisableRequest(EProcess process, const std::string& requesterID)
{
  // add to the proper map
  const bool addedOk = AddRequest( _processes[process]._disableRequests, requesterID );
  
  // warn if it was duplicated
  if ( !addedOk ) {
    PRINT_NAMED_WARNING("AIInformationAnalyzer.AddDisableRequest.DuplicatedID",
      "%s had already requested disabling %d",
      requesterID.c_str(),
      std::underlying_type<EProcess>::type(process));
  }
  else
  {
    PRINT_CH_INFO("AIInfoAnalysis", "AIInformationAnalyzer.AddDisableRequest",
      "'%s' requested to disable process '%s'", requesterID.c_str(), AIInformationAnalysis::StringFromEProcess(process));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInformationAnalyzer::RemoveDisableRequest(EProcess process, const std::string& requesterID)
{
  // remove from the proper map (I expect an entry to be there, so calling std::map[] should be fine)
  const bool removed = RemoveRequest( _processes[process]._disableRequests, requesterID );
  
  // warn if not removed
  if ( !removed ) {
    PRINT_NAMED_WARNING("AIInformationAnalyzer.AddDisableRequest.IDNotFound",
      "%s had not requested disabling %d before or had already removed the request.",
      requesterID.c_str(),
      std::underlying_type<EProcess>::type(process));
  }
  else
  {
    PRINT_CH_INFO("AIInfoAnalysis", "AIInformationAnalyzer.RemoveDisableRequest",
      "'%s' removed the request to disable process '%s'", requesterID.c_str(), AIInformationAnalysis::StringFromEProcess(process));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIInformationAnalyzer::IsProcessRunning(EProcess process) const
{
  // find the process and ask if it should run
  const auto& matchIt = _processes.find(process);
  if ( matchIt != _processes.end() ) {
    const bool shouldRun = ShouldRun( matchIt->second );
    return shouldRun;
  }
  
  // not even registered
  PRINT_NAMED_WARNING("AIInformationAnalyzer.IsProcessRunning.ProcessNotRegistered",
    "Process '%s' is not even registered", AIInformationAnalysis::StringFromEProcess(process) );
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIInformationAnalyzer::ShouldRun(const ProcessInfo& info)
{
  const bool canRun = info._disableRequests.empty();      // canRun if disable request are 0
  const bool wantsToRun = !info._enableRequests.empty();  // wants to run if enable requests are NOT 0
  const bool shouldRun = canRun && wantsToRun;
  return shouldRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIInformationAnalyzer::AddRequest(RequestSet& toSet, const std::string& requesterID)
{
  // add request, return result of insertion
  auto retPair = toSet.insert( requesterID );
  const bool added = retPair.second;
  return added;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIInformationAnalyzer::RemoveRequest(RequestSet& fromSet, const std::string& requesterID)
{
  const size_t removeCount = fromSet.erase(requesterID);
  const bool removed = (removeCount > 0);
  return removed;
}


} // namespace Cozmo
} // namespace Anki
