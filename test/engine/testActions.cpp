/**
 * File: testActions.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-04-30
 *
 * Description: Extensive unit tests for actions
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/actions/actionInterface.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "gtest/gtest.h"

using namespace Anki::Cozmo;


TEST(Action, CreateAndDestroy)
{
  Anki::Util::_errG = false;
  
  auto* action = new CompoundActionSequential();
  action->AddAction(new TurnInPlaceAction(0.0f, true));

  delete action;
  
  // TODO: test that everything is released properly. For now just test that there are no errors or crashes
  ASSERT_FALSE(Anki::Util::_errG);
}
