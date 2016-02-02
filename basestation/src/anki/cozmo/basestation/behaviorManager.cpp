
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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/demoBehaviorChooser.h"
#include "anki/cozmo/basestation/investorDemoFacesAndBlocksBehaviorChooser.h"
#include "anki/cozmo/basestation/investorDemoFacesChooser.h"
#include "anki/cozmo/basestation/investorDemoMotionBehaviorChooser.h"
#include "anki/cozmo/basestation/selectionBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "anki/cozmo/basestation/moodSystem/moodDebug.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/types/behaviorChooserType.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#include "anki/common/basestation/utils/timer.h"

#define DEBUG_BEHAVIOR_MGR 0

namespace Anki {
namespace Cozmo {
  
  
  BehaviorManager::BehaviorManager(Robot& robot)
  : _isInitialized(false)
  , _forceReInit(false)
  , _robot(robot)
  , _behaviorFactory(new BehaviorFactory())
  , _minBehaviorTime_sec(1)
  {

  }
  
  Result BehaviorManager::Init(const Json::Value &config)
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
    
    // TODO: Set configuration data from Json...
    
    // TODO: Only load behaviors specified by Json?
    
    SetupOctDemoBehaviorChooser(config);
    
    if (_robot.HasExternalInterface())
    {
      IExternalInterface* externalInterface = _robot.GetExternalInterface();
      _eventHandlers.push_back(externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::ActivateBehaviorChooser,
       [this, config] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
       {
         switch (event.GetData().Get_ActivateBehaviorChooser().behaviorChooserType)
         {
           case BehaviorChooserType::Demo:
           {
             SetupOctDemoBehaviorChooser(config);
             break;
           }
           case BehaviorChooserType::Selection:
           {
             // DEMO HACK: clean the idle animation here
             // TODO:(bn) better way to do this
             _robot.SetIdleAnimation("NONE");
             
             SetBehaviorChooser(new SelectionBehaviorChooser(_robot, config));
             break;
           }
           case BehaviorChooserType::InvestorDemoMotion:
           {
             SetBehaviorChooser( new InvestorDemoMotionBehaviorChooser(_robot, config) );

             // BehaviorFactory& behaviorFactory = GetBehaviorFactory();
             // AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToPickup, _robot, config)->AsReactionaryBehavior() );
             // AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToCliff,  _robot, config)->AsReactionaryBehavior() );
             // AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToPoke,   _robot, config)->AsReactionaryBehavior() );
             break;
           }
           case BehaviorChooserType::InvestorDemoFacesAndBlocks:
           {
             SetBehaviorChooser( new InvestorDemoFacesAndBlocksBehaviorChooser(_robot, config) );
             
             // BehaviorFactory& behaviorFactory = GetBehaviorFactory();
             // AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToPoke,   _robot, config)->AsReactionaryBehavior() );
             break;
           }
           case BehaviorChooserType::InvestorDemoFacesOnly:
           {
             SetBehaviorChooser( new InvestorDemoFacesBehaviorChooser(_robot, config) );
             break;
           }
           default:
           {
             PRINT_NAMED_WARNING("BehaviorManager.ActivateBehaviorChooser.InvalidChooser",
                                 "don't know how to create a chooser of type '%s'",
                                 BehaviorChooserTypeToString(
                                   event.GetData().Get_ActivateBehaviorChooser().behaviorChooserType));
             break;
           }
         }
       }));
    }
    _isInitialized = true;
    
    _lastSwitchTime_sec = 0.f;
    
