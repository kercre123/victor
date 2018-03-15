/**
 * File: latticePlanner.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-04
 *
 * Description: Cozmo wrapper for the xytheta lattice planner in
 * coretech/planning
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/namedColors/namedColors.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/navMap/memoryMap/memoryMapToPlanner.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ObservableObject.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/planning/engine/xythetaEnvironment.h"
#include "coretech/planning/engine/xythetaPlanner.h"
#include "coretech/planning/engine/xythetaPlannerContext.h"
#include "json/json.h"
#include "latticePlanner.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/threading/threadPriority.h"
#include <chrono>
#include <condition_variable>
#include <thread>


// TODO:(bn) ANKI_DEVELOPER_CODE?
#define LATTICE_PLANNER_DUMP_ENV_TO_CACHE 1

// base padding on the robot footprint
#define LATTICE_PLANNER_ROBOT_PADDING 7.0

// base padding on the obstacles
#define LATTICE_PLANNER_OBSTACLE_PADDING 6.0

// amount of padding to subtract for replan-checks. MUST be less than the above values
#define LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT 5.0

// scaling for robot size when inserting for Configuration Space expansion (to avoid clipping corners)
#define LATTICE_PLANNER_ROBOT_EXPANSION_SCALING 1.2 

// whether this planner should consider multiple goals
// todo: probably have this decided in the calling functions based on the world state/goal types/etc
#define LATTICE_PLANNER_MULTIPLE_GOALS 1

#define DEBUG_REPLAN_CHECKS 0

// a global max so planning doesn't run forever
#define LATTICE_PLANNER_MAX_EXPANSIONS 30000000

// this is in units of seconds per mm, meaning the robot would drive X
// seconds out of the way to avoid having to drive 1 mm through the
// penalty
#define DEFAULT_OBSTACLE_PENALTY 0.1f

// how far (in mm) away from the path the robot needs to be before it gives up and plans a new path
#define PLAN_ERROR_FOR_REPLAN 20.0

// adds a bunch of noisy prints related to threading
#define LATTICE_PLANNER_THREAD_DEBUG 0

const f32 TERMINAL_POINT_TURN_SPEED = 2; //rad/s
const f32 TERMINAL_POINT_TURN_ACCEL = 10.f;
const f32 TERMINAL_POINT_TURN_DECEL = 10.f;

// The angular tolerance to use for the point turn that is appended at the end of every path
const f32 TERMINAL_POINT_TURN_ANGLE_TOL = DEG_TO_RAD(5.f);

namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerImpl : private Util::noncopyable
{
public:

  LatticePlannerImpl(Robot* robot, Util::Data::DataPlatform* dataPlatform, const LatticePlanner* parent)
    : _robot(robot)
    , _blockWorld(&robot->GetBlockWorld())
    , _planner(_context)
    , _parent(parent)
  {
    if( dataPlatform == nullptr ) {
      PRINT_NAMED_ERROR("LatticePlanner.InvalidDataPlatform",
                        "LatticePlanner requires data platform to operate!");
    }
    else {
      Json::Value mprims;
      const bool success = dataPlatform->readAsJson(Util::Data::Scope::Resources,
                                                    "config/engine/cozmo_mprim.json",
                                                    mprims);
      if(success) {
        _context.env.Init(mprims);
      }
      else {
        PRINT_NAMED_ERROR("LatticePlanner.MotionPrimitiveJsonParseFailure",
                          "Failed to load motion primitives, Planner likely won't work.");
      }

      if( LATTICE_PLANNER_DUMP_ENV_TO_CACHE ) {
        const std::string envDumpDir("planner/contextDump/");
        _pathToEnvCache = dataPlatform->pathToResource(Util::Data::Scope::Cache, envDumpDir);

        if (!Util::FileUtils::CreateDirectory(_pathToEnvCache, false, true)) {
          PRINT_NAMED_ERROR("LatticePlanner.CreateDirFailed","%s", _pathToEnvCache.c_str());
        }

        std::string mprimFilename = "planner/contextDump/mprim.json";
        if( ! dataPlatform->writeAsJson(Util::Data::Scope::Cache, mprimFilename, mprims) ) {
          PRINT_NAMED_WARNING("LatticePlanner.MprimpCache", "could not write mprim object to cache");
        }
      }
    }

    if (nullptr != _robot && _robot->HasExternalInterface() )
    {
      auto helper = MakeAnkiEventUtil(*robot->GetExternalInterface(), *this, _signalHandles);
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::PlannerRunMode>();
    }


    _plannerThread = new std::thread( &LatticePlannerImpl::worker, this );
  }

  ~LatticePlannerImpl()
  {
    if( _plannerThread != nullptr ) {
      // need to stop the thread
      _stopThread = true;
      _threadRequest.notify_all(); // noexcept
      PRINT_CH_DEBUG("Planner", "LatticePlanner.DestroyThread.Join", "");
      try {
        _plannerThread->join();
        PRINT_CH_DEBUG("Planner", "LatticePlanner.DestroyThread.Joined", "");
      }
      catch (std::runtime_error& err) {
        PRINT_NAMED_ERROR("LatticePlannerImpl.Destroy.Exception",
                          "locking the context mutex threw: %s",
                          err.what());
        // this will probably crash when we call SafeDelete below
      }
      Util::SafeDelete(_plannerThread);
    }
  }

  template<typename T>
  void HandleMessage(const T& msg);

  // by default, this planner will run in a thread. If it is set to be synchronous, it will not
  void SetIsSynchronous(bool val);

  // imports and pads obstacles
  void ImportBlockworldObstaclesIfNeeded(const bool isReplanning, const ColorRGBA* vizColor = nullptr);

  EComputePathStatus StartPlanning(const Pose3d& startPose,
                                   bool forceReplanFromScratch);

  EPlannerStatus CheckPlanningStatus() const;

  bool GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path, Planning::GoalID& selectedTargetIndex);
  
  // Compares against context's env, returns true if segments in path comprise a safe plan.
  // Clears and fills in a list of path segments whose cumulative penalty doesnt exceed the max penalty
  bool CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const;

  void StopPlanning();

  void worker();

  // run the planner until it stops, update internal status
  void DoPlanning();

  Robot* _robot;
  BlockWorld*  _blockWorld;
  xythetaPlannerContext _context;
  xythetaPlanner _planner;
  xythetaPlan _totalPlan;
  std::vector<Pose3d> _targetPoses_orig;

  std::string _pathToEnvCache;

  int _searchNum = 0;

  // while planner thread is planning, it holds contextLock
  std::thread *_plannerThread = nullptr;
  std::recursive_mutex _contextMutex;
  std::condition_variable_any _threadRequest;
  volatile bool _timeToPlan = false;
  
  // this is a bool used to clean up and stop the thread (entirely)
  volatile bool _stopThread = false;

  // this is the bool passed in to the planner to allow it to stop early
  volatile bool _runPlanner = true;

  // This is toggled by the planner thread when it is crunching
  volatile bool _plannerRunning = false;

  // whether to run multi-goal planner (true) or just plan with respect to the nearest fatal-penalty-free goal
  bool _multiGoalPlanning = LATTICE_PLANNER_MULTIPLE_GOALS;
  
  // if true, always block when starting planning to return in the same tick
  bool _isSynchronous = false;

  // this is also locked by _contextMutex, due to laziness
  volatile EPlannerStatus _internalComputeStatus = EPlannerStatus::Error;

  const LatticePlanner* _parent;
  
  // the chosen best goal out of _targetPoses_orig
  GoalID _selectedGoalID = 0;
  
  // the last timestamp at which blockworld objects were imported
  TimeStamp_t _timeOfLastObjectsImport = 0;

  // useful for testing / debugging, wait this many extra milliseconds to simulate a slower planner
  volatile int _msToBlock = 0;

  std::vector<Signal::SmartHandle> _signalHandles;
};

//////////////////////////////////////////////////////////////////////////////// 
// LatticePlanner functions
//////////////////////////////////////////////////////////////////////////////// 

LatticePlanner::LatticePlanner(Robot* robot, Util::Data::DataPlatform* dataPlatform)
  : IPathPlanner("LatticePlanner")
{
  _impl = new LatticePlannerImpl(robot, dataPlatform, this);
}


LatticePlanner::~LatticePlanner()
{
  Util::SafeDelete(_impl);
}


EPlannerStatus LatticePlanner::CheckPlanningStatus() const
{
  return _impl->CheckPlanningStatus();
}

void LatticePlanner::StopPlanning()
{
  _impl->StopPlanning();
}

EComputePathStatus LatticePlanner::ComputePathHelper(const Pose3d& startPose,
                                                     const std::vector<Pose3d>& targetPoses)
{
  // This has to live in LatticePlanner instead of impl because it calls LatticePlanner::GetPlan at the end

  if( ! _impl->_contextMutex.try_lock() ) {

    // if the thread is actually computing, than thats an error (trying to compute while we are already
    // computing). But, it might just be the case that someone else has the lock for a bit, in which case we
    // should do a blocking lock. Note that there is no guarantee here that we don't block for an entire plan,
    // we just try to avoid it, if possible

    if( _impl->_timeToPlan || _impl->_plannerRunning ) {

      // thread is already running.
      PRINT_NAMED_WARNING("LatticePlanner.ComputePathHelper.AlreadyRunning",
                          "Tried to compute a new path, but the planner is already running! (timeToPlan %d, running %d)",
                          _impl->_timeToPlan,
                          _impl->_plannerRunning);
      return EComputePathStatus::Error;
    }
    else {
      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "ComputePathHelper: try lock failed, blocking");
      }

      _impl->_contextMutex.lock();
    }
  }

  std::lock_guard<std::recursive_mutex> lg(_impl->_contextMutex, std::adopt_lock);

  if( _impl->_timeToPlan ) {
    PRINT_NAMED_WARNING("LatticePlanner.ComputePathHelper.PreviousPlanRequested",
                        "timeToPlan already set, so another compute path call is already waiting to signal the planner");
    return EComputePathStatus::Error;
  }    

  
  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "ComputePathHelper: got lock");
  }


  /*
  const f32 Z_HEIGHT_DIFF_TOLERANCE = ROBOT_BOUNDING_Z * .2f;
  if(!NEAR(startPose.GetTranslation().z(), targetPose.GetTranslation().z(),  Z_HEIGHT_DIFF_TOLERANCE)) {
    PRINT_NAMED_ERROR("LatticePlannerImpl.GetPlan.DifferentHeights",
                      "Can't producea  plan for start and target on different levels (%f vs. %f)\n",
                      startPose.GetTranslation().z(), targetPose.GetTranslation().z());
    return PLAN_NEEDED_BUT_PLAN_FAILURE;
  }
   */
  
  // Save original target pose
  _impl->_targetPoses_orig = targetPoses;
  
  _impl->ImportBlockworldObstaclesIfNeeded(false);

  // Clear plan whenever we attempt to set goal
  _impl->_totalPlan.Clear();

  _impl->_context.goals_c.clear();
  _impl->_context.goals_c.reserve(targetPoses.size());
  
  for(GoalID goalID = 0; goalID<targetPoses.size(); ++goalID) {
    std::pair<GoalID, State_c> goal_cPair(std::piecewise_construct,
                                          std::forward_as_tuple(goalID),
                                          std::forward_as_tuple(targetPoses[goalID].GetTranslation().x(),
                                                                targetPoses[goalID].GetTranslation().y(),
                                                                targetPoses[goalID].GetRotationAngle<'Z'>().ToFloat()));
    if (_impl->_planner.GoalIsValid(goal_cPair)) {
      _impl->_context.goals_c.emplace_back(std::move(goal_cPair));
    }
  }
  

  if( _impl->_context.goals_c.empty() ) {

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug",
                    "ComputePathHelper: no valid goals, returning (release lock)");
    }

    return EComputePathStatus::Error;
  }

  EComputePathStatus res = ComputeNewPathIfNeeded(startPose, true);

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug",
                  "ComputePathHelper: release lock");
  }

  return res;
}

