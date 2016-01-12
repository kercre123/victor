/**
 * File: testProgressionSystem
 *
 * Author: Mark Wesley
 * Created: 10/14/15
 *
 * Description: Unit tests for Progression System and Stats
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=ProgressionSystem*
 **/


#include "gtest/gtest.h"
#include "anki/cozmo/basestation/progressionSystem/progressionManager.h"
#include <limits.h>


using namespace Anki::Cozmo;


TEST(ProgressionSystem, TestStats)
{
  ProgressionManager progressionManager;
  
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Excitement).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::FriendshipLevel).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::FriendshipScore).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Novelty).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Time).GetValue(),  0);
  progressionManager.GetStat(ProgressionStatType::Excitement).Add(10);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Excitement).GetValue(), 10);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Novelty).GetValue(),  0);
  
  // Check we clamp correctly when added beyond the max
  progressionManager.GetStat(ProgressionStatType::Excitement).Add(ProgressionStat::kStatValueMax);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Excitement).GetValue(), ProgressionStat::kStatValueMax);

  // Check we clamp correctly when adding enough to overflow
  progressionManager.GetStat(ProgressionStatType::Excitement).Add( std::numeric_limits<ProgressionStat::ValueType>::max() );
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Excitement).GetValue(), ProgressionStat::kStatValueMax);
  
  progressionManager.Reset();
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Excitement).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::FriendshipLevel).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::FriendshipScore).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Novelty).GetValue(),  0);
  EXPECT_EQ(progressionManager.GetStat(ProgressionStatType::Time).GetValue(),  0);
}