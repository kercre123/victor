#pragma once

#include "coretech/vision/engine/image.h"
#include <memory>
#include <mutex>

namespace Anki {
  namespace Vision {
    class ImageRGB565;
  }
}

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {

class Event;
template <typename T>
class Vector2;
  
typedef unsigned char   Uint8;
typedef unsigned int    Uint32;
  
  
class Texture
{
public:
  Texture(){}
  void create(Uint32 width, Uint32 height);
  void update(const Uint8* pixels);
  const Uint8* GetPixels() const { return _pixels; }
  Uint32 GetWidth() const { return _width; }
  Uint32 GetHeight() const { return _height; }
private:
  Uint32 _width;
  Uint32 _height;
  const Uint8* _pixels = nullptr;
};

class Sprite
{
public:
  void setTexture(const Texture& texture) { _texture = &texture; }
  void setScale(const Vector2<int>& size);
  void setScale(float scaleX, float scaleY);
  
  Anki::Vision::ImageRGB GetImage() const;
private:
  const Texture* _texture = nullptr; // the fullsize image from the game
  Uint32 _height=0;
  Uint32 _width=0;
};
class RenderWindow
{
public:
  void clear();
  void draw(const Sprite& sprite);
  void display();
  
  void create(Uint32 width, Uint32 height);
  void setFramerateLimit(Uint8 fr){}
  
  bool pollEvent(Event& ev);
  
  Vector2<int> getSize() const;
  
  // called on anim thread
  void GetScreen(Anki::Vision::ImageRGB565& screen);
  void AddEvent(Event&& ev);
private:
  Uint32 _width;
  Uint32 _height;
  std::mutex _mutex;
  Anki::Vision::ImageRGB _image; // this code is sloppier because I was avoiding including image in the .h but had to anyway :|
  int _frameCount=0;
  int _lastFrameCount=0;
  
};

class Color
{
public:
  Color() : Color(0,0,0,255) {}
  Color(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha=255);
  Color(Uint32 color);
  
  Uint32 toInteger () const;
  
  Uint8   r;
  Uint8   g;
  Uint8   b;
  Uint8   a;
  
  static const Color White;
};
  
}