EComputePathStatus LatticePlanner::ComputePath(const Pose3d& startPose,
                                               const Pose3d& targetPose)
{
  std::vector<Pose3d> targetPoses = {targetPose};
  EComputePathStatus res = ComputePathHelper(startPose, targetPoses);
  return res;
}

EComputePathStatus LatticePlanner::ComputePath(const Pose3d& startPose,
                                               const std::vector<Pose3d>& targetPoses)
{
  // This has to live in LatticePlanner instead of impl because it calls LatticePlanner::GetPlan internally

  if( ! _impl->_contextMutex.try_lock() ) {

    // if the thread is actually computing, than thats an error (trying to compute while we are already
    // computing). But, it might just be the case that someone else has the lock for a bit, in which case we
    // should do a blocking lock. Note that there is no guarantee here that we don't block for an entire plan,
    // we just try to avoid it, if possible

    if( _impl->_timeToPlan || _impl->_plannerRunning ) {

      // thread is already running.
      PRINT_NAMED_WARNING("LatticePlanner.ComputePath.AlreadyRunning",
                          "Tried to compute a new path, but the planner is already running! (timeToPlan %d, running %d)",
                          _impl->_timeToPlan,
                          _impl->_plannerRunning);
      return EComputePathStatus::Error;
    }
    else {
      _impl->_contextMutex.lock();
    }
  }

  std::lock_guard<std::recursive_mutex> lg( _impl->_contextMutex, std::adopt_lock);

  if( _impl->_timeToPlan ) {
    PRINT_NAMED_WARNING("LatticePlanner.ComputePath.PreviousPlanRequested",
                        "timeToPlan already set, so another compute path call is already waiting to signal the planner");
    return EComputePathStatus::Error;
  }    

  _impl->ImportBlockworldObstaclesIfNeeded(false, &NamedColors::BLOCK_BOUNDING_QUAD);

  // either search among all goals or just one
  
  if( _impl->_multiGoalPlanning ) {
  
    // ComputePathHelper will only copy a goal from targetPoses to the LatticePlannerImpl context
    // if it's not in collision (wrt MAX_OBSTACLE_COST), so just pass all targetPoses to ComputePathHelper
    EComputePathStatus res = ComputePathHelper(startPose, targetPoses);
    return res;
    
  } else {
  
    // Select the closest goal without a soft penalty. If that doesnt exist, select the closest non-colliding goal

    size_t numTargetPoses = targetPoses.size();
    GoalID bestTargetID = 0;
    float closestDist2 = 0;
    bool found = false;
    
    for(auto maxPenalty : (float[]){0.01f, MAX_OBSTACLE_COST}) {
      
      bestTargetID = 0;
      closestDist2 = 0;
      for(GoalID i=0; i<numTargetPoses; ++i) {
        float dist2 = (targetPoses[i].GetTranslation() - startPose.GetTranslation()).LengthSq();
        
        if(!found || dist2 < closestDist2) {
          State_c target_c(targetPoses[i].GetTranslation().x(),
                           targetPoses[i].GetTranslation().y(),
                           targetPoses[i].GetRotationAngle<'Z'>().ToFloat());
          GraphState target(_impl->_context.env.State_c2State(target_c));
          
          if(_impl->_context.env.GetCollisionPenalty(target) < maxPenalty) {
            closestDist2 = dist2;
            bestTargetID = i;
            found = true;
          }
        }
      }
      
      if(found) {
        // defer to the single-goal version of this method using the closest non-colliding goal
        EComputePathStatus res = ComputePath(startPose, targetPoses[bestTargetID]);
        _selectedTargetIdx = bestTargetID;
        return res;
      }
    }
    
    PRINT_CH_INFO("Planner", "LatticePlanner.ComputePath.NoValidTarget",
                  "could not find valid target out of %lu possible targets",
                  (unsigned long)numTargetPoses);
    return EComputePathStatus::Error;
    
  }
}

