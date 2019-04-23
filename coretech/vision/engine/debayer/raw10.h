/**
 * File: raw10.h
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU
 * 
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Coretech_Vision_Debayer_Raw10_H__
#define __Anki_Coretech_Vision_Debayer_Raw10_H__

#include "coretech/vision/engine/debayer.h"
#include <array>

namespace Anki {
namespace Vision {

/**
 * @brief Convert RAW10 RGGB bayer input to RGB24 or Y8 and QUARTER_or_EIGHTH (if requested).
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
  Result RAW10_to_RGB24_HALF(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_RGB24_QUARTER_or_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_FULL(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_HALF(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;
  Result RAW10_to_Y8_QUARTER_or_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const;

  std::map<Key,Function> _functions;
  std::array<u8, 1024> _gammaLUT;
};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Coretech_Vision_Debayer_Raw10_H__ */
