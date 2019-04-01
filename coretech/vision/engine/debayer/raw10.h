/**
 * File: raw10.h
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU without 
 *              acceleration.
 * 
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Coretech_Vision_Debayer_Raw10_H__
#define __Anki_Coretech_Vision_Debayer_Raw10_H__

#include "coretech/vision/engine/debayer.h"
#include <array>

namespace Anki {
namespace Vision {

class RAW10toRGB24: public Debayer::Op
{
public:
  RAW10toRGB24();
  virtual ~RAW10toRGB24() = default;
  Result operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const override;
  
private:
  std::array<u8, 1024> _gammaLUT;
};

class RAW10toY8: public Debayer::Op
{
public:
  RAW10toY8();
  virtual ~RAW10toY8() = default;
  Result operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const override;
  
private:
  std::array<u8, 1024> _gammaLUT;
};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Coretech_Vision_Debayer_Raw10_H__ */