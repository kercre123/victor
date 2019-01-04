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

#include "clad/types/offboardVision.h"
#include "coretech/neuralnets/neuralNetModel_interface.h"

// Forward declaration
class LocalUdpClient;

namespace Anki {
namespace NeuralNets {

class OffboardModel : public INeuralNetModel
{
public:
  
  explicit OffboardModel(const std::string& cachePath);
  
  virtual ~OffboardModel() = default;
  
  virtual Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints) override;
  
protected:
  
  virtual Result LoadModelInternal(const std::string& modelPath, const Json::Value& config) override;
  
private:
  
  Result DetectWithFileIO(const Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints);
  
  Result DetectWithCLAD(const Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints);
  
  bool WaitForResultFile(const std::string& resultFilename, std::list<Vision::SalientPoint>& salientPoints);
  bool WaitForResultCLAD(std::list<Vision::SalientPoint>& salientPoints);
  
  static Result ParseSalientPointsFromJson(const Json::Value& jsonSalientPoints,
                                           std::list<Vision::SalientPoint>& salientPoints);
  std::string _cachePath;
  int         _pollPeriod_ms;
  bool        _isVerbose = false;
  float       _timeoutDuration_sec = 10.f;
  Vision::OffboardCommsType _commsType = Vision::OffboardCommsType::FileIO;
  
  // For non-FileIO comms
  std::unique_ptr<LocalUdpClient> _udpClient;
  
}; // class Model

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_OffboardModel_H__ */
