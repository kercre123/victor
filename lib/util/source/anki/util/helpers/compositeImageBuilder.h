/**
 * File: compositeImageHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 1/31/2018
 *
 * Description: Helper which builds images by compositing sub images together
 * according to a JSON defined template
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Util_CompositeImageHelper_H__
#define __Util_CompositeImageHelper_H__

#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/image_impl.h"


namespace Anki {
namespace Util {

/*                                             - HOW TO USE -
*
*  This helper class builds a composite image by taking an arbitrary template specification
*  of the format specified below and using it to paste images into named quadrants of the final
*  returned image.
*
*  templateSpec should be an array of quadrant specifications taking the following form:
*    quadrantName : string - the name of the quadrant
*    x            : int - the x coordinate of the top left coordinate to paste the image into
*    y            : int - the y coordinate of the top left coordinate to paste the image into
*    width        : int - the expected width of the image to be pasted in
*    height       : int - the expected height of the sub image

*  implementationSpec should be an array of implementation fields taking the following form:
*    quadrantName : string - the quadrant to paste the image into
*    imagePath    : string - the file path to load the image in from

*  Errors will be printed if an attempt is made to load a sub image of improper dimensions
*
*
*                                             - IMPLEMENTATION DETAILS -
*  Note: Templates and implementation JSONS are only validated when the build function is called
*  if we want to ensure load time failures rather than runtime failures a seperate validation function
*  or stage should be intreduced
*/

namespace CompositeImageBuilderKeys{
static const char* kQuadrantNameKey = "quadrantName";
static const char* kImagePathKey    = "imagePath";
static const char* kCornerXKey      = "x";
static const char* kCornerYKey      = "y";
static const char* kWidthKey        = "width";
static const char* kHeightKey       = "height";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
class CompositeImageBuilder{
public: 
  struct TemplateEntry;
  struct ImplementationEntry;
  
  static void BuildCompositeImage(const Json::Value& templateSpec, 
                                  const Json::Value& implementationSpec,
                                  ImageType& outImage,
                                  bool requireAllArguments = true);

  static void BuildCompositeImage(std::vector<TemplateEntry>& templateSpec,
                                  std::map<std::string, ImplementationEntry>& implementationSpec,
                                  ImageType& outImage,
                                  bool requireAllArguments = true);
private:
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
struct CompositeImageBuilder<ImageType>::TemplateEntry{
  TemplateEntry(const std::string& quadrantName,
                int x, int y,
                int width, int height)
  : _quadrantName(quadrantName)
  , _x(x)
  , _y(y)
  , _width(width)
  , _height(height){}
  const std::string _quadrantName;
  const int _x;
  const int _y;
  const int _width;
  const int _height;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
struct CompositeImageBuilder<ImageType>::ImplementationEntry{
  ImplementationEntry(const std::string& imagePath)
  {
    ANKI_VERIFY(RESULT_OK == _image.Load(imagePath),
                "CompositeImageBuilder.ImplementationEntry.Constructor.LoadFailed",
                "Failed to load image at path %s",
                imagePath.c_str());
  }
  ImplementationEntry(const ImageType& image)
  {
    image.CopyTo(_image);
  }
  ImageType _image;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
void CompositeImageBuilder<ImageType>::BuildCompositeImage(const Json::Value& templateSpec, 
                                                           const Json::Value& implementationSpec,
                                                           ImageType& outImage,
                                                           bool requireAllArguments)
{
  using namespace CompositeImageBuilderKeys;

  std::vector<TemplateEntry> templateVector;
  std::map<std::string, ImplementationEntry> implementationMap;
  const std::string templateDebugStr = "CompositeImageBuilder.BuildCompositeImage.TemplateKey";
  for(auto& entry: templateSpec){
    const std::string qName = JsonTools::ParseString(entry, kQuadrantNameKey, templateDebugStr);
    const int x = JsonTools::ParseUint8(entry, kCornerXKey, templateDebugStr);
    const int y = JsonTools::ParseUint8(entry, kCornerYKey, templateDebugStr);
    const int width = JsonTools::ParseUint8(entry, kWidthKey, templateDebugStr);
    const int height = JsonTools::ParseUint8(entry, kHeightKey, templateDebugStr);
    templateVector.emplace_back(TemplateEntry(qName, x, y, width, height));
  }

  const std::string implDebugStr = "CompositeImageBuilder.BuildCompositeImage.SpecKey";
  for(auto& entry: implementationSpec){
    const std::string qName     = JsonTools::ParseString(entry, kQuadrantNameKey, implDebugStr);
    const std::string imagePath = JsonTools::ParseString(entry, kImagePathKey, implDebugStr);
    implementationMap.insert(std::make_pair(qName, ImplementationEntry(imagePath)));
  }

  CompositeImageBuilder<ImageType>::BuildCompositeImage(templateVector,
                                                        implementationMap,
                                                        outImage);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename ImageType>
void CompositeImageBuilder<ImageType>::BuildCompositeImage(std::vector<TemplateEntry>& templateSpec,
                                                           std::map<std::string, ImplementationEntry>& implementationSpec,
                                                           ImageType& outImage,
                                                           bool requireAllArguments)
{
  ANKI_VERIFY(!requireAllArguments ||
              (templateSpec.size() == implementationSpec.size()),
              "CompositeImageBuilder.BuildCompositeImage.SizeMismatch",
              "");
  for(auto& entry : templateSpec){
    const std::string& qName = entry._quadrantName;
    auto iter = implementationSpec.find(qName);
    if(iter == implementationSpec.end()){
      if(requireAllArguments){
        PRINT_NAMED_ERROR("CompositeImageBuilder.BuildCompositeImage.SpecMissingQuadrant",
                          "Quadrant name %s not found in implementation spec",
                          qName.c_str());
      }
      continue;
    }
    
    ImageType& subImage = iter->second._image;
    Point2f topCorner(entry._x,entry._y);
    outImage.DrawSubImage(subImage, topCorner);

    // dev only verification that image size is as expected
    ANKI_VERIFY(entry._width == subImage.GetNumCols(), 
                "CompositeImageBuilder.BuildCompositeImage.InvalidWidth",
                "Quadrant Name:%s Expected Width:%d, Image Width:%d",
                qName.c_str(), entry._width, subImage.GetNumCols());
    ANKI_VERIFY(entry._height == subImage.GetNumRows(), 
                "CompositeImageBuilder.BuildCompositeImage.InvalidHeight",
                "Quadrant Name:%s Expected Height:%d, Image Height:%d",
                qName.c_str(), entry._height, subImage.GetNumRows());
  }
}


}; // namespace Cozmo
}; // namespace Anki

#endif // __Util_CompositeImageHelper_H__