EComputePathStatus LatticePlanner::ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                          bool forceReplanFromScratch)
{
  if( !  _impl->_contextMutex.try_lock() ) {
    // thread is already running, in this case just say no plan needed
    return EComputePathStatus::NoPlanNeeded;
  }

  std::lock_guard<std::recursive_mutex> lg( _impl->_contextMutex, std::adopt_lock);
  
  EComputePathStatus ret = _impl->StartPlanning(startPose, forceReplanFromScratch);

  return ret;
}

bool LatticePlanner::PreloadObstacles()
{
  _impl->ImportBlockworldObstaclesIfNeeded(false);
  return true;
}
  
bool LatticePlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _impl->CheckIsPathSafe(path, startAngle, validPath);
}

void LatticePlanner::GetTestPath(const Pose3d& startPose, Planning::Path &path, const PathMotionProfile* motionProfile)
{
  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  _impl->_context.start = currentRobotState;

  xythetaPlan plan;
  _impl->_planner.GetTestPlan(plan);
  PRINT_CH_INFO("Planner", "GetTestPath.Plan", "Plan from xytheta planner:");
  _impl->_context.env.PrintPlan(plan);

  Planning::Path outputPath;
  _impl->_context.env.AppendToPath(plan, outputPath);

  if( motionProfile != nullptr ) {
    ApplyMotionProfile(outputPath, *motionProfile, path);
  }
  else {
    path = outputPath;
  }

  PRINT_CH_INFO("Planner", "GetTestPath.Path", "Planning::Path:");
  path.PrintPath();
}


bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose, Planning::Path &path)
{
  GoalID waste;
  return _impl->GetCompletePath(currentRobotPose, path, waste);
}

bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                              Planning::Path &path,
                                              Planning::GoalID& selectedTargetIndex)
{
  auto ret = _impl->GetCompletePath(currentRobotPose, path, selectedTargetIndex);
  _selectedTargetIdx = selectedTargetIndex;
  return ret;
}

void LatticePlanner::SetIsSynchronous(bool val)
{
  _impl->SetIsSynchronous(val);
}

void LatticePlanner::SetArtificialPlannerDelay_ms(int ms)
{
  _impl->_msToBlock = ms;
  PRINT_CH_INFO("Planner", "LatticePlanner.SetDelay", "Adding %dms of artificial delay", ms);
}

//////////////////////////////////////////////////////////////////////////////// 
// LatticePlannerImpl functions
//////////////////////////////////////////////////////////////////////////////// 

void LatticePlannerImpl::DoPlanning()
{
  _internalComputeStatus = EPlannerStatus::Running;

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "DoPlanning: running replan...");
  }

  if( _msToBlock > 0 ) {
    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "DoPlanning: block for %d ms", _msToBlock);
    }

    // for testing / debugging to make the planner slow. We still want it to stop when requested, so
    // periodically check _runPlanner
    const int kMaxNumToBlock_ms = 10;
    int msBlocked = 0;

    while(msBlocked < _msToBlock && _runPlanner ) {
      const int thisBlock_ms = std::min( kMaxNumToBlock_ms, _msToBlock - msBlocked );
      std::this_thread::sleep_for( std::chrono::milliseconds(thisBlock_ms) );
      msBlocked += thisBlock_ms;
    }

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "DoPlanning: Finished blocking wait");
    }
  }

  using namespace std::chrono;
  
  high_resolution_clock::time_point start = high_resolution_clock::now();
  const bool result = _planner.Replan(LATTICE_PLANNER_MAX_EXPANSIONS, &_runPlanner);
  high_resolution_clock::time_point end = high_resolution_clock::now();

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "DoPlanning: replan finished");
  }

  auto duration_ms = duration_cast<std::chrono::milliseconds>(end - start);

  const std::string& eventName = result ? "robot.lattice_planner_success" : "robot.lattice_planner_failure";
  
  Util::sEventF(eventName.c_str(),
                {{DDATA, std::to_string(duration_ms.count()).c_str()}},
                "%d:%d",
                _planner.GetLastNumExpansions(),
                _planner.GetLastNumConsiderations());

  if(!result) {
    _internalComputeStatus = EPlannerStatus::Error;
  }
  else if(_planner.GetPlan().Size() != 0) {
    // at this point _totalPlan may contain the valid old plan during replanning

    // verify that the append will be correct
    if(_totalPlan.Size() > 0) {
      GraphState endState = _context.env.GetPlanFinalState(_totalPlan);
      if(endState != _planner.GetPlan().start_) {
        PRINT_STREAM_ERROR("LatticePlanner.PlanMismatch",
                           "trying to append a plan with a mismatching state!\n"
                           << "endState = " << endState << std::endl
                           << "next plan start = " << _planner.GetPlan().start_ << std::endl);

        PRINT_CH_DEBUG("Planner", "LatticePlannerImpl", "totalPlan_:");
        _context.env.PrintPlan(_totalPlan);

        PRINT_CH_DEBUG("Planner", "LatticePlannerImpl", "new plan:");
        _context.env.PrintPlan(_planner.GetPlan());

        _internalComputeStatus = EPlannerStatus::Error;
      }
    }

    // the selected goal
    _selectedGoalID = _planner.GetChosenGoalID();

    PRINT_CH_DEBUG("Planner", "LatticePlannerImpl", "old plan");
    _context.env.PrintPlan(_totalPlan);

    _totalPlan.Append( _planner.GetPlan() );

    PRINT_CH_DEBUG("Planner", "LatticePlannerImpl", "new plan");
    _context.env.PrintPlan(_planner.GetPlan());

    _internalComputeStatus = EPlannerStatus::CompleteWithPlan;
  }
  else {
    // size == 0
    _internalComputeStatus = EPlannerStatus::CompleteNoPlan;
  }
}

