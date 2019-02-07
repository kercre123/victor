/**
 * File: faceEvalInterface.h
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: Provides some common functionality for various Face Evaluation tests, including shuffling and display.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Coretech_Vision_FaceEvalInterface_H__
#define __Anki_Coretech_Vision_FaceEvalInterface_H__

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"

#include "util/bitFlags/bitFlags.h"

#include <list>
#include <vector>

namespace Anki {
namespace Vision {

class FaceTracker;
class SimpleArgParser;
  
class IFaceEval
{
public:
  using FileList = std::vector<std::string>;
  
  virtual void Run() = 0;
  
  enum class FailureType : uint8_t
  {
    FalsePositive,
    FalseNegative,
    MissingParts,
  };
  using FailureFlags = Util::BitFlags8<FailureType>;
  
  // Does nothing if display is not enabled
  void DrawFaces(const Image& img, const std::list<TrackedFace>& faces,
                 const FailureFlags& failureFlags,
                 const std::string& displayStr = "",
                 int pauseTime_ms = 0,
                 const char* windowName = "Faces",
                 const std::string& saveFilename = "");
  
  enum class DisplayMode : uint8_t
  {
    Off,
    All,
    AnyFailure,
    NoFailure, // Only display success
    FalsePositives,
    FalseNegatives,
    MissingParts,
  };
  
  DisplayMode GetDisplayMode() const { return _params.enableDisplay; }
  
protected:
  
  IFaceEval(const SimpleArgParser& argParser);
  virtual ~IFaceEval();
  
  FaceTracker& GetFaceTracker();
  
  static void DrawFaceOnImage(const TrackedFace& face, Vision::ImageRGB& img);
  
  const std::string& GetOutputDir() const { return _params.outputDir; }
  
  void ShuffleHelper(std::vector<std::string>& strList);
  
private:
  
  struct {
    DisplayMode enableDisplay = DisplayMode::Off;
    f32 resizeFactor = 0.5f;
    std::string outputDir = ".";
    int randomSeed = -1;
  } _params;
  
  Camera _camera;
  std::unique_ptr<FaceTracker> _faceTracker;
  ImageRGB _dispImg;
  
};
    
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Coretech_Vision_FaceEvalInterface_H__ */
