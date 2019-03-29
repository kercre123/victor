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

namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {

// Someday we may want this to be an enumerated type
using OffboardProcType = std::string;

class OffboardProcessor
{
public:
  
  OffboardProcessor(const std::string& cachePath);
  
  virtual ~OffboardProcessor() = default;
  
  Result Init(const std::string& name,
              const OffboardCommsType commsType,
              const std::vector<OffboardProcType>& procTypes,
              const int pollingPeriod_ms,
              const float timeoutDuration_sec);
  
  // Blocking call! Use an IAsyncRunner if you want asynchronous running
  Result Detect(ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints);
  
private:
  
  Result DetectWithFileIO(const ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
  Result DetectWithCLAD(const ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
  bool WaitForResultFile(const std::string& resultFilename, std::list<SalientPoint>& salientPoints);
  bool WaitForResultCLAD(std::list<SalientPoint>& salientPoints);
  
  std::string _name;
  std::string _cachePath;
  int         _pollPeriod_ms;
  float       _timeoutDuration_sec = 10.f;
  TimeStamp_t _imageTimestamp = 0;
  s32         _imageRows = 0;
  s32         _imageCols = 0;
  
  OffboardCommsType _commsType = OffboardCommsType::FileIO;
  std::vector<OffboardProcType> _procTypes;
  
  // For non-FileIO comms
  std::unique_ptr<LocalUdpClient> _udpClient;
  
  bool Connect();
  bool IsConnected() const;
  
}; // class OffboardProcessor
  
} // namespace Vision
  
namespace NeuralNets {
  
class OffboardModel : public INeuralNetModel
{
public:
  
  OffboardModel(const std::string& cachePath);
  
  virtual ~OffboardModel();
  
  virtual Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints) override;
  
protected:
  
  virtual Result LoadModelInternal(const std::string& modelPath, const Json::Value& config) override;
  
private:
  
  std::unique_ptr<Vision::OffboardProcessor> _offboardProc;
  
}; // class OffboardModel

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_OffboardModel_H__ */
