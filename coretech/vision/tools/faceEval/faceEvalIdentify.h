/**
 * File: faceEvalIdentify.h
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: Evaluates face "identification": Some number of faces are pre-enrolled and a new face image
 *              is presented in order to see if it is one of the enrolled set.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Coretech_Vision_FaceEvalIdentify_H__
#define __Anki_Coretech_Vision_FaceEvalIdentify_H__

#include "coretech/vision/tools/faceEval/faceEvalInterface.h"

#include <set>

namespace Anki {
namespace Vision {

class SimpleArgParser;

class FaceEvalIdentify : public IFaceEval
{
public:
  FaceEvalIdentify(const std::list<std::string>& directories,
                   const SimpleArgParser& argParser);
  
  virtual void Run() override;
  
private:
  
  std::vector<std::string> _dirList;
  std::list<std::pair<std::string, int>> _testFileList;
  std::string _outSuffix;
  
  bool ReadEnrollList(const std::string& dir, std::set<std::string>& filesToEnroll);
  int EnrollFace(const std::set<std::string>& enrollFiles, const int faceID);
  void RunInternal();
  void TestIdentification();
  
  struct {
    float resizeFactor = 0.5f;
    float recognitionThreshold = 0.575f;
    float recognitionThreshold2 = 0.5f;
    float recognitionMargin = 0.1f;
    int maxEnrollees = -1;
    int maxEnrollmentsPerPerson = 5;
    int numRuns = 1;
    bool saveImages = false;
    
    //float minEyeDistToEnroll = 20;
    //Radians maxHeadAngleToEnroll = DEG_TO_RAD(20);
  } _params;

};

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Coretech_Vision_FaceEvalIdentify_H__ */



