/**
 * File: neuralNetModel_offboard.h
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: Implementation of INeuralNetModel interface class does not actually run forward inference through
 *              a neural network model but instead communicates with an "offboard" process via file I/O.
 *              This eventually could be a local laptop, the cloud, or simply another process on the same device.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_NeuralNets_OffboardModel_H__
#define __Anki_NeuralNets_OffboardModel_H__

#include "coretech/neuralnets/neuralNetModel_interface.h"

namespace Anki {
namespace NeuralNets {

class OffboardModel : public INeuralNetModel
{
public:
  
  explicit OffboardModel(const std::string& cachePath);
  
  virtual ~OffboardModel() = default;
  
  virtual Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints) override;
  
  bool IsVerbose() const { return _isVerbose; }
  
protected:
  
  virtual Result LoadModelInternal(const std::string& modelPath, const Json::Value& config) override;
  
private:
  
  std::string _cachePath;
  int         _pollPeriod_ms;
  bool        _isVerbose = false;
  float       _timeoutDuration_sec = 10.f;
  
}; // class Model

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_OffboardModel_H__ */
