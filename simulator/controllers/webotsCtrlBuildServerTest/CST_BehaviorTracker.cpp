/**
 * File: CST_BehaviorTracker.cpp
 *
 * Author: Kevin M. Karol
 * Created: 6/23/16
 *
 * Description: Provide analytics on the states cozmo spends time in
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/logging/logging.h"
#include <iostream>
#include <fstream>

namespace Anki {
namespace Cozmo {
    
enum class TestState {
  StartUpFreeplayMode,
  FreePlay,
  TestDone
};

// ============ Test class declaration ============
class CST_BehaviorTracker : public CozmoSimTestController {
public:
  CST_BehaviorTracker();

private:
  struct BehaviorStateChange {
    std::string oldBehavior = "";
    std::string newBehavior = "";
    float elapsedTime = 0.f;
  };
  
  
  virtual s32 UpdateInternal() override;
 
  //Constants
  const float kFreeplayLengthSeconds = 600;
  
  //Tracking Data
  double _startTime;
  std::vector<std::string> _freeplayBehaviorList;
  TestState _testState = TestState::StartUpFreeplayMode;
  std::vector<BehaviorStateChange> _stateChangeList;
  
  // Message handlers
  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  virtual void HandleEnabledBehaviorList(const ExternalInterface::RespondEnabledBehaviorList& msg) override;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_BehaviorTracker);

// =========== Test class implementation ===========
CST_BehaviorTracker::CST_BehaviorTracker()
{
  //Get the start time of the test
  _startTime = GetSupervisor()->getTime();
  
}
  

s32 CST_BehaviorTracker::UpdateInternal()
{
  switch (_testState) {
    case TestState::StartUpFreeplayMode:
    {
      MakeSynchronous();
      
      //Send message to start Cozmo in freeplay mode
      ExternalInterface::ActivateBehaviorChooser behavior;
      behavior.behaviorChooserType = BehaviorChooserType::Freeplay;

      ExternalInterface::MessageGameToEngine message;
      message.Set_ActivateBehaviorChooser(behavior);

      SendMessage(message);
      
      //Request the list of freeplay bahivors from the behavior chooser class
      //this will be stored to ensure cozmo enters all freeplay behavior types
      SendRequestEnabledBehaviorList();
      
      _testState = TestState::FreePlay;
      break;
    }
    case TestState::FreePlay:
    {
      
      //Check for the end of the test
      double currentTime;
      currentTime = GetSupervisor()->getTime();
      double timeElapsed = currentTime - _startTime;
            
      if(timeElapsed > kFreeplayLengthSeconds){
        //Final state
        BehaviorStateChange change;
        change.newBehavior = "END";
        change.oldBehavior = _stateChangeList.back().newBehavior;
        change.elapsedTime = timeElapsed;
        _stateChangeList.push_back(change);
        
        _testState = TestState::TestDone;
      }
      break;
    }
    case TestState::TestDone:
    {
      std::map<std::string, float> timeMap;
      std::map<std::string, float>::iterator timeMapIter;
      std::vector<BehaviorStateChange>::iterator stateChangeIter;
      float fullTime = 0.f;
      float timeDiff = 0.f;
      
      //Ensure all behaviors print
      for(const auto& behavior: _freeplayBehaviorList){
        timeMap.insert(std::make_pair(behavior, 0.f));
      }
      
      for(stateChangeIter = _stateChangeList.begin(); stateChangeIter != _stateChangeList.end(); ++stateChangeIter){
        float elapsedTime = stateChangeIter->elapsedTime;
        
        //Track the total time per behavior
        if(stateChangeIter != _stateChangeList.begin()){
          std::vector<BehaviorStateChange>::iterator prev = std::prev(stateChangeIter);
          timeDiff = elapsedTime - fullTime;
          fullTime = elapsedTime;
          
          timeMapIter = timeMap.find(prev->newBehavior);
          
          if(timeMapIter != timeMap.end()){
            timeMapIter->second += timeDiff;
          }else{
            timeMap.insert(std::make_pair(prev->newBehavior, timeDiff));
          }
        }
      } //End for stateChangeIter loop
      
      //Change timeMapIter from elapsed time to percentage total time
      if(fullTime > 0) {
        for(auto& entry: timeMap){
          entry.second = entry.second / fullTime;
        }
      }
      
      //Check to ensure all behaviors ran, or fail test
      //Currently deactivated until we know we will hit all behaviors in the specified time
      /**for(const auto& behavior: _freeplayBehaviorList){
        CST_EXPECT(, "Behavior not played during freeplay");
      }**/
      
      for(timeMapIter = timeMap.begin(); timeMapIter != timeMap.end(); ++timeMapIter){
        PRINT_NAMED_INFO("Webots.BehaviorTracker.TestData", "##teamcity[buildStatisticValue key='%s%s' value='%f']", "wbtsBehavior_", timeMapIter->first.c_str(), timeMapIter->second);
      }
      
      CST_EXIT();
      break;
    }
  }
  return _result;
}

// ================ Message handler callbacks ==================
void CST_BehaviorTracker::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  double currentTime = GetSupervisor()->getTime();
  BehaviorStateChange change;
  change.newBehavior = msg.newBehavior;
  change.oldBehavior = msg.oldBehavior;
  change.elapsedTime = currentTime -  _startTime;
  
  _stateChangeList.push_back(change);
}

void CST_BehaviorTracker::HandleEnabledBehaviorList(const ExternalInterface::RespondEnabledBehaviorList& msg)
{
  //what is persistence of clad?  copy or reference?
  _freeplayBehaviorList = msg.behaviors;
}

// ================ End of message handler callbacks ==================
  
} // end namespace Cozmo
} // end namespace Anki