    return RESULT_OK;
  }
  
  void BehaviorManager::SetupOctDemoBehaviorChooser(const Json::Value &config)
  {
    IBehaviorChooser* chooser = new DemoBehaviorChooser(_robot, config);
    SetBehaviorChooser( chooser );
    
    BehaviorFactory& behaviorFactory = GetBehaviorFactory();
    AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToPickup, _robot, config)->AsReactionaryBehavior() );
    AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToCliff,  _robot, config)->AsReactionaryBehavior() );
    AddReactionaryBehavior( behaviorFactory.CreateBehavior(BehaviorType::ReactToPoke,   _robot, config)->AsReactionaryBehavior() );

    // for now, these aren't working nicely, and wanted to test this system
    chooser->SetBannedBehaviorGroup( BehaviorGroup::EmotionalReaction );
  }
  
  // The AddReactionaryBehavior wrapper is responsible for setting up the callbacks so that important events will be
  // reacted to correctly - events will be given to the Chooser which may return a behavior to force switch to
  void BehaviorManager::AddReactionaryBehavior(IReactionaryBehavior* behavior)
  {
    // We map reactionary behaviors to the tag types they're going to care about
    _behaviorChooser->AddReactionaryBehavior(behavior);
    
    // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
    if (!_robot.HasExternalInterface()) {
      return;
    }
    
    // Callback for EngineToGame event that a reactionary behavior (possibly) cares about
    auto reactionsEngineToGameCallback = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
    {
      _forceSwitchBehavior = _behaviorChooser->GetReactionaryBehavior(_robot, event);
    };
    
    // Subscribe our own callback to these events
    IExternalInterface* interface = _robot.GetExternalInterface();
    for (auto tag : behavior->GetEngineToGameTags())
    {
      _eventHandlers.push_back(interface->Subscribe(tag, reactionsEngineToGameCallback));
    }
    
    // Callback for GameToEngine event that a reactionary behavior (possibly) cares about
    auto reactionsGameToEngineCallback = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      _forceSwitchBehavior = _behaviorChooser->GetReactionaryBehavior(_robot, event);
    };
    
    // Subscribe our own callback to these events
    for (auto tag : behavior->GetGameToEngineTags())
    {
      _eventHandlers.push_back(interface->Subscribe(tag, reactionsGameToEngineCallback));
    }
  }
  
  BehaviorManager::~BehaviorManager()
  {
    Util::SafeDelete(_behaviorChooser);
    Util::SafeDelete(_behaviorFactory);
  }
  
  void BehaviorManager::SwitchToNextBehavior(double currentTime_sec)
  {
    // If we're currently running our forced behavior but now switching away, clear it
    if (_currentBehavior == _forceSwitchBehavior)
    {
      _forceSwitchBehavior = nullptr;
    }
    
    // Initialize next behavior and make it the current one
    if (nullptr != _nextBehavior)
    {
      #if SEND_MOOD_TO_VIZ_DEBUG
      {
        VizInterface::NewBehaviorSelected newBehaviorSelected;
        newBehaviorSelected.newCurrentBehavior = _nextBehavior ? _nextBehavior->GetName() : "null";
        VizManager::getInstance()->SendNewBehaviorSelected(std::move(newBehaviorSelected));
      }
      #endif // SEND_MOOD_TO_VIZ_DEBUG
      
      const bool isResuming = (_nextBehavior == _resumeBehavior);
      const bool isSameBehavior = (_nextBehavior == _currentBehavior);
      _resumeBehavior = nullptr;
      if (!isSameBehavior && _currentBehavior && _nextBehavior->IsShortInterruption() && _currentBehavior->WantsToResume())
      {
        _resumeBehavior = _currentBehavior;
      }

      SetCurrentBehavior(_nextBehavior, currentTime_sec, isResuming);
      _nextBehavior = nullptr;
    }
  }
  
  Result BehaviorManager::Update(double currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
      return RESULT_FAIL;
    }
    
    _behaviorChooser->Update(currentTime_sec);
    
    // If we happen to have a behavior we really want to switch to, do so
    if (nullptr != _forceSwitchBehavior && _currentBehavior != _forceSwitchBehavior)
    {
      _nextBehavior = _forceSwitchBehavior;
      
      lastResult = InitNextBehaviorHelper(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.InitForcedBehavior",
                            "Failed trying to force next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
    }
    else if (nullptr == _currentBehavior ||
             currentTime_sec - _lastSwitchTime_sec > _minBehaviorTime_sec )
    // This check should not be needed. The current behavior should decide when it's done and
    // return Status::Complete, which will trigger a new selection immediately
    // ( nullptr != _currentBehavior && ! _currentBehavior->IsRunnable(_robot, currentTime_sec) ))
    {
      // We've been in the current behavior long enough to consider switching
      lastResult = SelectNextBehavior(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.SelectNextFailed",
                            "Failed trying to select next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
      
      
      if (_currentBehavior != _nextBehavior && nullptr != _nextBehavior)
      {
        std::string nextName = _nextBehavior->GetName();
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Update.SelectedNext",
                               "Selected next behavior '%s' at t=%.1f, last was t=%.1f",
                               nextName.c_str(), currentTime_sec, _lastSwitchTime_sec);
        
        _lastSwitchTime_sec = currentTime_sec;
      }
    }
    
    if(nullptr != _currentBehavior) {
      // We have a current behavior, update it.
      IBehavior::Status status = _currentBehavior->Update(currentTime_sec);
     
      switch(status)
      {
        case IBehavior::Status::Running:
          // Nothing to do! Just keep on truckin'....
          break;
          
        case IBehavior::Status::Complete:
          // Behavior complete, try to select and switch to next
          lastResult = SelectNextBehavior(currentTime_sec);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_WARNING("BehaviorManager.Update.Complete.SelectNextFailed",
                                "Failed trying to select next behavior.");
            lastResult = RESULT_OK;
          }
          SwitchToNextBehavior(currentTime_sec);
          break;
          
        case IBehavior::Status::Failure:
          PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                            "Behavior '%s' failed to Update().",
                            _currentBehavior->GetName().c_str());
          lastResult = RESULT_FAIL;
          
          // Force a re-init so if we reselect this behavior
          _forceReInit = true;
          SelectNextBehavior(currentTime_sec);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_WARNING("BehaviorManager.Update.Failure.SelectNextFailed",
                                "Failed trying to select next behavior.");
            lastResult = RESULT_OK;
          }
          SwitchToNextBehavior(currentTime_sec);
          // WARNING: While working here I realized that lastResult is not updated with the result of SelectNextBehavior
          // this may be because we want to notify outside that the current behavior failed. But we actually try to
          // recover from it nicely, so we may want to update lastResult after all. It seems calling code is ignoring
          // the result code anyway..
          break;
          
        default:
          PRINT_NAMED_ERROR("BehaviorManager.Update.UnknownStatus",
                            "Behavior '%s' returned unknown status %d",
                            _currentBehavior->GetName().c_str(), status);
          lastResult = RESULT_FAIL;
      } // switch(status)
    }
    else if(nullptr != _nextBehavior) {
      // No current behavior, but next behavior defined, so switch to it.
      SwitchToNextBehavior(currentTime_sec);
    }
    
    return lastResult;
  } // Update()
  
  
  Result BehaviorManager::InitNextBehaviorHelper(float currentTime_sec)
  {
    Result initResult = RESULT_OK;
    
    // Initialize the selected behavior, if it's not the one we're already running
    if(_nextBehavior != _currentBehavior || _forceReInit)
    {
      _forceReInit = false;
      if(nullptr != _currentBehavior) {
        // Interrupt the current behavior that's running if there is one. It will continue
        // to run on calls to Update() until it completes and then we will switch
        // to the selected next behavior
        const bool isShortInterrupt = _nextBehavior && _nextBehavior->IsShortInterruption();
        initResult = _currentBehavior->Interrupt(currentTime_sec, isShortInterrupt);
        
        if (nullptr != _nextBehavior)
        {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManger.InitNextBehaviorHelper.Selected",
                                 "Selected %s to run next.", _nextBehavior->GetName().c_str());
        }
      }
    }
    return initResult;
  }
  
  Result BehaviorManager::SelectNextBehavior(double currentTime_sec)
  {
    _nextBehavior = _behaviorChooser->ChooseNextBehavior(_robot, currentTime_sec);

    if(nullptr == _nextBehavior) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.NoneRunnable", "");
      return RESULT_FAIL;
    }
    
    // Initialize the selected behavior
    return InitNextBehaviorHelper(currentTime_sec);
    
  } // SelectNextBehavior()
  
  Result BehaviorManager::SelectNextBehavior(const std::string& name, double currentTime_sec)
  {
    _nextBehavior = _behaviorChooser->GetBehaviorByName(name);
    if(nullptr == _nextBehavior) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.UnknownName",
                        "No behavior named '%s'", name.c_str());
      return RESULT_FAIL;
    }
    else if(_nextBehavior->IsRunnable(_robot, currentTime_sec) == false) {
      PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NotRunnable",
                        "Behavior '%s' is not runnable.", name.c_str());
      return RESULT_FAIL;
    }
    
    return InitNextBehaviorHelper(currentTime_sec);
  }
  
  void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
  {
    // These behavior pointers might be invalidated, so clear them
    // SetCurrentBehavior ensures that any existing current behavior is stopped first
    
    SetCurrentBehavior(nullptr, BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), false);
    _nextBehavior = nullptr;
    _forceSwitchBehavior = nullptr;
    _resumeBehavior = nullptr;

    if( _behaviorChooser != nullptr ) {
      PRINT_NAMED_INFO("BehaviorManager.SetBehaviorChooser.DeleteOld",
                       "deleting behavior chooser '%s'",
                       _behaviorChooser->GetName());
    }
    
    Util::SafeDelete(_behaviorChooser);
    
    _behaviorChooser = newChooser;

    // force the new behavior chooser to select something now, instead of waiting for it to be ready
    SelectNextBehavior(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());
  }
  
  void BehaviorManager::SetCurrentBehavior(IBehavior* newBehavior, double currentTime_sec, bool isResuming)
  {
    // stop current
    if (_currentBehavior) {
      _currentBehavior->SetIsRunning(false);
      _currentBehavior->Stop(currentTime_sec);
    }
    
    // set current <- new
    _currentBehavior = newBehavior;
    
    // initialize new
    if (_currentBehavior) {
    
      const Result initRet = _nextBehavior->Init(currentTime_sec, isResuming);
      if ( initRet != RESULT_OK ) {
        PRINT_NAMED_ERROR("BehaviorManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        _currentBehavior->GetName().c_str());
      }
      else {
        PRINT_NAMED_DEBUG("BehaviorManger.InitBehavior.Success",
                          "Behavior '%s' initialized",
                          _currentBehavior->GetName().c_str());
      }
      
      // flag as the running behavior
      _currentBehavior->SetIsRunning(true);
    }
    
  }

  IBehavior* BehaviorManager::LoadBehaviorFromJson(const Json::Value& behaviorJson)
  {
    IBehavior* newBehavior = _behaviorFactory->CreateBehavior(behaviorJson, _robot);
    return newBehavior;
  }
  
  void BehaviorManager::ClearAllBehaviorOverrides()
  {
    const BehaviorFactory::NameToBehaviorMap& nameToBehaviorMap = _behaviorFactory->GetBehaviorMap();
    for(const auto& it : nameToBehaviorMap)
    {
      IBehavior* behavior = it.second;
      behavior->SetOverrideScore(-1.0f);
    }
  }
  
  bool BehaviorManager::OverrideBehaviorScore(const std::string& behaviorName, float newScore)
  {
    IBehavior* behavior = _behaviorFactory->FindBehaviorByName(behaviorName);
    if (behavior)
    {
      behavior->SetOverrideScore(newScore);
      return true;
    }
    return false;
  }
  
  void BehaviorManager::HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message)
  {
    switch (message.GetTag())
    {
      case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableAllBehaviors:
      {
        const auto& msg = message.Get_SetEnableAllBehaviors();
        if (_behaviorChooser)
        {
          _behaviorChooser->EnableAllBehaviors(msg.enable);
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.SetEnableAllBehaviorGroups.NullChooser",
                              "Ignoring EnableAllBehaviorGroups(%d)", (int)msg.enable);
        }
        break;
      }
      case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableBehaviorGroup:
      {
        const auto& msg = message.Get_SetEnableBehaviorGroup();
        if (_behaviorChooser)
        {
          _behaviorChooser->EnableBehaviorGroup(msg.behaviorGroup, msg.enable);
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.SetEnableBehaviorGroup.NullChooser",
                              "Ignoring EnableBehaviorGroup('%s', %d)", BehaviorGroupToString(msg.behaviorGroup), (int)msg.enable);
        }
        break;
      }
      case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableBehavior:
      {
        const auto& msg = message.Get_SetEnableBehavior();
        if (_behaviorChooser)
        {
          _behaviorChooser->EnableBehavior(msg.behaviorName, msg.enable);
        }
        else
        {
          PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.DisableBehaviorGroup.NullChooser",
                              "Ignoring DisableBehaviorGroup('%s', %d)", msg.behaviorName.c_str(), (int)msg.enable);
        }
        break;
      }
      case ExternalInterface::BehaviorManagerMessageUnionTag::ClearAllBehaviorScoreOverrides:
      {
        ClearAllBehaviorOverrides();
        break;
      }
      case ExternalInterface::BehaviorManagerMessageUnionTag::OverrideBehaviorScore:
      {
        const auto& msg = message.Get_OverrideBehaviorScore();
        OverrideBehaviorScore(msg.behaviorName, msg.newScore);
        break;
      }
      default:
      {
        PRINT_NAMED_ERROR("BehaviorManager.HandleEvent.UnhandledMessageUnionTag",
                          "Unexpected tag %u '%s'", (uint32_t)message.GetTag(),
                          BehaviorManagerMessageUnionTagToString(message.GetTag()));
        assert(0);
        break;
      }
    }
  }
  
} // namespace Cozmo
} // namespace Anki