void LatticePlannerImpl::worker()
{
  Anki::Util::SetThreadName(pthread_self(), "LatticePlanner");
  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    std::cout << "hello from planner worker thread! I am object " << this
              << " running in thread " << std::this_thread::get_id() << std::endl;
  }

  while(!_stopThread) {

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "about to lock");
    }

    std::unique_lock<std::recursive_mutex> lock(_contextMutex);

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "got context lock in thread");
    }

    while( !_timeToPlan && !_stopThread ) {
      _threadRequest.wait(lock);

      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "got cv in thread: %d %d", _timeToPlan, _stopThread);
      }
    }

    if( _timeToPlan ) {
      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "running planner in thread!");
      }

      _plannerRunning = true;
      _timeToPlan = false;

      DoPlanning();

      _plannerRunning = false;
    }
  }

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    std::cout << "Planner worker thread returning: object" << this
              << " running in thread " << std::this_thread::get_id() << std::endl;
  }

}

template<>
void LatticePlannerImpl::HandleMessage(const ExternalInterface::PlannerRunMode& msg)
{
  SetIsSynchronous(msg.isSync);
}

void LatticePlannerImpl::SetIsSynchronous(bool val)
{
  _isSynchronous = val;
  PRINT_CH_INFO("Planner", "LatticePlanner.IsSync",
                "%s",
                val ? "true" : "false");
}

void LatticePlannerImpl::StopPlanning()
{
  if( _plannerThread != nullptr && _plannerRunning ) {
    _runPlanner = false;
  }
}

EPlannerStatus LatticePlannerImpl::CheckPlanningStatus() const
{
  // read-only access is safe here
  return _internalComputeStatus;
}


