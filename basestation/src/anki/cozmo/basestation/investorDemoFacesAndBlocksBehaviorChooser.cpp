/**
 * File: investorDemoFacesAndBlocksBehaviorChooser.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-11-25
 *
 * Description: The behavior chooser for the investor demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorBlockPlay.h"
#include "anki/cozmo/basestation/behaviors/behaviorFindFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/investorDemoFacesAndBlocksBehaviorChooser.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

// overwrite the scores to be a fixed priority
// TODO:(bn) maybe we should have ANOTHER behavior chooser, FixedPriorityChooser or something, which does this
class BehaviorFindFaces_investorDemo : public BehaviorFindFaces
{
public:
  
  // FIXME - constructor should be private, only creatable via BehaviorFactory
  BehaviorFindFaces_investorDemo(Robot& robot, const Json::Value& config)
    : BehaviorFindFaces(robot, config)
  {
    DEMO_HACK_SetName("FindFaces_ID"); // to avoid clash in factory with subclass
  }
  
  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override {
    return 0.1f;
  }
};

class BehaviorInteractWithFaces_investorDemo : public BehaviorInteractWithFaces
{
public:
  
  // FIXME - constructor should be private, only creatable via BehaviorFactory
  BehaviorInteractWithFaces_investorDemo(Robot& robot, const Json::Value& config)
    : BehaviorInteractWithFaces(robot, config)
  {
    DEMO_HACK_SetName("Faces_ID"); // to avoid clash in factory with subclass
  }

  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override {
    return 0.2f;
  }
};

class BehaviorBlockPlay_investorDemo : public BehaviorBlockPlay
{
public:
  
  // FIXME - constructor should be private, only creatable via BehaviorFactory
  BehaviorBlockPlay_investorDemo(Robot& robot, const Json::Value& config)
    : BehaviorBlockPlay(robot, config)
  {
    DEMO_HACK_SetName("BlockPlay_ID"); // to avoid clash in factory with subclass
  }

  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override {
    return 0.5f;
  }
};


InvestorDemoFacesAndBlocksBehaviorChooser::InvestorDemoFacesAndBlocksBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
{
  SetupBehaviors(robot, config);

  // enable live idle animation
  robot.SetIdleAnimation(AnimationStreamer::LiveAnimation);

  // pick the appropriate vision modes
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion,  false);
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingFaces,   true);
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
}


void InvestorDemoFacesAndBlocksBehaviorChooser::AddNonFactoryBehavior(BehaviorFactory& behaviorFactory, IBehavior* newBehavior)
{
  behaviorFactory.DEMO_HACK_AddToFactory(newBehavior);
  super::AddBehavior( newBehavior );
}
  
  
void InvestorDemoFacesAndBlocksBehaviorChooser::SetupBehaviors(Robot& robot, const Json::Value& config)
{
  BehaviorFactory& behaviorFactory = robot.GetBehaviorFactory();
  
  // super::AddBehavior( behaviorFactory.CreateBehavior(BehaviorType::NoneBehavior, robot, config) );
  // super::AddBehavior( behaviorFactory.CreateBehavior(BehaviorType::Fidget,       robot, config) );

  AddNonFactoryBehavior( behaviorFactory, new BehaviorFindFaces_investorDemo(robot, config) );
  AddNonFactoryBehavior( behaviorFactory, new BehaviorInteractWithFaces_investorDemo(robot, config) );
  AddNonFactoryBehavior( behaviorFactory, new BehaviorBlockPlay_investorDemo(robot, config) );
}

Result InvestorDemoFacesAndBlocksBehaviorChooser::Update(double currentTime_sec)
{
  return super::Update(currentTime_sec);
}

}
}
