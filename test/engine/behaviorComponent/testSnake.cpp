/**
 * File: testSnake.cpp
 *
 * Author: ross
 * Created: 2018 feb 27
 *
 * Description: Test of snake
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "anki/cozmo/shared/cozmoConfig.h"
#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGame.h"
#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGameSolver.h"
#include "engine/robot.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/random/randomGenerator.h"

using namespace Anki;
using namespace Anki::Vector;

#define TEST_PRINT_SNAKE_GAME 0
void PrintGame(const SnakeGame& game, int k)
{
  if( !TEST_PRINT_SNAKE_GAME ) {
    return;
  }
  std::cout << std::endl;
  int height = game.GetHeight();
  int width = game.GetWidth();
  std::vector<std::string> rows;
  for( int j=-1; j<height+1; ++j ) {
    std::stringstream ss;
    for( int i=-1; i<width+1; ++i ) {
      if( i<0 || j<0 || i>=game.GetWidth() || j>=game.GetHeight() ) {
        ss << "%";
        continue;
      }
      if( game.GetSnake().IsSnakeAt(i,j) ) {
        ss << "X";
      } else if( game.GetFood().x == i && game.GetFood().y == j ) {
        ss << "O";
      } else {
        ss << " ";
      }
    }
    rows.push_back( ss.str() );
  }
  // print in reverse (up is +)
  for( auto it = rows.rbegin(); it != rows.rend(); ++it ) {
    std::cout << *it;
    if( it + 1 == rows.rend() ) {
      std::cout << " k=" << k;
    }
    std::cout << std::endl;
  }
  
}

TEST(TestSnake, Uncontrolled)
{
  Util::RandomGenerator rng(123);
  
  int displayHeight = 20;
  int displayWidth = 100;
  int initLength = 5;
  SnakeGame game(displayWidth, displayHeight, initLength, rng );
  SnakeGameSolver solver(game, 0.0f, 0.0f, 0.0f, rng);
  
  PrintGame( game, 0 );
  
  bool died = false;
  for( int k=0; k<200; ++k  ) {
    game.Update();
    //solver.Update();
    
    if( game.GameOver() ) {
      EXPECT_EQ( k, 95 );
      died = true;
      break;
    }
    PrintGame( game, k );
  }
  EXPECT_TRUE(died);
  EXPECT_EQ( game.GetScore(), 0 );
  
}

TEST(TestSnake, Controlled)
{
  // this test: play three games. In the first, we're trying pretty hard. not all the way, which would
  // make us never lose. but pretty hard. In the next two, we're sloppy, first because of mistakes,
  // and then because of wrong turns. This tests absolute and relative scores for the three games
  
  Util::RandomGenerator rng(321);
  
  int displayHeight = 20;
  int displayWidth = 30;
  
  int kTrying = 0;
  bool lostTrying = false;
  int scoreTrying = 0;
  int numScoreChangesTrying = 0;
  
  int kSloppy = 0;
  bool lostSloppy = false;
  int scoreSloppy = 0;
  int numScoreChangesSloppy = 0;
  
  auto run = [&](int tryingAmount, int& k, bool& lost, int& score, int& numScoreChanges) {
    
    int initLength = 5;
    SnakeGame game(displayWidth, displayHeight, initLength, rng );
    
    float mistakeRate = tryingAmount<=1 ? 0.01 : 0.1;
    float pWrongTurn = tryingAmount==1 ? 0.1f : 0.0f;
    SnakeGameSolver solver( game, mistakeRate, 0.0f, pWrongTurn, rng);
    k=0;
    for( ; k<10000; ++k  ) {
      game.Update();
      
      if( k>= 1402 ) {
        std::cout << "fsdf";
      }
      if( game.GameOver() ) {
        lost = true;
        std::cout << "YOU LOSE at step" << k << ", is that all you got?!" << std::endl;
        break;
      }
      
      EXPECT_GE( game.GetScore(), score );
      if( game.GetScore() > score ) {
        ++numScoreChanges;
        score = game.GetScore();
      }
      
      PrintGame( game, k );
      
      ASSERT_EQ( game.GetSnake().GetLength(), score + 5 );
      
      solver.ChooseAndApplyMove();
    }
  };
  
  run(0, kTrying, lostTrying, scoreTrying, numScoreChangesTrying);
  
  EXPECT_GT(kTrying, 600);
  EXPECT_GT(numScoreChangesTrying, 30);
  EXPECT_TRUE( lostTrying );
  
  for( int i=1; i<=2; ++i ) {
    lostSloppy=false;
    kSloppy=0;
    scoreSloppy=0;
    numScoreChangesSloppy=0;
    
    run(1, kSloppy, lostSloppy, scoreSloppy, numScoreChangesSloppy);
    
    EXPECT_TRUE( lostSloppy );
    EXPECT_LT(kSloppy, kTrying);
    EXPECT_LT(scoreSloppy, scoreTrying);
    EXPECT_LT(numScoreChangesSloppy, numScoreChangesTrying);
  }
  
}

