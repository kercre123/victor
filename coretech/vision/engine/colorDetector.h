/**
 * File: colorDetector.h
 *
 * Author: Patrick Doran
 * Date: 2018/11/12
 */

#ifndef __Anki_Vision_ColorDetector_H__
#define __Anki_Vision_ColorDetector_H__

#include "clad/types/salientPointTypes.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/colorClassifier.h"
#include "coretech/vision/engine/colorPixelTypes.h"
#include "engine/debugImageList.h"

#include "json/json.h"

#include <list>
#include <unordered_map>

namespace Anki
{
namespace Vision
{

class ImageRGB;

class ColorDetector
{
public:
  ColorDetector (const Json::Value& config);
  virtual ~ColorDetector ();

  Result Init();
  Result Detect(const ImageRGB& inputImage,
                std::list<SalientPoint>& salientPoints,
                std::list<std::pair<std::string, ImageRGB>>& debugImageRGBs);

private:
  enum class ColorSpace : int {
    INVALID = -1,
    RGB = 0,
    YUV
  };
  static const std::unordered_map<std::string,ColorSpace> FORMAT_STR;
  static ColorSpace StringToColorSpace(const std::string& value);


  // TODO: Decide what we really care about. Label names or colors.
  struct Label
  {
    std::string name;
    PixelRGB color;
  };

  bool Load(const Json::Value& config);

  ColorClassifier _classifier;
  std::vector<Label> _labels;
  ColorDetector::ColorSpace _colorSpace;
  f32 _classifierThreshold;

};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Vision_ColorDetector_H__ */
