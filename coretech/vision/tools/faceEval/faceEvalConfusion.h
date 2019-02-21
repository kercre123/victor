/**
 * File: faceEvalConfusion.h
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: Evaluates pairwise face "confusion": The match score between all pairs of faces is evaluated. This is
 *              a more direct evaluation of pairwise face discrimination, without the complexities of enrollment
 *              and there being multiple faces (and multiple images of each of those faces) in a database.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Coretech_Vision_FaceConfusionEval_H__
#define __Anki_Coretech_Vision_FaceConfusionEval_H__

#include "coretech/vision/tools/faceEval/faceEvalInterface.h"

namespace Anki {
namespace Vision {

class SimpleArgParser;
  
class FaceEvalConfusion : public IFaceEval
{
public:
  FaceEvalConfusion(const FileList& files,
                    const SimpleArgParser& argParser);
  
  virtual void Run() override;
  
private:
  
  std::vector<std::string> _fileList;
  
  struct {
    float resizeFactor = 0.5f;
    float recognitionThreshold = 0.75f;
    float minEyeDistToEnroll = 20;
    int maxEnrollees = -1;
    Radians maxHeadAngleToEnroll = DEG_TO_RAD(20);
  } _params;
  
  struct EnrolledFace
  {
    std::string identity;
    std::string file;
    TrackedFace face;
  };
  
  void DisplayHelper(FaceID_t faceID, const EnrolledFace& filename, float score, const char* windowName, int pauseTime=5) const;
  void ShowTiledImage(const std::map<int, EnrolledFace>& enrolledFaces) const;
  bool IsEnrollable(const TrackedFace& face) const;
};

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Coretech_Vision_FaceConfusionEval_H__ */


