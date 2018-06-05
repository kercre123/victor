/**
* File: devBehaviorComponentMessageHandler.cpp
*
* Author: Kevin M. Karol
* Created: 10/24/17
*
* Description: Component that receives dev messages and performs actions on
* the behaviorComponent in response
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRerun.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "webServerProcess/src/webService.h"

#include "clad/types/behaviorComponent/userIntent.h"

namespace Anki {
namespace Cozmo {

namespace {
static const BehaviorID kBehaviorIDForDevMessage = BEHAVIOR_ID(DevExecuteBehaviorRerun);
static const BehaviorID kWaitBehaviorID = BEHAVIOR_ID(Wait);
const std::string kWebVizModuleNameBehaviors = "behaviors";
const std::string kWebVizModuleNameIntents = "intents";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::DevBehaviorComponentMessageHandler(Robot& robot)
: IDependencyManagedComponent<BCComponentID>(this, BCComponentID::DevBehaviorComponentMessageHandler)
, _robot(robot)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::~DevBehaviorComponentMessageHandler()
{
  _eventHandles.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBehaviorComponentMessageHandler::GetInitDependencies(BCCompIDSet& dependencies) const
{ 
  dependencies.insert(BCComponentID::BehaviorExternalInterface);
  dependencies.insert(BCComponentID::BehaviorSystemManager);
  dependencies.insert(BCComponentID::BehaviorContainer);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBehaviorComponentMessageHandler::InitDependent(Robot* robot, const BCCompMap& dependentComponents) 
{
  auto& bContainer = dependentComponents.GetValue<BehaviorContainer>();
  auto& bsm = dependentComponents.GetValue<BehaviorSystemManager>();
  auto& bei = dependentComponents.GetValue<BehaviorExternalInterface>();

  if(_robot.HasExternalInterface()){
    
    SubscribeToWebViz( bei, bsm );
    
    auto handlerCallback = [this, &bContainer, &bsm, &bei](const GameToEngineEvent& event) {
      const auto& msg = event.GetData().Get_ExecuteBehaviorByID();
      
      BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString(msg.behaviorID);
      ICozmoBehaviorPtr behaviorToRun = bContainer.FindBehaviorByID(behaviorID);
      if(behaviorToRun != nullptr){
        ICozmoBehaviorPtr newRerunBaseBehavior =
          WrapRequestedBehaviorInDispatcherRerun(bContainer,
                                                 behaviorID,
                                                 msg.numRuns);
        newRerunBaseBehavior->Init(bei);
        bsm.ResetBehaviorStack(newRerunBaseBehavior.get());
        _rerunBehavior = newRerunBaseBehavior;
      }
    };

    _eventHandles.push_back(
      _robot.GetExternalInterface()->Subscribe(GameToEngineTag::ExecuteBehaviorByID, 
                                               handlerCallback)
    );


    // =========== Handle DebugScreenMode message =================
    // TODO: VIC-2416 - Rename this class since it is used in release.
    // Get base behavior to execute when exiting debug screens
    if(_robot.GetContext() == nullptr){
      PRINT_NAMED_WARNING("DevBehaviorComponentMessageHandler.InitDependent.NoContext", "");
      return;
    }
    auto dataLoader = _robot.GetContext()->GetDataLoader();
    if(dataLoader == nullptr){
      PRINT_NAMED_WARNING("DevBehaviorComponentMessageHandler.InitDependent.NoDataLoader", "");
      return;
    }
    const Json::Value& freeplayBehaviorConfig = dataLoader->GetVictorFreeplayBehaviorConfig();
    if(freeplayBehaviorConfig.empty()){
      PRINT_NAMED_WARNING("DevBehaviorComponentMessageHandler.InitDependent.NoDefaultFreeplayBehavior", "");
      return;
    }
    BehaviorID freeplayBehaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(freeplayBehaviorConfig);
    ICozmoBehaviorPtr freeplayBehavior = bContainer.FindBehaviorByID(freeplayBehaviorID);
    ICozmoBehaviorPtr waitBehavior = bContainer.FindBehaviorByID(kWaitBehaviorID);    

    // Go to Wait behavior when debug screens are enabled so as 
    // not to interrupt while user is trying to look at something.
    // Go to freeplayBehavior as defined in victor_behavior_config.json
    // when leaving debug screens
    auto debugScreenModeHandler = [&bsm, freeplayBehavior, waitBehavior](const RobotToEngineEvent& event) {
      const auto& msg = event.GetData().Get_debugScreenMode();
      PRINT_CH_INFO("BehaviorSystem", "DevBehaviorComponentMessageHandler.DebugScreenModeChange", "%d", msg.enabled);

      if (msg.enabled) {
        bsm.ResetBehaviorStack(waitBehavior.get());
      } else {
        bsm.ResetBehaviorStack(freeplayBehavior.get());
      }

    };
    _eventHandles.push_back(
      _robot.GetRobotMessageHandler()->Subscribe(RobotInterface::RobotToEngineTag::debugScreenMode, debugScreenModeHandler)
    );
    // =========== End handling of DebugScreenMode message =================


    SetupUserIntentEvents();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBehaviorComponentMessageHandler::SetupUserIntentEvents()
{
  
  auto EI = _robot.GetExternalInterface();
  
  // fake trigger word
  auto fakeTriggerWordCallback = [this]( const GameToEngineEvent& event ) {
    PRINT_CH_INFO("BehaviorSystem","DevBehaviorComponentMessageHandler.ReceivedFakeTriggerWordDetected","");
    auto& uic = _robot.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    uic.SetTriggerWordPending();
  };
  _eventHandles.push_back( EI->Subscribe( GameToEngineTag::FakeTriggerWordDetected, fakeTriggerWordCallback ) );
  
  // fake cloud intent
  auto fakeCloudIntentCallback = [this]( const GameToEngineEvent& event ) {
    PRINT_CH_INFO("BehaviorSystem","DevBehaviorComponentMessageHandler.FakeCloudIntentReceived","");
    auto& uic = _robot.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    const auto& intent = event.GetData().Get_FakeCloudIntent().intent;
    if( (intent.find("{") != std::string::npos) // super awesome json detection
        && (intent.find("}") != std::string::npos) )
    {
      uic.SetCloudIntentPendingFromJSON( intent );
    } else {
      std::string jsonIntent = "{\"intent\": \"" + intent + "\"}";
      uic.SetCloudIntentPendingFromJSON( jsonIntent );
    }
  };
  _eventHandles.push_back( EI->Subscribe( GameToEngineTag::FakeCloudIntent, fakeCloudIntentCallback ) );
  
  // fake user intent
  auto fakeUserIntentCallback = [this]( const GameToEngineEvent& event ) {
    PRINT_CH_INFO("BehaviorSystem","DevBehaviorComponentMessageHandler.FakeUserIntentReceived","");
    auto& uic = _robot.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    const auto& intent = event.GetData().Get_FakeUserIntent().intent;
    
    UserIntentTag tag;
    if( UserIntentTagFromString(intent, tag) ) {
      uic.DevSetUserIntentPending(tag);
    } else {
      PRINT_CH_INFO("BehaviorSystem",
                    "DevBehaviorComponentMessageHandler.FakeUserIntentReceived.Invalid",
                    "Invalid intent '%s'",
                    intent.c_str());
    }
  };
  _eventHandles.push_back( EI->Subscribe( GameToEngineTag::FakeUserIntent, fakeUserIntentCallback ) );
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr DevBehaviorComponentMessageHandler::WrapRequestedBehaviorInDispatcherRerun(BehaviorContainer& bContainer, 
                                                                                             BehaviorID requestedBehaviorID, 
                                                                                             const int numRuns)
{
  // See if behaviorID has already been created
  ICozmoBehaviorPtr rerunDispatcher = bContainer.FindBehaviorByID(kBehaviorIDForDevMessage);
  if(rerunDispatcher != nullptr){
    bContainer.RemoveBehaviorFromMap(rerunDispatcher);
  }

  Json::Value config = BehaviorDispatcherRerun::CreateConfig(kBehaviorIDForDevMessage, requestedBehaviorID, numRuns);
  const bool createdOK = bContainer.CreateAndStoreBehavior(config);
  if( ANKI_VERIFY(createdOK,
                  "DevBehaviorComponentMessageHandler.CreateRerunWrapper.Fail",
                  "Couldn't create behavior wrapper") ) {
    rerunDispatcher = bContainer.FindBehaviorByID( kBehaviorIDForDevMessage );
  }
  
  return rerunDispatcher;
}
  

void DevBehaviorComponentMessageHandler::SubscribeToWebViz(BehaviorExternalInterface& bei, const BehaviorSystemManager& bsm)
{
  DEV_ASSERT( _eventHandles.empty(), "only call once" );
  
  const auto* context = _robot.GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      
      auto onSubscribedBehaviors = [&bei, &bsm](const std::function<void(const Json::Value&)>& sendToClient) {
        // resend just that client the new tree
        Json::Value data = bsm.BuildDebugBehaviorTree( bei );
        sendToClient( data );
        
        // also send them the list of behaviorIDs that can be created
        Json::Value allBehaviors;
        auto& list = allBehaviors["list"];
        for( uint8_t i=0; i<BehaviorTypesWrapper::GetBehaviorIDNumEntries(); ++i ) {
          list.append( EnumToString( static_cast<BehaviorID>(i) ) );
        }
        sendToClient( list );
      };
      auto onDataBehaviors = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
        // client wants us to run a specific behavior
        const auto& name = data["behaviorName"];
        if( name.isString() ) {
          PRINT_CH_DEBUG("BehaviorSystem",
                         "BehaviorStack.SubscribeToWebViz.Transition",
                         "WebViz just instructed us to transition to '%s'",
                         name.asString().c_str());
          auto* ei = _robot.GetExternalInterface();
          const int numRuns = 1;
          using namespace ExternalInterface;
          ei->Broadcast(MessageGameToEngine(ExecuteBehaviorByID( name.asString(), numRuns )));
        }
      };
      
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameBehaviors ).ScopedSubscribe( onSubscribedBehaviors ) );
      _eventHandles.emplace_back( webService->OnWebVizData( kWebVizModuleNameBehaviors ).ScopedSubscribe( onDataBehaviors ) );
      
      // now for user/cloud intents
      
      auto onSubscribedIntents = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // client just connected, send all {user/cloud} intents and current pending {user/cloud} intent
        
        auto& uic = _robot.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
        
        Json::Value allCloudIntents = Json::arrayValue;
        std::vector<std::string> allCloudIntentsStr = uic.DevGetCloudIntentsList();
        for( const auto& elem : allCloudIntentsStr ) {
          allCloudIntents.append( elem );
        }
        
        Json::Value allUserIntents = Json::arrayValue;
        const char* currentUserIntent = nullptr;
        using Tag = std::underlying_type<UserIntentTag>::type;
        for( Tag i{0}; i<static_cast<Tag>( USER_INTENT(test_SEPARATOR) ); ++i ) {
          UserIntentTag intent = static_cast<UserIntentTag>( i );
          const char* const intentStr = UserIntentTagToString( intent );
          allUserIntents.append( intentStr );
          
          if( uic.IsUserIntentPending( intent ) ) {
            currentUserIntent = UserIntentTagToString( intent );
          }
        }
        
        // send everything we've got!
        Json::Value toSend = Json::arrayValue;
        
        if( currentUserIntent != nullptr ) {
          Json::Value blob;
          blob["intentType"] = "user";
          blob["type"] = "current-intent";
          blob["value"] = currentUserIntent;
          toSend.append( blob );
        }
        
        if( !allUserIntents.isNull() ) {
          Json::Value blob;
          blob["intentType"] = "user";
          blob["type"] = "all-intents";
          blob["list"] = allUserIntents;
          toSend.append( blob );
        }
        
        if( !allCloudIntents.isNull() ) {
          Json::Value blob;
          blob["intentType"] = "cloud";
          blob["type"] = "all-intents";
          blob["list"] = allCloudIntents;
          toSend.append( blob );
        }
        
        sendToClient( toSend );
      };
      auto onDataIntents = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
        auto& uic = _robot.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
        const auto& type = data["intentType"].asString();
        const auto& request = data["request"].asString();
        if( type == "user" ) {
          UserIntentTag tag;
          if( UserIntentTagFromString(request, tag) ) {
            uic.DevSetUserIntentPending(tag);
          } else {
            PRINT_CH_INFO("BehaviorSystem",
                          "DevBehaviorComponentMessageHandler.WebVizUserIntentReceived.Invalid",
                          "Invalid intent '%s'",
                          request.c_str());
          }
        } else if( type == "cloud" ) {
          if( (request.find("{") != std::string::npos) // super awesome json detection
             && (request.find("}") != std::string::npos) )
          {
            uic.SetCloudIntentPendingFromJSON( request );
          } else {
            std::string jsonIntent = "{\"intent\": \"" + request + "\"}";
            uic.SetCloudIntentPendingFromJSON( jsonIntent );
          }
        }
      };
      
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameIntents ).ScopedSubscribe( onSubscribedIntents ) );
      _eventHandles.emplace_back( webService->OnWebVizData( kWebVizModuleNameIntents ).ScopedSubscribe( onDataIntents ) );
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

