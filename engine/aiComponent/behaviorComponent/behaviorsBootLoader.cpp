/**
 * File: behaviorsBootLoader.cpp
 *
 * Author: ross
 * Created: 2018 jun 2
 *
 * Description: Component that decides what behavior stack to initialize upon engine loading. This is
 *              based on factory state and dev options, but primarily is based on onboarding state.
 *              Since onboarding only occurs once in a robot's lifetime, this component "reboots"
 *              the behavior stack when onboarding ends, instead of having one giant stack containing
 *              both onboarding and regular behaviors.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingCoordinator.h"
#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/unitTestKey.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "util/console/consoleInterface.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {
  
namespace {
 
# if !ALEXA_ACOUSTIC_TEST
  CONSOLE_VAR(bool, kAcousticTestMode, "Alexa", false);
# endif

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorsBootLoader::BehaviorsBootLoader(const Json::Value& config)
  : IDependencyManagedComponent( this, BCComponentID::BehaviorsBootLoader )
{
  _behaviors.factoryBehavior = BEHAVIOR_ID(PlaypenTest);
  _behaviors.prDemoBehavior = BEHAVIOR_ID(InitPRDemo);
  _behaviors.selfTestBehavior = BEHAVIOR_ID(SelfTest);
  _behaviors.acousticTestBehavior = BEHAVIOR_ID(AcousticTestMode);
  
  if( ANKI_VERIFY(!config.empty(), "BehaviorsBootLoader.Ctor.InvalidConfig", "Empty config") ) {
    
    auto setFromJSON = [&config](const char* key) {
      const std::string behaviorIDStr = JsonTools::ParseString(config, key, "BehaviorsBootLoader");
      return BehaviorTypesWrapper::BehaviorIDFromString(behaviorIDStr);
    };
    _behaviors.onboardingBehavior = setFromJSON("onboardingBehavior");
    _behaviors.normalBaseBehavior = setFromJSON("normalBaseBehavior");
    _behaviors.postOnboardingBehavior = setFromJSON("postOnboardingBehavior");
    _behaviors.devBaseBehavior = setFromJSON("devBaseBehavior");
  } else {
    _behaviors.onboardingBehavior = BEHAVIOR_ID(Wait);
    _behaviors.normalBaseBehavior = BEHAVIOR_ID(Wait);
    _behaviors.postOnboardingBehavior = BEHAVIOR_ID(Wait);
    _behaviors.devBaseBehavior = BEHAVIOR_ID(Wait);
  }
  
# if ALEXA_ACOUSTIC_TEST
  {
    _behaviors.postOnboardingBehavior = _behaviors.acousticTestBehavior;
    _behaviors.normalBaseBehavior = _behaviors.acousticTestBehavior;
  }
# endif
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorsBootLoader::BehaviorsBootLoader( IBehavior* overrideBehavior, const UnitTestKey& key )
  : IDependencyManagedComponent( this, BCComponentID::BehaviorsBootLoader )
{
  _overrideBehavior = overrideBehavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  _robot = robot;
  
  using namespace Util;
  const Data::DataPlatform* platform = robot->GetContextDataPlatform();
  _saveFolder = platform->pathToResource( Data::Scope::Persistent, BehaviorOnboardingCoordinator::kOnboardingFolder );
  _saveFolder = FileUtils::AddTrailingFileSeparator( _saveFolder );
  
  if( FileUtils::DirectoryDoesNotExist( _saveFolder ) ) {
    FileUtils::CreateDirectory( _saveFolder, false, true );
  }
  
  _externalInterface = robot->GetExternalInterface();
  
  _behaviorContainer = dependentComps.GetComponentPtr<BehaviorContainer>();

  const bool inPRDemo = robot->GetContext()->GetFeatureGate()->IsFeatureEnabled( FeatureType::PRDemo );
  
  // If this is the factory test forcibly set _bootBehavior as playpen as long as the robot has not been through packout
  bool startInPlaypen = false;
# if FACTORY_TEST
  {
    startInPlaypen = !Factory::GetEMR()->fields.PACKED_OUT_FLAG && !OSState::getInstance()->IsInRecoveryMode();
  }
# endif
  if(startInPlaypen) {
    SetNewBehavior( _behaviors.factoryBehavior );
  } else if( _overrideBehavior != nullptr ) {
    _bootBehavior = _overrideBehavior;
  } else if( inPRDemo ) {
    SetNewBehavior( _behaviors.prDemoBehavior );
  } else {
    InitOnboarding();
  }

  auto* msgHandler = robot->GetRobotMessageHandler();
  if(msgHandler != nullptr)
  {
    auto startSelfTestLambda = [this, robot](const AnkiEvent<RobotInterface::RobotToEngine>& event)
      {
        // When the self test starts, save the current boot behavior as well as the
        // bottom of the behavior stack in order to restore them when the self test ends
        // Note: The bottom of the stack should be the wait behavior due to having to go through
        // the customer care screen to start the self test
        _prevBootBehavior = GetBootBehavior();
        
        auto& bsm = robot->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<BehaviorSystemManager>();
        _prevBottomOfStackBehavior = const_cast<IBehavior*>(bsm.GetBaseBehavior());

        // Switch to the self test behavior
        SetNewBehavior(_behaviors.selfTestBehavior);
      };

    _eventHandles.push_back(msgHandler->Subscribe(RobotInterface::RobotToEngineTag::startSelfTest,
                                                  startSelfTestLambda));
  }
                                                                  
  auto* ei = _externalInterface;
  if( ei != nullptr ) {
    // listen for messages from the robot that changed onboarding state. don't worry about anything from the app, since it
    // can't tell the robot to change its onboarding state
    auto onOnboardingStage = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& onboardingStageEvt){
      const auto& msg = onboardingStageEvt.GetData().Get_OnboardingState();
      const auto newState = msg.stage;
      // save the stage so the app knows it, but only change the stack if requested, so that, if desired,
      // onboarding can remain in app onboarding (Complete). Even if not desired, set the _bootBehavior
      // without changing the behavior stack so that entering and leaving CC screen then pulls the new
      // behavior
      _stage = newState;
      const bool requestStackReset = !msg.forceSkipStackReset;
      if( static_cast<u8>(newState) < static_cast<u8>(OnboardingStages::Complete) ) {
        RestartOnboarding();
      } else if( newState == OnboardingStages::Complete ) {
        SetNewBehavior( _behaviors.postOnboardingBehavior, requestStackReset );
      } else if( newState == OnboardingStages::DevDoNothing ) {
        SetNewBehavior( _behaviors.devBaseBehavior, requestStackReset );
      }
    };
    _eventHandles.push_back( ei->Subscribe(ExternalInterface::MessageEngineToGameTag::OnboardingState,
                                           onOnboardingStage) );


    auto selfTestEndLambda = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& selfTestEnd){
      _selfTestEnded = true;
    };
    
    _eventHandles.push_back(ei->Subscribe(ExternalInterface::MessageEngineToGameTag::SelfTestEnd,
                                          selfTestEndLambda));
  
    auto setDevDoNothingFunc = [this](ConsoleFunctionContextRef context){
      _stage = OnboardingStages::DevDoNothing;
      const bool requestStackReset = true;
      SetNewBehavior(_behaviors.devBaseBehavior, requestStackReset);
      Json::Value toSave;
      toSave[BehaviorOnboardingCoordinator::kOnboardingStageKey] = OnboardingStagesToString(_stage);
      const std::string filename = _saveFolder + BehaviorOnboardingCoordinator::kOnboardingFilename;
      Util::FileUtils::WriteFile( filename, toSave.toStyledString() );

    };
    _consoleFuncs.emplace_front("Set active mode and boot mode to DevDoNothing",
                                std::move(setDevDoNothingFunc),
                                "BehaviorBootLoader",
                                "");

    auto setNormalOperationFunc = [this](ConsoleFunctionContextRef context){
      _stage = OnboardingStages::Complete;
      const bool requestStackReset = true;
      SetNewBehavior( _behaviors.normalBaseBehavior, requestStackReset );
      Json::Value toSave;
      toSave[BehaviorOnboardingCoordinator::kOnboardingStageKey] = OnboardingStagesToString(_stage);
      const std::string filename = _saveFolder + BehaviorOnboardingCoordinator::kOnboardingFilename;
      Util::FileUtils::WriteFile( filename, toSave.toStyledString() );
    };
    _consoleFuncs.emplace_front("Set active mode and boot mode to Normal",
                                std::move(setNormalOperationFunc),
                                "BehaviorBootLoader",
                                "");
  }

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::UpdateDependent(const BCCompMap& dependentComps)
{
  if( _behaviorToSwitchTo != nullptr ) {
    auto* bsm = dependentComps.GetComponentPtr<BehaviorSystemManager>();
    if( bsm != nullptr ) {
      bsm->ResetBehaviorStack( _behaviorToSwitchTo );
      _behaviorToSwitchTo = nullptr;
    }

    if( _countUntilResetOnboarding > 0 ) {
      // onboarding was just stopped because we plan to reset it. Set the stage
      _stage = OnboardingStages::NotStarted;
    }
  }

  if( (_countUntilResetOnboarding > 0) && (_behaviorToSwitchTo == nullptr) ) {
    // waiting to restart onboarding
    --_countUntilResetOnboarding;
    if( _countUntilResetOnboarding == 0 ) {
      // flag to start onboarding
      SetNewBehavior( _behaviors.onboardingBehavior );
    }
  }

  if( _selfTestEnded ) {
    // Self test has ended so reset the behavior stack to what
    // it was before the self ran as well as the boot behavior
    _selfTestEnded = false;

    auto* bsm = dependentComps.GetComponentPtr<BehaviorSystemManager>();
    bsm->ResetBehaviorStack(_prevBottomOfStackBehavior);
    
    _bootBehavior = _prevBootBehavior;
      
    _prevBootBehavior = nullptr;
    _prevBottomOfStackBehavior = nullptr;
  }
  
  // wait until we know we can communicate with anim to send anything
  if( _robot->GetSyncRobotAcked() ) {
#   if ALEXA_ACOUSTIC_TEST
    const bool enterAcousticTestMode = (_bootBehaviorID != _behaviors.onboardingBehavior);
#   else
    const bool enterAcousticTestMode = kAcousticTestMode;
    if( kAcousticTestMode && (_bootBehaviorID != _behaviors.acousticTestBehavior) ) {
      SetNewBehavior( _behaviors.acousticTestBehavior );
    }
#   endif
    if( enterAcousticTestMode != _wasAcousticTestMode ) {
      _wasAcousticTestMode = enterAcousticTestMode;
      
      RobotInterface::AcousticTestEnabled msg{ enterAcousticTestMode };
      _robot->SendMessage( RobotInterface::EngineToRobot( std::move(msg) ) );
    }
  }
}
  
void BehaviorsBootLoader::InitOnboarding()
{
  const std::string filename = _saveFolder + BehaviorOnboardingCoordinator::kOnboardingFilename;
  const std::string fileContents = Util::FileUtils::ReadFile( filename );
  
  Json::Reader reader;
  Json::Value onboardingStateJSON;
  
  _stage = OnboardingStages::NotStarted;
# if defined(ANKI_PLATFORM_OSX) 
  {
    // always start in DevDoNothing stage on mac if no persistent file exists, so that webots work
    // can continue with a simple console var DevDispatchAfterShake. We can't use Complete since
    // that would interfere with webots tests
    _stage = OnboardingStages::DevDoNothing;
  }
# endif
  
  if( !fileContents.empty() && reader.parse( fileContents, onboardingStateJSON ) ) {
    if( ANKI_VERIFY( onboardingStateJSON[BehaviorOnboardingCoordinator::kOnboardingStageKey].isString(), "BehaviorsBootLoader.InitOnboarding.InvalidKey", "" ) )
    {
      const auto& stageStr = onboardingStateJSON[BehaviorOnboardingCoordinator::kOnboardingStageKey].asString();
      ANKI_VERIFY( OnboardingStagesFromString(stageStr, _stage),
                  "BehaviorsBootLoader.InitOnboarding.InvalidStage",
                  "Stage %s is invalid", stageStr.c_str() );
    }
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorsBootLoader.InitOnboarding.OnboardingStage",
                "Robot booted with onboarding state %s", OnboardingStagesToString(_stage));
  
  if( static_cast<u8>(_stage) < static_cast<u8>(OnboardingStages::Complete) ) {
    SetNewBehavior( _behaviors.onboardingBehavior );
  } else if( _stage == OnboardingStages::Complete ) {
    SetNewBehavior( _behaviors.normalBaseBehavior );
  } else if( _stage == OnboardingStages::DevDoNothing ) {
    SetNewBehavior( _behaviors.devBaseBehavior );
  } else {
    DEV_ASSERT(false, "BehaviorsBootLoader.InitOnboarding.UnknownStage");
  }
  
  auto* ei = _externalInterface;
  if( ei != nullptr ) {
    ExternalInterface::SetOnboardingStage message;
    message.stage = _stage;
    ei->Broadcast(ExternalInterface::MessageGameToEngine(std::move(message)));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorsBootLoader::GetBootBehavior()
{
  ANKI_VERIFY( _bootBehavior != nullptr, "BehaviorsBootLoader.GetBootBehavior.BootNotInitialized", "Requested behavior before initialization");
  _hasGrabbedBootBehavior = true;
  return _bootBehavior;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::SetNewBehavior(BehaviorID behaviorID, bool requestStackReset)
{
  _bootBehaviorID = behaviorID;
  DEV_ASSERT(_behaviorContainer != nullptr, "BehaviorsBootLoader.SetNewBehavior.NoBC");
  
  IBehavior* behavior = _behaviorContainer->FindBehaviorByID(behaviorID).get();
  ANKI_VERIFY(behavior != nullptr,
              "BehaviorsBootLoader.SetNewBehavior.Invalid",
              "No %s", BehaviorTypesWrapper::BehaviorIDToString(behaviorID));
    
  SetNewBehavior(behavior, requestStackReset);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::SetNewBehavior(IBehavior* behavior, bool requestStackReset)
{
  if(behavior != nullptr)
  {
    if( (behavior != _bootBehavior) || _pendingBehavior ) {
      if( _hasGrabbedBootBehavior && requestStackReset ) {
        // need to wait until Update to reset the stack, since the bsm claims
        // this class as a dependent and may not have finished init
        _behaviorToSwitchTo = behavior;
      }
      _bootBehavior = behavior;
      
      // If not resetting the stack, flag so that it's possible to call SetNewBehavior() again
      // with the same behaviorID when it's time to actually switch behaviors. Otherwise this if
      // block won't run
      _pendingBehavior = _hasGrabbedBootBehavior && !requestStackReset;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::RestartOnboarding()
{

  // hacky way of de-activating the current onboarding and then re-activating it. Flag to start the Wait behavior,
  // when it starts change the stage within BehaviorOnboarding, pause a few ticks, then flag to start onboarding again.
  SetNewBehavior( BEHAVIOR_ID(Wait) );
  _countUntilResetOnboarding = 20;

  DASMSG(onboarding_restart, "onboarding.restart", "User requested to start onboarding from the beginning");
  DASMSG_SEND();
}

}
}
