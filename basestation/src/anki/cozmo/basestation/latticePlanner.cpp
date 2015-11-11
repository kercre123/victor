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

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaPlannerContext.h"
#include "json/json.h"
#include "latticePlanner.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
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

#define DEBUG_REPLAN_CHECKS 0

// a global max so planning doesn't run forever
#define LATTICE_PLANNER_MAX_EXPANSIONS 30000000

// this is in units of seconds per mm, meaning the robot would drive X
// seconds out of the way to avoid having to drive 1 mm through the
// penalty
#define DEFAULT_OBSTACLE_PENALTY 0.1

// how far (in mm) away from the path the robot needs to be before it gives up and plans a new path
#define PLAN_ERROR_FOR_REPLAN 20.0

// adds a bunch of noisy prints related to threading
#define LATTICE_PLANNER_THREAD_DEBUG 0

// The min absolute difference between the commanded end pose angle
// and the end pose angle of the generated plan that is required
// in order for a point turn to be appended to the path such that
// the end pose angle of the path is the commanded one.
const f32 TERMINAL_POINT_TURN_CORRECTION_THRESH_RAD = DEG_TO_RAD_F32(2.f);

const f32 TERMINAL_POINT_TURN_SPEED = 2; //rad/s
const f32 TERMINAL_POINT_TURN_ACCEL = 100.f;
const f32 TERMINAL_POINT_TURN_DECEL = 100.f;


namespace Anki {
namespace Cozmo {

using namespace Planning;

class LatticePlannerImpl : private Util::noncopyable
{
public:

