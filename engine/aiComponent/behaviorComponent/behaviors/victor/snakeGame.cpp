/**
 * File: snakeGame.cpp
 *
 * Author: ross
 * Created: 2018-02-27
 *
 * Description: the game of snake
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGame.h"
#include "util/random/randomGenerator.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnakeGame::SnakeGame(unsigned int width, unsigned int height, unsigned int initLength, Util::RandomGenerator& rng)
  : _width( width )
  , _height( height )
  , _initLength( initLength )
  , _rng( rng )
  , _food( 0,0 )
  , _alive( true )
  , _score( 0 )
{
  DEV_ASSERT( _width > _initLength, "" );
  DEV_ASSERT( _height > _initLength, "" );
  
  // random direction
  _direction = static_cast<Direction>(_rng.RandIntInRange(0, 3));
  
  // get random initial snake position, up against a wall and pointing away from it
  int x=0;
  int y=0;
  if( _direction == Direction::UP ) {
    x = _rng.RandIntInRange(0, _width-1);
    y = 0;
  } else if ( _direction == Direction::DOWN ) {
    x = _rng.RandIntInRange(0, _width-1);
    y = height - 1;
  } else if( _direction == Direction::LEFT ) {
    x = width - 1;
    y = _rng.RandIntInRange(0, _height-1);
  } else if( _direction == Direction::RIGHT ) {
    x = 0;
    y = _rng.RandIntInRange(0, _height-1);
  }
  _snake.Init( Point{x,y} );
  for( size_t i=0; i<_initLength-1; ++i ) {
    PushToSnakeHead();
  }
  
  RandomizeFood();
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnakeGame::~SnakeGame()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnakeGame::Update()
{
  if( !_alive ) {
    return;
  }
  
  // move the snake head +1 in its current direction. Don't move the tail yet
  PushToSnakeHead();
  
  if( _alive && (_snake.GetHead() == _food) ) {
    ++_score;
    RandomizeFood();
    // don't pop the tail, thereby increasing the length
  } else {
    // subtract from the tail
    _snake.PopTail();
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnakeGame::Point SnakeGame::Snake::GetNextStep( Direction direction ) const
{
  const auto& p = body.back();
  Point next( p.x, p.y);
  if( direction == Direction::UP ) {
    ++next.y;
  } else if ( direction == Direction::DOWN ) {
    --next.y;
  } else if( direction == Direction::LEFT ) {
    --next.x;
  } else if( direction == Direction::RIGHT ) {
    ++next.x;
  }
  return next;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnakeGame::Snake::AddToHead( Direction direction )
{
  Point next = GetNextStep(direction);
  body.push_back( next );
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SnakeGame::Snake::IsSnakeAt(unsigned int x, unsigned int y) const
{
  for( const auto& p : body ) {
    if( p.x == x && p.y == y ) {
      return true;
    }
  }
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnakeGame::PushToSnakeHead()
{
  Point next = _snake.GetNextStep( _direction );
  
  if( (next.x < 0) || (next.x >= _width) || (next.y < 0) || (next.y >= _height) ) {
    _alive = false;
    return;
  }
  
  if( _snake.IsSnakeAt(next.x,next.y) ) {
    _alive = false;
    return;
  }
  
  _snake.AddToHead( _direction );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnakeGame::RandomizeFood()
{
  while(true) { // todo: bounded while, or even first generate a list of open spots and then select from that
    _food.x = _rng.RandIntInRange(0, _width-1);
    _food.y = _rng.RandIntInRange(0, _height-1);
    if( !_snake.IsSnakeAt(_food.x, _food.y) ) {
      return;
    }
  }
}

}
}
