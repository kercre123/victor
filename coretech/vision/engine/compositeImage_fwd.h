/**
* File: compositeImage_fwd.h
*
* Author: Kevin M. Karol
* Created: 1/31/2018
*
* Description: Defines the building blocks of CompositeImages which
* are images built from a bunch of sub images loaded into "quadrants" within the composite image
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_CompositeImage_fwd_H__
#define __Vision_CompositeImage_fwd_H__

#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/image_impl.h"


namespace Anki {
namespace Vision {

/*                                             - HOW TO USE -
*
*  This class defines the components of a composite image
*  Composite images are built of named quadrants which have a specified size and position
*  within a larger image 

*  CompositeImageLayout defines quadrant names and how they're laid out within a composite image
*  It can be defined from JSON as an array of quadrant specifications with the following fields:
*    quadrantName : string - the name of the quadrant
*    x            : int - the x coordinate of the top left coordinate to paste the image into
*    y            : int - the y coordinate of the top left coordinate to paste the image into
*    width        : int - the expected width of the image to be pasted in
*    height       : int - the expected height of the sub image
*
*  The 0,0 coordinate of an image is the top left corner:
*    - positive x is right when facing the image
*    - positive y is down
*  
*  CompositeImageImpl maps quadrant names to the images that should be loaded into them
*  This allows a one to many relationship between Layouts and Impls
*  It can be defined from JSON as an array of quadrant specifications with the following fields:
*    quadrantName : string - the name of the quadrant to place the image inside of
*    imagePath    : string - the full path to the image to load into the quadrant 
*
*/

namespace CompositeImageBuilderKeys{
static const char* kQuadrantNameKey = "quadrantName";
static const char* kImagePathKey    = "imagePath";
static const char* kCornerXKey      = "x";
static const char* kCornerYKey      = "y";
static const char* kWidthKey        = "width";
static const char* kHeightKey       = "height";
}

// forward declaration
class CompositeImageImpl;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImageLayout{
public:
  struct QuadrantDefinition;
  CompositeImageLayout(const Json::Value& templateSpec);
  CompositeImageLayout(std::vector<QuadrantDefinition>&& templateSpec)
  : _quadrants(std::move(templateSpec)){}

  // Checks imlementation to ensure all quadrants are valid for the current template
  // when requireAllQuadrants is true function returns false if implementation doesn't contain
  // all quadrants defined by the template
  bool IsValidImplementation(const CompositeImageImpl& impl, bool requireAllQudrants = false);

  const std::vector<QuadrantDefinition>& GetLayout() const { return _quadrants;}

private:
  std::vector<QuadrantDefinition> _quadrants;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImageImpl{
public:
  struct QuadrantImpl;
  CompositeImageImpl(const Json::Value& implSpec);
  CompositeImageImpl(std::vector<QuadrantImpl>&& implSpec)
  : _quadrants(std::move(implSpec)){}

  const std::vector<QuadrantImpl>& GetImplementation() const { return _quadrants;}
  
private:
  std::vector<QuadrantImpl> _quadrants;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct CompositeImageLayout::QuadrantDefinition{
  QuadrantDefinition(const std::string& quadrantName,
                     const Point2i& topLeftCorner, 
                     int width, int height)
  : quadrantName(quadrantName)
  , topLeftCorner(topLeftCorner)
  , width(width)
  , height(height){}

  const std::string quadrantName;
  Point2i           topLeftCorner;
  const int         width;
  const int         height;
};




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct CompositeImageImpl::QuadrantImpl{
  // If image is greyscale it will be converted to RGBA using greyscale for the alpha channel
  QuadrantImpl(const std::string& quadrantName, const std::string& imagePath, bool isGreyscale);
  QuadrantImpl(const std::string& quadrantName, const ImageRGBA& inImage)
  : quadrantName(quadrantName)
  {
    inImage.CopyTo(image);
  }

  std::string quadrantName;
  ImageRGBA   image;
};


}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImage_fwd_H__
