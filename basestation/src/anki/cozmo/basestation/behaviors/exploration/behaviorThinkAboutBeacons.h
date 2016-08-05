/**
 * File: behaviorThinkAboutBeacons
 *
 * Author: Raul
 * Created: 07/25/16
 *
 * Decription: behavior for when Cozmo needs to make decisions about AIbeacons. This allows playing animations or
 *  showing intent rather than if making the decision was just a module somewhere else in the AI.
 *
 * Copyright: Anki, Inc. 2016
 *
**/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorThinkAboutBeacons_H__
#define __Cozmo_Basestation_Behaviors_BehaviorThinkAboutBeacons_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorThinkAboutBeacons : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorThinkAboutBeacons(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // destructor
  virtual ~BehaviorThinkAboutBeacons();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // true if currently there are edges that Cozmo would like to visit
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // behavior configuration passed on initialization
  struct Configuration
  {
    std::string newAreaAnimTrigger; // animation to play when we discover a new area
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set configuration values from the given config
  void LoadConfig(const Json::Value& config);
  
  // select where to put a new beacon and add it to the whiteboard
  void SelectNewBeacon(Robot& robot);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // parsed configurations params from json
  Configuration _configParams;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
