#include "cozmoAnim/doom/sf/Graphics.h"
#include "cozmoAnim/doom/sf/Event.h"
#include "cozmoAnim/doom/sf/Vector2.h"
#include "coretech/vision/engine/image.h"
#include "coretech/common/engine/math/matrix_impl.h"

#include <queue>

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Color Color::White(255,255,255);

Color::Color(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
  r = red;
  g = green;
  b = blue;
  a = alpha;
}

Color::Color(Uint32 color)
{
  a = (color >> (8*0)) & 0xFF;
  b = (color >> (8*1)) & 0xFF;
  g = (color >> (8*2)) & 0xFF;
  r = (color >> (8*3)) & 0xFF;
}
  
Uint32 Color::toInteger() const
{
  Uint32 rgb = r<<24 | g<<16 | b<<8 | a;
  return rgb;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// texture
  
void Texture::create(Uint32 width, Uint32 height)
{
  _width = width;
  _height = height;
  
}
  
void Texture::update(const Uint8* pixels)
{
  _pixels = pixels;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// sprite
void Sprite::setScale(const Vector2<int>& size)
{
  setScale(size.x, size.y);
}

void Sprite::setScale(float scaleX, float scaleY)
{
  DEV_ASSERT( _texture != nullptr, "");
  _width = scaleX * _texture->GetWidth();
  _height = scaleY * _texture->GetHeight();
  
}
  
Anki::Vision::ImageRGB Sprite::GetImage() const
{
  DEV_ASSERT( _texture != nullptr, "null texture" );
  
  // todo: three copies here, could be 1 if done manually.
  
  // src image. copy 1
  Anki::Vision::ImageRGB src( _texture->GetHeight(), _texture->GetWidth() );
  for( int i=0; i<_texture->GetHeight(); ++i ) {
    Anki::Vision::PixelRGB* row = src.GetRow(i);
    for( int j=0; j<_texture->GetWidth(); ++j ) {
      Uint32 offset = 3*i*_texture->GetWidth() + 3*j;
      Uint8 r = *(_texture->GetPixels() + offset + 0);
      Uint8 g = *(_texture->GetPixels() + offset + 1);
      Uint8 b = *(_texture->GetPixels() + offset + 2);
      row[j] = Anki::Vision::PixelRGB(r,g,b);
    }
  }
  
  // dst image
  Anki::Vision::ImageRGB dst(_height, _width);
  
  // resize. copy 2
  src.ResizeKeepAspectRatio( dst );
  
  // add border. copy 3
  if( (dst.GetNumCols() < _width) || (dst.GetNumRows() < _height) ) {
    Anki::Vision::ImageRGB ret(_height, _width);
    int top = (_height - dst.GetNumRows()) / 2;
    int bottom = (_height - dst.GetNumRows()) - top;
    int left = (_width - dst.GetNumCols()) / 2;
    int right = (_width - dst.GetNumCols()) - left;
    cv::copyMakeBorder(dst.get_CvMat_(), ret.get_CvMat_(), top, bottom, left, right, cv::BORDER_CONSTANT, 0 );
    return ret;
  } else {
    return dst;
  }
  
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// render window
  
std::queue<sf::Event> _events; // w/e.
  
void RenderWindow::create(Uint32 width, Uint32 height)
{
  _width = width;
  _height = height;
  
  
}
Vector2<int> RenderWindow::getSize() const
{
  return Vector2<int>(_width, _height);
  
}
  
void RenderWindow::clear()
{
  
}
void RenderWindow::draw(const Sprite& sprite)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _image = sprite.GetImage();
  ++_frameCount;
}
  
void RenderWindow::display()
{
  
}
  
void RenderWindow::GetScreen(Anki::Vision::ImageRGB565& screen)
{
  std::lock_guard<std::mutex> lock(_mutex);
  screen.SetFromImageRGB(_image);
  //PRINT_NAMED_WARNING("DOOM", "%d frames drawn per tick (@33 tps), total %d", _frameCount - _lastFrameCount, _frameCount);
  _lastFrameCount = _frameCount;
}
  
bool RenderWindow::pollEvent(Event& ev)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if( _events.empty() ) {
    return false;
  }
  
  ev = _events.front();
  _events.pop();
  return true;
}
  
void RenderWindow::AddEvent(sf::Event &&ev)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _events.emplace(std::move(ev));
}
  


}
