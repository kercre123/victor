/**
 * File: faceEvalDetection.h
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: Evaluates the ability to find a face an image, independent of recognition/identification.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Coretech_Vision_FaceDetectionEval_H__
#define __Anki_Coretech_Vision_FaceDetectionEval_H__

#include "coretech/vision/tools/faceEval/faceEvalInterface.h"

namespace Anki {
namespace Vision {
  
class SimpleArgParser;
  
class FaceEvalDetection : public IFaceEval
{
public:
  FaceEvalDetection(const FileList& faceFiles,
                    const FileList& backgroundFiles,
                    const SimpleArgParser& argParser);
  
  virtual void Run() override;
  
private:
  struct TestFile
  {
    std::string filename;
    int         numFaces;
  };
  std::vector<TestFile> _testFiles;
  
  struct Params
  {
    int  numTimesToShowImage = 2;
    float resizeFactor = 0.5f;
    float detectionThreshold = 0.5f;
    int detectionAngle = 45; // up +- this angle, in degrees
    bool verbose = false;
    bool useStillMode = false;
    int delayCount = 0;
    
  } _params;
};

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Coretech_Vision_FaceDetectionEval_H__ */
  
