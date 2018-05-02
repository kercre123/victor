/**
 * File: stubRobotMessageHandler.h
 *
 * Author: Brad Neuman
 * Created: 2017-08-15
 *
 * Description: Test helper to stub out messages to/from the robot
 *
 * Copyright: Anki, Inc. 2017
 *
 **/



#ifndef __Test_Helpers_Messaging_StubRobotMessageHandler_H__
#define __Test_Helpers_Messaging_StubRobotMessageHandler_H__

namespace Anki {
namespace Cozmo {

class StubMessageHandler : public RobotInterface::MessageHandler
{
public:

  virtual void Init(const Json::Value& config, RobotManager* robotMgr, const CozmoContext* context) override {
    // do nothing
  }

  virtual Result ProcessMessages() override {
    return RESULT_OK;
  }

  virtual Result SendMessage(const RobotInterface::EngineToRobot& msg,
                             bool reliable = true,
                             bool hot = false) override {
    _msgsToRobot.push_back(msg);
    // always succeed
    return Result::RESULT_OK;
  }

  // return true if there's an execute path message in the outgoing queue, and set the path id in arguments
  bool FindStartedExecutePathMsg(int& pathID) {
    for( const auto& msg : _msgsToRobot ) {
      if( msg.GetTag() == RobotInterface::EngineToRobotTag::executePath ) {
        pathID = msg.Get_executePath().pathID;
        return true;
      }
    }
    return false;
  }

  bool FindClearPathMsg(int& pathID) {
    for( const auto& msg : _msgsToRobot ) {
      if( msg.GetTag() == RobotInterface::EngineToRobotTag::clearPath ) {
        pathID = msg.Get_clearPath().pathID;
        return true;
      }
    }
    return false;
  }

  const std::vector<RobotInterface::EngineToRobot>& GetMsgsToRobot() const {return _msgsToRobot;}

  // Message storage grows without bounds, so it's a good idea to clear out from time to time. This functions
  // will clear all saved messages from engine to robot
  void ClearMsgsToRobot() { _msgsToRobot.clear(); }

private:

  // Holds all messages sent from engine to the robot.
  // NOTE: this grows without bounds, so if you have a long-running test, you should clear it periodically
  std::vector<RobotInterface::EngineToRobot> _msgsToRobot;

};

}
}

#endif
