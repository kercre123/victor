/**
 * File: GroundPlaneClassifier.h
 *
 * Author: Lorenzo Riano
 * Created: 11/29/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_GroundplaneClassifier_h
#define __Anki_Cozmo_Basestation_GroundplaneClassifier_h

#include "anki/common/basestation/math/polygon.h"
#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "engine/debugImageList.h"
#include "engine/vision/drivingSurfaceClassifier.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
class CozmoContext;
struct OverheadEdgeFrame;

class FeaturesExtractor {
public:
  virtual std::vector<DrivingSurfaceClassifier::FeatureType>
  Extract(const Vision::ImageRGB& image, int row, int col) const = 0;
};

class MeanStdFeaturesExtractor : public FeaturesExtractor {
public:

  MeanStdFeaturesExtractor(int padding) : _padding(padding) {};

  std::vector<DrivingSurfaceClassifier::FeatureType>
  Extract(const Vision::ImageRGB& image, int row, int col) const override;

private:
  int _padding;
};

class SinglePixelFeaturesExtraction : public FeaturesExtractor {
public:
  std::vector<DrivingSurfaceClassifier::FeatureType> Extract(const Vision::ImageRGB& image, int row, int col) const override;
};

/****************************************************************
 *                     Helper Functions                         *
 ****************************************************************/

void ClassifyImage(const DrivingSurfaceClassifier& clf, const Anki::Cozmo::FeaturesExtractor& extractor,
                   const Vision::ImageRGB& image, Vision::Image outputMask);

template<typename T1, typename T2>
void convertToVector(const cv::Mat& mat, std::vector<std::vector<T2>>& vec)
{
  DEV_ASSERT(mat.type() == cv::DataType<T1>::type, "convertTo.WrongMatrixType");

  int nRows = mat.rows;
  int nCols = mat.cols;
  vec.clear();
  vec.reserve(nRows);

  for(int i = 0; i < nRows; ++i)
  {
    const T1* p = mat.ptr<T1>(i);
    std::vector<T2> singleRow;
    singleRow.reserve(nRows);
    for (int j = 0; j < nCols; ++j)
    {
      singleRow.push_back(cv::saturate_cast<T2>(p[j]));
    }
    vec.push_back(singleRow);
  }
}

template<typename T1, typename T2>
void convertToVector(const cv::Mat& mat, std::vector<T2>& vec)
{
  DEV_ASSERT(mat.type() == cv::DataType<T1>::type, "convertToVector.WrongMatrixType");
  DEV_ASSERT(mat.rows == 1, "convertToVector.OnlySingleRowAllowed");

  int nCols = mat.cols;
  vec.clear();
  vec.reserve(nCols);

  const T1* p = mat.ptr<T1>(0);
  for (int j = 0; j < nCols; ++j)
  {
    vec.push_back(cv::saturate_cast<T2>(p[j]));
  }

}

class GroundPlaneClassifier
{
public:
  GroundPlaneClassifier(const Json::Value& config, const CozmoContext *context);

  Result Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                DebugImageList <Vision::ImageRGB>& debugImageRGBs,
                std::list<OverheadEdgeFrame>& outEdges);

  bool IsInitizialized() const {
    return _initialized;
  }

  const DrivingSurfaceClassifier& GetClassifier() const {
    return *(_classifier.get());
  }

  static Vision::Image processClassifiedImage(const Vision::Image& binaryImage); //TODO remove static and put const

protected:
  std::unique_ptr<DrivingSurfaceClassifier> _classifier;
  std::unique_ptr<FeaturesExtractor> _extractor;
  const CozmoContext* _context;
  bool _initialized = false;

  void trainClassifier(const std::string& path);
  bool loadClassifier(const std::string& filename);

};

} // namespace Anki
} // namespace Cozmo


#endif //__Anki_Cozmo_Basestation_GroundplaneClassifier_h
