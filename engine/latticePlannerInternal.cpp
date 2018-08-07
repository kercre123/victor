/**
 * File: latticePlannerInternal.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-04
 *
 * Description: implementation of Cozmo wrapper for the xytheta lattice planner in
 *  coretech/planning
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "engine/ankiEventUtil.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapToPlanner.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ObservableObject.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/namedColors/namedColors.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/planning/engine/xythetaEnvironment.h"
#include "coretech/planning/engine/xythetaPlanner.h"
#include "coretech/planning/engine/xythetaPlannerContext.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"

#include "json/json.h"
#include "util/jsonWriter/jsonWriter.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"

#include "latticePlanner.h"
#include "latticePlannerInternal.h"

#include <chrono>

#define DEBUG_REPLAN_CHECKS 0

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


// a global max so planning doesn't run forever
#define LATTICE_PLANNER_MAX_EXPANSIONS 30000000

// this is in units of seconds per mm, meaning the robot would drive X
// seconds out of the way to avoid having to drive 1 mm through the
// penalty
#define DEFAULT_OBSTACLE_PENALTY 0.1f

// how far (in mm) away from the path the robot needs to be before it gives up and plans a new path
#define PLAN_ERROR_FOR_REPLAN 20.0


const f32 TERMINAL_POINT_TURN_SPEED = 2; //rad/s
const f32 TERMINAL_POINT_TURN_ACCEL = 10.f;
const f32 TERMINAL_POINT_TURN_DECEL = 10.f;

// The angular tolerance to use for the point turn that is appended at the end of every path
const f32 TERMINAL_POINT_TURN_ANGLE_TOL = DEG_TO_RAD(5.f);

namespace Anki {
namespace Vector {
  
// option to add a small delay for planning to mimic the robot
CONSOLE_VAR_RANGED( int, kArtificialPlanningDelay_ms, "Planner", 0, 0, 3900 );

LatticePlannerInternal::LatticePlannerInternal(Robot* robot, Util::Data::DataPlatform* dataPlatform, const LatticePlanner* parent)
  : _robot(robot)
  , _blockWorld(&robot->GetBlockWorld())
  , _parent(parent)
  , _planner(_context)
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


  _plannerThread = new std::thread( &LatticePlannerInternal::worker, this );
}

LatticePlannerInternal::~LatticePlannerInternal()
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
      PRINT_NAMED_ERROR("LatticePlannerInternal.Destroy.Exception",
                        "locking the context mutex threw: %s",
                        err.what());
      // this will probably crash when we call SafeDelete below
    }
    Util::SafeDelete(_plannerThread);
  }
}

void LatticePlannerInternal::DoPlanning()
{
  _internalComputeStatus = EPlannerStatus::Running;
  _errorType             = EPlannerErrorType::None;

  if( LATTICE_PLANNER_THREAD_DEBUG ) {
    PRINT_CH_INFO("Planner", "LatticePlanner.ThreadDebug", "DoPlanning: running replan...");
  }

  if( _msToBlock == 0 ) {
    _msToBlock = kArtificialPlanningDelay_ms;
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

  Util::sInfoF(eventName.c_str(),
               {{DDATA, std::to_string(duration_ms.count()).c_str()}},
               "%d:%d",
               _planner.GetLastNumExpansions(),
               _planner.GetLastNumConsiderations());

  if(!result) {
    _internalComputeStatus = EPlannerStatus::Error;
    _errorType = EPlannerErrorType::PlannerFailed;
  }
  else if(_planner.GetPlan().Size() != 0) {
    // at this point _totalPlan may contain the valid old plan during replanning

    // verify that the append will be correct
    if(_totalPlan.Size() > 0) {
      GraphState endState = _context.env.GetActionSpace().GetPlanFinalState(_totalPlan);
      if(endState != _planner.GetPlan().start_) {
        PRINT_STREAM_ERROR("LatticePlanner.PlanMismatch",
                           "trying to append a plan with a mismatching state!\n"
                           << "endState = " << endState << std::endl
                           << "next plan start = " << _planner.GetPlan().start_ << std::endl);

        PRINT_CH_DEBUG("Planner", "LatticePlannerInternal", "totalPlan_:");
        _context.env.GetActionSpace().PrintPlan(_totalPlan);

        PRINT_CH_DEBUG("Planner", "LatticePlannerInternal", "new plan:");
        _context.env.GetActionSpace().PrintPlan(_planner.GetPlan());

        _internalComputeStatus = EPlannerStatus::Error;
        _errorType = EPlannerErrorType::InvalidAppendant;
        return;
      }
    }

    // the selected goal
    _selectedGoalID = _planner.GetChosenGoalID();

    PRINT_CH_DEBUG("Planner", "LatticePlannerInternal", "old plan");
    _context.env.GetActionSpace().PrintPlan(_totalPlan);

    _totalPlan.Append( _planner.GetPlan() );

    PRINT_CH_DEBUG("Planner", "LatticePlannerInternal", "new plan");
    _context.env.GetActionSpace().PrintPlan(_planner.GetPlan());

    _internalComputeStatus = EPlannerStatus::CompleteWithPlan;
  }
  else {
    // size == 0
    _internalComputeStatus = EPlannerStatus::CompleteNoPlan;
  }
}

void LatticePlannerInternal::worker()
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
void LatticePlannerInternal::HandleMessage(const ExternalInterface::PlannerRunMode& msg)
{
  SetIsSynchronous(msg.isSync);
}

void LatticePlannerInternal::SetIsSynchronous(bool val)
{
  _isSynchronous = val;
  PRINT_CH_INFO("Planner", "LatticePlanner.IsSync",
                "%s",
                val ? "true" : "false");
}

void LatticePlannerInternal::StopPlanning()
{
  if( _plannerThread != nullptr && _plannerRunning ) {
    _runPlanner = false;
  }
}

EPlannerStatus LatticePlannerInternal::CheckPlanningStatus() const
{
  // read-only access is safe here
  return _internalComputeStatus;
}
  
EPlannerErrorType LatticePlannerInternal::GetErrorType() const
{
  // read-only access is safe here
  return _errorType;
}

void LatticePlannerInternal::ImportBlockworldObstaclesIfNeeded(const bool isReplanning, const ColorRGBA* vizColor)
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

  const RobotTimeStamp_t timeOfLastChange = _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp();
  const bool didObjectsChange = (_timeOfLastObjectsImport < timeOfLastChange);

  if(!isReplanning ||  // get obstacles if they've changed or we're not replanning
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
      {MemoryMapTypes::EContentType::ObstacleProx          , false},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , true},
      {MemoryMapTypes::EContentType::InterestingEdge       , true},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithProx),
      "This array does not define all types once and only once.");

    constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithCliff =
    {
      {MemoryMapTypes::EContentType::Unknown               , true},
      {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
      {MemoryMapTypes::EContentType::ClearOfCliff          , true},
      {MemoryMapTypes::EContentType::ObstacleObservable    , true },
      {MemoryMapTypes::EContentType::ObstacleProx          , true},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , false},
      {MemoryMapTypes::EContentType::InterestingEdge       , true},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithCliff),
      "This array does not define all types once and only once.");
    
    constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithCharger =
    {
      {MemoryMapTypes::EContentType::Unknown               , true},
      {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
      {MemoryMapTypes::EContentType::ClearOfCliff          , true},
      {MemoryMapTypes::EContentType::ObstacleObservable    , true},
      {MemoryMapTypes::EContentType::ObstacleProx          , true},
      {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
      {MemoryMapTypes::EContentType::Cliff                 , true},
      {MemoryMapTypes::EContentType::InterestingEdge       , true},
      {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
    };
    static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithCharger),
                  "This array does not define all types once and only once.");

    // GetNavMap Polys
    std::vector<ConvexPolygon> convexHulls;
    INavMap* memoryMap = _robot->GetMapComponent().GetCurrentMemoryMap();

    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithInterestingEdges, MemoryMapTypes::EContentType::InterestingEdge, convexHulls);
    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithNotInterestingEdges, MemoryMapTypes::EContentType::NotInterestingEdge, convexHulls);
    GetConvexHullsByType(memoryMap, typesToCalculateBordersWithCliff, MemoryMapTypes::EContentType::Cliff, convexHulls);
    
    // VIC-3804 In order to use DriveToPose actions inside the habitat, we need to turn off
    //          prox obstacles so that we may reach positions where the robot can see the
    //          white line inside the habitat. Planning gets stalled when the robot observes
    //          the habitat wall, and often times out otherwise.
    if(_robot->GetMapComponent().GetUseProxObstaclesInPlanning()) {
      GetConvexHullsByType(memoryMap, typesToCalculateBordersWithProx, MemoryMapTypes::EContentType::ObstacleProx, convexHulls);
    }
    
    MemoryMapTypes::MemoryMapDataConstList observableObjectData;
    MemoryMapTypes::NodePredicate pred =
      [](MemoryMapTypes::MemoryMapDataConstPtr d) -> bool
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

    for(GraphTheta theta=0; theta < GraphState::numAngles_; ++theta) {

      float thetaRads = _context.env.GetActionSpace().LookupTheta(theta);

      // Since the planner is given the driveCenter poses to be the robot's origin, compute the actual robot
      // origin given a driveCenter pose at (0,0,0) in order to get the actual bounding box of the robot.
      Pose3d robotDriveCenterPose = Pose3d(thetaRads, Z_AXIS_3D(), {0.0f, 0.0f, 0.0f});
      Pose3d robotOriginPose;
      _robot->ComputeOriginPose(robotDriveCenterPose, robotOriginPose);

      // Get the robot polygon, and inflate it by a bit to handle error
      Poly2f robotPoly(_robot->GetBoundingQuadXY(robotOriginPose, robotPadding).Scale(LATTICE_PLANNER_ROBOT_EXPANSION_SCALING) );

      const Pose2d& robotPose = (_robot->GetPose());
      const State_c robotState(robotPose.GetX(), robotPose.GetY(), robotPose.GetAngle().ToFloat());
      for(const auto& boundingPoly : convexHulls) {
        _context.env.AddObstacleWithExpansion(boundingPoly, robotPoly, theta, DEFAULT_OBSTACLE_PENALTY);

        // only draw the angle we are currently at
        if(vizColor != nullptr &&
           robotState.GetGraphTheta() == theta
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
      //   PRINT_CH_INFO("Planner", "LatticePlannerInternal.ImportBlockworldObstaclesIfNeeded.ImportedObstacles",
      //                    "imported %d obstacles form blockworld",
      //                    numAdded);
      // }
    }

    // PRINT_CH_INFO("Planner", "LatticePlannerInternal.ImportBlockworldObstaclesIfNeeded.ImportedAngles",
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



EComputePathStatus LatticePlannerInternal::StartPlanning(const Pose3d& startPose,
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
             "LatticePlannerInternal.StartPlanning.StartPoseNotWrtRoot");

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
  const float maxDistanceToFollowOldPlan_mm = 300.0;
  if(_context.forceReplanFromScratch ||
     ! _context.env.PlanIsSafe(_totalPlan,
                               maxDistanceToFollowOldPlan_mm,
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

    // to print debugging info
    if( false ) {
      float waste = 0.0f;
      _context.env.FindClosestPlanSegmentToPose(_totalPlan, currentRobotState, waste, true);
    }

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
      PRINT_CH_INFO("Planner", "LatticePlannerInternal.GetPlan", "searchNum: %d", _searchNum);

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

bool LatticePlannerInternal::GetCompletePath(const Pose3d& currentRobotPose,
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
             "LatticePlannerInternal.GetCompletePath.RobotPoseNotWrtRoot");
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
    PRINT_NAMED_INFO("LatticePlanner.GetCompletePath.RobotPositionError",
                     "%04d %f",
                     planIdx,
                     offsetFromPlan);
    _totalPlan.Clear();
    _internalComputeStatus = EPlannerStatus::Error;
    _errorType = EPlannerErrorType::TooFarFromPlan;
    return false;
  }

  PRINT_CH_DEBUG("Planner", "LatticePlannerInternal", "total path:");
  _context.env.GetActionSpace().AppendToPath(_totalPlan, path, planIdx);
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


bool LatticePlannerInternal::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  return _planner.PathIsSafe(path, startAngle, validPath);
}

}
}
