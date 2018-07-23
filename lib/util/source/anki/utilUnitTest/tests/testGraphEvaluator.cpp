/**
 * File: testGraphEvaluator
 *
 * Author: Mark Wesley
 * Created: 10/13/15
 *
 * Description: Unit tests for graphEvaluator2d
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=GraphEvaluator*
 **/


#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"
#include "json/json.h"


TEST(GraphEvaluator2d, OneNode)
{
  // A 1-node graph should always return the y value for that 1 node
  
  const float kNodeValX = 0.5f;
  const float kNodeValY = 1234.567f;
  Anki::Util::GraphEvaluator2d testGraph( {{kNodeValX, kNodeValY}} );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 1);

  EXPECT_FLOAT_EQ(testGraph.EvaluateY(  - 10.0f), kNodeValY);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     0.0f), kNodeValY);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(kNodeValX), kNodeValY);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    10.0f), kNodeValY);
  
  float xFound = 0.0f;
  EXPECT_EQ( testGraph.FindFirstX(   100.0f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(kNodeValY, xFound), true  );
  EXPECT_FLOAT_EQ(xFound, 0.5f);
  EXPECT_EQ( testGraph.FindFirstX(  2000.0f, xFound), false );
  
  // For one point, the slope is defined as 0
  EXPECT_EQ( testGraph.GetSlopeAt(kNodeValX), 0.0f );
}


TEST(GraphEvaluator2d, TwoNodesUp)
{
  // A 2-node graph should always return the y value for [0] or [1] if out of bounds, and lerped otherwise

  const float kNodeValX1 =   0.5f;
  const float kNodeValY1 =  10.3f;
  const float kNodeValX2 = 100.5f;
  const float kNodeValY2 =  60.3f;
  
  Anki::Util::GraphEvaluator2d testGraph( {{kNodeValX1, kNodeValY1}, {kNodeValX2, kNodeValY2}} );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(  -10.0f), kNodeValY1);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   25.5f), 22.8f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   50.5f), 35.3f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY( 1000.0f), kNodeValY2);
  
  float xFound = 0.0f;
  EXPECT_EQ( testGraph.FindFirstX(    10.2f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(    10.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.5f);
  EXPECT_EQ( testGraph.FindFirstX(    22.8f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  25.5f);
  EXPECT_EQ( testGraph.FindFirstX(    35.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  50.5f);
  EXPECT_EQ( testGraph.FindFirstX(    60.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound, 100.5f);
  EXPECT_EQ( testGraph.FindFirstX(    60.4f, xFound), false );
  
  const float expectedSlope = (kNodeValY2 - kNodeValY1) / (kNodeValX2 - kNodeValX1);
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX1 - 1.0f), 0.0f );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX1), expectedSlope );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX2), expectedSlope );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX2 + 1.0f), 0.0f );
}


TEST(GraphEvaluator2d, TwoNodesDown)
{
  // A 2-node graph should always return the y value for [0] or [1] if out of bounds, and lerped otherwise
  
  const float kNodeValX1 =   0.5f;
  const float kNodeValY1 =  60.3f;
  const float kNodeValX2 = 100.5f;
  const float kNodeValY2 =  10.3f;
  
  Anki::Util::GraphEvaluator2d testGraph( {{kNodeValX1, kNodeValY1}, {kNodeValX2, kNodeValY2}} );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(  -10.0f), kNodeValY1);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   25.5f), 47.8f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   50.5f), 35.3f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY( 1000.0f), kNodeValY2);
  
  float xFound = 0.0f;
  EXPECT_EQ( testGraph.FindFirstX(    10.2f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(    10.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound, 100.5f);
  EXPECT_EQ( testGraph.FindFirstX(    35.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  50.5f);
  EXPECT_EQ( testGraph.FindFirstX(    47.8f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  25.5f);
  EXPECT_EQ( testGraph.FindFirstX(    60.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.5f);
  EXPECT_EQ( testGraph.FindFirstX(    60.4f, xFound), false );
  
  const float expectedSlope = (kNodeValY2 - kNodeValY1) / (kNodeValX2 - kNodeValX1);
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX1 - 1.0f), 0.0f );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX1), expectedSlope );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX2), expectedSlope );
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX2 + 1.0f), 0.0f );
}


