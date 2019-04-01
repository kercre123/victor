/**
 * File: neon/raw10.h
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU using Neon
 *              acceleration.
 * 
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

class RAW10toRGB24: public Debayer::Op
{
public:
  RAW10toRGB24();
  virtual ~RAW10toRGB24() = default;
  Result operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const override;
  
private:
  std::array<u8, 128> _gammaLUT;
};

class RAW10toY8: public Debayer::Op
{
public:
  RAW10toY8();
  virtual ~RAW10toY8() = default;
  Result operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const override;
  
private:
  std::array<u8, 128> _gammaLUT;
};

} /* namespace Neon */
} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Coretech_Vision_Debayer_Neon_Raw10_H__ */
#endif /* __ARM_NEON */