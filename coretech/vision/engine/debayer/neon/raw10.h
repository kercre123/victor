/**
 * File: neon/raw10.h
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU using Neon
 *              acceleration.
 *
 * Copyright: Anki, Inc. 2019
 */

#ifdef __ARM_NEON__
#ifndef __Anki_Coretech_Vision_Debayer_Neon_Raw10_H__
#define __Anki_Coretech_Vision_Debayer_Neon_Raw10_H__

#include "coretech/vision/engine/debayer.h"

#include <array>

namespace Anki {
namespace Vision {
namespace Neon {

/**
 * @brief Convert RAW10 Bayer images to RGB24 images using neon and nearest neighbor debayering
 * @details Convert RAW10 Bayer images to RGB24 using neon by ignoring the bottom 3 bits and gamma correcting
 * the remaining 7 bits via a lookup table. This results in data loss of the input images and some flat areas depending
 * on the gamma value. If gamma is greater than 1 (i.e. 1/G < 1), then this results in flatter coloring in dark areas.
 * 
 * Neon lookup table instructions can only use a table of 32 (2^5) values in a single instruction. We can increase 
 * this by looping over blocks of 32 values of the lookup table. By iterating 4 times, we can remap up to 128 input 
 * values (2^7). Beyond this point, lookup tables in neon become inefficient; it is faster to use an array. Computing
 * gamma correction directly using neon instructions is also less efficient than a normal lookup table without neon.
 * 
 * While the images are not the 'prettiest' they could be, they still are sufficient for most perception algorithms.
 * In the case of needing a perception quality image, then losing some information from the least significant bits will
 * not drastically effect the results of most perception algorithms.
 */
class HandleRAW10: public Debayer::Op
{
public:
  HandleRAW10(f32 gamma);
  virtual ~HandleRAW10() = default;
  Result operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const override;
  
private:
  using Key = std::tuple<Debayer::Scale, Debayer::OutputFormat>;
  
  using Function = std::function<Result(const Debayer::InArgs&, Debayer::OutArgs&)>;

  static Key MakeKey(Debayer::Scale scale, Debayer::OutputFormat format)
  {
    return std::make_tuple(scale, format);
  }

  Result RAW10_to_RGB24_FULL(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_RGB24_HALF_or_QUARTER(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_RGB24_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_FULL(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_HALF_or_QUARTER(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;

  std::map<Key,Function> _functions;
  std::array<u8, 128> _gammaLUT;
};

} /* namespace Neon */
} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Coretech_Vision_Debayer_Neon_Raw10_H__ */
#endif /* __ARM_NEON */
