#include "debayer.h"

#include <array>
#include <cmath>

#include "debayer/raw10.h"
#include "util/logging/logging.h"

#ifdef __ARM_NEON__
#include "debayer/neon/raw10.h"
#endif

namespace Anki {
namespace Vision {

Debayer::Debayer()
{
#ifdef __ARM_NEON__
  // Photo quality
  // NOTE: Photo quality photos on Vector are done on CPU using a lookup table because the performance is better.
  // See the following for details:
  // * VIC-13121 Evaluate GPU Performance
  // * VIC-5225 Gamma correction
  // https://ankiinc.atlassian.net/wiki/spaces/vision/pages/531729526/Debayering
  {
    auto op = std::make_shared<RAW10toRGB24>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::RGB24, op);
  }
  {
    auto op = std::make_shared<RAW10toY8>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::Y8,    op);
  }

  // Perception Quality
  {
    auto op = std::make_shared<Neon::RAW10toRGB24>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::RGB24, op);
  }
  {
    auto op = std::make_shared<Neon::RAW10toY8>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::Y8,    op);
  }
#else
  // RAW10 Bayer to RGB24
  {
    auto op = std::make_shared<RAW10toRGB24>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::RGB24, op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::RGB24, op);
  }
  // RAW10 Bayer to Y8
  {
    auto op = std::make_shared<RAW10toY8>();
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::FULL,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::HALF,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::QUARTER, OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PHOTO,      Scale::EIGHTH,  OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::FULL,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::HALF,    OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::QUARTER, OutputFormat::Y8,    op);
    SetOp(Layout::RAW10, Pattern::RGGB, Method::PERCEPTION, Scale::EIGHTH,  OutputFormat::Y8,    op);
  }
#endif
}

void Debayer::SetOp(Debayer::Key key, std::shared_ptr<Debayer::Op> op)
{
  _ops[key] = op;
}

std::shared_ptr<Debayer::Op> Debayer::GetOp(Debayer::Key key)
{
  auto iter = _ops.find(key);
  if (iter == _ops.end()){
    return nullptr;
  } else {
    return iter->second;
  }
}

void Debayer::RemoveOp(Debayer::Key key)
{
  _ops.erase(key);
}

Result Debayer::Invoke(Debayer::Method method, const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  auto key = Debayer::MakeKey(inArgs.layout, inArgs.pattern, method, outArgs.scale, outArgs.format);
  auto iter = _ops.find(key);
  if (iter == _ops.end())
  {
    // ERROR, couldn't find an operator matching these options 
    LOG_ERROR("Debayer.Operator.NoOperatorFail", "");
    return RESULT_FAIL; 
  }
  if (iter->second == nullptr)
  {
    // ERROR, the operator registered to this key is a nullptr
    LOG_ERROR("Debayer.Operator.OperatorIsNullFail", "");
    return RESULT_FAIL;
  }
  return (*iter->second.get())(inArgs, outArgs);
}

} /* namespace Vision */
} /* namespace Anki */