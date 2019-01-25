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
  
  Result ParseSalientPointsFromJson(const Json::Value& jsonSalientPoints,
                                    const Vision::OffboardProcType procType,
                                    std::list<Vision::SalientPoint>& salientPoints);
  
  Result ParseSceneDescriptionFromJson(const Json::Value& jsonSalientPoints,
                                       std::list<Vision::SalientPoint>& salientPoints);
  
  Result ParseObjectDetectionsFromJson(const Json::Value& jsonSalientPoints,
                                       std::list<Vision::SalientPoint>& salientPoints);
  
  Result ParseFaceDataFromJson(const Json::Value& jsonSalientPoints,
                               std::list<Vision::SalientPoint>& salientPoints);
  
  Result ParseTextDetectionsFromJson(const Json::Value& detectionResult,
                                     std::list<Vision::SalientPoint>& salientPoints);
  
  std::string _cachePath;
  int         _pollPeriod_ms;
  bool        _isVerbose = false;
  float       _timeoutDuration_sec = 10.f;
  TimeStamp_t _imageTimestamp = 0;
  s32         _imageRows = 0;
  s32         _imageCols = 0;
  
  Vision::OffboardCommsType _commsType = Vision::OffboardCommsType::FileIO;
  std::vector<Vision::OffboardProcType> _procTypes;
  
  // For non-FileIO comms
  std::unique_ptr<LocalUdpClient> _udpClient;
  
  bool Connect();
  bool IsConnected() const;
  
}; // class Model

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_OffboardModel_H__ */
