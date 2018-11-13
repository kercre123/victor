/**
 * File: colorClassifier.h
 *
 * Author: Patrick Doran
 * Date: 11/22/2018
 */

#ifndef __Anki_Vision_ColorClassifier_H__
#define __Anki_Vision_ColorClassifier_H__

#include <string>
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/colorPixelTypes.h"

namespace Anki
{
namespace Vision
{

class ImageRGB;
class Image;

class ColorClassifier
{
public:
  struct Label
  {
    std::string label;
    PixelRGB color;
  };

  static const Label UNKNOWN_LABEL;
  static const u8 UNKNOWN_INDEX = -1; // u8 max value

  ColorClassifier ();
  ~ColorClassifier ();

  void Classify(const ImageRGB& image, Image& labels) const;
  void ColorImage(const Image& labels, ImageRGB& image) const;
  Label GetLabel(u8 index) const;
  inline std::vector<Label> GetLabels() const { return _labels; }

  void Train(const std::string& directory);
  void Save(const std::string& path) const;
  void Load(const std::string& path);

private:
  u8 Classify(PixelRGB color) const;
  void Reset();

  std::vector<Label> _labels;
  bool _trained;
};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Vision_ColorClassifier_H__ */