TEST(GraphEvaluator2d, TwoNodesSameX)
{
  // A 2-node graph should always return the y value for [0] or [1] if out of bounds, and lerped otherwise
  // where 2 neighboring nodes share the same x value, the y value will never be lerped between them and will instead
  // jump from the 1st node to the 2nd once x becomes > than the x value.
  
  const float kNodeValX  =   0.5f;
  const float kNodeValY1 =  10.3f;
  const float kNodeValY2 =  60.3f;
  
  Anki::Util::GraphEvaluator2d testGraph( {{kNodeValX, kNodeValY1}, {kNodeValX, kNodeValY2}} );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -10.0f), kNodeValY1);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(kNodeValX-0.00001f), kNodeValY1);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(kNodeValX),          kNodeValY1);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(kNodeValX+0.00001f), kNodeValY2);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(  1000.0f), kNodeValY2);
  
  float xFound = 0.0f;
  EXPECT_EQ( testGraph.FindFirstX(    10.2f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(    10.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.5f);
  EXPECT_EQ( testGraph.FindFirstX(    15.3f, xFound), false ); // (both nodes are on same x so interval y values are ignored)
  EXPECT_EQ( testGraph.FindFirstX(    60.3f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.5f);
  EXPECT_EQ( testGraph.FindFirstX(    60.4f, xFound), false );
  
  EXPECT_FLOAT_EQ( testGraph.GetSlopeAt(kNodeValX), std::numeric_limits<float>::max() );
}


TEST(GraphEvaluator2d, EightNodes)
{
  Anki::Util::GraphEvaluator2d testGraph( {{-5.0f,  -10.0f}, {-3.0f,  -2.0f}, {-1.0f,   0.0f}, { -1.0f, 100.0f},
                                           { 1.0f, -100.0f}, { 3.0f, -90.0f}, { 5.0f, -90.0f}, { 7.0f, -91.0f}} );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 8);

  EXPECT_FLOAT_EQ(testGraph.EvaluateY(-1000.0f), -10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(  -10.0f), -10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -5.0f), -10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -4.5f),  -8.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -4.0f),  -6.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -3.5f),  -4.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -3.0f),  -2.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -2.5f),  -1.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -2.0f),  -1.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -1.5f),  -0.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -1.00000001f),   0.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -1.0),           0.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -0.9999999f),  100.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -0.5f),   50.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    0.0f),   0.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    0.5f),  -50.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    1.0f), -100.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    1.5f),  -97.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    2.0f),  -95.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    2.5f),  -92.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    3.0f),  -90.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    3.5f),  -90.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    4.0f),  -90.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    4.5f),  -90.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    5.0f),  -90.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    5.5f),  -90.25f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    6.0f),  -90.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    6.5f),  -90.75f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    7.0f),  -91.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   10.0f),  -91.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(10000.0f),  -91.0f);
  
  float xFound = 0.0f;
  EXPECT_EQ( testGraph.FindFirstX(    -10.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -5.0f);
  EXPECT_EQ( testGraph.FindFirstX(     -8.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -4.5f);
  EXPECT_EQ( testGraph.FindFirstX(     -4.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -3.5f);
  EXPECT_EQ( testGraph.FindFirstX(     -2.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -3.0f);
  EXPECT_EQ( testGraph.FindFirstX(     -0.5f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -1.5f);
  EXPECT_EQ( testGraph.FindFirstX(      0.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -1.0f);
  EXPECT_EQ( testGraph.FindFirstX(    100.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -1.0f);
  EXPECT_EQ( testGraph.FindFirstX(    100.5f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(     50.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,  -0.5f);
  EXPECT_EQ( testGraph.FindFirstX(    -50.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.5f);
  EXPECT_EQ( testGraph.FindFirstX(   -100.0f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   1.0f);
  EXPECT_EQ( testGraph.FindFirstX(   -100.5f, xFound), false );
  
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(-6.0f), 0.0f);
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(-5.0f), (  -2.f - -10.f)/(-3.f - -5.f));
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(-4.0f), (  -2.f - -10.f)/(-3.f - -5.f));
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(-1.0f), (   0.f -  -2.f)/(-1.f - -3.f));
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(-0.9f), (-100.f - 100.f)/( 1.f - -1.f));
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(7.f),   ( -91.f - -90.f)/( 7.f -  5.f));
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(7.1f),  0.0f);
}

TEST(GraphEvaluator2d, SlopeEdgeCase)
{
  Anki::Util::GraphEvaluator2d testGraph( {{0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 100.0f}} );
  
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(0.5f), 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(1.0f), 1.0f); // slope should not be infinite
  EXPECT_FLOAT_EQ(testGraph.GetSlopeAt(2.0f), 0.0f);
}


TEST(GraphEvaluator2d, BadDataNodeOrder)
{
  // Should fail to add the 2nd node as it's x is < than previous node

  Anki::Util::GraphEvaluator2d testGraph( {{0.0f, 10.0f}, {-1.0f, 8.0f}, {1.0f, 1000.0f}}, false);

  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(   -10.00f),   10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    -1.00f),   10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     0.00f),   10.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     0.25f),  257.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     0.50f),  505.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     0.75f),  752.5f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(     1.00f), 1000.0f);
  EXPECT_FLOAT_EQ(testGraph.EvaluateY(    10.00f), 1000.0f);
  
  float xFound = -1.0f;
  EXPECT_EQ( testGraph.FindFirstX(     9.999f, xFound), false );
  EXPECT_EQ( testGraph.FindFirstX(    10.000f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.00f);
  EXPECT_EQ( testGraph.FindFirstX(   257.500f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.25f);
  EXPECT_EQ( testGraph.FindFirstX(   505.000f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.50f);
  EXPECT_EQ( testGraph.FindFirstX(   752.500f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   0.75f);
  EXPECT_EQ( testGraph.FindFirstX(  1000.000f, xFound), true  );
  EXPECT_FLOAT_EQ(xFound,   1.00f);
  EXPECT_EQ( testGraph.FindFirstX(  1000.500f, xFound), false  );
}


TEST(GraphEvaluator2d, MoveConstructor)
{
  Anki::Util::GraphEvaluator2d testGraph( {{1.0f, 2.0f}, {3.0f, 4.0f}});
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  Anki::Util::GraphEvaluator2d testGraph2( std::move(testGraph) ); // Move Constructor
  
  // testBuff is now probably empty (but not guarenteed), testBuff2 should be equivalent to old testBuff

  //EXPECT_EQ(testGraph.GetNumNodes(), 0); // can't guarentee this as it depends on std::vector implementation (but works in XCode and helps verify that we're invoking the move version)
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
  
  // check we can reserve testGraph again and add to it without it affecting testGraph2
  
  testGraph.Clear();
  testGraph.AddNode(5.0f,  6.0f);
  testGraph.AddNode(7.0f,  8.0f);
  testGraph.AddNode(9.0f, 10.0f);
  
  EXPECT_EQ(testGraph.GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x,  5.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y,  6.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x,  7.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y,  8.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._x,  9.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._y, 10.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
}


TEST(GraphEvaluator2d, MoveAssignment)
{
  Anki::Util::GraphEvaluator2d testGraph( {{1.0f, 2.0f}, {3.0f, 4.0f}});
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  Anki::Util::GraphEvaluator2d testGraph2;
  testGraph2 = std::move(testGraph); // Move Assignment
  
  //  testBuff is now probably empty (but not guarenteed), testBuff2 should be equivalent to old testBuff
  
  //EXPECT_EQ(testGraph.GetNumNodes(), 0); // can't guarentee this as it depends on std::vector implementation (but works in XCode and helps verify that we're invoking the move version)
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
  
  // check we can reserve testGraph again and add to it without it affecting testGraph2
  
  testGraph.Clear();
  testGraph.AddNode(5.0f,  6.0f);
  testGraph.AddNode(7.0f,  8.0f);
  testGraph.AddNode(9.0f, 10.0f);
  
  EXPECT_EQ(testGraph.GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x,  5.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y,  6.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x,  7.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y,  8.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._x,  9.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._y, 10.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
}


TEST(GraphEvaluator2d, CopyConstructor)
{
  Anki::Util::GraphEvaluator2d testGraph( {{1.0f, 2.0f}, {3.0f, 4.0f}});
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  Anki::Util::GraphEvaluator2d testGraph2( testGraph ); // Copy Constructor
  
  // testBuff and testBuff2 should now be equivalent (although independent if modified in the future)
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
  
  // check we can reserve testGraph again and add to it without it affecting testGraph2
  
  testGraph.Clear();
  testGraph.AddNode(5.0f,  6.0f);
  testGraph.AddNode(7.0f,  8.0f);
  testGraph.AddNode(9.0f, 10.0f);
  
  EXPECT_EQ(testGraph.GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x,  5.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y,  6.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x,  7.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y,  8.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._x,  9.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._y, 10.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
}


TEST(GraphEvaluator2d, CopyAssignment)
{
  Anki::Util::GraphEvaluator2d testGraph( {{1.0f, 2.0f}, {3.0f, 4.0f}});
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  Anki::Util::GraphEvaluator2d testGraph2;
  testGraph2 = testGraph; // Copy Assignment
  
  // testBuff and testBuff2 should now be equivalent (although independent if modified in the future)
  
  EXPECT_EQ(testGraph.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y, 4.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
  
  // check we can reserve testGraph again and add to it without it affecting testGraph2
  
  testGraph.Clear();
  testGraph.AddNode(5.0f,  6.0f);
  testGraph.AddNode(7.0f,  8.0f);
  testGraph.AddNode(9.0f, 10.0f);
  
  EXPECT_EQ(testGraph.GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._x,  5.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(0)._y,  6.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._x,  7.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(1)._y,  8.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._x,  9.0f);
  EXPECT_FLOAT_EQ(testGraph.GetNode(2)._y, 10.0f);
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._x, 1.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(0)._y, 2.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._x, 3.0f);
  EXPECT_FLOAT_EQ(testGraph2.GetNode(1)._y, 4.0f);
}


TEST(GraphEvaluator2d, RoundTripJson)
{
  Anki::Util::GraphEvaluator2d testGraph( {{-5.0f, 0.0f}, {0.0f, -10.0f}, {1.5f, 2.0f}, {2.0f, 123.456f}, {20.0f, 50.0f}} );
  
  Json::Value graphJson;
  const bool writeRes = testGraph.WriteToJson(graphJson);
  
  EXPECT_TRUE( writeRes );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 5);
  EXPECT_EQ(testGraph.GetNode(0)._x,  -5.0f);
  EXPECT_EQ(testGraph.GetNode(0)._y,   0.0f);
  EXPECT_EQ(testGraph.GetNode(1)._x,   0.0f);
  EXPECT_EQ(testGraph.GetNode(1)._y, -10.0f);
  EXPECT_EQ(testGraph.GetNode(2)._x,   1.5f);
  EXPECT_EQ(testGraph.GetNode(2)._y,   2.0f);
  EXPECT_EQ(testGraph.GetNode(3)._x,   2.0f);
  EXPECT_EQ(testGraph.GetNode(3)._y, 123.456f);
  EXPECT_EQ(testGraph.GetNode(4)._x,  20.0f);
  EXPECT_EQ(testGraph.GetNode(4)._y,  50.0f);
  
  Anki::Util::GraphEvaluator2d testGraph2;
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 0);
  
  const bool readRes = testGraph2.ReadFromJson(graphJson);
  
  EXPECT_TRUE( readRes );
  
  EXPECT_EQ(testGraph2.GetNumNodes(), 5);
  EXPECT_EQ(testGraph2.GetNode(0)._x,  -5.0f);
  EXPECT_EQ(testGraph2.GetNode(0)._y,   0.0f);
  EXPECT_EQ(testGraph2.GetNode(1)._x,   0.0f);
  EXPECT_EQ(testGraph2.GetNode(1)._y, -10.0f);
  EXPECT_EQ(testGraph2.GetNode(2)._x,   1.5f);
  EXPECT_EQ(testGraph2.GetNode(2)._y,   2.0f);
  EXPECT_EQ(testGraph2.GetNode(3)._x,   2.0f);
  EXPECT_EQ(testGraph2.GetNode(3)._y, 123.456f);
  EXPECT_EQ(testGraph2.GetNode(4)._x,  20.0f);
  EXPECT_EQ(testGraph2.GetNode(4)._y,  50.0f);
}


const char* kTestGraph2dJson =
"{\n"
"   \"nodes\" : [\n"
"      {\n"
"         \"x\" : -1,\n"
"         \"y\" : 0\n"
"      },\n"
"      {\n"
"         \"x\" : 0.5,\n"
"         \"y\" : 1\n"
"      },\n"
"      {\n"
"         \"x\" : 1,\n"
"         \"y\" : 0.60000002384185791\n"
"      }\n"
"   ]\n"
"}\n\n";


TEST(GraphEvaluator2d, ReadJson)
{
  Anki::Util::GraphEvaluator2d testGraph;
  
  Json::Value  graphJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestGraph2dJson, graphJson, false);
  ASSERT_TRUE(parsedOK);
  
  const bool readRes = testGraph.ReadFromJson(graphJson);
  EXPECT_TRUE( readRes );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 3);
  EXPECT_EQ(testGraph.GetNode(0)._x,  -1.0f);
  EXPECT_EQ(testGraph.GetNode(0)._y,   0.0f);
  EXPECT_EQ(testGraph.GetNode(1)._x,   0.5f);
  EXPECT_EQ(testGraph.GetNode(1)._y,   1.0f);
  EXPECT_EQ(testGraph.GetNode(2)._x,   1.0f);
  EXPECT_EQ(testGraph.GetNode(2)._y,   0.6f);
}


// Bad data - misspelled x and y entries
const char* kBadTestGraph2dJson =
"{\n"
"   \"nodes\" : [\n"
"      {\n"
"         \"notx\" : -1,\n"
"         \"noty\" :  1\n"
"      }\n"
"   ]\n"
"}\n\n";


TEST(GraphEvaluator2d, ReadBadJson)
{
  Anki::Util::GraphEvaluator2d testGraph;
  
  Json::Value  graphJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBadTestGraph2dJson, graphJson, false);
  ASSERT_TRUE(parsedOK);
  
  const bool readRes = testGraph.ReadFromJson(graphJson);
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 0);
}


