/**
 * File: neuralNetModel_offboard.cpp
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: See header file.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/jsonTools.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"

#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace NeuralNets {
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffboardModel::OffboardModel(const std::string& cachePath)
: _offboardProc(new Vision::OffboardProcessor(cachePath))
{
  
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffboardModel::~OffboardModel()
{
  
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::LoadModelInternal(const std::string& modelPath, const Json::Value& config)
{
  return _offboardProc->Init(GetName(), config);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  return _offboardProc->Detect(img, salientPoints);
}
  
} // namespace NeuralNets
  
} // namespace Anki

