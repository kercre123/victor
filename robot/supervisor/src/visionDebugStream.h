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
        
        Result Initialize();
        
        Result SendBenchmarksOnly(MemoryStack scratch);
        
        Result SendFiducialDetection(const Array<u8> &image,
                                         const FixedLengthList<VisionMarker> &markers,
                                         MemoryStack ccmScratch,
                                         MemoryStack onchipScratch,
                                         MemoryStack offchipScratch);
        
        Result SendTrackingUpdate(const Array<u8> &image,
                                      const Tracker &tracker,
                                      const TrackerParameters &parameters,
                                      const u8 meanGrayvalueError,
                                      const f32 percentMatchingGrayvalues,
                                      MemoryStack ccmScratch,
                                      MemoryStack onchipScratch,
                                      MemoryStack offchipScratch);
                                      
        Result SendFaceDetections(
          const Array<u8> &image, 
          const FixedLengthList<Rectangle<s32> > &detectedFaces,
          const s32 detectedFacesImageWidth,
          MemoryStack ccmScratch, 
          MemoryStack onchipScratch, 
          MemoryStack offchipScratch)                                      ;
        
        //Result SendPrintf(const char * string);
        
        Result SendArray(const Array<u8> &array, const char * objectName);
          
        Result SendImage(const Array<u8> &array, const f32 exposureTime, const char * objectName, MemoryStack scratch);
        
        Result SendBinaryImage(const Array<u8> &grayscaleImage,
                               const char * objectName,
                                   const Tracker &tracker,
                                   const TrackerParameters &parameters,
                                   MemoryStack ccmScratch,
                                   MemoryStack onchipScratch,
                                   MemoryStack offchipScratch);
        
#if DOCKING_ALGORITHM ==  DOCKING_BINARY_TRACKER
        Result SendBinaryTracker(const TemplateTracker::BinaryTracker &tracker,
                                     MemoryStack ccmScratch,
                                     MemoryStack onchipScratch,
                                     MemoryStack offchipScratch);
#endif
        
      } // namespace DebugStream
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_VISIONDEBUGSTREAM_H
