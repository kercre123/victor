/**
 * File: brightColorDetector.h
 *
 * Author: Patrick Doran
 * Date: 08/22/2018
 *
 * Description: Detector for bright colors
 *
 * Copyright: Anki, Inc. 2018
 */
#ifndef __Anki_Vision_BrightColorDetector_H__
#define __Anki_Vision_BrightColorDetector_H__

#include "coretech/common/shared/types.h"
#include "clad/types/salientPointTypes.h"
#include <list>

namespace Anki {
namespace Vision {

class Camera;
class ImageRGB;

class BrightColorDetector
{
public:

  BrightColorDetector (const Camera& camera);
  ~BrightColorDetector ();

  Result Init();
  Result Detect(const ImageRGB& inputImage,
                std::list<SalientPoint>& salientPoints);

private:
  /**
   * @brief Compute colorfulness based on Hasler and Susstrunk, "Measuring colourfulness in natural images"
   * @details Compute the colorfulness core. From "Measuring colourfulness in natural images", the output score
   * is a range of [0,?] with scores having meaning across seven possible thresholds for "colorfulness.
   *
   * Not Colorful - 0
   * Slightly Colorful - 15
   * Moderately Colorful - 33
   * Average Colorful - 45
   * Quite Colorful - 59
   * Highly Colorful - 82
   * Extremely Colorful - 109
   *
   * @param image - input image to compute score for
   * @return Score between [0,?] representing colorfulness
   *
   * @note Requires ANKICORETECH_USE_OPENCV to be on, otherwise it always returns 0.f
   * @todo Ignore region arguments
   */
  float GetScore(const ImageRGB& image);

  const Camera& _camera;

  struct Parameters;
  std::unique_ptr<Parameters> _params;

  class Memory;
  std::unique_ptr<Memory> _memory;
};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Vision_BrightColorDetector_H_ */
