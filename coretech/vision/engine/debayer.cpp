/**
 * File: debayer.cpp
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering functor definition. The architecture here is a common "Debayer" functor class that can have
 *              several different debayer operators registered to it via "Keys". These keys are just the set of options
 *              that a user may choose to create the different types of output. The interface is agnostic to image type
 *              because the interface takes an pointer to bytes as the "image". The interface only cares about the 
 *              bytes it's given, how they're laid out, and what the output options are.
 * 
 *              The goal with this approach is that we can easily separate out the different types of debayering that
 *              are available/implemented on a platform and then quickly be able to switch between them via the
 *              different options.
 * 
 * See Also: platform/debayer - it contains a regression tool that can create debayering output images as well as
 *           test to see if anything is changed. The stored images are the bytes that came from the existing debayering
 *           functions and this list can be extended if there are more options supported later.
 *
 * Copyright: Anki, Inc. 2019
 */

#include "debayer.h"

#include <array>
#include <cmath>

#include "util/logging/logging.h"

#include "debayer/raw10.h"

#ifdef __ARM_NEON__
#include "debayer/neon/raw10.h"
#endif

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {

Debayer& Debayer::Instance()
{
  static Debayer s_DebayerInstance;
  return s_DebayerInstance;
}

Debayer::Debayer()
: _gamma(1.0f)
{
  _ops = Debayer::GetDefaultOpMap(_gamma);
}

void Debayer::SetGamma(f32 gamma)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (!Util::IsFltNear(_gamma,gamma)){
    _gamma = gamma;
    _ops = Debayer::GetDefaultOpMap(_gamma);
  }
}

void Debayer::SetOp(Debayer::Key key, std::shared_ptr<Debayer::Op> op)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _ops[key] = op;
}

std::shared_ptr<Debayer::Op> Debayer::GetOp(Debayer::Key key)
{
  std::lock_guard<std::mutex> lock(_mutex);
  auto iter = _ops.find(key);
  if (iter == _ops.end()){
    return nullptr;
  } else {
    return iter->second;
  }
}

void Debayer::RemoveOp(Debayer::Key key)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _ops.erase(key);
}

Result Debayer::Invoke(Debayer::Method method, const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  std::lock_guard<std::mutex> lock(_mutex);

  auto key = Debayer::MakeKey(inArgs.layout, inArgs.pattern, method, outArgs.scale, outArgs.format);
  auto iter = _ops.find(key);
  if (iter == _ops.end())
  {
    // ERROR, couldn't find an operator matching these options 
    LOG_ERROR("Debayer.Invoke.NoOpFail", "");
    return RESULT_FAIL; 
  }
  if (iter->second == nullptr)
  {
    // ERROR, the operator registered to this key is a nullptr
    LOG_ERROR("Debayer.Invoke.OpIsNullFail", "");
    return RESULT_FAIL;
  }
  return (*iter->second.get())(inArgs, outArgs);
}

Debayer::OpMap Debayer::GetDefaultOpMap(f32 gamma)
{
  // x^(1/G), so we don't have to compute the division each time, classes should be configured to just do powf(x,G)
  gamma = (Util::IsFltNear(gamma,0.0f))? 1.0f : 1.0f/gamma;
  OpMap ops;
#ifdef __ARM_NEON__
  // Photo quality
  // NOTE: Photo quality photos on Vector are done on CPU using a lookup table because the performance is better.
  // See the following for details:
  // * VIC-13121 Evaluate GPU Performance
  // * VIC-5225 Gamma correction
  // https://ankiinc.atlassian.net/wiki/spaces/vision/pages/531729526/Debayering
  // NOTE: The performance of photo and perception quality are not referring to the implementations here. The reality
  // is that they are now slower for downscaled values. The reason is that the first implementation of downscaling 
  // actually skipped whole blocks of data which created a lot of jagged edges. We suspected that the jaggies would
  // have a negative effect on marker detection. As such, the algorithms were rewritten to do better scaling. It's still
  // nearest neighbor, but rather than take full advantage of loading and using whole blocks of RAW10 data (and using 
  // neon SIMD where possible), we actually have to load more blocks and to achieve the sampling we want. The truth is 
  // that it does make for smoother lines, but it takes longer. Thus the gains of Neon over CPU are not as signficant.
  // The most expensive part for Neon continues to be the Gamma Correction LUT.
  {
    auto op = std::make_shared<HandleRAW10>(gamma);
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::Y8)]    = op;
  }

  // Perception Quality
  {
    auto op = std::make_shared<Neon::HandleRAW10>(gamma);
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::Y8)]    = op;
  }
#else
  // All RAW10 Bayer
  {
    auto op = std::make_shared<HandleRAW10>(gamma);
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::RGB24)] = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::Y8)]    = op;
    ops[Debayer::MakeKey(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::Y8)]    = op;
  }
#endif
  return ops;
}

bool Debayer::SampleRateFromScale(Scale scale, u8& sampleRate)
{
  {
    switch (scale)
    {
    case Debayer::Scale::FULL: { sampleRate = 1; break; }
    case Debayer::Scale::HALF: { sampleRate = 2; break; }
    case Debayer::Scale::QUARTER: { sampleRate = 4; break; }
    case Debayer::Scale::EIGHTH: { sampleRate = 8; break; }
    default:
      LOG_ERROR("Debayer.SampleRateFromScale.UnknownScale","");
      return false;
    }
    return true;
  }
}



} /* namespace Vision */
} /* namespace Anki */