void LatticePlannerImpl::ImportBlockworldObstaclesIfNeeded(const bool isReplanning, const ColorRGBA* vizColor)
{

  if( ! _contextMutex.try_lock() ) {
    // thread is already running.
    PRINT_NAMED_WARNING("LatticePlanner.ImportBlockworldObstaclesIfNeeded.alreadyRunning",
                        "Tried to compute a new path, but the planner is already running!");
    return;
  }

  std::lock_guard<std::recursive_mutex> lg(_contextMutex, std::adopt_lock);

  float obstaclePadding = LATTICE_PLANNER_OBSTACLE_PADDING;
  if(isReplanning) {
    obstaclePadding -= LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT;
  }

  float robotPadding = LATTICE_PLANNER_ROBOT_PADDING;
  if(isReplanning) {
    robotPadding -= LATTICE_PLANNER_RPLAN_PADDING_SUBTRACT;
  }

  const TimeStamp_t timeOfLastChange = _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp();
  const bool didObjectsChange = (_timeOfLastObjectsImport < timeOfLastChange);
  
  if(!isReplanning ||  // get obstacles if theyve changed or we're not replanning
     didObjectsChange)
  {
    _timeOfLastObjectsImport = timeOfLastChange;
    
    // Configuration of memory map to check for obstacles
    constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithInterestingEdges =
    {
      {MemoryMapTypes::EContentType::Unknown               , true},
      {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
      {MemoryMapTypes::EContentType::ClearOfCliff          , true},
      {MemoryMapTypes::EContentType::ObstacleObservable    , true},
      {MemoryMapTypes::EContentType::ObstacleCharger       , true},
      {MemoryMapTypes::EContentType::ObstacleChargerRemoved, true},
      {MemoryMapTypes::EContentType::ObstacleProx          , true},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , true},
      {MemoryMapTypes::EContentType::InterestingEdge       , false},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithInterestingEdges),
      "This array does not define all types once and only once.");
      
      
    constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithNotInterestingEdges =
    {
      {MemoryMapTypes::EContentType::Unknown               , true},
      {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
      {MemoryMapTypes::EContentType::ClearOfCliff          , true},
      {MemoryMapTypes::EContentType::ObstacleObservable    , true},
      {MemoryMapTypes::EContentType::ObstacleCharger       , true},
      {MemoryMapTypes::EContentType::ObstacleChargerRemoved, true},
      {MemoryMapTypes::EContentType::ObstacleProx          , true},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , true},
      {MemoryMapTypes::EContentType::InterestingEdge       , true},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithNotInterestingEdges),
      "This array does not define all types once and only once.");

    constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithProx =
    {
      {MemoryMapTypes::EContentType::Unknown               , true},
      {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
      {MemoryMapTypes::EContentType::ClearOfCliff          , true},
      {MemoryMapTypes::EContentType::ObstacleObservable    , true },
      {MemoryMapTypes::EContentType::ObstacleCharger       , true},
      {MemoryMapTypes::EContentType::ObstacleChargerRemoved, true},
      {MemoryMapTypes::EContentType::ObstacleProx          , false},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , true},
      {MemoryMapTypes::EContentType::InterestingEdge       , true},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithProx),
      "This array does not define all types once and only once.");
    
    // GetNavMap Polys
    std::vector<ConvexPolygon> convexHulls;
    INavMap* memoryMap = _robot->GetMapComponent().GetCurrentMemoryMap();
    
    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithInterestingEdges, MemoryMapTypes::EContentType::InterestingEdge, convexHulls);    
    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithNotInterestingEdges, MemoryMapTypes::EContentType::NotInterestingEdge, convexHulls);
    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithProx, MemoryMapTypes::EContentType::ObstacleProx, convexHulls);    
   
    MemoryMapTypes::MemoryMapDataConstList observableObjectData;
    MemoryMapTypes::NodePredicate pred = 
      [](MemoryMapTypes::MemoryMapDataPtr d) -> bool
      {
          return (d->type == MemoryMapTypes::EContentType::ObstacleObservable);
      };
      
    memoryMap->FindContentIf(pred, observableObjectData);
    for (const auto& nodeData : observableObjectData) 
    {
      auto castPtr = MemoryMapData::MemoryMapDataCast<const MemoryMapData_ObservableObject>( nodeData );
      convexHulls.emplace_back( castPtr->boundingPoly );
    }

    // expand and force CHs to be clockwise for C-space expansion
    for(auto& hull : convexHulls) {
      hull.RadialExpand(obstaclePadding);
      hull.SetClockDirection(ConvexPolygon::CW);
    }  
    
    // clear old obstacles
    // note: (mrw) This is most definitely a hack. Right now VizManager does not enforce that new objects
    //       are inserted with unique IDs. Since the vizManager renders polygons as paths, for now, to 
    //       prevent ID collision with the robot path, always set our start index to be one higher the the 
    //       robot ID (this relies on any call the render the robot path to set its ID to the robot ID, 
    //       but that cannot be enforced here). See ticket (VIC-647) for generating unique ids in vizManager 
    //       to prevent future collisions.

    unsigned int startIdx = _robot->GetID() + 1;      
    if(vizColor != nullptr) {
      for (int i = startIdx; i <= _context.env.GetNumObstacles() + startIdx; ++i)
      {
        _robot->GetContext()->GetVizManager()->ErasePath(i);
      }
    }
    _context.env.ClearObstacles();

    unsigned int numAdded = 0;
    unsigned int vizID = startIdx;

    Planning::GraphTheta numAngles = (GraphTheta) _context.env.GetNumAngles();
    for(GraphTheta theta=0; theta < numAngles; ++theta) {

      float thetaRads = _context.env.GetTheta_c(theta);

      // Since the planner is given the driveCenter poses to be the robot's origin, compute the actual robot
      // origin given a driveCenter pose at (0,0,0) in order to get the actual bounding box of the robot.
      Pose3d robotDriveCenterPose = Pose3d(thetaRads, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f});
      Pose3d robotOriginPose;
      _robot->ComputeOriginPose(robotDriveCenterPose, robotOriginPose);
      
      // Get the robot polygon, and inflate it by a bit to handle error
      Poly2f robotPoly;
      robotPoly.ImportQuad2d(_robot->GetBoundingQuadXY(robotOriginPose, robotPadding)
                                    .Scale(LATTICE_PLANNER_ROBOT_EXPANSION_SCALING) );
      
      for(const auto& boundingPoly : convexHulls) {
        _context.env.AddObstacleWithExpansion(boundingPoly, robotPoly, theta, DEFAULT_OBSTACLE_PENALTY);

        // only draw the angle we are currently at
        if(vizColor != nullptr &&
           _context.env.GetTheta(_robot->GetPose().GetRotationAngle<'Z'>().ToFloat()) == theta
           // && !isReplanning
          ) {

          // TODO: manage the quadID better so we don't conflict
          // TODO:(bn) handle isReplanning with color??
          // _robot->GetContext()->GetVizManager()->DrawPlannerObstacle(isReplanning, vizID++ , expandedPoly, *vizColor);

          // TODO:(bn) figure out a good way to visualize the
          // multi-angle stuff. For now just draw the poly with
          // padding
          _robot->GetContext()->GetVizManager()->DrawPoly (
            vizID++, boundingPoly, *vizColor );
        }
        numAdded++;
      }

      // if(theta == 0) {
      //   PRINT_CH_INFO("Planner", "LatticePlannerImpl.ImportBlockworldObstaclesIfNeeded.ImportedObstacles",
      //                    "imported %d obstacles form blockworld",
      //                    numAdded);
      // }
    }

    // PRINT_CH_INFO("Planner", "LatticePlannerImpl.ImportBlockworldObstaclesIfNeeded.ImportedAngles",
    //                  "imported %d total obstacles for %d angles",
    //                  numAdded,
    //                  numAngles);
  }
  else {
    PRINT_CH_DEBUG("Planner", "LatticePlanner.ImportBlockworldObstaclesIfNeeded.NoUpdateNeeded",
                   "robot padding %f, obstacle padding %f , didBlocksChange %d",
                   robotPadding,
                   obstaclePadding,
                   didObjectsChange);
  }
}



