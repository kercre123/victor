/**
 * File: neuralNetModel_offboard.h
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: Implementation of INeuralNetModel interface class does not actually run forward inference through
 *              a neural network model but instead communicates with an "offboard" process using a
 *              Vision::OffboardProcessor. (Really just a thin wrapper around OffboardProcessor to match
 *              the INeuralNetModel interface, so it can be used by a NeuralNetRunner.)
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_NeuralNets_OffboardModel_H__
#define __Anki_NeuralNets_OffboardModel_H__

#include "coretech/neuralnets/neuralNetModel_interface.h"
#include "coretech/vision/engine/offboardProcessor.h"

namespace Json {
  class Value;
}

namespace Anki {
namespace NeuralNets {
  
class OffboardModel : public INeuralNetModel
{
public:
  
  OffboardModel(const std::string& cachePath);
  
  virtual ~OffboardModel();
  
  virtual Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints) override;
  
  using Filenames = Vision::OffboardProcessor::Filenames;
  using JsonKeys  = Vision::OffboardProcessor::JsonKeys;
  
protected:
  
  virtual Result LoadModelInternal(const std::string& modelPath, const Json::Value& config) override;
  
private:
  
  std::unique_ptr<Vision::OffboardProcessor> _offboardProc;
  
}; // class OffboardModel

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_OffboardModel_H__ */
