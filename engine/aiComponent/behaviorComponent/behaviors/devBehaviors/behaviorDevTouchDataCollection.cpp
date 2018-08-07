/**
 * File: BehaviorDevTouchDataCollection.cpp
 *
 * Author: Arjun Menon
 * Date:   01/08/2018 
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevTouchDataCollection.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

namespace {

bool _rcvdNextDatasetRequest = false;

#if ANKI_DEV_CHEATS
  // console func helper --- links webots keyboard controller to control behavior
  void NextDatasetState(ConsoleFunctionContextRef context)
  {
    _rcvdNextDatasetRequest = true;
  }
  CONSOLE_FUNC(NextDatasetState, "NextDatasetState");
#endif
}

std::pair<std::string,std::string> BehaviorDevTouchDataCollection::ToString(DataAnnotation a) const {
  const std::map<DataAnnotation,std::pair<std::string,std::string>> mm {
    {NoTouch,           {"NoTouch",          "NT"}},
    {SustainedContact,  {"SustainedContact", "SC"}},
    {PettingSoft,       {"PettingSoft",      "PS"}},
    {PettingHard,       {"PettingHard",      "PH"}}
  };

  auto got = mm.find(a);
  DEV_ASSERT(got!=mm.end(), "DevTouchDataCollection.DataAnnotation.MissingMapping");
  return got->second;
}

std::pair<std::string,std::string> BehaviorDevTouchDataCollection::ToString(CollectionMode cm) const {
  const std::map<CollectionMode,std::pair<std::string,std::string>> mm {
    {IdleChrgOff, {"IdleChrgOff", "IF"}},
    {IdleChrgOn,  {"IdleChrgOn", "IN"}},
    {MoveChrgOff, {"MoveChrgOff", "MF"}},
    {MoveChrgOn,  {"MoveChrgOn", "MN"}}
  };

  auto got = mm.find(cm);
  DEV_ASSERT(got!=mm.end(), "DevTouchDataCollection.CollectionMode.MissingMapping");
  return got->second;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTouchDataCollection::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTouchDataCollection::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BehaviorDevTouchDataCollection::BehaviorDevTouchDataCollection(const Json::Value& config)
: ICozmoBehavior(config)
{
  // listening for raw touch input data
  SubscribeToTags({
    RobotInterface::RobotToEngineTag::state
  });
}

void BehaviorDevTouchDataCollection::HandleWhileActivated(const RobotToEngineEvent& event)
{
  BehaviorExternalInterface& bexi = GetBEI();
  
  switch(event.GetData().GetTag()) {
    case RobotInterface::RobotToEngineTag::state:
    {
      const RobotState& payload = event.GetData().Get_state();

      static size_t ticksNotMeasured = 0;
      if( RobotConfigMatchesExpected(bexi) ) {
        for(int i=0; i<STATE_MESSAGE_FREQUENCY; ++i) {
          _dVars.touchValues.push_back(payload.backpackTouchSensorRaw[i]);
        }
        ticksNotMeasured = 0;
        if( _dVars.touchValues.size()%600 == 0 ) {
          PRINT_CH_INFO("Behaviors", "TouchDataCollection", "3 seconds collected");
        }
      } else {
        ticksNotMeasured++;
        if(ticksNotMeasured > 100) {
          PRINT_CH_INFO("Behaviors","TouchDataCollection","NotLoggingExpected.%s",ToString(_dVars.collectMode).first.c_str());
          ticksNotMeasured = 0;
        }
      }
      break;
    }
    default:
    {
      PRINT_NAMED_WARNING("BehaviorDevTouchDataCollection.HandleWhileRunning.InvalidRobotToEngineTag",
          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorDevTouchDataCollection::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::InitBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::OnBehaviorActivated()
{
  _dVars.touchValues.clear();
  if( _dVars.collectMode == MoveChrgOff || _dVars.collectMode == MoveChrgOn ) {
    EnqueueMotorActions();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::EnqueueMotorActions()
{
  // DriveStraightAction* driveAction = new DriveStraightAction(100, 50, false);
  
  static bool goingUp = true;
  
  auto preset = goingUp ? MoveLiftToHeightAction::Preset::HIGH_DOCK :
                          MoveLiftToHeightAction::Preset::LOW_DOCK;
  
  auto angle = goingUp ? DEG_TO_RAD(44) : DEG_TO_RAD(-22);

  MoveLiftToHeightAction* liftAction = new MoveLiftToHeightAction(preset);
  liftAction->SetDuration(2.0);

  MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction(angle);
  headAction->SetDuration(2.0);

  IActionRunner* allMotors = new CompoundActionParallel({
      headAction,
      liftAction,
  });

  ActionResultCallback arc = std::bind(&BehaviorDevTouchDataCollection::LoopMotorsActionCallback,this,std::placeholders::_1);

  DelegateIfInControl(allMotors, arc);

  goingUp = !goingUp;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorDevTouchDataCollection::RobotConfigMatchesExpected(BehaviorExternalInterface& bexi) const
{
  const bool onCharger = bexi.GetRobotInfo().IsOnChargerContacts();
  
  const bool isMotoring = IsControlDelegated();

  switch(_dVars.collectMode) {
    case IdleChrgOff:
      return !onCharger && !isMotoring;
    case IdleChrgOn:
      return onCharger && !isMotoring;
    case MoveChrgOff:
      return !onCharger && isMotoring;
    case MoveChrgOn:
      return onCharger && isMotoring;
  }

  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::BehaviorUpdate()
{
  BehaviorExternalInterface& bexi = GetBEI();
  
  if(!IsActivated()){
    return;
  }

  if(_rcvdNextDatasetRequest) {
    OnNextDataset(bexi, _dVars.collectMode, _dVars.annotation);
    
    int ic = (int)_dVars.collectMode;
    int ia = (int)_dVars.annotation;
    ic++;
    if(ic==4) {
      ic = 0; //reset
      ia++;
      if(ia==4) {
        ia = 0;
      }
    }

    _dVars.collectMode = static_cast<CollectionMode>(ic);
    _dVars.annotation = static_cast<DataAnnotation>(ia);
    PRINT_CH_INFO("Behaviors", "TouchDataCollection",
                  "Collecting for '%s' under '%s'", 
                  ToString(_dVars.annotation).first.c_str(), 
                  ToString(_dVars.collectMode).first.c_str());

    if( _dVars.collectMode == MoveChrgOff || _dVars.collectMode == MoveChrgOn ) {
      EnqueueMotorActions();
    }

    _dVars.touchValues.clear();

    _rcvdNextDatasetRequest = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::LoopMotorsActionCallback(ActionResult res)
{
  // don't loop unless we are in the correct mode
  if( _dVars.collectMode == MoveChrgOff || _dVars.collectMode == MoveChrgOn ) {
    EnqueueMotorActions();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::OnNextDataset(BehaviorExternalInterface& bexi,
                                                    CollectionMode cm, 
                                                    DataAnnotation da) const
{
  std::string serialno;
  std::stringstream ss;
  ss << std::hex << bexi.GetRobotInfo().GetHeadSerialNumber();
  serialno = ss.str();

  std::string outputPath = bexi.GetRobotInfo().GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "touchDataCollection");
  outputPath += "/";
  outputPath += serialno;

  Util::FileUtils::CreateDirectory(outputPath); 

  std::stringstream filePathSS;
  filePathSS << outputPath << "/";
  filePathSS << ToString(cm).second;
  filePathSS << "_";
  filePathSS << ToString(da).second;
  filePathSS << ".txt";
  
  std::string filePath = filePathSS.str();

  PRINT_CH_INFO("Behaviors","WriteToFile","%zd values to %s", _dVars.touchValues.size(), filePath.c_str());

  if(Util::FileUtils::FileExists(filePath)) {
    PRINT_NAMED_WARNING("BehaviorDevTouchDataCollection.FileAlreadyExists","Overwriting %s", filePath.c_str());
  }

  std::stringstream contents;
  for(auto& val : _dVars.touchValues) {
    contents << (int)val << std::endl;
  }

  Util::FileUtils::WriteFile(filePath, contents.str());
  
  if(!Util::FileUtils::FileExists(filePath)) {
    PRINT_NAMED_WARNING("BehaviorDevTouchDataCollection.FailedToWrite","%s", filePath.c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevTouchDataCollection::OnBehaviorDeactivated()
{
}

} // Cozmo
} // Anki
