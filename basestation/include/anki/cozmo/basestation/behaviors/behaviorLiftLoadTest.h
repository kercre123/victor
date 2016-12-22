/**
 * File: behaviorLiftLoadTest.h
 *
 * Author: Kevin Yoon
 * Date:   11/11/2016
 *
 * Description: Tests the lift load detection system
 *              Saves attempt information to log webotsCtrlGameEngine/temp
 *
 *              Init conditions:
 *                - Cozmo's lift is down. A cube is optionally placed against it.
 *
 *              Behavior:
 *                - Raise and lower lift. Optionally with a cube attached
 *                - Records the result of the lift load check
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLiftLoadTest_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLiftLoadTest_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#include <fstream>

namespace Anki {
  namespace Cozmo {
    
    class BehaviorLiftLoadTest : public IBehavior
    {
      protected:
        friend class BehaviorFactory;
        BehaviorLiftLoadTest(Robot& robot, const Json::Value& config);
      
      public:
      
        virtual ~BehaviorLiftLoadTest() { }
      
        virtual bool IsRunnableInternal(const Robot& robot) const override;
        virtual bool CarryingObjectHandledInternally() const override { return false;}
      private:
      
        virtual Result InitInternal(Robot& robot) override;
      
        virtual IBehavior::Status UpdateInternal(Robot& robot) override;
      
        virtual void StopInternal(Robot& robot) override;
        
        virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
        virtual void HandleWhileRunning(const RobotToEngineEvent& event, const Robot& robot) override;
      
        virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) override;
      
        void PrintStats();
      
        enum class State {
          Init,           // Main test loop
          WaitForPutdown, // Robot was picked up, now waiting for putdown
          TestComplete    // Test complete
        };
      
        void SetCurrState(State s);
        const char* StateToString(const State m);
        void UpdateStateName();
      
        State _currentState = State::Init;
      
        Signal::SmartHandle _signalHandle;

        void Write(const std::string& s);
      
        Util::RollingFileLogger _logger;
      
        bool _abortTest = false;
        bool _canRun    = true;
      
        // Stats for test/attempts
        int _numLiftRaises        = 0;
        int _numHadLoad           = 0;
        bool _loadStatusReceived = false;
        bool _hasLoad            = false;
    };
  }
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDockingTest_H__