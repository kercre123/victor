/**
 * File: pathComponentTest.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-08-14
 *
 * Description: Unit tests for the path component
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "util/helpers/includeGTest.h"

#define private public
#define protected public

#include "engine/components/pathComponent.h"
#include "engine/cozmoContext.h"
#include "engine/xyPlanner.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "test/engine/helpers/messaging/stubRobotMessageHandler.h"

using namespace Anki;
using namespace Cozmo;

extern Anki::Cozmo::CozmoContext* cozmoContext;

#define EXPECT_STATUS_EQ(x, y) EXPECT_EQ((x), (y)) << "expected " << ERobotDriveToPoseStatusToString(x) \
                                                   << " got " << ERobotDriveToPoseStatusToString(y)


class PathComponentTest : public testing::Test
{
protected:

  virtual void SetUp() override {
    _msgHandler = new StubMessageHandler;

    // stub in our own outgoing message handler
    cozmoContext->GetRobotManager()->_robotMessageHandler.reset(_msgHandler);

    _robot.reset(new Robot(1, cozmoContext));
    _pathComponent = &(_robot->GetPathComponent());

    _pathComponent->_longPathPlanner.reset(new XYPlanner(_robot.get(), true));
    XYPlanner* planner = dynamic_cast<XYPlanner*>(_pathComponent->_longPathPlanner.get());
    ASSERT_TRUE(planner != nullptr);

    _robot->FakeSyncRobotAck();

    // Fake a state message update for robot
    RobotState stateMsg = _robot->GetDefaultRobotState();

    bool result = _robot->UpdateFullRobotState(stateMsg);
    ASSERT_EQ(result, RESULT_OK);
  }

private:
  std::unique_ptr<Robot> _robot;
  PathComponent* _pathComponent = nullptr;
  StubMessageHandler* _msgHandler = nullptr;

  //
  // Convenience function to execute one tick of path component.
  // This works because path component doesn't actually have any
  // dependent components.
  //
  void Update(Anki::Cozmo::PathComponent * pathComponent)
  {
    static const Anki::Cozmo::RobotCompMap dependentComps;
    pathComponent->UpdateDependent(dependentComps);
  }

};

TEST_F(PathComponentTest, Create)
{
  ASSERT_TRUE(_robot != nullptr);
  ASSERT_TRUE(_pathComponent != nullptr);
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready);
}


TEST_F(PathComponentTest, BasicPlan)
{
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready)
    << "status should start out as READY";

  Pose3d goal( 0, Z_AXIS_3D(), Vec3f(200,0,0), _robot->GetPose() );

  _pathComponent->StartDrivingToPose( { goal } );

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::ComputingPath)
    << "planning should be computing for at least one tick";

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "planning should now be complete";


  ASSERT_FALSE(_msgHandler->GetMsgsToRobot().empty()) << "should have some messages";

  int pathID = -1;
  const bool found = _msgHandler->FindStartedExecutePathMsg(pathID);

  EXPECT_TRUE(found) << "should have send execute path message";
  EXPECT_EQ(pathID, 1) << "first path should be path id 1";

  // clear out messages
  _msgHandler->ClearMsgsToRobot();

  for( int i=0; i<10; ++i ) {
    Update(_pathComponent);
  }
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "should still be waiting";

  {
    // send back following path message
    RobotInterface::PathFollowingEvent startedPathEvent;
    startedPathEvent.pathID = pathID;
    startedPathEvent.eventType = PathEventType::PATH_STARTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(startedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::FollowingPath)
    << "should be following now";

  for( int i=0; i<10; ++i ) {
    Update(_pathComponent);
  }
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::FollowingPath);

  {
    // send finished message
    RobotInterface::PathFollowingEvent startedPathEvent;
    startedPathEvent.pathID = pathID;
    startedPathEvent.eventType = PathEventType::PATH_COMPLETED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(startedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready);
}


TEST_F(PathComponentTest, AbortPlanBug)
{
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready)
    << "status should start out as READY";

  Pose3d goal( 0, Z_AXIS_3D(), Vec3f(200,0,0), _robot->GetPose() );

  _pathComponent->StartDrivingToPose( { goal } );

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::ComputingPath)
    << "planning should be computing for at least one tick";

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "planning should now be complete";


  ASSERT_FALSE(_msgHandler->GetMsgsToRobot().empty()) << "should have some messages";

  int pathID = -1;
  const bool found = _msgHandler->FindStartedExecutePathMsg(pathID);

  EXPECT_TRUE(found) << "should have send execute path message";
  EXPECT_EQ(pathID, 1) << "first path should be path id 1";

  // clear out messages
  _msgHandler->ClearMsgsToRobot();

  for( int i=0; i<10; ++i ) {
    Update(_pathComponent);
  }
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "should still be waiting";

  // abort the path
  const Result res = _pathComponent->Abort();
  EXPECT_EQ(res, Result::RESULT_OK) << "abort failed";

  Update(_pathComponent);

  EXPECT_FALSE(_pathComponent->IsActive())
    << "path should have been aborted (test bug)";

  {
    // send back following path message now, simulating the case where this message was on the wire while we
    // aborted the message
    RobotInterface::PathFollowingEvent startedPathEvent;
    startedPathEvent.pathID = pathID;
    startedPathEvent.eventType = PathEventType::PATH_STARTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(startedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);

  {
    int cancelPathID;
    const bool found = _msgHandler->FindClearPathMsg(cancelPathID);
    EXPECT_TRUE(found) << "did not find clear path message from abort";
    EXPECT_TRUE( cancelPathID == 0 || cancelPathID == pathID )
      << "cancel path id " << cancelPathID << " doesn't match send path id " << pathID;
  }

  for( int i=0; i<4; ++i ) {
    Update(_pathComponent);
  }

  {
    // now finally send up that we canceled the path
    RobotInterface::PathFollowingEvent stoppedPathEvent;
    stoppedPathEvent.pathID = pathID;
    stoppedPathEvent.eventType = PathEventType::PATH_INTERRUPTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(stoppedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  for( int i=0; i<4; ++i ) {
    Update(_pathComponent);
  }

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready);
}


TEST_F(PathComponentTest, AbortPlanComplex)
{
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::Ready)
    << "status should start out as READY";

  Pose3d goal( 0, Z_AXIS_3D(), Vec3f(200,0,0), _robot->GetPose() );

  _pathComponent->StartDrivingToPose( { goal } );

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::ComputingPath)
    << "planning should be computing for at least one tick";

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "planning should now be complete";


  ASSERT_FALSE(_msgHandler->GetMsgsToRobot().empty()) << "should have some messages";

  int pathID0 = -1;
  {
    const bool found = _msgHandler->FindStartedExecutePathMsg(pathID0);

    EXPECT_TRUE(found) << "should have send execute path message";
    EXPECT_EQ(pathID0, 1) << "first path should be path id 1";
  }

  // clear out messages
  _msgHandler->ClearMsgsToRobot();

  for( int i=0; i<10; ++i ) {
    Update(_pathComponent);
  }
  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath)
    << "should still be waiting";

  // abort the path
  const Result res = _pathComponent->Abort();
  EXPECT_EQ(res, Result::RESULT_OK) << "abort failed";

  Update(_pathComponent);

  EXPECT_FALSE(_pathComponent->IsActive())
    << "path should have been aborted (test bug)";

  // now start a new path
  Pose3d goal2( 0, Z_AXIS_3D(), Vec3f(0,200,0), _robot->GetPose() );
  _pathComponent->StartDrivingToPose( { goal2 } );

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::ComputingPath);

  {
    // send back following path message now, simulating the case where this message was on the wire while we
    // aborted the message
    RobotInterface::PathFollowingEvent startedPathEvent;
    startedPathEvent.pathID = pathID0;
    startedPathEvent.eventType = PathEventType::PATH_STARTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(startedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);
  Update(_pathComponent);
  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath);

  int pathID1 = -1;
  {
    const bool found = _msgHandler->FindStartedExecutePathMsg(pathID1);
    EXPECT_TRUE(found) << "should have send execute path message";
    EXPECT_EQ(pathID1, 2) << "second path should be path id 2";
  }

  {
    int cancelPathID;
    const bool found = _msgHandler->FindClearPathMsg(cancelPathID);
    EXPECT_TRUE(found) << "did not find clear path message from abort";
    EXPECT_TRUE( cancelPathID == 0 || cancelPathID == pathID0 )
      << "cancel path id " << cancelPathID << " doesn't match send path id " << pathID0;
  }

  for( int i=0; i<4; ++i ) {
    Update(_pathComponent);
  }

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath);

  {
    // now send up that we canceled the path
    RobotInterface::PathFollowingEvent stoppedPathEvent;
    stoppedPathEvent.pathID = pathID0;
    stoppedPathEvent.eventType = PathEventType::PATH_INTERRUPTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(stoppedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);
  Update(_pathComponent);
  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::WaitingToBeginPath);

  {
    // now finally send up that we're starting the new path
    RobotInterface::PathFollowingEvent startedPathEvent;
    startedPathEvent.pathID = pathID1;
    startedPathEvent.eventType = PathEventType::PATH_STARTED;
    RobotInterface::RobotToEngine msg;
    msg.Set_pathFollowingEvent(startedPathEvent);

    _msgHandler->Broadcast(msg);
  }

  Update(_pathComponent);

  EXPECT_STATUS_EQ(_pathComponent->GetDriveToPoseStatus(), ERobotDriveToPoseStatus::FollowingPath);
}
