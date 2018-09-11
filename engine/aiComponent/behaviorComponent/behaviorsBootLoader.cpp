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
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p0/behaviorOnboarding1p0.h"
#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorsBootLoader::BehaviorsBootLoader(const Json::Value& config)
  : IDependencyManagedComponent( this, BCComponentID::BehaviorsBootLoader )
{
  _behaviors.factoryBehavior = BEHAVIOR_ID(PlaypenTest);
  _behaviors.prDemoBehavior = BEHAVIOR_ID(InitPRDemo);
  
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
  using namespace Util;
  const Data::DataPlatform* platform = robot->GetContextDataPlatform();
  _saveFolder = platform->pathToResource( Data::Scope::Persistent, BehaviorOnboarding1p0::kOnboardingFolder );
  _saveFolder = FileUtils::AddTrailingFileSeparator( _saveFolder );
  
  if( FileUtils::DirectoryDoesNotExist( _saveFolder ) ) {
    FileUtils::CreateDirectory( _saveFolder, false, true );
  }
  
  _externalInterface = robot->GetExternalInterface();
  _gatewayInterface = robot->GetGatewayInterface();
  
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
        SetNewBehavior( _behaviors.onboardingBehavior, requestStackReset );
      } else if( newState == OnboardingStages::Complete ) {
        SetNewBehavior( _behaviors.postOnboardingBehavior, requestStackReset );
      } else if( newState == OnboardingStages::DevDoNothing ) {
        SetNewBehavior( _behaviors.devBaseBehavior, requestStackReset );
      }
    };

    _eventHandles.push_back( ei->Subscribe(ExternalInterface::MessageEngineToGameTag::OnboardingState,
                                           onOnboardingStage) );
  }
  
  auto* gi = _gatewayInterface;
  if( gi != nullptr ) {
    auto onRequestOnboardingState = [gi,this](const AnkiEvent<external_interface::GatewayWrapper>& appEvent){
      auto* onboardingState = new external_interface::OnboardingState{ CladProtoTypeTranslator::ToProtoEnum(_stage) };
      gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingState) );
    };
    _eventHandles.push_back( gi->Subscribe(external_interface::GatewayWrapperTag::kOnboardingStateRequest,
                                           onRequestOnboardingState) );
    
    auto onRequestOnboardingComplete = [gi,this](const AnkiEvent<external_interface::GatewayWrapper>& appEvent){
      const bool completed = (_stage == OnboardingStages::Complete) || (_stage == OnboardingStages::DevDoNothing);
      auto* onboardingComplete = new external_interface::OnboardingCompleteResponse{ completed };
      gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingComplete) );
    };
    _eventHandles.push_back( gi->Subscribe(external_interface::GatewayWrapperTag::kOnboardingCompleteRequest,
                                           onRequestOnboardingComplete) );
    
    auto onRestart = [this](const AnkiEvent<external_interface::GatewayWrapper>& appEvent){
      RestartOnboarding();
    };
    _eventHandles.push_back( gi->Subscribe(external_interface::GatewayWrapperTag::kOnboardingRestart,
                                           onRestart) );
  }
  
  auto resetOnboarding = [ei,robot,this](ConsoleFunctionContextRef context) {
    if( ei != nullptr ) {
      ei->Broadcast(ExternalInterface::MessageGameToEngine{ExternalInterface::EraseAllEnrolledFaces{}});
      robot->GetCubeCommsComponent().ForgetPreferredCube();
    }
    RestartOnboarding();
  };
  _consoleFuncs.emplace_front( "ResetOnboarding", std::move(resetOnboarding), "Onboarding", "" );
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
      // explicitly pass the stage so we don't have to worry about when messages are received
      std::shared_ptr<BehaviorOnboarding1p0> castPtr;
      _behaviorContainer->FindBehaviorByIDAndDowncast(BEHAVIOR_ID(Onboarding), BEHAVIOR_CLASS(Onboarding1p0), castPtr);
      if( castPtr != nullptr ) {
        castPtr->SetOnboardingStage(_stage);
      }
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
}
  
void BehaviorsBootLoader::InitOnboarding()
{
  const std::string filename = _saveFolder + BehaviorOnboarding1p0::kOnboardingFilename;
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
    if( ANKI_VERIFY( onboardingStateJSON[BehaviorOnboarding1p0::kOnboardingStageKey].isString(), "BehaviorsBootLoader.InitOnboarding.InvalidKey", "" ) )
    {
      const auto& stageStr = onboardingStateJSON[BehaviorOnboarding1p0::kOnboardingStageKey].asString();
      ANKI_VERIFY( OnboardingStagesFromString(stageStr, _stage),
                  "BehaviorsBootLoader.InitOnboarding.InvalidStage",
                  "Stage %s is invalid", stageStr.c_str() );
    }
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorsBootLoader.InitOnboarding.OnboardingStage",
                "Robot booted with onboarding state %s", OnboardingStagesToString(_stage));
  
  if( static_cast<u8>(_stage) < static_cast<u8>(OnboardingStages::Complete) ) {
    // explicitly pass the stage so we don't have to worry about when messages are received
    std::shared_ptr<BehaviorOnboarding1p0> castPtr;
    _behaviorContainer->FindBehaviorByIDAndDowncast(BEHAVIOR_ID(Onboarding), BEHAVIOR_CLASS(Onboarding1p0), castPtr);
    if( castPtr != nullptr ) {
      castPtr->SetOnboardingStage(_stage);
    }
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
  DEV_ASSERT(_behaviorContainer != nullptr, "BehaviorsBootLoader.SetNewBehavior.NoBC");
  
  IBehavior* behavior = _behaviorContainer->FindBehaviorByID(behaviorID).get();
  if( ANKI_VERIFY(behavior != nullptr,
              "BehaviorsBootLoader.SetNewBehavior.Invalid",
              "No %s", BehaviorTypesWrapper::BehaviorIDToString(behaviorID)) )
  {
    if( behavior != _bootBehavior ) {
      if( _hasGrabbedBootBehavior && requestStackReset ) {
        // need to wait until Update to reset the stack, since the bsm claims
        // this class as a dependent and may not have finished init
        _behaviorToSwitchTo = behavior;
      }
      _bootBehavior = behavior;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorsBootLoader::RestartOnboarding()
{
  Util::FileUtils::DeleteFile( _saveFolder + BehaviorOnboarding1p0::kOnboardingFilename );
  
  // hacky way of de-activating the current onboarding and then re-activating it. Flag to start the Wait behavior,
  // when it starts change the stage within BehaviorOnboarding, pause a few ticks, then flag to start onboarding again.
  SetNewBehavior( BEHAVIOR_ID(Wait) );
  _countUntilResetOnboarding = 20;
  
  DASMSG(onboarding_restart, "onboarding.restart", "User requested to start onboarding from the beginning");
  DASMSG_SEND();
}

}
}
