#ifndef ANKI_VISIONDEBUGSTREAM_H
#define ANKI_VISIONDEBUGSTREAM_H

#include "anki/common/types.h"
#include "anki/common/robot/array2d_declarations.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include "visionParameters.h"

namespace Anki {
  
  using namespace Embedded;
  
  namespace Cozmo {
    namespace VisionSystem {
      namespace DebugStream {
        
        ReturnCode Initialize();
        
        ReturnCode SendFiducialDetection(const Array<u8> &image,
                                         const FixedLengthList<VisionMarker> &markers,
                                         MemoryStack ccmScratch,
                                         MemoryStack onchipScratch,
                                         MemoryStack offchipScratch);
        
        ReturnCode SendTrackingUpdate(const Array<u8> &image,
                                      const Tracker &tracker,
                                      const TrackerParameters &parameters,
                                      const u8 meanGrayvalueError,
                                      const f32 percentMatchingGrayvalues,
                                      MemoryStack ccmScratch,
                                      MemoryStack onchipScratch,
                                      MemoryStack offchipScratch);
        
        //ReturnCode SendPrintf(const char * string);
        
        ReturnCode SendArray(const Array<u8> &array);
        
        ReturnCode SendBinaryImage(const Array<u8> &grayscaleImage,
                                   const Tracker &tracker,
                                   const TrackerParameters &parameters,
                                   MemoryStack ccmScratch,
                                   MemoryStack onchipScratch,
                                   MemoryStack offchipScratch);
        
#if DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
        ReturnCode SendBinaryTracker(const TemplateTracker::BinaryTracker &tracker,
                                     MemoryStack ccmScratch,
                                     MemoryStack onchipScratch,
                                     MemoryStack offchipScratch);
#endif
        
      } // namespace DebugStream
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_VISIONDEBUGSTREAM_H
