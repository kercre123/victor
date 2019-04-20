/**
 * File: snakeGameSolver.cpp
 *
 * Author: ross
 * Created: 2018-02-27
 *
 * Description: greedy solver for the game of snake
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGameSolver.h"

#include "engine/aiComponent/behaviorComponent/behaviors/victor/snakeGame.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

#include <list>
#include <vector>
#include <unordered_map>

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnakeGameSolver::SnakeGameSolver( SnakeGame& game,
                                  float probMistakesFlat,
                                  float probMistakesPerLength,
                                  float probWrongTurnsFlat,
                                  const Util::RandomGenerator& rng )
  : _game( game )
  , _rng( rng )
  , _pMistakesFlat( probMistakesFlat )
  , _pMistakesPerLength( probMistakesPerLength )
  , _pWrongTurns( probWrongTurnsFlat )
{
  
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnakeGameSolver::~SnakeGameSolver()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnakeGameSolver::ChooseAndApplyMove()
{
  if( _initialLength == 0 ) {
    _initialLength = static_cast<unsigned int>(_game.GetSnake().GetLength());
  }
  DEV_ASSERT( !_game.GameOver(),"" );
  
  // brief description to find the next direction to apply:
  // find the shortest path to the food. If it exists, move the snake along the entirety of the path
  // to the goal to see if it ends up in a bind -- if it would then be able to reach its tail, use
  // the first step of the path to the food.
  // If that doesn't work, find the longest path to the tail. If it exists, apply the first step of that path.
  // If that doesn't work, move the snake opposite of the direction of the food
  
  const auto& food = _game.GetFood();
  const auto& snake = _game.GetSnake();
  
  auto initDirection = _game.GetDirection();
  SnakeGame::Direction direction = initDirection;
  bool hasDirection = false;
  
  auto stepToDirection = [](const SnakeGame::Point& head, const SnakeGame::Point& firstStep) {
    SnakeGame::Direction direction = SnakeGame::Direction::UP;
    if( firstStep.x - head.x == 1 ) {
      direction = SnakeGame::Direction::RIGHT;
    } else if( firstStep.x - head.x == -1 ) {
      direction = SnakeGame::Direction::LEFT;
    } else if( firstStep.y - head.y == 1 ) {
      direction = SnakeGame::Direction::UP;
    } else if( firstStep.y - head.y == -1 ) {
      direction = SnakeGame::Direction::DOWN;
    } else {
      //assert
    }
    return direction;
  };
  
  std::vector<SnakeGame::Point> pathToFood;
  if( BFS( snake.GetHead(), food, snake, pathToFood ) ) {
    auto snakeCopy = snake;
    for( const auto& pt : pathToFood ) {
      snakeCopy.body.push_back( pt );
      snakeCopy.body.pop_front();
    }
    
    // make sure the shortest path doesn't put is in a dead end. compute path from future state to tail
    // there's a bug here -- the next path doesn't include the extra unit the snake obtained for hitting
    // the food. this would mean changing my BFS, so I'm just ignoring it. this is a greedy algorithm
    // after all.
    std::vector<SnakeGame::Point> pathToTail;
    if( BFS( snakeCopy.GetHead(), snakeCopy.GetTail(), snakeCopy, pathToTail) ) {
      const auto& firstStep = pathToFood.front();
      direction = stepToDirection( snake.GetHead(), firstStep );
      hasDirection = true;
    }
  }
  
  if( !hasDirection ) {
    // find the longest path to the tail by starting with the shortest path and Longify-ing it
    std::vector<SnakeGame::Point> pathToTail;
    if( BFS( snake.GetHead(), snake.GetTail(), snake, pathToTail) ) {
      // repeatedly perturb to take over more room until there is no more
      Longify( snake, snake.GetHead(), pathToTail );
      // use the first step
      const auto& firstStep = pathToTail.front();
      direction = stepToDirection( snake.GetHead(), firstStep );
      hasDirection = true;
    } else {
      // there's no path to the tail either.
      // choose a direction far away from the food
      if( (_game.GetDirection() == SnakeGame::Direction::UP)
          || (_game.GetDirection() == SnakeGame::Direction::DOWN) )
      {
        direction = (snake.GetHead().x >= food.x)
                    ? SnakeGame::Direction::RIGHT
                    : SnakeGame::Direction::LEFT;
      } else if( (_game.GetDirection() == SnakeGame::Direction::RIGHT)
                 || (_game.GetDirection() == SnakeGame::Direction::LEFT) )
      {
        direction = (snake.GetHead().y >= food.y)
                    ? SnakeGame::Direction::UP
                    : SnakeGame::Direction::DOWN;
      }
      hasDirection = true;
    }
  }
  
  // factor in some chance of a mistake or wrong turn
  const bool wrongTurn =  (direction != _game.GetDirection()) && (_rng.RandDbl() < _pWrongTurns);
  const float pMistake = _pMistakesFlat + _pMistakesPerLength*(snake.GetLength() - _initialLength);
  const bool wrongMove = (_rng.RandDbl() < pMistake);
  if( wrongTurn || wrongMove ) {
    do { // todo not this
      direction = static_cast<SnakeGame::Direction>( _rng.RandIntInRange(0, 3) );
    } while ( direction == initDirection );
  }
  
  ASSERT_NAMED(hasDirection, "");
  _game.SetDirection( direction );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SnakeGameSolver::BFS(const SnakeGame::Point& from,
                          const SnakeGame::Point& to,
                          const SnakeGame::Snake& snake,
                          std::vector<SnakeGame::Point>& path) const
{
  const int numNodes = _game.GetWidth() * _game.GetHeight();
  std::vector<bool> visited( numNodes, false );
  
  // todo: consolidate these
  std::list<int> queue;
  std::unordered_map<int, int> bps; // backpointers
  std::unordered_map<int, SnakeGame::Direction> bpDirections; // the direction of the bp
  
  // todo: avoid using these so much during the search
  auto sub2ind = [this](const SnakeGame::Point& pt) {
    return (_game.GetWidth() * pt.y) + pt.x;
  };
  auto ind2sub = [this](int ind) -> SnakeGame::Point {
    SnakeGame::Point pt(0,0);
    const int w = _game.GetWidth();
    pt.x = ind % w;
    pt.y = ind / w;
    return pt;
  };
  
  auto start = sub2ind(from);
  visited[start] = true;
  queue.push_back( start );
  
  bpDirections[start] = _game.GetDirection();
  
  // todo: optimize this. this is shitty af.
  
  while( !queue.empty() ) {
    // pop
    auto nodeInd = queue.front();
    queue.pop_front();
    
    auto pt = ind2sub(nodeInd);
    
    auto bpDirection = bpDirections[nodeInd];
    
    std::vector<std::pair<int, SnakeGame::Direction>> adjacents; // todo: precompute these or at least simplify it
    std::vector<std::pair<int, int>> directions; // x,y, each == +=1
    
    // to avoid kinky routes, always push the direction we're traveling into the queue
    // first. unless you like kinky stuff, then just use a constant direction vector
    if( bpDirection == SnakeGame::Direction::UP ) {
      directions = { {0,1}, {-1,0}, {1,0} };
    } else if( bpDirection == SnakeGame::Direction::DOWN ) {
      directions = { {0,-1}, {-1,0}, {1,0} };
    } else if( bpDirection == SnakeGame::Direction::LEFT ) {
      directions = { {-1,0}, {0,1}, {0,-1} };
    } else if( bpDirection == SnakeGame::Direction::RIGHT ) {
      directions = { {1,0}, {0,1}, {0,-1} };
    }
    
    for( const auto& direction : directions ) {
      SnakeGame::Point testPoint(pt.x + direction.first, pt.y + direction.second);
      if( testPoint == to ) {
        // BFS, can finish here
        auto node = sub2ind(to);
        bps[node] = nodeInd;
        
        path.clear();
        path.push_back(to);
        while(true) { // todo: not this
          auto parent = bps[node];
          if( parent == start ) {
            break;
          }
          path.push_back(ind2sub(parent));
          node = parent;
        }
        std::reverse( path.begin(), path.end() );
        // success
        return true;
      }
      
      // if the node lies on top of a snake our outside the boundaries, don't choose it
      if( !snake.IsSnakeAt( testPoint.x ,testPoint.y )
            && (testPoint.x>=0)
            && (testPoint.y>=0)
            && (testPoint.x<_game.GetWidth())
            && (testPoint.y<_game.GetHeight()) )
      {
        SnakeGame::Direction thisDirection; // we shouldnt need to find this again
        if( testPoint.x < pt.x ) {
          thisDirection = SnakeGame::Direction::LEFT;
        } else if( testPoint.x > pt.x ) {
          thisDirection = SnakeGame::Direction::RIGHT;
        } else if( testPoint.y < pt.y ) {
          thisDirection = SnakeGame::Direction::DOWN;
        } else if( testPoint.y > pt.y ) {
          thisDirection = SnakeGame::Direction::UP;
        }
        adjacents.emplace_back( sub2ind( testPoint ), thisDirection );
      }
    }
    
    for( const auto& adjPair : adjacents ) {
      if( !visited[adjPair.first] ) {
        visited[adjPair.first] = true;
        queue.push_back( adjPair.first );
        bps[adjPair.first] = nodeInd;
        bpDirections[adjPair.first] = adjPair.second;
      }
    }
  }
  
  // fail
  return false;
}
  
std::vector<SnakeGame::Point> SnakeGameSolver::Longify( const SnakeGame::Snake& snake,
                                                        const SnakeGame::Point& start,
                                                        const std::vector<SnakeGame::Point>& initPath ) const
{
  auto width = _game.GetWidth();
  auto height = _game.GetHeight();
  auto pt2ind = [&width](const SnakeGame::Point& pt) {
    return (width * pt.y) + pt.x;
  };
  auto sub2ind = [&width](int x, int y) {
    return (width * y) + x;
  };
  
  // a list that will start as initPath, but end up perturbed into a longer path
  std::list<SnakeGame::Point> path;
  // those nodes that are part of the snake (i.e., obstacles) or in current path. When the path
  // is perturbed, nodes are added, but they never need to be removed
  std::vector<bool> used( width*height, false );
  
  
  // the start needs to be added here, since the path doesn't normally include the start
  path.push_back( start );
  used[pt2ind(start)] = true;
  
  // copy the rest of the initial path into a list and mark it as used
  for( const auto& p : initPath ) {
    path.push_back( p );
    used[ pt2ind(p) ] = true;
  }
  
  // add the snake as an obstacle (used)
  for( const auto& p : snake.body ) {
    used[ pt2ind(p) ] = true;
  }
  
  // starting from the beginning of the path, try to turn a segment of length 1 that connects two
  // nodes into a path of length 3 by "taking over" the two adjacent nodes in the direction
  // perpendicular to the original segment. Keep trying from the start until it is no longer
  // possible, then move to the next segment, etc.
  
  auto it = path.begin();
  while( std::next(it) != path.end() ) {
    for( ; std::next(it) != path.end(); ++it ) {
      // can [it, it+1] be perturbed? It can happen in two directions.
      // get current direction.
      auto a = it;
      auto b = std::next(a);
      
      int dx = (b->x - a->x);
      int dy = (b->y - a->y);
      if( dx == 1 || dx == -1 ) { // right or left
        // try moving up or down.
        if( (b->y > 0)
            && !used[ sub2ind( b->x, b->y - 1 ) ]
            && !used[ sub2ind( a->x, a->y - 1 ) ] )
        {
          // insert just prior to (it+1)==b, twice
          SnakeGame::Point new1{a->x, a->y - 1};
          SnakeGame::Point new2{b->x, b->y - 1};
          path.insert( b, new1 );
          path.insert( b, new2 );
          used[ pt2ind(new1) ] = true;
          used[ pt2ind(new2) ] = true;
          break;
        }
        else if( (b->y < _game.GetHeight() - 1)
                 && !used[ sub2ind( b->x, b->y + 1 ) ]
                 && !used[ sub2ind( a->x, a->y + 1 ) ] )
                
        {
          SnakeGame::Point new1{a->x, a->y + 1};
          SnakeGame::Point new2{b->x, b->y + 1};
          path.insert( b, new1 );
          path.insert( b, new2 );
          used[ pt2ind(new1) ] = true;
          used[ pt2ind(new2) ] = true;
          break;
        }
      } else if( dy == 1 || dy == -1 ) { // up or down
        // try movign left or right
        if( (b->x > 0)
            && !used[ sub2ind( b->x - 1, b->y ) ]
            && !used[ sub2ind( a->x - 1, a->y ) ] )
        {
          SnakeGame::Point new1{a->x - 1, a->y};
          SnakeGame::Point new2{b->x - 1, b->y};
          path.insert( b, new1 );
          path.insert( b, new2 );
          used[ pt2ind(new1) ] = true;
          used[ pt2ind(new2) ] = true;
          break;
        }
        else if( (b->x < _game.GetWidth()-1)
                 && !used[ sub2ind( b->x + 1, b->y ) ]
                 && !used[ sub2ind( a->x + 1, a->y ) ] )
                
        {
          SnakeGame::Point new1{a->x + 1, a->y};
          SnakeGame::Point new2{b->x + 1, b->y};
          path.insert( b, new1 );
          path.insert( b, new2 );
          used[ pt2ind(new1) ] = true;
          used[ pt2ind(new2) ] = true;
          break;
        }
      } else {
        DEV_ASSERT(false, "");
      }
    }
  }
  
  std::vector<SnakeGame::Point> res;
  res.reserve( path.size() - 1 ); // skip the first
  for( auto it = path.begin(); it != path.end(); ++it ) {
    if( it != path.begin() ) {
      res.push_back( *it );
    }
  }
  return res;
}

}
}

