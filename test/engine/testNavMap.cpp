/**
 * File: testNavMap
 *
 * Author: ross
 * Created: 10/14/15
 *
 * Description: Unit tests for nav map-related methods
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=TestNavMap.*
 **/

#include "gtest/gtest.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "engine/cozmoContext.h"
#include "engine/navMap/iNavMap.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMap.h"
#include "engine/navMap/memoryMap/data/memoryMapData.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/robot.h"

using namespace Anki;
using namespace Anki::Vector;

extern CozmoContext* cozmoContext;

TEST( TestNavMap, FillBorder)
{
  // tests the content transformations performed by fillborder, but not the locations
  // of the quads.
  // four region sizes:
  //  bigQuad: a clear space
  //  littleProx1: a prox contained within bigQuad
  //  medEdge: an edge contained within bigQuad
  //  littleProx2: a prox contained within medEdge
  // we will fill from clear space to prox obstacles and replace it with a cliff.
  // Only the littleProx1 should be replaced, not littleProx2
  
  Robot robot(0, cozmoContext);
  INavMap* navMap = robot.GetMapComponent().GetCurrentMemoryMap().get();
  ASSERT_TRUE( navMap != nullptr );
  // use memory map for access into things that are protected in inavmap
  MemoryMap* memoryMap = dynamic_cast<MemoryMap*>( navMap );
  ASSERT_TRUE( memoryMap != nullptr );
  
  MemoryMapData baseArea( EContentType::ClearOfObstacle, 0 );
  Point2f robotPos = robot.GetPose().GetTranslation();
  Poly2f bigQuad( {{robotPos.x(), robotPos.y()},
                  {robotPos.x() + 500, robotPos.y()},
                  {robotPos.x() + 500, robotPos.y() + 500},
                  {robotPos.x(), robotPos.y()+500}} );
  memoryMap->Insert( bigQuad, baseArea );
  
  MemoryMapData_ProxObstacle proxObstacle( MemoryMapData_ProxObstacle::EXPLORED, {0.0f, 0.0f, 0.0f}, 0 );
  Poly2f littleProx1( {{robotPos.x() + 100, robotPos.y() + 100},
                      {robotPos.x() + 110, robotPos.y()},
                      {robotPos.x() + 110, robotPos.y() + 110},
                      {robotPos.x() + 100, robotPos.y() + 110}} );
  memoryMap->Insert( littleProx1, proxObstacle );
  
  MemoryMapData edgeType( EContentType::InterestingEdge, 0 );
  Poly2f medEdge( {{robotPos.x() + 200, robotPos.y() + 200},
                  {robotPos.x() + 400, robotPos.y() + 200},
                  {robotPos.x() + 400, robotPos.y() + 400},
                  {robotPos.x() + 200, robotPos.y() + 400}} );
  memoryMap->Insert( medEdge, edgeType );
  
  MemoryMapData_ProxObstacle proxObstacle2( MemoryMapData_ProxObstacle::EXPLORED, {0.0f, 0.0f, 0.0f}, 0 );
  Poly2f littleProx2( {{robotPos.x() + 210, robotPos.y() + 210},
                      {robotPos.x() + 220, robotPos.y() + 210},
                      {robotPos.x() + 220, robotPos.y() + 220},
                      {robotPos.x() + 210, robotPos.y() + 220}} );
  memoryMap->Insert( littleProx2, proxObstacle2 );
  
  
  bool hasClear = false;
  bool hasProx = false;
  bool hasCliff = false;
  bool hasEdge = false;
  memoryMap->AnyOf( bigQuad, [&](MemoryMapDataConstPtr ptr) {
    hasClear |= ptr->type == EContentType::ClearOfObstacle;
    hasProx  |= ptr->type == EContentType::ObstacleProx;
    hasCliff |= ptr->type == EContentType::Cliff;
    hasEdge  |= ptr->type == EContentType::InterestingEdge;
    return false;
  });
  EXPECT_TRUE( hasClear );
  EXPECT_TRUE( hasProx );
  EXPECT_TRUE( hasEdge );
  EXPECT_FALSE( hasCliff );
  
  // also check in the medium quad
  hasClear = false;
  hasProx = false;
  hasCliff = false;
  hasEdge = false;
  memoryMap->AnyOf( medEdge, [&](MemoryMapDataConstPtr ptr) {
    hasClear |= ptr->type == EContentType::ClearOfObstacle;
    hasProx  |= ptr->type == EContentType::ObstacleProx;
    hasCliff |= ptr->type == EContentType::Cliff;
    hasEdge  |= ptr->type == EContentType::InterestingEdge;
    return false;
  });
  EXPECT_FALSE( hasClear );
  EXPECT_TRUE( hasProx );
  EXPECT_TRUE( hasEdge );
  EXPECT_FALSE( hasCliff );
  
  constexpr FullContentArray clearOfObstacleTypes =
  {
    {EContentType::Unknown               , false},
    {EContentType::ClearOfObstacle       , true},
    {EContentType::ClearOfCliff          , false},
    {EContentType::ObstacleObservable    , false },
    {EContentType::ObstacleProx          , false },
    {EContentType::ObstacleUnrecognized  , false },
    {EContentType::Cliff                 , false},
    {EContentType::InterestingEdge       , false},
    {EContentType::NotInterestingEdge    , false }
  };
  ASSERT_TRUE( IsSequentialArray(clearOfObstacleTypes) );
  
  MemoryMapData_Cliff toReplace( Pose3d{""}, 0 );
  memoryMap->FillBorder(EContentType::ObstacleProx, clearOfObstacleTypes, toReplace.Clone());
  
  // check the big quad
  hasClear = false;
  hasProx = false;
  hasCliff = false;
  hasEdge = false;
  memoryMap->AnyOf( bigQuad, [&](MemoryMapDataConstPtr ptr) {
    hasClear |= ptr->type == EContentType::ClearOfObstacle;
    hasProx  |= ptr->type == EContentType::ObstacleProx;
    hasCliff |= ptr->type == EContentType::Cliff;
    hasEdge  |= ptr->type == EContentType::InterestingEdge;
    return false;
  });
  EXPECT_TRUE( hasClear );
  EXPECT_TRUE( hasProx );
  EXPECT_TRUE( hasEdge );
  EXPECT_TRUE( hasCliff );
  
  // and the medium quad
  hasClear = false;
  hasProx = false;
  hasCliff = false;
  hasEdge = false;
  memoryMap->AnyOf( medEdge, [&](MemoryMapDataConstPtr ptr) {
    hasClear |= ptr->type == EContentType::ClearOfObstacle;
    hasProx  |= ptr->type == EContentType::ObstacleProx;
    hasCliff |= ptr->type == EContentType::Cliff;
    hasEdge  |= ptr->type == EContentType::InterestingEdge;
    return false;
  });
  EXPECT_FALSE( hasClear );
  EXPECT_TRUE( hasProx );
  EXPECT_TRUE( hasEdge );
  EXPECT_FALSE( hasCliff );
  
  // and the medium quad
  hasClear = false;
  hasProx = false;
  hasCliff = false;
  hasEdge = false;
  memoryMap->AnyOf( littleProx1, [&](MemoryMapDataConstPtr ptr) {
    hasClear |= ptr->type == EContentType::ClearOfObstacle;
    hasProx  |= ptr->type == EContentType::ObstacleProx;
    hasCliff |= ptr->type == EContentType::Cliff;
    hasEdge  |= ptr->type == EContentType::InterestingEdge;
    return false;
  });
  EXPECT_FALSE( hasClear );
  EXPECT_FALSE( hasProx );
  EXPECT_FALSE( hasEdge );
  EXPECT_TRUE( hasCliff );
  
  
  
}

