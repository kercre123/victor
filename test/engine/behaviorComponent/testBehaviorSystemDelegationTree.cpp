/**
* File: testBehaviorDleegationTree
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Set of test functions that walk through cozmo's data defined
* behavior tree that ensure all possible behavior states are valid and
* within design's constraints
*
* Copyright: Anki, Inc. 2017
*
* --gtest_filter=DelegationTree*
**/

#include "gtest/gtest.h"

TEST(DelegationTree, FullTreeWalkthrough)
{
  // TODO: Read in the current behavior system configuration
  // and then walk through the full tree appropirately activating
  // and deactivating all delegates to ensure the tree is valid
}



TEST(DelegationTree, DesignedControlTest)
{
  // TODO: Read in the current behavior system configuration
  // then walk through the tree and ensure that all design requirements
  // are met. E.G. every behavior must be able to transition to V.C. when
  // necessary.
  
}
