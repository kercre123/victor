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

#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboarding.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/unitTestKey.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorsBootLoader::BehaviorsBootLoader(const Json::Value& config)
  : IDependencyManagedComponent( this, BCComponentID::BehaviorsBootLoader )
{
  _behaviors.factoryBehavior = BEHAVIOR_ID(PlaypenTest);
  
  if( ANKI_VERIFY(!config.empty(), "BehaviorsBootLoader.Ctor.InvalidConfig", "Empty config") ) {
    
    auto setFromJSON = [&config](const char* key) {
      const std::string behaviorIDStr = JsonTools::ParseString(config, key, "BehaviorsBootLoader");
      return BehaviorTypesWrapper::BehaviorIDFromString(behaviorIDStr);
    };
    _behaviors.onboardingBehavior = setFromJSON("onboardingBehavior");
    _behaviors.normalBaseBehavior = setFromJSON("normalBaseBehavior");
    _behaviors.devBaseBehavior = setFromJSON("devBaseBehavior");
  } else {
    _behaviors.onboardingBehavior = BEHAVIOR_ID(Wait);
    _behaviors.normalBaseBehavior = BEHAVIOR_ID(Wait);
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
  _saveFolder = platform->pathToResource( Data::Scope::Persistent, BehaviorOnboarding::kOnboardingFolder );
  _saveFolder = FileUtils::AddTrailingFileSeparator( _saveFolder );
  
  if( FileUtils::DirectoryDoesNotExist( _saveFolder ) ) {
    FileUtils::CreateDirectory( _saveFolder, false, true );
  }
  
  _externalInterface = robot->GetExternalInterface();
  _gatewayInterface = robot->GetGatewayInterface();
  
  _behaviorContainer = dependentComps.GetComponentPtr<BehaviorContainer>();
  
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
  } else {
    InitOnboarding();
  }
                                                                  
  auto* ei = _externalInterface;
  if( ei != nullptr ) {
    // listen for messages from the robot that changed onboarding state. don't worry about anything from the app, since it
    // can't tell the robot to change its onboarding state
    auto onOnboardingStage = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& onboardingStageEvt){
      const auto newState = onboardingStageEvt.GetData().Get_OnboardingState().stage;
      if( static_cast<u8>(newState) < static_cast<u8>(OnboardingStages::Complete) ) {
        SetNewBehavior( _behaviors.onboardingBehavior );
      } else if( newState == OnboardingStages::Complete ) {
        SetNewBehavior( _behaviors.normalBaseBehavior );
      } else if( newState == OnboardingStages::DevDoNothing ) {
        SetNewBehavior( _behaviors.devBaseBehavior );
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
  }
  
  auto resetOnboarding = [ei,robot,this](ConsoleFunctionContextRef context) {
    Util::FileUtils::DeleteFile( _saveFolder + BehaviorOnboarding::kOnboardingFilename );
    Util::FileUtils::DeleteFile( _saveFolder );
    if( ei != nullptr ) {
      ei->Broadcast(ExternalInterface::MessageGameToEngine{ExternalInterface::EraseAllEnrolledFaces{}});
      
      robot->GetCubeCommsComponent().ForgetPreferredCube();
    }
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
  }
}
  
void BehaviorsBootLoader::InitOnboarding()
{
  const std::string filename = _saveFolder + BehaviorOnboarding::kOnboardingFilename;
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
    if( ANKI_VERIFY( onboardingStateJSON[BehaviorOnboarding::kOnboardingStageKey].isString(), "BehaviorsBootLoader.InitOnboarding.InvalidKey", "" ) )
    {
      const auto& stageStr = onboardingStateJSON[BehaviorOnboarding::kOnboardingStageKey].asString();
      ANKI_VERIFY( OnboardingStagesFromString(stageStr, _stage),
                  "BehaviorsBootLoader.InitOnboarding.InvalidStage",
                  "Stage %s is invalid", stageStr.c_str() );
    }
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorsBootLoader.InitOnboarding.OnboardingStage",
                "Robot booted with onboarding state %s", OnboardingStagesToString(_stage));
  
  if( static_cast<u8>(_stage) < static_cast<u8>(OnboardingStages::Complete) ) {
    // explicitly pass the stage so we don't have to worry about when messages are received
    std::shared_ptr<BehaviorOnboarding> castPtr;
    _behaviorContainer->FindBehaviorByIDAndDowncast(BEHAVIOR_ID(Onboarding), BEHAVIOR_CLASS(Onboarding), castPtr);
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
void BehaviorsBootLoader::SetNewBehavior(BehaviorID behaviorID)
{
  DEV_ASSERT(_behaviorContainer != nullptr, "BehaviorsBootLoader.SetNewBehavior.NoBC");
  
  IBehavior* behavior = _behaviorContainer->FindBehaviorByID(behaviorID).get();
  if( ANKI_VERIFY(behavior != nullptr,
              "BehaviorsBootLoader.SetNewBehavior.Invalid",
              "No %s", BehaviorTypesWrapper::BehaviorIDToString(behaviorID)) )
  {
    if( behavior != _bootBehavior ) {
      if( _hasGrabbedBootBehavior ) {
        // need to wait until Update to reset the stack, since the bsm claims
        // this class as a dependent and may not have finished init
        _behaviorToSwitchTo = behavior;
      }
      _bootBehavior = behavior;
    }
  }
}

}
}