EComputePathStatus LatticePlannerImpl::StartPlanning(const Pose3d& startPose,
                                                     bool forceReplanFromScratch)
{
  using namespace std;

  // double check that we have the mutex. It's recursive, so this adds a small performance hit, but is safer
  if( ! _contextMutex.try_lock() ) {
    // thread is already running.
    PRINT_NAMED_ERROR("LatticePlanner.StartPlanning.InternalThreadingError",
                      "Somehow failed to get mutex inside StartPlanning, but we should already have it at this point");
    return EComputePathStatus::Error;
  }

  std::lock_guard<std::recursive_mutex> lg(_contextMutex, std::adopt_lock);

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "StartPlanning: got lock");
  }


  State_c lastSafeState;
  xythetaPlan validOldPlan;
  size_t planIdx = 0;

  _context.forceReplanFromScratch = forceReplanFromScratch;

  DEV_ASSERT(startPose.IsRoot() || startPose.GetParent().IsRoot(),
             "LatticePlannerImpl.StartPlanning.StartPoseNotWrtRoot");
  
  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  //_robot->GetContext()->GetVizManager()->EraseAllQuads();
  if( ! _context.forceReplanFromScratch ) {
    ImportBlockworldObstaclesIfNeeded(true, &NamedColors::REPLAN_BLOCK_BOUNDING_QUAD);

    // planIdx is the number of plan actions to execute before getting to the starting point closest to start
    float offsetFromPlan = 0.0;
    planIdx = _context.env.FindClosestPlanSegmentToPose(_totalPlan,
                                                        currentRobotState,
                                                        offsetFromPlan);

    if(offsetFromPlan >= PLAN_ERROR_FOR_REPLAN) {
      PRINT_CH_INFO("Planner", "LatticePlanner.GetPlan.ForcePlan",
                    "Current state is %f away from the plan (planIdx %zu), forcing replan from scratch",
                    offsetFromPlan,
                    planIdx);
      _totalPlan.Clear();
    }
  }
  else {
    _totalPlan.Clear();
  }

  // TODO:(bn) param. This is linked to other uses of this parameter as well
  const float maxDistancetoFollowOldPlan_mm = 40.0;

  if(_context.forceReplanFromScratch ||
     ! _context.env.PlanIsSafe(_totalPlan,
                               maxDistancetoFollowOldPlan_mm,
                               static_cast<int>(planIdx),
                               lastSafeState,
                               validOldPlan)) {
    // at this point, we know the plan isn't completely safe. lastSafeState will be set to the furthest state
    // along the plan (after planIdx) which is safe. validOldPlan will contain a partial plan starting at
    // planIdx and ending at lastSafeState

    _totalPlan = validOldPlan;

    if(!_context.forceReplanFromScratch) {
      PRINT_CH_INFO("Planner", "LatticePlanner.GetPlan.OldPlanUnsafe",
                    "old plan unsafe! Will replan, starting from %zu, keeping %zu actions from oldPlan.",
                    planIdx, validOldPlan.Size());
    }

    PRINT_STREAM_INFO("LatticePlanner", "currentRobotState:"<<currentRobotState);

    // uncomment to print debugging info
    // impl_->env_.FindClosestPlanSegmentToPose(impl_->totalPlan_, currentRobotState, true);

    if(validOldPlan.Size() == 0) {
      // if we can't safely complete the action we are currently
      // executing, then valid old plan will be empty and we will
      // replan from the current state of the robot
      lastSafeState = currentRobotState;
    }


    _context.start = lastSafeState;

    if(!_planner.StartIsValid()) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ReplanIfNeeded.InvalidStart", "could not set start");
      return EComputePathStatus::Error;
    }
    else if(!_planner.GoalsAreValid()) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ReplanIfNeeded.InvalidGoals", "Goals may have moved into collision");
      return EComputePathStatus::Error;
    }
    else {
      // use real padding for re-plan
      ImportBlockworldObstaclesIfNeeded(false, &NamedColors::BLOCK_BOUNDING_QUAD);

      std::stringstream ss;
      ss <<  "from (" << lastSafeState.x_mm << ", " << lastSafeState.y_mm << ", " << lastSafeState.theta << ") to ";
      for(const auto& goalPair : _context.goals_c) {
        ss << (int)goalPair.first
           << ":(" << goalPair.second.x_mm
           << ", " << goalPair.second.y_mm
           << ", " << goalPair.second.theta
           << ") ";
      }
      PRINT_CH_INFO("Planner", "LatticePlanner.ReplanIfNeeded.Replanning", "%s", ss.str().c_str());

      _context.env.PrepareForPlanning();

      _searchNum++;
      PRINT_CH_INFO("Planner", "LatticePlannerImpl.GetPlan", "searchNum: %d", _searchNum);

      if( LATTICE_PLANNER_DUMP_ENV_TO_CACHE ) {
        std::stringstream filenameSS;
        filenameSS << _pathToEnvCache << "context_" << _searchNum << ".json";

        std::string filename = filenameSS.str();

        if(_searchNum == 1) {
          PRINT_CH_INFO("Planner", "LatticePlanner.EnvDump",
                        "dumping planner context to files like '%s'",
                        filename.c_str());
        }

        Util::JsonWriter contextDumpWriter(filename);
        _context.Dump(contextDumpWriter);
        contextDumpWriter.Close();
      }

      _runPlanner = true;

      if( _isSynchronous ) {
        PRINT_CH_INFO("Planner", "LatticePlanner.RunSynchronous.DoPlanning",
                      "Do planning now in StartPlanning...");        
        DoPlanning();
      }
      else {
        _timeToPlan = true;
        _internalComputeStatus = EPlannerStatus::Running;
        _threadRequest.notify_all();
      }
    }

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug",
                    "StartPlanning: return running (release lock)");
    }
    
    return EComputePathStatus::Running;
  }
  else {
#if DEBUG_REPLAN_CHECKS
    using namespace std;
    if(validOldPlan.Size() > 0) {
      std::cout<<"LatticePlanner: safely checked plan starting at action "<<planIdx<<" from "<<validOldPlan.start_
               <<" to "<<_context.env.GetPlanFinalState(validOldPlan)<< " goal = "<<_planner.GetGoal()
               <<" ("<<validOldPlan.Size()
               <<" valid actions, totalPlan_.Size = "<<_totalPlan.Size()<<")\n";
      std::cout<<"There are "<<_context.env.GetNumObstacles()<<" obstacles\n";
    }
    else {
      PRINT_CH_INFO("Planner", "LatticePlanner.EmptyOldPlan", "Plan safe, but validOldPlan is empty");
    }
#endif

    return EComputePathStatus::NoPlanNeeded;
  }
}

