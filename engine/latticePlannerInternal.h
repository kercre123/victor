/**
 * File: latticePlannerInternal.h
 *
 * Author: Brad Neuman
 * Created: 2015-09-16
 *
 * Description: This is the implementation of cozmo interface to the xytheta lattice planner in coretech. 
 *  It does full collision avoidance and planning in (x,y,theta) space.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __LATTICEPLANNER_INTERNAL_H__
#define __LATTICEPLANNER_INTERNAL_H__

#include "util/helpers/noncopyable.h"

#include <thread>
#include <condition_variable>


// adds a bunch of noisy prints related to threading
#define LATTICE_PLANNER_THREAD_DEBUG 0


namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerInternal : private Util::noncopyable
{
public:

  LatticePlannerInternal(Robot* robot, Util::Data::DataPlatform* dataPlatform, const LatticePlanner* parent);
  ~LatticePlannerInternal();

  template<typename T>
  void HandleMessage(const T& msg);

  // by default, this planner will run in a thread. If it is set to be synchronous, it will not
  void SetIsSynchronous(bool val);

  // imports and pads obstacles
  void ImportBlockworldObstaclesIfNeeded(const bool isReplanning, const ColorRGBA* vizColor = nullptr);

  EComputePathStatus StartPlanning(const Pose3d& startPose, bool forceReplanFromScratch);

  EPlannerStatus CheckPlanningStatus() const;

  bool GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path, Planning::GoalID& selectedTargetIndex);
  
  // Compares against context's env, returns true if segments in path comprise a safe plan.
  // Clears and fills in a list of path segments whose cumulative penalty doesnt exceed the max penalty
  bool CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const;

  void StopPlanning();

  void worker();

  // run the planner until it stops, update internal status
  void DoPlanning();

  Robot*                _robot;
  BlockWorld*           _blockWorld;
  const LatticePlanner* _parent;
  xythetaPlannerContext _context;
  xythetaPlanner        _planner;

  int                   _searchNum = 0;               // number of searches
  TimeStamp_t           _timeOfLastObjectsImport = 0; // the last timestamp at which blockworld objects were imported
  GoalID                _selectedGoalID = 0;          // the chosen best goal out of _targetPoses_orig
  xythetaPlan           _totalPlan;                   // current plan if the planner has already run
  std::vector<Pose3d>   _targetPoses_orig;            // possible goals before converting to GoadID and loading to _context

  std::string                       _pathToEnvCache;  // where to dump _context for debugging
  std::vector<Signal::SmartHandle>  _signalHandles;   // external interface signals

  // thread handling
  bool                        _isSynchronous = false; // if true, always block when starting planning to return in the same tick
  std::thread*                _plannerThread = nullptr;
  std::recursive_mutex        _contextMutex;
  std::condition_variable_any _threadRequest;

  volatile bool           _timeToPlan            = false;                 // start planning now if it isn't already
  volatile bool           _runPlanner            = true;                  // passed in to the planner to allow it to stop early
  volatile bool           _stopThread            = false;                 // clean up and stop the thread (entirely)
  volatile bool           _plannerRunning        = false;                 // toggled by the planner thread when it is crunching
  volatile EPlannerStatus _internalComputeStatus = EPlannerStatus::Error; // this is also locked by _contextMutex, due to laziness
  volatile int            _msToBlock             = 0;                     // wait this many extra milliseconds to simulate a slower planner (test only)
};

} // Cozmo
} // Anki

#endif