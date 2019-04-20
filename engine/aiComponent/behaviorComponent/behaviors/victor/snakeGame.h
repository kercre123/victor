/**
 * File: snakeGame.h
 *
 * Author: ross
 * Created: 2018-02-27
 *
 * Description: the game of snake
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_SnakeGame__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_SnakeGame__

#include <deque>
#include <cstdint>

namespace Anki {

namespace Util {
  class RandomGenerator;
}
  
namespace Vector {
  
class SnakeGame
{
public:
  // todo: move all these elsewhere
  enum class Direction : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT
  };
  struct Point {
    Point( int xx, int yy ) : x(xx), y(yy) {}
    bool operator==(const Point& other) { return x==other.x && y==other.y; }
    int x;
    int y;
  };
  struct Snake
  {
    void Init(const Point& init) { body.push_back(init); }
    // see the result of each movement
    Point GetNextStep( Direction direction ) const;
    // add one to the head in this direction.
    void AddToHead( Direction direction );
    void PopTail() { body.pop_front(); }
    Point& GetHead() { return body.back(); }
    const Point& GetHead() const { return body.back(); }
    Point& GetTail() { return body.front(); }
    const Point& GetTail() const { return body.front(); }
    size_t GetLength() const { return body.size(); }
    bool IsSnakeAt(unsigned int x, unsigned int y) const;
    std::deque<Point> body; // snake body, from tail to head, i.e., push_back() adds to the head
  };
  
  // width and height is the space INTERIOR TO WALLS. The boundary is negative or > width-1/height-1
  SnakeGame( unsigned int width, unsigned int height, unsigned int initLength, Util::RandomGenerator& rng );
  
  virtual ~SnakeGame();

  void Update();
  
  // player can set the next direction
  void SetDirection( Direction direction ) { _direction = direction; }
  
  Direction GetDirection() const { return _direction; }
  const Point& GetFood() const { return _food; }
  const Snake& GetSnake() const { return _snake; }
  unsigned int GetWidth() const { return _width; }
  unsigned int GetHeight() const { return _height; }
  int GetScore() const { return _score; }
  
  bool GameOver() const { return !_alive; }
  
private:
  
  // adds one to the head end of the snake. Doesn't remove from the tail (todo: it's confusing not
  // having a simple "move" function. fix it)
  void PushToSnakeHead();
  
  void RandomizeFood();
  
  unsigned int _width;
  unsigned int _height;
  
  unsigned int _initLength;
  
  Util::RandomGenerator& _rng;
  
  Direction _direction;
  
  Point _food;
  
  Snake _snake;
  
  bool _alive;
  
  unsigned int _score;
  
};


}
}

#endif