// Bad data - misspelled nodes entry
const char* kBad2TestGraph2dJson =
"{\n"
"   \"nodez\" : [\n"
"   ]\n"
"}\n\n";


TEST(GraphEvaluator2d, ReadBadJson2)
{
  Anki::Util::GraphEvaluator2d testGraph;
  
  Json::Value  graphJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kBad2TestGraph2dJson, graphJson, false);
  ASSERT_TRUE(parsedOK);
  
  const bool readRes = testGraph.ReadFromJson(graphJson);
  EXPECT_FALSE( readRes );
  
  EXPECT_EQ(testGraph.GetNumNodes(), 0);
}


TEST(GraphEvaluator2d, EqualityOperators)
{
  Anki::Util::GraphEvaluator2d::Node node1(1.0f, 2.0f);
  Anki::Util::GraphEvaluator2d::Node node2(1.0f, 2.0f);
  
  EXPECT_TRUE( node1 == node2);
  EXPECT_FALSE(node1 != node2);
  
  node2._x = 1.1f;
  
  EXPECT_TRUE( node1 != node2);
  EXPECT_FALSE(node1 == node2);

  node2._x = 1.0f;
  node2._y = 1.1f;
  
  EXPECT_TRUE( node1 != node2);
  EXPECT_FALSE(node1 == node2);

  node2._y = 2.0f;
  
  EXPECT_TRUE( node1 == node2);
  EXPECT_FALSE(node1 != node2);
  
  Anki::Util::GraphEvaluator2d testGraph1( {{-5.0f,  -10.0f}, {-3.0f,  -2.0f}, {-1.0f,   0.0f}, { -1.0f, 100.0f}} );
  Anki::Util::GraphEvaluator2d testGraph2 = testGraph1;
  EXPECT_TRUE( testGraph1 == testGraph2);
  EXPECT_FALSE(testGraph1 != testGraph2);
  
  testGraph2.AddNode(0.0f, 0.0f);
  EXPECT_TRUE( testGraph1 != testGraph2);
  EXPECT_FALSE(testGraph1 == testGraph2);
}


