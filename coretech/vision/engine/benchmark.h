/**
 * File: benchmark.h
 *
 * Author: Andrew Stein
 * Date:   11-30-2017
 *
 * Description: Vision system component for benchmarking operations. Add new methods as needed to 
 *              compare performance of various ways of implementing operations on images (e.g. using OpenCV, 
 *              directly looping over the pixels, using a lookup table, etc. Each method is tied to a "Mode" 
 *              which can be enabled/disabled via console vars as well.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/


#ifndef __Anki_Vision_Benchmark_H__
#define __Anki_Vision_Benchmark_H__

#include "coretech/common/shared/types.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "util/bitFlags/bitFlags.h"

#include <vector>

namespace Anki {
namespace Vision {
  
// Forward declarations:
class ImageCache;

class Benchmark : Profiler
{
public:
  
  Benchmark();
  
  // There should be one Mode for each method below
  // We use a helper macro internally, so names should match, with Mode prefixed by "Do"
  enum class Mode : u8 {     // Increase storage type as needed
    DoScalarSubtraction = 1, // First mode must be "1"
    DoSquaredDiffFromScalar,
    DoNormalizeRGB,
    DoDiffWithPrevious,
    DoRatioWithPrevious,
  };
  
  void EnableMode(Mode mode, bool enable);
  
  Result Update(ImageCache& imageCache);
  
private:
  
  Result ScalarSubtraction(const ImageRGB& image);
  
  Result SquaredDiffFromScalar(const ImageRGB& image);
  
  Result NormalizeRGB(const ImageRGB& image);
  
  Result DiffWithPrevious(const ImageRGB& image);
  
  Result RatioWithPrevious(const ImageRGB& image);

  using ModeType = std::underlying_type<Mode>::type;
  Util::BitFlags<ModeType, Mode> _enabledModes;
  
  ImageRGB _prevImage;
  
  std::vector<std::vector<u8>> _normLUT;
  void CreateNormalizeLUT();
  
  std::vector<std::vector<f32>> _ratioLUT;
  void CreateRatioLUT();
};

  
inline void Benchmark::EnableMode(Benchmark::Mode mode, bool enable) {
  _enabledModes.SetBitFlag(mode, enable);
}
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_Benchmark_H__ */