bool LatticePlannerImpl::GetCompletePath(const Pose3d& currentRobotPose,
                                         Planning::Path &path,
                                         Planning::GoalID& selectedTargetIndex)
{

  if( _internalComputeStatus == EPlannerStatus::Error ||
      _internalComputeStatus == EPlannerStatus::Running ) {
    return false;
  }

  selectedTargetIndex = _selectedGoalID;


  // consider trimming actions if the robot has moved past the beginning of the path
  DEV_ASSERT(currentRobotPose.IsRoot() || currentRobotPose.GetParent().IsRoot(),
             "LatticePlannerImpl.GetCompletePath.RobotPoseNotWrtRoot");
  State_c currentRobotState(currentRobotPose.GetTranslation().x(),
                            currentRobotPose.GetTranslation().y(),
                            currentRobotPose.GetRotationAngle<'Z'>().ToFloat());

  float offsetFromPlan = 0.0;
  int planIdx = Util::numeric_cast<int>(_context.env.FindClosestPlanSegmentToPose(_totalPlan,
                                                                                  currentRobotState,
                                                                                  offsetFromPlan) );

  PRINT_CH_INFO("Planner", "LatticePlanner.GetCompletePath.Offset",
                "Robot is %f from the plan, at index %d",
                offsetFromPlan,
                planIdx);

  if(offsetFromPlan >= PLAN_ERROR_FOR_REPLAN) {
    LOG_EVENT("LatticePlanner.GetCompletePath.RobotPositionError",
              "%04d %f",
              planIdx,
              offsetFromPlan);
    _totalPlan.Clear();
    _internalComputeStatus = EPlannerStatus::Error;
    return false;
  }

  PRINT_CH_DEBUG("Planner", "LatticePlannerImpl", "total path:");
  _context.env.AppendToPath(_totalPlan, path, planIdx);
  path.PrintPath();
 
  
  // Do final check on how close the plan's goal angle is to the originally requested goal angle
  // and either add a point turn at the end or modify the last point turn action.
  u8 numSegments = path.GetNumSegments();
  assert(_selectedGoalID < _targetPoses_orig.size());
  Radians desiredGoalAngle = _targetPoses_orig[_selectedGoalID].GetRotationAngle<'Z'>();
    
  if (numSegments > 0) {

    // Get last non-point turn segment of path
    PathSegment& lastSeg = path[numSegments-1];

    f32 end_x, end_y, end_angle;
    lastSeg.GetEndPose(end_x, end_y, end_angle);
      
    while(lastSeg.GetType() == Planning::PST_POINT_TURN) {
      path.PopBack(1);
      --numSegments;
        
      if (numSegments > 0) {
        lastSeg = path[numSegments-1];
        lastSeg.GetEndPose(end_x, end_y, end_angle);
      } else {
        break;
      }
    }
      
    Radians plannedGoalAngle(end_angle);
    f32 angDiff = (desiredGoalAngle - plannedGoalAngle).ToFloat();
      
    
    f32 turnDir = angDiff > 0 ? 1.f : -1.f;
    f32 rotSpeed = TERMINAL_POINT_TURN_SPEED * turnDir;
      
    PRINT_CH_INFO("Planner", 
                  "LatticePlanner.ReplanIfNeeded.FinalAngleCorrection",
                  "LatticePlanner: Final angle off by %f rad. DesiredAng = %f, endAngle = %f, rotSpeed = %f. "
                  "Adding point turn.",
                  angDiff, desiredGoalAngle.ToFloat(), end_angle, rotSpeed );
      
    path.AppendPointTurn(end_x, end_y, plannedGoalAngle.ToFloat(), desiredGoalAngle.ToFloat(),
                         rotSpeed,
                         TERMINAL_POINT_TURN_ACCEL,
                         TERMINAL_POINT_TURN_DECEL,
                         TERMINAL_POINT_TURN_ANGLE_TOL,
                         true);

  }

  return true;  
}


bool LatticePlannerImpl::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _planner.PathIsSafe(path, startAngle, validPath);
}


}
}
