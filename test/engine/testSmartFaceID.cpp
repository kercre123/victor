/**
 * File: testSmartFaceID.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-04-14
 *
 * Description: tests for face ID smart container
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "gtest/gtest.h"

#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/smartFaceId.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"


using namespace Anki;
using namespace Anki::Cozmo;

extern CozmoContext* cozmoContext;

void DeleteFace(Robot& robot, int faceID)
{
  using namespace ExternalInterface;
  robot.Broadcast(MessageEngineToGame(RobotDeletedFace(faceID)));
}

void ChangeFaceID(Robot& robot, int oldID, int newID)
{
  using namespace ExternalInterface;
  robot.Broadcast(MessageEngineToGame(RobotChangedObservedFaceID(oldID, newID)));
}


TEST(SmartFaceID, Empty)
{
  SmartFaceID sfid;

  EXPECT_FALSE( sfid.IsValid() );
  EXPECT_TRUE(sfid.MatchesFaceID(0));
}

TEST(SmartFaceID, Invalid)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(0);

  EXPECT_FALSE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(0));
}

TEST(SmartFaceID, Valid)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(42);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(42));
}

TEST(SmartFaceID, DeletedSimple)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(42);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(42));

  DeleteFace(robot, 42);

  EXPECT_FALSE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(0));  
}

TEST(SmartFaceID, ChangedSimple)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(42);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(42));

  ChangeFaceID(robot, 42, 20);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(20));
}

TEST(SmartFaceID, ComplexSingleFace)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(42);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(42));

  // none of these should effect sfid
  DeleteFace(robot, 12);
  DeleteFace(robot, 20);
  ChangeFaceID(robot, 20, 30);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(42));

  ChangeFaceID(robot, 42, 20);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(20));

  // now he shouldn't care about 42
  ChangeFaceID(robot, 42, 50);
  DeleteFace(robot, 42);
  DeleteFace(robot, 50);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(20));

  robot.GetFaceWorld().UpdateSmartFaceToID(55, sfid);
  
  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(55));

  DeleteFace(robot, 20);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(55));

  DeleteFace(robot, 55);

  EXPECT_FALSE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(0));
}


TEST(SmartFaceID, CopyConstructor)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(42);

  ChangeFaceID(robot, 42, 20);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(20));

  SmartFaceID sfid2(sfid);

  EXPECT_TRUE(sfid2 == sfid);
  EXPECT_FALSE(sfid2 < sfid);
  EXPECT_FALSE(sfid < sfid2);
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(20));

  ChangeFaceID(robot, 20, 30);

  EXPECT_TRUE(sfid2 == sfid);
  EXPECT_FALSE(sfid2 < sfid);
  EXPECT_FALSE(sfid < sfid2);
  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(30));
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(30));

  // split sfid2 to be something else
  robot.GetFaceWorld().UpdateSmartFaceToID(10, sfid2);

  EXPECT_FALSE(sfid2 == sfid);
  EXPECT_TRUE(sfid2 < sfid);
  EXPECT_FALSE(sfid < sfid2);
  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(30));
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(10));

  ChangeFaceID(robot, 30, 99);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(99));
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(10));

  ChangeFaceID(robot, 30, 7);
  DeleteFace(robot, 30);
  DeleteFace(robot, 7);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(99));
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(10));

  DeleteFace(robot, 99);

  EXPECT_FALSE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(0));
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(10));

  DeleteFace(robot, 10);

  EXPECT_FALSE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(0));
  EXPECT_FALSE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(0));
}

TEST(SmartFaceID, CopyWithDelete)
{
  Robot robot(0, cozmoContext);

  SmartFaceID* sfidPtr = new SmartFaceID(robot.GetFaceWorld().GetSmartFaceID(42));
  
  EXPECT_TRUE(sfidPtr->IsValid());
  EXPECT_TRUE(sfidPtr->MatchesFaceID(42));

  {
    SmartFaceID sfid2(*sfidPtr);

    EXPECT_TRUE(sfid2 == *sfidPtr);
    EXPECT_FALSE(sfid2 < *sfidPtr);
    EXPECT_FALSE(*sfidPtr < sfid2);
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(42));
    EXPECT_TRUE(sfid2.IsValid());
    EXPECT_TRUE(sfid2.MatchesFaceID(42));

    ChangeFaceID(robot, 42, 52);

    EXPECT_TRUE(sfid2 == *sfidPtr);
    EXPECT_FALSE(sfid2 < *sfidPtr);
    EXPECT_FALSE(*sfidPtr < sfid2);
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(52));
    EXPECT_TRUE(sfid2.IsValid());
    EXPECT_TRUE(sfid2.MatchesFaceID(52));

    robot.GetFaceWorld().UpdateSmartFaceToID(10, sfid2);
    EXPECT_FALSE(sfid2 == *sfidPtr);
    EXPECT_TRUE(sfid2 < *sfidPtr);
    EXPECT_FALSE(*sfidPtr < sfid2);    
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(52));
    EXPECT_TRUE(sfid2.IsValid());
    EXPECT_TRUE(sfid2.MatchesFaceID(10));

    ChangeFaceID(robot, 10, 11);
    DeleteFace(robot, 11);
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(52));
    EXPECT_FALSE(sfid2.IsValid());
    EXPECT_TRUE(sfid2.MatchesFaceID(0));
  }

  EXPECT_TRUE(sfidPtr->IsValid());
  EXPECT_TRUE(sfidPtr->MatchesFaceID(52));
  ChangeFaceID(robot, 52, 55);

  {
    // test assignment to an empty id
    SmartFaceID sfid3;
    sfid3 = *sfidPtr;

    EXPECT_TRUE(sfid3 == *sfidPtr);
    EXPECT_FALSE(sfid3 < *sfidPtr);
    EXPECT_FALSE(*sfidPtr < sfid3);
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(55));
    EXPECT_TRUE(sfid3.IsValid());
    EXPECT_TRUE(sfid3.MatchesFaceID(55));

    SmartFaceID sfid4 = robot.GetFaceWorld().GetSmartFaceID(100);

    EXPECT_FALSE(sfid4 == *sfidPtr);
    EXPECT_FALSE(sfid4 < *sfidPtr);
    EXPECT_TRUE(*sfidPtr < sfid4);    
    EXPECT_TRUE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(100));

    sfid4 = *sfidPtr;

    EXPECT_TRUE(sfid4 == *sfidPtr);
    EXPECT_FALSE(sfid4 < *sfidPtr);
    EXPECT_FALSE(*sfidPtr < sfid4);    
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(55));
    EXPECT_TRUE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(55));

    DeleteFace(robot, 100);

    EXPECT_TRUE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(55));
    
    SmartFaceID sfid5(*sfidPtr);
    
    EXPECT_TRUE(sfidPtr->IsValid());
    EXPECT_TRUE(sfidPtr->MatchesFaceID(55));
    EXPECT_TRUE(sfid5.IsValid());
    EXPECT_TRUE(sfid5.MatchesFaceID(55));

    delete sfidPtr;

    EXPECT_TRUE(sfid3.IsValid());
    EXPECT_TRUE(sfid3.MatchesFaceID(55));
    EXPECT_TRUE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(55));
    EXPECT_TRUE(sfid5.IsValid());
    EXPECT_TRUE(sfid5.MatchesFaceID(55));

    ChangeFaceID(robot, 55, 99);

    EXPECT_TRUE(sfid3.IsValid());
    EXPECT_TRUE(sfid3.MatchesFaceID(99));
    EXPECT_TRUE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(99));
    EXPECT_TRUE(sfid5.IsValid());
    EXPECT_TRUE(sfid5.MatchesFaceID(99));

    DeleteFace(robot, 42);
    DeleteFace(robot, 10);
    DeleteFace(robot, 11);
    DeleteFace(robot, 52);
    DeleteFace(robot, 55);
    DeleteFace(robot, 99);

    EXPECT_FALSE(sfid3.IsValid());
    EXPECT_TRUE(sfid3.MatchesFaceID(0));
    EXPECT_FALSE(sfid4.IsValid());
    EXPECT_TRUE(sfid4.MatchesFaceID(0));
    EXPECT_FALSE(sfid5.IsValid());
    EXPECT_TRUE(sfid5.MatchesFaceID(0));

    EXPECT_TRUE(sfid3 == sfid4);
    EXPECT_FALSE(sfid4 < sfid3);
    EXPECT_FALSE(sfid3 < sfid4);
  }
}

TEST(SmartFaceID, RValue)
{
  Robot robot(0, cozmoContext);

  SmartFaceID sfid = robot.GetFaceWorld().GetSmartFaceID(1);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(1));

  ChangeFaceID(robot, 1, 2);

  EXPECT_TRUE(sfid.IsValid());
  EXPECT_TRUE(sfid.MatchesFaceID(2));

  // sfid is invalid now
  SmartFaceID sfid2(std::move(sfid));
  
  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(2));

  ChangeFaceID(robot, 2, 3);
  DeleteFace(robot, 2);

  EXPECT_TRUE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(3));

  DeleteFace(robot, 3);

  EXPECT_FALSE(sfid2.IsValid());
  EXPECT_TRUE(sfid2.MatchesFaceID(0));
}
