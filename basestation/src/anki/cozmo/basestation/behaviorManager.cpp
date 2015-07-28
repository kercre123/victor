
/**
 * File: behaviorManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "pathPlanner.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/cozmo/shared/RobotMessageDefinitions.h"


#include <iomanip>

// The angle wrt the mat at which the user is expected to be.
// For happy head-nodding demo purposes.
#define USER_LOC_ANGLE_WRT_MAT -1.57

#define JUNE_DEMO_START_X 150.0
#define JUNE_DEMO_START_Y -120.0
#define JUNE_DEMO_START_THETA 0.0

namespace Anki {
  namespace Cozmo {
    
    static const ActionList::SlotHandle DriveAndManipulateSlot = 0;
    
    // List of sound IDs
    typedef enum {
      SOUND_TADA
      ,SOUND_NOPROBLEMO
      ,SOUND_INPUT
      ,SOUND_SWEAR
      ,SOUND_STARTOVER
      ,SOUND_NOTIMPRESSED
      ,SOUND_60PERCENT
      ,SOUND_DROID
      ,SOUND_DEMO_START
      ,SOUND_WAITING4DICE
      ,SOUND_WAITING4DICE2DISAPPEAR
      ,SOUND_OK_GOT_IT
      ,SOUND_OK_DONE
      ,SOUND_POWER_ON
      ,SOUND_POWER_DOWN
      ,SOUND_PHEW
      ,SOUND_OOH
      ,SOUND_SCREAM
      ,SOUND_WHATS_NEXT
      ,SOUND_HELP_ME
      ,SOUND_SCAN
      ,SOUND_HAPPY_CHASE
      ,SOUND_FLEES
      ,SOUND_SINGING
      ,SOUND_SHOOT
      ,SOUND_SHOT_HIT
      ,SOUND_SHOT_MISSED
      ,NUM_SOUNDS
    } SoundID_t;

    
    const std::string& GetSoundName(SoundID_t soundID)
    {
      // Table of sound files relative to root dir
      static const std::map<SoundID_t, std::string> soundTable =
      {
        // Cozmo default sound scheme
        {SOUND_STARTOVER, "demo/WaitingForDice2.wav"}
        ,{SOUND_NOTIMPRESSED, "demo/OKGotIt.wav"}
        
        ,{SOUND_WAITING4DICE, "demo/WaitingForDice1.wav"}
        ,{SOUND_WAITING4DICE2DISAPPEAR, "demo/WaitingForDice2.wav"}
        ,{SOUND_OK_GOT_IT, "demo/OKGotIt.wav"}
        ,{SOUND_OK_DONE, "demo/OKDone.wav"}
        
        // Movie sound scheme
        ,{SOUND_TADA, "misc/tada.mp3"}
        ,{SOUND_NOPROBLEMO, "misc/nproblem.wav"}
        ,{SOUND_INPUT, "misc/input.wav"}
        ,{SOUND_SWEAR, "misc/swear.wav"}
        ,{SOUND_STARTOVER, "anchorman/startover.wav"}
        ,{SOUND_NOTIMPRESSED, "anchorman/notimpressed.wav"}
        ,{SOUND_60PERCENT, "anchorman/60percent.wav"}
        ,{SOUND_DROID, "droid/droid.wav"}
        
        ,{SOUND_DEMO_START, "misc/swear.wav"}
        ,{SOUND_WAITING4DICE, "misc/input.wav"}
        ,{SOUND_WAITING4DICE2DISAPPEAR, "misc/input.wav"}
        ,{SOUND_OK_GOT_IT, "misc/nproblem.wav"}
        ,{SOUND_OK_DONE, "anchorman/60percent.wav"}
        
        // CREEP Sound Scheme
        // TODO: Update these mappings for real playtest sounds
        ,{SOUND_POWER_ON,     "creep/Robot-PowerOn1Rev2.mp3"}
        ,{SOUND_POWER_DOWN,  "creep/Robot-PowerDown13B.mp3"}
        ,{SOUND_PHEW,        "creep/Robot-ReliefPhew1.mp3"}
        ,{SOUND_SCREAM,      "creep/Robot-Scream7.mp3"}
        ,{SOUND_OOH,         "creep/Robot-OohScream12.mp3"}
        ,{SOUND_WHATS_NEXT,  "creep/Robot-WhatsNext1A.mp3"}
        ,{SOUND_HELP_ME,     "creep/Robot-WahHelpMe2.mp3"}
        ,{SOUND_SCAN,        "creep/Robot-Scanning2Rev1.mp3"}
        ,{SOUND_HAPPY_CHASE, "creep/Robot-Happy2.mp3"}
        ,{SOUND_FLEES,       "creep/Robot-Happy1.mp3"}
        ,{SOUND_SINGING,     "creep/Robot-Singing1Part1-2.mp3"}
        ,{SOUND_SHOOT,       "codeMonsterShooter/shoot.wav"}
        ,{SOUND_SHOT_HIT,    "codeMonsterShooter/hit.wav"}
        ,{SOUND_SHOT_MISSED, "codeMonsterShooter/miss.wav"}
      };
      
      static const std::string DEFAULT("");

      auto result = soundTable.find(soundID);
      if(result == soundTable.end()) {
        PRINT_NAMED_WARNING("SoundManager.GetSoundFile.UndefinedID",
                            "No file defined for sound ID %d.\n",
                            soundID);
        return DEFAULT;
      } else {
        return result->second;
      }
    }

    
    static Result ScaredReaction(Robot* robot, Vision::ObservedMarker* marker)
    {
      PRINT_STREAM_INFO("BehaviorManager.ScaredReaction", "Saw Scary Block!");
      robot->SetBehaviorState(BehaviorManager::SCARED_FLEE);
      return RESULT_OK;
    }
    
    static Result ExcitedReaction(Robot* robot, Vision::ObservedMarker* marker)
    {
      PRINT_STREAM_INFO("BehaviorManager.ExcitedReaction", "Saw Exciting Block!");
      robot->SetBehaviorState(BehaviorManager::EXCITABLE_CHASE);
      return RESULT_OK;
    }
    
    static bool IsMarkerCloseEnoughAndCentered(const Vision::ObservedMarker* marker, const u16 ncols)
    {
      bool result = false;
      
      // Parameters:
      const f32 minDiagSize       = 50.f;
      const f32 maxDistFromCenter = 35.f;
      
      const f32 diag1 = (marker->GetImageCorners()[Quad::TopLeft]  - marker->GetImageCorners()[Quad::BottomRight]).Length();
      const f32 diag2 = (marker->GetImageCorners()[Quad::TopRight] - marker->GetImageCorners()[Quad::BottomLeft]).Length();
      
      // If the marker is large enough in our field of view (this is a proxy for
      // "close enough" without needing to compute actual pose)
      if(diag1 >= minDiagSize && diag2 >= minDiagSize) {
        // If the marker is centered in the field of view (this is a proxy for
        // "robot is facing the marker")
        const Point2f centroid = marker->GetImageCorners().ComputeCentroid();
        if(std::abs(centroid.x() - static_cast<f32>(ncols/2)) < maxDistFromCenter) {
          result = true;
        }
      }
      
      return result;
    }
    
    static Result ArrowCallback(Robot* robot, Vision::ObservedMarker* marker)
    {
      Result lastResult = RESULT_OK;
     
      // Parameters (pass in?)
      const f32 driveSpeed = 30.f;
      
      if(robot->IsIdle() && IsMarkerCloseEnoughAndCentered(marker, robot->GetCamera().GetCalibration().GetNcols())) {
        
        Vec2f upVector = marker->GetImageCorners()[Quad::TopLeft] - marker->GetImageCorners()[Quad::BottomLeft];
        
        // Decide what to do based on the orientation of the arrow
        // NOTE: Remember that Y axis points down in image coordinates.
        
        const f32 angle = atan2(upVector.y(), upVector.x());
        
        if(angle >= -3.f*M_PI_4 && angle < -M_PI_4) { // UP
          PRINT_STREAM_INFO("BehaviorManager.ArrowCallback", "UP Arrow!");
          lastResult = robot->DriveWheels(driveSpeed, driveSpeed);
        }
        else if(angle >= -M_PI_4 && angle < M_PI_4) { // RIGHT
          PRINT_STREAM_INFO("BehaviorManager.ArrowCallback", "RIGHT Arrow!");
          //lastResult = robot->QueueAction(new TurnInPlaceAction(-M_PI_2));
          robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new TurnInPlaceAction(-M_PI_2));
        }
        else if(angle >= M_PI_4 && angle < 3*M_PI_4) { // DOWN
          PRINT_STREAM_INFO("BehaviorManager.ArrowCallback", "DOWN Arrow!");
          lastResult = robot->DriveWheels(-driveSpeed, -driveSpeed);
        }
        else if(angle >= 3*M_PI_4 || angle < -3*M_PI_4) { // LEFT
          PRINT_STREAM_INFO("BehaviorManager.ArrowCallback", "LEFT Arrow!");
          //lastResult = robot->QueueAction(new TurnInPlaceAction(M_PI_2));
          robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new TurnInPlaceAction(M_PI_2));
        }
        else {
          PRINT_STREAM_ERROR("BehaviorManager.ArrowCallback.UnexpectedAngle",
                             std::setprecision(5) <<
                            "Unexpected angle for arrow marker: " << angle << " radians (" << (angle*180.f/M_PI) << " degrees)");
          lastResult = RESULT_FAIL;
        }
      } // IfMarkerIsCloseEnoughAndCentered()
      
      return lastResult;
      
    } // ArrowCallback()
    
    static Result TurnAroundCallback(Robot* robot, Vision::ObservedMarker* marker)
    {
      Result lastResult = RESULT_OK;
      
      if(robot->IsIdle() && IsMarkerCloseEnoughAndCentered(marker, robot->GetCamera().GetCalibration().GetNcols())) {
        PRINT_STREAM_INFO("BehaviorManager.TurnAroundCallback", "TURNAROUND Arrow!");
        //lastResult = robot->QueueAction(new TurnInPlaceAction(M_PI));
        robot->GetActionList().QueueActionAtEnd(DriveAndManipulateSlot, new TurnInPlaceAction(M_PI));
      } // IfMarkerIsCloseEnoughAndCentered()
      
      return lastResult;
    } // TurnAroundCallback()
    
    static Result StopCallback(Robot* robot, Vision::ObservedMarker* marker)
    {
      Result lastResult = RESULT_OK;

      if(IsMarkerCloseEnoughAndCentered(marker, robot->GetCamera().GetCalibration().GetNcols())) {
        lastResult = robot->StopAllMotors();
      }
      
      return lastResult;
    }
    
    
    BehaviorManager::BehaviorManager(Robot* robot)
    : _mode(None)
    , _robot(robot)
    {
      Reset();
      
      // NOTE: Do not _use_ the _robot pointer in this constructor because
      //  this constructor is being called from Robot's constructor.
      
      
      CORETECH_ASSERT(_robot != nullptr);
    }

    void BehaviorManager::StartMode(Mode mode)
    {
      Reset();
      Mode fromMode = _mode;
      _mode = mode;
      switch(mode) {
        case None:
          CoreTechPrint("Starting NONE behavior\n");

          if(fromMode == CREEP) {
            // If switching out of CREEP mode, go back to sleep.
            _robot->PlayAnimation("ANIM_POWER_DOWN", 1);
          }
          
          //_robot->AbortAll();
          
          break;
          
        case June2014DiceDemo:
          CoreTechPrint("Starting June demo behavior\n");
          _state     = WAITING_FOR_ROBOT;
          _nextState = DRIVE_TO_START;
          _updateFcn = &BehaviorManager::Update_June2014DiceDemo;
          _idleState = IDLE_NONE;
          _timesIdle = 0;
          SoundManager::getInstance()->Play(GetSoundName(SOUND_DEMO_START));
          break;
          
        case ReactToMarkers:
          CoreTechPrint("Starting ReactToMarkers behavior\n");
          
          // Testing Reactions:
          _robot->AddReactionCallback(Vision::MARKER_ARROW,         &ArrowCallback);
          _robot->AddReactionCallback(Vision::MARKER_STOPWITHHAND,  &StopCallback);
          _robot->AddReactionCallback(Vision::MARKER_CIRCULARARROW, &TurnAroundCallback);
          
          // Once the callbacks are added
          StartMode(None);
          break;
          
        case CREEP:
        {
          CoreTechPrint("Starting Cozmo Robotic Emotional Engagement Playtest (CREEP)\n");
          
          _updateFcn = &BehaviorManager::Update_CREEP;
          
          _state     = SLEEPING;
          _nextState = SLEEPING;
          
          // Start off in sleeping mode
          _robot->PlayAnimation("ANIM_SLEEPING");
          
          VizManager::getInstance()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::YELLOW, GetBehaviorStateName(_state).c_str());
          
          //SoundManager::getInstance()->SetScheme(SOUND_SCHEME_CREEP);
         
          // Transitions are played once when changing states. Use "NUM_STATES" to specify
          // "any" state. Only one transition will be played, if multiple ones
          // apply (due to use of "any" state) using the following priority:
          //
          //  1. transition defined for this specific current / next state pair
          //  2. transition defined from any state to next state
          //  3. transition defined from current state to any state

          _transitionAnimations[SLEEPING][NUM_STATES]        = "ANIM_WAKE_UP";
          _transitionAnimations[SLEEPING][IDLE]              = "ANIM_WAKE_UP";
          _transitionAnimations[NUM_STATES][SCARED_FLEE]     = "ANIM_SCREAM";
          _transitionAnimations[SCARED_FLEE][SCAN]           = "ANIM_RELIEF";
          _transitionAnimations[NUM_STATES][EXCITABLE_CHASE] = "ANIM_ALERT";
          _transitionAnimations[NUM_STATES][IDLE]            = "ANIM_GOTO_READY";
          _transitionAnimations[NUM_STATES][ACKNOWLEDGEMENT_NOD] = "ANIM_HEAD_NOD";
          _transitionAnimations[NUM_STATES][SLEEPING]        = "ANIM_POWER_DOWN";

          // State animations play after any transition above is played, in a loop
          _stateAnimations[SLEEPING]         = "ANIM_SLEEPING";
          _stateAnimations[EXCITABLE_CHASE]  = "ANIM_EXCITABLE_CHASE";
          _stateAnimations[SCAN]             = "ANIM_SCAN";
          _stateAnimations[SCARED_FLEE]      = "ANIM_SCARED_FLEE";
          _stateAnimations[DANCE_WITH_BLOCK] = "ANIM_DANCING";
          _stateAnimations[HELP_ME_STATE]    = "ANIM_HELPME_FRUSTRATED";
          _stateAnimations[WHAT_NEXT]        = "ANIM_WHAT_NEXT";
          _stateAnimations[IDLE]             = "ANIM_BLINK";
          _stateAnimations[ACKNOWLEDGEMENT_NOD]  = "ANIM_BLINK";
 
          
          // Automatically switch states as reactions to certain markers:
          _robot->AddReactionCallback(Vision::MARKER_BEE,    &ScaredReaction);
          _robot->AddReactionCallback(Vision::MARKER_SPIDER, &ScaredReaction);
          _robot->AddReactionCallback(Vision::MARKER_KITTY,  &ExcitedReaction);
          
          break;
        } // case CREEP
          
          
        default:
          PRINT_NAMED_ERROR("BehaviorManager.InvalidMode", "Invalid behavior mode");
          return;
      }
      
      //assert(_updateFcn != nullptr);
      
    } // StartMode()
    
    
    const std::string& BehaviorManager::GetBehaviorStateName(BehaviorState state) const
    {
      static const std::map<BehaviorState, std::string> nameLUT = {
        {EXCITABLE_CHASE, "EXCITABLE_CHASE"},
        {SCARED_FLEE,     "SCARED_FLEE"},
        {DANCE_WITH_BLOCK,"DANCE_WITH_BLOCK"},
        {SCAN,            "SCAN"},
        {HELP_ME_STATE,   "HELP_ME_STATE"},
        {SLEEPING,        "SLEEPING"},
        {WAITING_FOR_ROBOT, "WAITING_FOR_ROBOT"},
        {WHAT_NEXT        , "WHAT_NEXT"},
        {IDLE             , "IDLE"},
        {ACKNOWLEDGEMENT_NOD, "ACKNOWLEDGEMENT_NOD"},
      };
      
      static const std::string UNKNOWN("UNKNOWN");
      
      auto nameIter = nameLUT.find(state);
      if(nameIter == nameLUT.end()) {
        PRINT_NAMED_WARNING("BehaviorManager.GetBehaviorStateName.UnknownName",
                            "No string name stored for behavior state %d.\n", state);
        return UNKNOWN;
      } else {
        return nameIter->second;
      }
    }
    
    
    void BehaviorManager::SetNextState(BehaviorState nextState)
    {
      bool validState = false;
      switch(nextState)
      {
        case EXCITABLE_CHASE:
        case SCARED_FLEE:
        case DANCE_WITH_BLOCK:
        case SCAN:
        case HELP_ME_STATE:
        case WHAT_NEXT:
        case IDLE:
        {
          if(_mode == CREEP) {
            validState = true;
          }
          break;
        }
        
        case DRIVE_TO_START:
        case WAITING_TO_SEE_DICE:
        case WAITING_FOR_DICE_TO_DISAPPEAR:
        case GOTO_EXPLORATION_POSE:
        case START_EXPLORING_TURN:
        case BACKING_UP:
        case BEGIN_EXPLORING:
        case EXPLORING:
        case CHECK_IT_OUT_UP:
        case CHECK_IT_OUT_DOWN:
        case FACE_USER:
        case HAPPY_NODDING:
        case BACK_AND_FORTH_EXCITED:
        {
          if(_mode == June2014DiceDemo) {
            validState = true;
          }
          break;
        }
          
        case ACKNOWLEDGEMENT_NOD:
        {
           // True for both of the above
          if(_mode == June2014DiceDemo || _mode == CREEP) {
            validState = true;
          }
          break;
        }
          
        default:
          validState = false;
          
      } // switch(nextState)
      
      
      if(validState) {
        _nextState = nextState;
      } else {
        
        PRINT_NAMED_ERROR("BehaviorManager.SetNextState.InvalidStateForMode",
                          "Invalid state for current mode.\n");
      }
    } // SetNextState()
    
    BehaviorManager::Mode BehaviorManager::GetMode() const
    {
      return _mode;
    }
    
    void BehaviorManager::Reset()
    {
      _state = WAITING_FOR_ROBOT;
      _nextState = _state;
      _updateFcn = nullptr;
      
      // June2014DiceDemo
      _explorationStartAngle = 0;
      _objectToPickUp.UnSet();
      _objectToPlaceOn.UnSet();
      
    } // Reset()
    
    const ObjectID BehaviorManager::GetObjectOfInterest() const
    {
      return _robot->GetBlockWorld().GetSelectedObject();
    }
    
    
    void BehaviorManager::Update()
    {
      // Shared states
      switch(_state) {
        case WAITING_FOR_ROBOT:
        {
          // Nothing to do here anymore: we should not be "waiting" on a robot
          // because BehaviorManager is now part of a robot!
          //_state = _nextState;
          break;
        }
        default:
          if (_updateFcn) {
            _updateFcn(this);
          } else {
            _state = _nextState = WAITING_FOR_ROBOT;
          }
          break;
      }

      
    } // Update()
    
    

    /********************************************************
     * June2014DiceDemo
     *
     * Look for two dice rolls. Look for the block with the 
     * number corresponding to the first roll and pick it up.
     * Place it on the block with the number corresponding to
     * the second roll.
     *
     ********************************************************/
    void BehaviorManager::Update_June2014DiceDemo()
    {

      static const ActionList::SlotHandle TraversalSlot = 0;
      
      constexpr float checkItOutAngleUp = DEG_TO_RAD(15);
      constexpr float checkItOutAngleDown = DEG_TO_RAD(-10);
      constexpr float checkItOutSpeed = 0.4;

      switch(_state) {

        case DRIVE_TO_START:
        {
          // Wait for robot to be IDLE
          if(_robot->IsIdle()) {
            Pose3d startPose(JUNE_DEMO_START_THETA,
                             Z_AXIS_3D(),
                             {{JUNE_DEMO_START_X, JUNE_DEMO_START_Y, 0.f}});
            CoreTechPrint("Driving to demo start location\n");
            _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(startPose));

            _state = WAITING_TO_SEE_DICE;

            _robot->SetDefaultLights(0x00808000);
          }

          break;
        }
          
        case WAITING_FOR_DICE_TO_DISAPPEAR:
        {
          const BlockWorld::ObjectsMapByID_t& diceBlocks = _robot->GetBlockWorld().GetExistingObjectsByType(Block::Type::DICE);
          
          if(diceBlocks.empty()) {
            
            // Check to see if the dice block has been gone for long enough
            const TimeStamp_t timeSinceSeenDice_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _diceDeletionTime;
            if(timeSinceSeenDice_ms > TimeBetweenDice_ms) {
              CoreTechPrint("First dice is gone: ready for next dice!\n");
              _state = WAITING_TO_SEE_DICE;
            }
          } else {
            _robot->GetBlockWorld().ClearObjectsByType(Block::Type::DICE);
            _diceDeletionTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
            if (_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
              // Keep clearing blocks until we don't see them anymore
              CoreTechPrint("Please move first dice away.\n");
              _robot->PlayAnimation("ANIM_HEAD_NOD", 2);
              _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 5;
              SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE2DISAPPEAR));
            }
          }
          break;
        }
          
        case WAITING_TO_SEE_DICE:
        {
          /*
          // DEBUG!!!
          _objectToPickUp = Block::NUMBER5_BLOCK_TYPE;
          _objectToPlaceOn = Block::NUMBER6_BLOCK_TYPE;
          _state = BEGIN_EXPLORING;
          break;
          */
          
          const f32 diceViewingHeadAngle = DEG_TO_RAD(-15);

          // Wait for robot to be IDLE
          if(_robot->IsIdle())
          {
            const BlockWorld::ObjectsMapByID_t& diceBlocks = _robot->GetBlockWorld().GetExistingObjectsByType(Block::Type::DICE);
            if(!diceBlocks.empty()) {
              
              if(diceBlocks.size() > 1) {
                // Multiple dice blocks in the world, keep deleting them all
                // until we only see one
                CoreTechPrint("More than one dice block found!\n");
                _robot->GetBlockWorld().ClearObjectsByType(Block::Type::DICE);
                
              } else {
                
                Block* diceBlock = dynamic_cast<Block*>(diceBlocks.begin()->second);
                CORETECH_ASSERT(diceBlock != nullptr);
                
                // Get all the observed markers on the dice and look for the one
                // facing up (i.e. the one that is nearly aligned with the z axis)
                // TODO: expose the threshold here?
                const TimeStamp_t timeWindow = _robot->GetLastMsgTimestamp() - 500;
                const f32 dotprodThresh = 1.f - cos(DEG_TO_RAD(20));
                std::vector<const Vision::KnownMarker*> diceMarkers;
                diceBlock->GetObservedMarkers(diceMarkers, timeWindow);
                
                const Vision::KnownMarker* topMarker = nullptr;
                for(auto marker : diceMarkers) {
                  //const f32 dotprod = DotProduct(marker->ComputeNormal(), Z_AXIS_3D());
                  Pose3d markerWrtRobotOrigin;
                  if(marker->GetPose().GetWithRespectTo(_robot->GetPose().FindOrigin(), markerWrtRobotOrigin) == false) {
                    PRINT_NAMED_ERROR("BehaviorManager.Update_June2014DiceDemo.MarkerOriginNotRobotOrigin",
                                      "Marker should share the same origin as the robot that observed it.\n");
                    Reset();
                  }
                  const f32 dotprod = marker->ComputeNormal(markerWrtRobotOrigin).z();
                  if(NEAR(dotprod, 1.f, dotprodThresh)) {
                    topMarker = marker;
                  }
                }
                
                // If dice exists in world but we haven't seen it for a while, delete it.
                if (diceMarkers.empty()) {
                  diceBlock->GetObservedMarkers(diceMarkers, _robot->GetLastMsgTimestamp() - 2000);
                  if (diceMarkers.empty()) {
                    CoreTechPrint("Haven't see dice marker for a while. Deleting dice.");
                    _robot->GetBlockWorld().ClearObjectsByType(Block::Type::DICE);
                    break;
                  }
                }
                
                if(topMarker != nullptr) {
                  // We found and observed the top marker on the dice. Use it to
                  // set which block we are looking for.
                  
                  // Don't forget to remove the dice as an ignore type for
                  // planning, since we _do_ want to avoid it as an obstacle
                  // when driving to pick and place blocks
                  _robot->GetPathPlanner()->RemoveIgnoreType(Block::Type::DICE);
                  
                  ObjectType blockToLookFor;
                  switch(static_cast<Vision::MarkerType>(topMarker->GetCode()))
                  {
                      /* 
                       BREAKING THIS TO USE NUMBERS ON ACTIVE BLOCKS
                    case Vision::MARKER_DICE1:
                    {
                      blockToLookFor = Block::Type::NUMBER1;
                      break;
                    }
                    case Vision::MARKER_DICE2:
                    {
                      blockToLookFor = Block::Type::NUMBER2;
                      break;
                    }
                    case Vision::MARKER_DICE3:
                    {
                      blockToLookFor = Block::Type::NUMBER3;
                      break;
                    }
                    case Vision::MARKER_DICE4:
                    {
                      blockToLookFor = Block::Type::NUMBER4;
                      break;
                    }
                    case Vision::MARKER_DICE5:
                    {
                      blockToLookFor = Block::Type::NUMBER5;
                      break;
                    }
                    case Vision::MARKER_DICE6:
                    {
                      blockToLookFor = Block::Type::NUMBER6;
                      break;
                    }
                      */
                      
                    default:
                      PRINT_NAMED_ERROR("BehaviorManager.UnknownDiceMarker",
                                        "Found unexpected marker on dice: %s!",
                                        Vision::MarkerTypeStrings[topMarker->GetCode()]);
                      StartMode(None);
                      return;
                  } // switch(topMarker->GetCode())
                  
                  CoreTechPrint("Found top marker on dice: %s!\n",
                                Vision::MarkerTypeStrings[topMarker->GetCode()]);
                  
                  if(_objectToPickUp.IsUnknown()) {
                    
                    _objectToPickUp = blockToLookFor;
                    _objectToPlaceOn.SetToUnknown();
                    
                    CoreTechPrint("Set blockToPickUp = %s\n", _objectToPickUp.GetName().c_str());
                    
                    // Wait for first dice to disappear
                    _state = WAITING_FOR_DICE_TO_DISAPPEAR;

                    SoundManager::getInstance()->Play(GetSoundName(SOUND_OK_GOT_IT));
                    
                    _waitUntilTime = 0;
                  } else {

                    if(blockToLookFor == _objectToPickUp) {
                      CoreTechPrint("Can't put a object on itself!\n");
                      // TODO:(bn) left and right + sad noise?
                    }
                    else {

                      _objectToPlaceOn = blockToLookFor;
                    
                      CoreTechPrint("Set objectToPlaceOn = %s\n", _objectToPlaceOn.GetName().c_str());

                      _robot->PlayAnimation("ANIM_HEAD_NOD", 2);
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2.5;

                      _state = BEGIN_EXPLORING;

                      SoundManager::getInstance()->Play(GetSoundName(SOUND_OK_GOT_IT));
                    }
                  }
                } else {
                  
                  CoreTechPrint("Found dice, but not its top marker.\n");
                  
                  //dockBlock_ = dynamic_cast<Block*>(diceBlock);
                  //CORETECH_THROW_IF(dockBlock_ == nullptr);
                  
                  // Try driving closer to dice
                  // Since we are purposefully trying to get really close to the
                  // dice, ignore it as an obstacle.  We'll consider an obstacle
                  // again later, when we start driving around to pick and place.
                  _robot->GetPathPlanner()->AddIgnoreType(Block::Type::DICE);
                  
                  Vec3f position( _robot->GetPose().GetTranslation() );
                  position -= diceBlock->GetPose().GetTranslation();
                  f32 actualDistToDice = position.Length();
                  f32 desiredDistToDice = ROBOT_BOUNDING_X_FRONT + 0.5f*diceBlock->GetSize().Length() + 5.f;

                  if (actualDistToDice > desiredDistToDice + 5) {
                    position.MakeUnitLength();
                    position *= desiredDistToDice;
                  
                    Radians angle = atan2(position.y(), position.x()) + PI_F;
                    position += diceBlock->GetPose().GetTranslation();
                    
                    _goalPose = Pose3d(angle, Z_AXIS_3D(), {{position.x(), position.y(), 0.f}});
                    
                    _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(_goalPose));

                  } else {
                    CoreTechPrint("Move dice closer!\n");
                  }
                  
                } // IF / ELSE top marker seen
                
              } // IF only one dice
              
              _timesIdle = 0;

            } // IF any diceBlocks available
            
            else {

              constexpr int numIdleForFrustrated = 3;
              constexpr float headUpWaitingAngle = DEG_TO_RAD(20);
              constexpr float headUpWaitingAngleFrustrated = DEG_TO_RAD(25);
              // Can't see dice
              switch(_idleState) {
                case IDLE_NONE:
                {
                  // if its been long enough, look up
                  if (_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
                    if(++_timesIdle >= numIdleForFrustrated) {
                      SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));

                      _originalPose = _robot->GetPose();

                      Pose3d userFacingPose = _robot->GetPose();
                      userFacingPose.SetRotation(USER_LOC_ANGLE_WRT_MAT, Z_AXIS_3D());
                      _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(userFacingPose));
                      CoreTechPrint("idle: facing user\n");

                      _idleState = IDLE_FACING_USER;
                    }
                    else {
                      CoreTechPrint("idle: looking up\n");
                      _robot->MoveHeadToAngle(headUpWaitingAngle, 3.0, 10);
                      _idleState = IDLE_LOOKING_UP;
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.7;
                    }
                  }
                  break;
                }

                case IDLE_LOOKING_UP:
                {
                  // once we get to the top, play the sound

                  if (_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
                    CoreTechPrint("idle: playing sound\n");
                    SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                    _idleState = IDLE_PLAYING_SOUND;
                    if(_timesIdle >= numIdleForFrustrated) {
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2.0;
                      SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                      SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                    }
                    else {
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5;
                    }
                  }
                  break;
                }

                case IDLE_PLAYING_SOUND:
                {
                  // once the sound is done, look back down
                  if (_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
                    CoreTechPrint("idle: looking back down\n");
                    _robot->MoveHeadToAngle(diceViewingHeadAngle, 1.5, 10);
                    if(_timesIdle >= numIdleForFrustrated) {
                      SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;
                      _idleState = IDLE_LOOKING_DOWN;
                    }
                    else {
                      _idleState = IDLE_NONE;
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 5;
                    }
                  }
                  break;
                }

                case IDLE_FACING_USER:
                {
                  // once we get there, look up
                  if(_robot->IsIdle()) {
                    SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                    CoreTechPrint("idle: looking up\n");
                    _robot->MoveHeadToAngle(headUpWaitingAngleFrustrated, 3.0, 10);
                    _idleState = IDLE_LOOKING_UP;
                    _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;
                  }
                  break;
                }

                case IDLE_LOOKING_DOWN:
                {
                  // once we are looking back down, turn back to the original pose
                  if(_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() &&
                     _robot->IsIdle()) {

                    CoreTechPrint("idle: turning back\n");
                    SoundManager::getInstance()->Play(GetSoundName(SOUND_WAITING4DICE));
                    _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(_originalPose));
                    _idleState = IDLE_TURNING_BACK;
                    _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.25;
                  }
                  break;
                }

                case IDLE_TURNING_BACK:
                {
                  if (_waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
                    if(_robot->IsIdle()) {
                      CoreTechPrint("idle: waiting for dice\n");
                      _timesIdle = 0;
                      _idleState = IDLE_NONE;
                      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 5;
                    }
                  }
                  break;
                }

                default:
                CoreTechPrint("ERROR: invalid idle state %d\n", _idleState);
              }
            }
          } // IF robot is IDLE
          
          break;
        } // case WAITING_FOR_FIRST_DICE
          
        case BACKING_UP:
        {
          const f32 currentDistance = (_robot->GetPose().GetTranslation() -
                                       _goalPose.GetTranslation()).Length();
          
          if(currentDistance >= _desiredBackupDistance )
          {
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
            _robot->DriveWheels(0.f, 0.f);
            _state = _nextState;
          }
          
          break;
        } // case BACKING_UP
        case GOTO_EXPLORATION_POSE:
        {
          const BlockWorld::ObjectsMapByID_t& blocks = _robot->GetBlockWorld().GetExistingObjectsByType(_objectTypeOfInterest);
          if (_robot->IsIdle() || !blocks.empty()) {
            _state = START_EXPLORING_TURN;
          }
          break;
        } // case GOTO_EXPLORATION_POSE
        case BEGIN_EXPLORING:
        {
          // For now, "exploration" is just spinning in place to
          // try to locate blocks
          if(!_robot->IsMoving() && _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            
            if(_robot->IsCarryingObject()) {
              _objectTypeOfInterest = _objectToPlaceOn;
            } else {
              _objectTypeOfInterest = _objectToPickUp;
            }
            
            
            // If we already know where the blockOfInterest is, then go straight to it
            const BlockWorld::ObjectsMapByID_t& blocks = _robot->GetBlockWorld().GetExistingObjectsByType(_objectTypeOfInterest);
            if(blocks.empty()) {
              // Compute desired pose at mat center
              Pose3d robotPose = _robot->GetPose();
              f32 targetAngle = _explorationStartAngle.ToFloat();
              if (_explorationStartAngle == 0) {
                // If this is the first time we're exploring, then start exploring at the pose
                // we expect to be in when we reach the mat center. Other start exploring at the angle
                // we last stopped exploring.
                targetAngle = atan2(robotPose.GetTranslation().y(), robotPose.GetTranslation().x()) + PI_F;
              }
              Pose3d targetPose(targetAngle, Z_AXIS_3D(), Vec3f(0,0,0));
              
              if (ComputeDistanceBetween(targetPose, robotPose) > 50.f) {
                PRINT_STREAM_INFO("BehaviorManager.DiceDemo", std::setprecision(5) << "Going to mat center for exploration (" << targetPose.GetTranslation().x() << " " << targetPose.GetTranslation().y() << " " << targetAngle << ")");
                _robot->GetPathPlanner()->AddIgnoreType(Block::Type::DICE);
                _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(targetPose));
              }

              _state = GOTO_EXPLORATION_POSE;
            } else {
              _state = EXPLORING;
            }
          }
          
          break;
        } // case BEGIN_EXPLORING
        case START_EXPLORING_TURN:
        {
          PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Beginning exploring");
          _robot->GetPathPlanner()->RemoveIgnoreType(Block::Type::DICE);
          _robot->DriveWheels(8.f, -8.f);
          _robot->MoveHeadToAngle(DEG_TO_RAD(-10), 1, 1);
          _explorationStartAngle = _robot->GetPose().GetRotationAngle<'Z'>();
          _isTurning = true;
          _state = EXPLORING;
          break;
        }
        case EXPLORING:
        {
          // If we've spotted the block we're looking for, stop exploring, and
          // execute a path to that block
          const BlockWorld::ObjectsMapByID_t& blocks = _robot->GetBlockWorld().GetExistingObjectsByType(_objectTypeOfInterest);
          if(!blocks.empty()) {
            // Dock with the first block of the right type that we see
            // TODO: choose the closest?
            Block* dockBlock = dynamic_cast<Block*>(blocks.begin()->second);
            CORETECH_THROW_IF(dockBlock == nullptr);
            
            _robot->DriveWheels(0.f, 0.f);
            
            _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPickAndPlaceObjectAction(dockBlock->GetID()));
            
            _state = EXECUTING_DOCK;
            
            _wasCarryingBlockAtDockingStart = _robot->IsCarryingObject();

            SoundManager::getInstance()->Play(GetSoundName(SOUND_OK_GOT_IT));
            
            PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "STARTING DOCKING");
            break;
          }
          
          // Repeat turn-stop behavior for more reliable block detection
          Radians currAngle = _robot->GetPose().GetRotationAngle<'Z'>();
          if (_isTurning && (std::abs((_explorationStartAngle - currAngle).ToFloat()) > DEG_TO_RAD(40))) {
            PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Exploration - pause turning. Looking for " << _objectTypeOfInterest.GetName().c_str());
            _robot->DriveWheels(0.f,0.f);
            _isTurning = false;
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
          } else if (!_isTurning && _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            _state = START_EXPLORING_TURN;
          }
          
          break;
        } // EXPLORING
   
        case EXECUTING_DOCK:
        {
          // Wait for the robot to go back to IDLE
          if(_robot->IsIdle())
          {
            const bool donePickingUp = _robot->IsCarryingObject() &&
                                       _robot->GetBlockWorld().GetObjectByID(_robot->GetCarryingObject())->GetType() == _objectToPickUp;
            if(donePickingUp) {
              PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Picked up block " << _robot->GetCarryingObject().GetValue() << " successfully! Going back to exploring for block to place on.");
              
              _state = BEGIN_EXPLORING;
              
              SoundManager::getInstance()->Play(GetSoundName(SOUND_NOTIMPRESSED));
              
              return;
            } // if donePickingUp
            
            const bool donePlacing = !_robot->IsCarryingObject() && _wasCarryingBlockAtDockingStart;
            if(donePlacing) {
              PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Placed block " << _objectToPickUp.GetValue() << " on " << _objectToPlaceOn.GetValue() << " successfully! Going back to waiting for dice.");

              _robot->MoveHeadToAngle(checkItOutAngleUp, checkItOutSpeed, 10);
              _state = CHECK_IT_OUT_UP;
              _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2.f;

              // TODO:(bn) sound: minor success??
              
              return;
            } // if donePlacing
            
            
            // Either pickup or placement failed
            const bool pickupFailed = !_robot->IsCarryingObject();
            if (pickupFailed) {
              PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Block pickup failed. Retrying...");
            } else {
              PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "Block placement failed. Retrying...");
            }
            
            // Backup to re-explore the block
            _robot->MoveHeadToAngle(DEG_TO_RAD(-5), 10, 10);
            _robot->DriveWheels(-20.f, -20.f);
            _state = BACKING_UP;
            _nextState = BEGIN_EXPLORING;
            _desiredBackupDistance = 30;
            _goalPose = _robot->GetPose();
            
            SoundManager::getInstance()->Play(GetSoundName(SOUND_STARTOVER));
            
          } // if robot IDLE
          
          break;
        } // case EXECUTING_DOCK

        case CHECK_IT_OUT_UP:
        {
          // Wait for the robot to go back to IDLE
          if(_robot->IsIdle() &&
             _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())
          {
            // TODO:(bn) small happy chirp sound
            _robot->MoveHeadToAngle(checkItOutAngleDown, checkItOutSpeed, 10);
            _state = CHECK_IT_OUT_DOWN;
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2.f;
          }
          break;
        }

        case CHECK_IT_OUT_DOWN:
        {
          // Wait for the robot to go back to IDLE
          if(_robot->IsIdle() &&
             _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())
          {
            // Compute pose that makes robot face user
            Pose3d userFacingPose = _robot->GetPose();
            userFacingPose.SetRotation(USER_LOC_ANGLE_WRT_MAT, Z_AXIS_3D());
            _robot->GetActionList().QueueActionAtEnd(TraversalSlot, new DriveToPoseAction(userFacingPose));

            SoundManager::getInstance()->Play(GetSoundName(SOUND_OK_GOT_IT));
            _state = FACE_USER;
          }
          break;
        }

        case FACE_USER:
        {
          // Wait for the robot to go back to IDLE
          if(_robot->IsIdle())
          {
            // Start nodding
            _robot->PlayAnimation("ANIM_HEAD_NOD");
            _state = HAPPY_NODDING;
            PRINT_STREAM_INFO("BehaviorManager.DiceDemo", "NODDING_HEAD");
            SoundManager::getInstance()->Play(GetSoundName(SOUND_OK_DONE));
            
            // Compute time to stop nodding
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;
          }
          break;
        } // case FACE_USER
        case HAPPY_NODDING:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime) {
            _robot->PlayAnimation("ANIM_BACK_AND_FORTH_EXCITED");
            _robot->MoveHeadToAngle(DEG_TO_RAD(-10), 1, 1);
            
            // Compute time to stop back and forth
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.5;
            _state = BACK_AND_FORTH_EXCITED;
          }
          break;
        } // case HAPPY_NODDING
        case BACK_AND_FORTH_EXCITED:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime) {
            _robot->PlayAnimation("ANIM_IDLE");
            _robot->GetBlockWorld().ClearAllExistingObjects();
            StartMode(June2014DiceDemo);
          }
          break;
        }
        default:
        {
          PRINT_STREAM_ERROR("BehaviorManager.UnknownBehaviorState",
                            "Transitioned to unknown state " << _state << "!");
          StartMode(None);
          return;
        }
      } // switch(_state)
      
    } // Update_June2014DiceDemo()

    
    void BehaviorManager::Update_CREEP()
    {
      if(_state != _nextState) {

        std::string transitionAnimName("");
        
        // First see if there is a transition specific to this to/from pair
        auto fromIter = _transitionAnimations.find(_state);
        if(fromIter != _transitionAnimations.end()) {
          auto toIter = fromIter->second.find(_nextState);
          if(toIter != fromIter->second.end()) {
            transitionAnimName = toIter->second;
          }
        }

        // Next see if there is a transition from any state to next state
        if(transitionAnimName.empty()) {
          auto fromAnyIter = _transitionAnimations.find(NUM_STATES);
          if(fromAnyIter != _transitionAnimations.end()) {
            auto toIter = fromAnyIter->second.find(_nextState);
            if(toIter != fromAnyIter->second.end()) {
              transitionAnimName = toIter->second;
            }
          }
        }
        
        // Next see if there is a transition from current state to any state
        if(transitionAnimName.empty()) {
          if(fromIter != _transitionAnimations.end()) {
            auto toAnyIter = fromIter->second.find(NUM_STATES);
            if(toAnyIter != fromIter->second.end()) {
              transitionAnimName = toAnyIter->second;
            }
          }
        }
        
        // See if there is state animation for the next state
        std::string stateAnimationName("");
        auto stateIter = _stateAnimations.find(_nextState);
        if(stateIter != _stateAnimations.end()) {
          stateAnimationName = stateIter->second;
        }
        
        if(!transitionAnimName.empty()) {
          if(!stateAnimationName.empty()) {
            // There is a transition and state animatino defined
            //_robot->TransitionToStateAnimation(transitionAnimName.c_str(),
            //                                   stateAnimationName.c_str());
          } else {
            // Transition animation but no state animation: just play the
            // transition animation once
            _robot->PlayAnimation(transitionAnimName, 1);
          }
        } else if(!stateAnimationName.empty()) {
          // No transition animation: just loop the state animation
          _robot->PlayAnimation(stateAnimationName, 0);
        }
        
        _state = _nextState;

        VizManager::getInstance()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::YELLOW, GetBehaviorStateName(_state).c_str());
      }
      
    } // Update_CREEP()
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //                NEW BEHAVIOR ENGINE API EXPLORATION
    //
    
    IBehavior::Status OCD_Behavior::Init(Robot& robot)
    {
      
      // Register to listen for "block" observed signals/messages instead of
      // polling the robot on every call to GetReward()?
      
    }
    
    IBehavior::Status OCD_Behavior::Update(Robot& robot)
    {
      switch(_currentState)
      {
          
      }
    }
    
    void OCD_Behavior::HandleObservedObject(RobotObservedObject &msg)
    {
      // if the object is a BLOCK or ACTIVE_BLOCK, add its ID to the list we care about
      _objectsOfInterest.insert(msg.objectID);
    }
    
    void OCD_Behavior::HandleDeletedObject(RobotDeletedObject &msg)
    {
      // remove the object if we knew about it
      _objectsOfInterest.erase(msg.objectID);
    }
    
    bool OCD_Behavior::GetRewardBid(Robot& robot, Reward& reward)
    {
      const BlockWorld& blockWorld = robot.GetBlockWorld();
      
      //const EmotionMgr emo = robot.GetEmotionMgr();
      
      const BlockWorld::ObjectsMapByType_t& blocks = blockWorld.GetExistingObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
      
      const BlockWorld::ObjectsMapByType_t& lightCubes = blockWorld.GetExistingObjectsByFamily(BlockWorld::ObjectFamily::ACTIVE_BLOCKS);
      
      if(blocks.empty() && lightCubes.empty()) {
        // If there are no blocks to get OCD about, this isn't a viable behavior
        return false;
      }
     
      // TODO: Populate a reward object based on how many poorly-arranged blocks there are
        
      return true;
    } // GetReward()
    
  } // namespace Cozmo
} // namespace Anki
