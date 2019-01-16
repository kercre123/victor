/**
 * File: visionModeDependencies.h
 *
 * Author: Andrew Stein
 * Created: 01/14/2019
 *
 * Description: Captures vision mode dependencies to determine when one should run.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Anki_Vector_VisionModeDependencies_H__
#define __Anki_Vector_VisionModeDependencies_H__

#include "clad/types/visionModes.h"
#include "coretech/common/shared/types.h"

#include <map>

namespace Anki {
namespace Vector {

// Forward declarations
class VisionModeSet;
struct VisionProcessingResult;

class VisionModeDependencies
{
public:
  
  VisionModeDependencies();
  
  // Checks if the given vision mode wants to run based on what's already in the given processing result
  //  and what vision modes are enabled
  // If true is returned, the TimeStamp_t argument indicates the timestamp of the image that should be used
  bool ShouldRun(const VisionMode& forMode,
                 const VisionProcessingResult& result,
                 const VisionModeSet& visionModesEnabled,
                 TimeStamp_t& t) const;
  
private:
  
  using ShouldRunFcn = std::function<bool(const VisionProcessingResult&,
                                          const VisionModeSet& visionModesEnabled,
                                          TimeStamp_t&)>;
  
  static bool ShouldRunOffboardPersonDetection(const VisionProcessingResult& result,
                                               const VisionModeSet& visionModesEnabled,
                                               TimeStamp_t& usingTimestamp);
  
  static bool ShouldRunPersonClassification(const VisionProcessingResult& result,
                                            const VisionModeSet& visionModesEnabled,
                                            TimeStamp_t& usingTimestamp);
  
  void RegisterShouldRunFunction(const VisionMode& forMode, const ShouldRunFcn& shouldRunFcn);
  
  std::map<VisionMode, ShouldRunFcn> _shouldRunMap;
  
};

} // namespace Vector
} // namespace Anki

#endif /* __Anki_Vector_VisionModeDependencies_H__ */