  LatticePlannerImpl(const Robot* robot, Util::Data::DataPlatform* dataPlatform, const LatticePlanner* parent)
    : _robot(robot)
    , _planner(_context)
    , _searchNum(0)
    , _plannerThread(nullptr)
    , _timeToPlan(false)
    , _stopThread(false)
    , _runPlanner(true)
    , _plannerRunning(false)
    , _internalComputeStatus(EPlannerStatus::Error)
    , _parent(parent)
  {
    if( dataPlatform == nullptr ) {
      PRINT_NAMED_ERROR("LatticePlanner.InvalidDataPlatform",
                        "LatticePlanner requires data platform to operate!");
    }
    else {
      Json::Value mprims;
      const bool success = dataPlatform->readAsJson(Util::Data::Scope::Resources,
                                                    "config/basestation/config/cozmo_mprim.json",
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

    _plannerThread = new std::thread( &LatticePlannerImpl::worker, this );
  }

  ~LatticePlannerImpl() {
    if( _plannerThread != nullptr ) {
      // need to stop the thread
      _stopThread = true;
      _threadRequest.notify_all(); // noexcept
      PRINT_NAMED_EVENT("LatticePlanner.DestoryThread.Join", "");
      try {
        _plannerThread->join();
        PRINT_NAMED_EVENT("LatticePlanner.DestoryThread.Joined", "");
      }
      catch (std::runtime_error& err) {
        PRINT_NAMED_ERROR("LatticePlannerImpl.Destory.Exception",
                          "locking the context mutex threw: %s",
                          err.what());
        // this will probably crash when we call SafeDelete below
      }
      Util::SafeDelete(_plannerThread);
    }
  }

  // imports and pads obstacles
  void ImportBlockworldObstacles(const bool isReplanning, const ColorRGBA* vizColor = nullptr);

  EComputePathStatus StartPlanning(const Pose3d& startPose,
                                   bool forceReplanFromScratch);

  EPlannerStatus CheckPlanningStatus() const;

  bool GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path, size_t& selectedTargetIndex);

  void StopPlanning();

  void worker();

  const Robot* _robot;
  xythetaPlannerContext _context;
  xythetaPlanner _planner;
  xythetaPlan _totalPlan;
  Pose3d _targetPose_orig;

  std::string _pathToEnvCache;

  int _searchNum;

  // while planner thread is planning, it holds contextLock
  std::thread *_plannerThread;
  std::recursive_mutex _contextMutex;
  std::condition_variable_any _threadRequest;
  bool _timeToPlan;
  
  // this is a bool used to clean up and stop the thread (entirely)
  bool _stopThread;

  // this is the bool passed in to the planner to allow it to stop early
  bool _runPlanner;

  // This is toggled by the planner thread when it is crunching
  bool _plannerRunning;

  // this is also locked by _contextMutex, due to laziness
  EPlannerStatus _internalComputeStatus;

  const LatticePlanner* _parent;
};

//////////////////////////////////////////////////////////////////////////////// 
// LatticePlanner functions
//////////////////////////////////////////////////////////////////////////////// 

LatticePlanner::LatticePlanner(const Robot* robot, Util::Data::DataPlatform* dataPlatform)
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
                                                     const Pose3d& targetPose)
{
  // This has to live in LatticePlanner instead of impl because it calls LatticePlanner::GetPlan at the end

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

  std::lock_guard<std::recursive_mutex> lg(_impl->_contextMutex, std::adopt_lock);

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
  _impl->_targetPose_orig = targetPose;
  
  State_c target(targetPose.GetTranslation().x(),
                 targetPose.GetTranslation().y(),
                 targetPose.GetRotationAngle<'Z'>().ToFloat());

  _impl->ImportBlockworldObstacles(false);

  // Clear plan whenever we attempt to set goal
  _impl->_totalPlan.Clear();

  _impl->_context.goal = target;
  
  if( ! _impl->_planner.GoalIsValid() ) {
    return EComputePathStatus::Error;
  }

  return ComputeNewPathIfNeeded(startPose, true);
}

EComputePathStatus LatticePlanner::ComputePath(const Pose3d& startPose,
                                               const Pose3d& targetPose)
{
  _selectedTargetIdx = 0;
  return ComputePathHelper(startPose, targetPose);
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

  _impl->ImportBlockworldObstacles(false, &NamedColors::BLOCK_BOUNDING_QUAD);

  // for now just select the closest non-colliding goal as the true
  // goal. Eventually I'll implement a real multi-goal planner that
  // decides on its own which goal it wants

  bool found = false;
  size_t numTargetPoses = targetPoses.size();
  size_t bestTargetIdx = 0;
  float closestDist2 = 0;

  // first try to find a pose with no soft collisions, then try to find one with no fatal collisions
  for(auto maxPenalty : (float[]){0.01, MAX_OBSTACLE_COST}) {

    bestTargetIdx = 0;
    closestDist2 = 0;
    for(size_t i=0; i<numTargetPoses; ++i) {
      float dist2 = (targetPoses[i].GetTranslation() - startPose.GetTranslation()).LengthSq();

      if(!found || dist2 < closestDist2) {
        State_c target_c(targetPoses[i].GetTranslation().x(),
                         targetPoses[i].GetTranslation().y(),
                         targetPoses[i].GetRotationAngle<'Z'>().ToFloat());
        State target(_impl->_context.env.State_c2State(target_c));
        
        if(_impl->_context.env.GetCollisionPenalty(target) < maxPenalty) {
          closestDist2 = dist2;
          bestTargetIdx = i;
          found = true;
        }
      }
    }

    if(found)
      break;
  }

  if(found) {
    _selectedTargetIdx = bestTargetIdx;
    return ComputePathHelper(startPose, targetPoses[bestTargetIdx]);
  }
  else {
    PRINT_NAMED_INFO("LatticePlanner.ComputePath.NoValidTarget",
                     "could not find valid target out of %lu possible targets\n",
                     numTargetPoses);
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

  if( ret == EComputePathStatus::Running ) {
    // set internal status to running now, don't wait for the thread to actually start
    _impl->_internalComputeStatus = EPlannerStatus::Running;
  }

  return ret;
}

void LatticePlanner::GetTestPath(const Pose3d& startPose, Planning::Path &path)
{
  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  _impl->_context.start = currentRobotState;

  xythetaPlan plan;
  _impl->_planner.GetTestPlan(plan);
  printf("test plan:\n");
  _impl->_context.env.PrintPlan(plan);

  path.Clear();
  _impl->_context.env.AppendToPath(plan, path);
  printf("test path:\n");
  path.PrintPath();
}


bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose, Planning::Path &path)
{
  size_t waste;
  return _impl->GetCompletePath(currentRobotPose, path, waste);
}

bool LatticePlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose, Planning::Path &path, size_t& selectedTargetIndex)
{
  return _impl->GetCompletePath(currentRobotPose, path, selectedTargetIndex);
}

//////////////////////////////////////////////////////////////////////////////// 
// LatticePlannerImpl functions
//////////////////////////////////////////////////////////////////////////////// 

void LatticePlannerImpl::worker()
{
  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    std::cout << "hello from planner worker thread! I am object " << this
              << " running in thread " << std::this_thread::get_id() << std::endl;
  }

