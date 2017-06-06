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
#ifndef __Cozmo_Basestation_AIInformationAnalyzer_H__
#define __Cozmo_Basestation_AIInformationAnalyzer_H__

#include "aiInformationAnalysisProcessTypes.h"
// processes
#include "informationAnalysisProcesses/aiInfoAnalysisProcCalculateInterestingRegions.h"

#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"

#include <map>
#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIInformationAnalyzer
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIInformationAnalyzer
{
  // make processes friends so they can access private API not available to public (alternatively could use an interface)
  friend void AIInfoAnalysisProcCalculateInterestingRegions(AIInformationAnalyzer& analyzer, Robot& robot);

public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  using EProcess = AIInformationAnalysis::EProcess;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  AIInformationAnalyzer();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // runs the processes currently requested
  void Update(Robot& robot);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Process requests
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // (un)requests enabling/disabling a process from the given ID
  // how it works:
  // if there are DisableRequest --> process does NOT run
  // else if there are EnableRequests --> process does run
  // else --> process does NOT run
  void AddEnableRequest(EProcess process, const std::string& requesterID);     // request enable
  void RemoveEnableRequest(EProcess process, const std::string& requesterID);  // remove a request to enable
  void AddDisableRequest(EProcess process, const std::string& requesterID);    // request disable
  void RemoveDisableRequest(EProcess process, const std::string& requesterID); // remove a request to disable
  
  // returns true if the given process is currently set to run, false otherwise (regarldess of if it ever ran)
  bool IsProcessRunning(EProcess process) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Results of processes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns the last detected interesting regions
  const INavMemoryMap::BorderRegionVector& GetDetectedInterestingRegions() const { return _interestingRegions; }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // future: this could be a base class AnalyzerProcess, and have every process be a subclass of it, splitting
  // each feature into separate components. For the moment, without having many use cases, I think it's ok
  // to have them in this same class
  using RequestSet = std::set<std::string>;
  struct ProcessInfo
  {
    using Callback = std::function<void(AIInformationAnalyzer& analyzer, Robot& robot)>;
    // construction/destruction
    ProcessInfo() {}
    ProcessInfo(const Callback& callback) : _callback(callback) {}
    // attributes
    RequestSet _enableRequests;
    RequestSet _disableRequests;
    Callback   _callback;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Process helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns true if the given process should run, false otherwise
  static bool ShouldRun(const ProcessInfo& info);
  
  // helpers to modify enable/disable sets. Return success of operation (true) or failure (false)
  static bool AddRequest(RequestSet& toSet, const std::string& requesterID);
  static bool RemoveRequest(RequestSet& fromSet, const std::string& requesterID);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // table with which processes are enabled/disabled
  using ProcessInfoTable = std::map<EProcess, ProcessInfo>;
  ProcessInfoTable _processes;
  
  // storage for interesting regions process
  INavMemoryMap::BorderRegionVector _interestingRegions;
};

} // namespace Cozmo
} // namespace Anki

#endif //
