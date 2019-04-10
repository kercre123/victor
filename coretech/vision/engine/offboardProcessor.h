/**
 * File: offboardProcessor.h
 *
 * Author: Andrew Stein
 * Date:   4/1/2019
 *
 * Description: Helper class for processing images "offboard", generally meaning "not on the robot": in the cloud,
 *              on a laptop, etc. The FileIO mode also allows simply processing with a separate process on the robot
 *              as well.
 *
 *              Note that this processor is synchronous / blocking. If you additionally want asynchronous behavior
 *              use in conjunction with an IAsyncRunner.
 *
 * Copyright: Anki, Inc. 2019
 **/

#ifndef __Anki_Vision_OffboardProcessor_H__
#define __Anki_Vision_OffboardProcessor_H__

#include "clad/types/offboardVision.h"
#include "clad/types/salientPointTypes.h"

#include "coretech/common/shared/types.h"

#include <list>
#include <memory>
#include <vector>

// Forward declaration
class LocalUdpClient;

namespace Anki {
namespace Vision {

// Forward declaration
class ImageRGB;
  
class OffboardProcessor
{
public:
  
  explicit OffboardProcessor(const std::string& cachePath);
  
  ~OffboardProcessor();
  
  // Someday we may want this to be an enumerated type
  using ProcType = std::string;
  
  Result Init(const std::string& name, const Json::Value& config);
  
  // Blocking call! Use an IAsyncRunner if you want asynchronous running
  Result Detect(ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
  // Use these instead of hard-coded strings to refer to the underlying files and
  // Json keys used by an OffboardProcessor
  struct Filenames {
    static const char* const Timestamp;
    static const char* const Image;
    static const char* const Result;
  };
  
  struct JsonKeys {
    static const char* const CommsType;
    static const char* const ProcType;
    static const char* const TimeoutDuration;
    static const char* const PollingPeriod;
  };
  
private:
  
  Result DetectWithFileIO(const ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
  Result DetectWithCLAD(const ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
  bool WaitForResultFile(const std::string& resultFilename, std::list<SalientPoint>& salientPoints);
  bool WaitForResultCLAD(std::list<SalientPoint>& salientPoints);
  
  struct
  {
    std::string                    name;
    OffboardCommsType              commsType;
    std::vector<ProcType>          procTypes;
    int                            pollingPeriod_ms;
    float                          timeoutDuration_sec;
  } _params;
  
  std::string _cachePath;
  TimeStamp_t _imageTimestamp = 0;
  s32         _imageRows = 0;
  s32         _imageCols = 0;
  
  // For non-FileIO comms
  std::unique_ptr<LocalUdpClient> _udpClient;
  
  bool Connect();
  bool IsConnected() const;
  
}; // class OffboardProcessor

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_OffboardProcessor_H__ */