  while(!_stopThread) {

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      printf("about to lock\n");
    }

    std::unique_lock<std::recursive_mutex> lock(_contextMutex);

    if( LATTICE_PLANNER_THREAD_DEBUG ) {
      printf("got context lock in thread\n");
    }

    while( !_timeToPlan && !_stopThread ) {
      _threadRequest.wait(lock);

      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        printf("got cv in thread: %d %d\n", _timeToPlan, _stopThread);
      }
    }

    if( _timeToPlan ) {
      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        printf("running planner in thread!\n");
      }

      _internalComputeStatus = EPlannerStatus::Running;
      _plannerRunning = true;
      _timeToPlan = false;
      bool result = _planner.Replan(LATTICE_PLANNER_MAX_EXPANSIONS, &_runPlanner);
      _plannerRunning = false;

      if( LATTICE_PLANNER_THREAD_DEBUG ) {
        printf("planner in thread returned %d\n", result);
      }

      if(!result) {
        _internalComputeStatus = EPlannerStatus::Error;
      }
      else if(_planner.GetPlan().Size() != 0) {
        // at this point _totalPlan may contain the valid old plan during replanning

        // verify that the append will be correct
        if(_totalPlan.Size() > 0) {
          State endState = _context.env.GetPlanFinalState(_totalPlan);
          if(endState != _planner.GetPlan().start_) {
            PRINT_STREAM_ERROR("LatticePlanner.PlanMismatch",
                               "trying to append a plan with a mismatching state!\n"
                               << "endState = " << endState << std::endl
                               << "next plan start = " << _planner.GetPlan().start_ << std::endl);

            std::cout<<"\ntotalPlan_:\n";
            _context.env.PrintPlan(_totalPlan);

            std::cout<<"\nnew plan:\n";
            _context.env.PrintPlan(_planner.GetPlan());

            _internalComputeStatus = EPlannerStatus::Error;
          }
        }

        printf("old plan:\n");
        _context.env.PrintPlan(_totalPlan);

        _totalPlan.Append( _planner.GetPlan() );

        printf("new plan:\n");
        _context.env.PrintPlan(_planner.GetPlan());

        if( _planner.GetPlan().Size() == 0 ) {
          _internalComputeStatus = EPlannerStatus::CompleteNoPlan;
        }
        else {
          _internalComputeStatus = EPlannerStatus::CompleteWithPlan;
        }
      }
    }
  }

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    std::cout << "Planner worker thread returning: object" << this
              << " running in thread " << std::this_thread::get_id() << std::endl;
  }

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


void LatticePlannerImpl::ImportBlockworldObstacles(const bool isReplanning, const ColorRGBA* vizColor)
{

  if( ! _contextMutex.try_lock() ) {
    // thread is already running.
    PRINT_NAMED_WARNING("LatticePlanner.ImportBlockworldObstacles.alreadyRunning",
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

  const bool didObjecsChange = _robot->GetBlockWorld().DidObjectsChange();
  
  if(!isReplanning ||  // if not replanning, this must be a fresh, new plan, so we always should get obstacles
     didObjecsChange)
  {
    std::vector<std::pair<Quad2f,ObjectID> > boundingBoxes;

    _robot->GetBlockWorld().GetObstacles(boundingBoxes, obstaclePadding);
    
    _context.env.ClearObstacles();
    
    if(vizColor != nullptr) {
      VizManager::getInstance()->EraseAllPlannerObstacles(isReplanning);
    }

    unsigned int numAdded = 0;
    unsigned int vizID = 0;

    Planning::StateTheta numAngles = (StateTheta) _context.env.GetNumAngles();
    for(StateTheta theta=0; theta < numAngles; ++theta) {

      float thetaRads = _context.env.GetTheta_c(theta);

      // Since the planner is given the driveCenter poses to be the robot's origin, compute the actual robot
      // origin given a driveCenter pose at (0,0,0) in order to get the actual bounding box of the robot.
      Pose3d robotDriveCenterPose = Pose3d(thetaRads, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f});
      Pose3d robotOriginPose;
      _robot->ComputeOriginPose(robotDriveCenterPose, robotOriginPose);
      
      // Get the robot polygon, and inflate it by a bit to handle error
      Poly2f robotPoly;
      robotPoly.ImportQuad2d(_robot->GetBoundingQuadXY(robotOriginPose, robotPadding) );

      for(auto boundingQuad : boundingBoxes) {

        Poly2f boundingPoly;
        boundingPoly.ImportQuad2d(boundingQuad.first);

        /*const FastPolygon& expandedPoly =*/
        _context.env.AddObstacleWithExpansion(boundingPoly, robotPoly, theta, DEFAULT_OBSTACLE_PENALTY);

        // only draw the angle we are currently at
        if(vizColor != nullptr &&
           _context.env.GetTheta(_robot->GetPose().GetRotationAngle<'Z'>().ToFloat()) == theta
           // && !isReplanning
          ) {

          // TODO: manage the quadID better so we don't conflict
          // TODO:(bn) handle isReplanning with color??
          // VizManager::getInstance()->DrawPlannerObstacle(isReplanning, vizID++ , expandedPoly, *vizColor);

          // TODO:(bn) figure out a good way to visualize the
          // multi-angle stuff. For now just draw the quads with
          // padding
          VizManager::getInstance()->DrawQuad (
            isReplanning ? VizQuadType::VIZ_QUAD_PLANNER_OBSTACLE_REPLAN : VizQuadType::VIZ_QUAD_PLANNER_OBSTACLE,
            vizID++, boundingQuad.first, 0.1f, *vizColor );
        }
        numAdded++;
      }

      // if(theta == 0) {
      //   PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedObstacles",
      //                    "imported %d obstacles form blockworld",
      //                    numAdded);
      // }
    }

    // PRINT_NAMED_INFO("LatticePlannerImpl.ImportBlockworldObstacles.ImportedAngles",
    //                  "imported %d total obstacles for %d angles",
    //                  numAdded,
    //                  numAngles);
  }
  else {
    PRINT_NAMED_INFO("LatticePlanner.ImportBlockworldObstacles.NoUpdateNeeded",
                     "robot padding %f, obstacle padding %f , didBlocksChange %d",
                     robotPadding,
                     obstaclePadding,
                     didObjecsChange);
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

  State_c lastSafeState;
  xythetaPlan validOldPlan;
  size_t planIdx = 0;

  _context.forceReplanFromScratch = forceReplanFromScratch;

  State_c currentRobotState(startPose.GetTranslation().x(),
                            startPose.GetTranslation().y(),
                            startPose.GetRotationAngle<'Z'>().ToFloat());

  //VizManager::getInstance()->EraseAllQuads();
  if( ! _context.forceReplanFromScratch ) {
    ImportBlockworldObstacles(true, &NamedColors::REPLAN_BLOCK_BOUNDING_QUAD);

    // planIdx is the number of plan actions to execute before getting to the starting point closest to start
    float offsetFromPlan = 0.0;
    planIdx = _context.env.FindClosestPlanSegmentToPose(_totalPlan,
                                                        currentRobotState,
                                                        offsetFromPlan);

    if(offsetFromPlan >= PLAN_ERROR_FOR_REPLAN) {
      PRINT_NAMED_INFO("LatticePlanner.GetPlan.ForcePlan",
                       "Current state is %f away from the plan (planIdx %zu), forcing replan from scratch\n",
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
      PRINT_NAMED_INFO("LatticePlanner.GetPlan.OldPlanUnsafe",
                       "old plan unsafe! Will replan, starting from %zu, keeping %zu actions from oldPlan.\n",
                       planIdx, validOldPlan.Size());
    }

    std::cout<<"currentRobotState: "<<currentRobotState<<endl;

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
      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.InvalidStart", "could not set start\n");
      return EComputePathStatus::Error;
    }
    else if(!_planner.GoalIsValid()) {
      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.InvalidGoal", "Goal may have moved into collision.\n");
      return EComputePathStatus::Error;
    }
    else {
      // use real padding for re-plan
      ImportBlockworldObstacles(false, &NamedColors::BLOCK_BOUNDING_QUAD);

      PRINT_NAMED_INFO("LatticePlanner.ReplanIfNeeded.Replanning",
                       "from (%f, %f, %f) to (%f %f %f)\n",
                       lastSafeState.x_mm, lastSafeState.y_mm, lastSafeState.theta,
                       _context.goal.x_mm, _context.goal.y_mm, _context.goal.theta);

      _context.env.PrepareForPlanning();

      _searchNum++;
      PRINT_NAMED_INFO("LatticePlannerImpl.GetPlan", "searchNum: %d", _searchNum);

      if( LATTICE_PLANNER_DUMP_ENV_TO_CACHE ) {
        std::stringstream filenameSS;
        filenameSS << _pathToEnvCache << "context_" << _searchNum << ".json";

        std::string filename = filenameSS.str();

        if(_searchNum == 1) {
          PRINT_NAMED_INFO("LatticePlanner.EnvDump",
                           "dumping planner context to files like '%s'",
                           filename.c_str());
        }

        Util::JsonWriter contextDumpWriter(filename);
        _context.Dump(contextDumpWriter);
        contextDumpWriter.Close();
      }

      _runPlanner = true;
      _timeToPlan = true;
      _threadRequest.notify_all();
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
      printf("LatticePlanner: Plan safe, but validOldPlan is empty\n");
    }
#endif

    return EComputePathStatus::NoPlanNeeded;
  }
}

bool LatticePlannerImpl::GetCompletePath(const Pose3d& currentRobotPose,
                                         Planning::Path &path,
                                         size_t& selectedTargetIndex)
{

  if( _internalComputeStatus == EPlannerStatus::Error ||
      _internalComputeStatus == EPlannerStatus::Running ) {
    return false;
  }

  selectedTargetIndex = _parent->_selectedTargetIdx;


  // consider trimming actions if the robot has moved past the beginning of the path
  State_c currentRobotState(currentRobotPose.GetTranslation().x(),
                            currentRobotPose.GetTranslation().y(),
                            currentRobotPose.GetRotationAngle<'Z'>().ToFloat());

  float offsetFromPlan = 0.0;
  int planIdx = Util::numeric_cast<int>(_context.env.FindClosestPlanSegmentToPose(_totalPlan,
                                                                                  currentRobotState,
                                                                                  offsetFromPlan) );

  PRINT_NAMED_INFO("LatticePlanner.GetCompletePath.Offset",
                   "Robot is %f from the plan, at index %d",
                   offsetFromPlan,
                   planIdx);

  if(offsetFromPlan >= PLAN_ERROR_FOR_REPLAN) {
    PRINT_NAMED_EVENT("LatticePlanner.GetCompletePath.RobotPositionError",
                      "%04d %f",
                      planIdx,
                      offsetFromPlan);
    _totalPlan.Clear();
    _internalComputeStatus = EPlannerStatus::Error;
    return false;
  }

  printf("total path:\n");
  _context.env.AppendToPath(_totalPlan, path, planIdx);
  path.PrintPath();
 
  
  // Do final check on how close the plan's goal angle is to the originally requested goal angle
  // and either add a point turn at the end or modify the last point turn action.
  u8 numSegments = path.GetNumSegments();
  Radians desiredGoalAngle = _targetPose_orig.GetRotationAngle<'Z'>();
    
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
      
    if (std::abs(angDiff) > TERMINAL_POINT_TURN_CORRECTION_THRESH_RAD) {
        
      f32 turnDir = angDiff > 0 ? 1.f : -1.f;
      f32 rotSpeed = TERMINAL_POINT_TURN_SPEED * turnDir;
        
      PRINT_NAMED_INFO(
        "LatticePlanner.ReplanIfNeeded.FinalAngleCorrection",
        "LatticePlanner: Final angle off by %f rad. DesiredAng = %f, endAngle = %f, rotSpeed = %f. Adding point turn.",
        angDiff, desiredGoalAngle.ToFloat(), end_angle, rotSpeed );
        
      path.AppendPointTurn(0, end_x, end_y, desiredGoalAngle.ToFloat(),
                           rotSpeed,
                           TERMINAL_POINT_TURN_ACCEL,
                           TERMINAL_POINT_TURN_DECEL,
                           true);
    }
  }

  return true;  
}


}
}
