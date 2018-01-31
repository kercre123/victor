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

#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"
#include "engine/debugImageList.h"
#include "engine/vision/rawPixelsClassifier.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
class CozmoContext;
struct OverheadEdgeFrame;

/****************************************************************
 *                     Features Extractors                      *
 ****************************************************************/

class FeaturesExtractor {
public:
  virtual std::vector<RawPixelsClassifier::FeatureType>
  Extract(const Vision::ImageRGB& image, int row, int col) const = 0;

  virtual Anki::Array2d<RawPixelsClassifier::FeatureType>
  Extract(const Vision::ImageRGB& image) const = 0;

};

class MeanStdFeaturesExtractor : public FeaturesExtractor {
public:

  explicit MeanStdFeaturesExtractor(int padding) : _padding(padding), _profiler("MeanStdFeaturesExtractor"){}

  Array2d<RawPixelsClassifier::FeatureType> Extract(const Vision::ImageRGB& image) const override;;

  std::vector<RawPixelsClassifier::FeatureType>
  Extract(const Vision::ImageRGB& image, int row, int col) const override;

private:
  int _padding;
  mutable uchar* _prevImageData = nullptr; //used to check if a new image is being used
  mutable cv::Mat _meanImage;
  mutable cv::MatIterator_<cv::Vec3f> _meanMatIterator;
  mutable Anki::Vision::Profiler _profiler;
};

class SinglePixelFeaturesExtraction : public FeaturesExtractor {
public:
  std::vector<RawPixelsClassifier::FeatureType> Extract(const Vision::ImageRGB& image, int row, int col) const override;

  Array2d<RawPixelsClassifier::FeatureType> Extract(const Vision::ImageRGB& image) const override;
};

/****************************************************************
 *                     Helper Functions                         *
 ****************************************************************/

void ClassifyImage(const RawPixelsClassifier& clf, const Anki::Cozmo::FeaturesExtractor& extractor,
                   const Vision::ImageRGB& image, Vision::Image& outputMask, Anki::Vision::Profiler *profiler = nullptr);

void ClassifyImage(const DTRawPixelsClassifier& clf, const Anki::Cozmo::MeanStdFeaturesExtractor& extractor,
                   const Vision::ImageRGB& image, Vision::Image& outputMask, Anki::Vision::Profiler *profiler = nullptr);

template<typename T1, typename T2>
void convertToVector(const cv::Mat& mat, std::vector<std::vector<T2>>& vec)
{
  DEV_ASSERT(mat.type() == cv::DataType<T1>::type, "convertToVector.WrongMatrixType");
  DEV_ASSERT(mat.channels() == 1, "convertToVector.WrongNuberOfChannels");

  int nRows = mat.rows;
  int nCols = mat.cols;
  vec = std::vector<std::vector<T2>>(nRows, std::vector<T2>(nCols));

  for(int i = 0; i < nRows; ++i)
  {
    const T1* p = mat.ptr<T1>(i);
    std::vector<T2>& singleRow  = vec[i];
    for (int j = 0; j < nCols; ++j)
    {
      singleRow[j] = cv::saturate_cast<T2>(p[j]);
    }
  }
}

template<typename T1, typename T2>
void convertToVector(const cv::Mat& mat, std::vector<T2>& vec)
{
  DEV_ASSERT(mat.type() == cv::DataType<T1>::type, "convertToVector.WrongMatrixType");
  DEV_ASSERT(mat.rows == 1, "convertToVector.OnlySingleRowAllowed");
  DEV_ASSERT(mat.channels() == 1, "convertToVector.WrongNuberOfChannels");

  int nCols = mat.cols;
  vec = std::vector<T2>(nCols);

  const T1* p = mat.ptr<T1>(0);
  for (int j = 0; j < nCols; ++j)
  {
    vec[j] = cv::saturate_cast<T2>(p[j]);
  }

}

/****************************************************************
 *                    Ground Plane Classifier                   *
 ****************************************************************/

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

  const RawPixelsClassifier& GetClassifier() const {
    return *(_classifier.get());
  }

  static Vision::Image processClassifiedImage(const Vision::Image& binaryImage); //TODO remove static and put const

protected:
  std::unique_ptr<DTRawPixelsClassifier> _classifier;
  std::unique_ptr<MeanStdFeaturesExtractor> _extractor;
  const CozmoContext* _context;
  bool _initialized = false;
  Anki::Vision::Profiler _profiler;

  void trainClassifier(const std::string& path);
  bool loadClassifier(const std::string& filename);

};

} // namespace Anki
} // namespace Cozmo


#endif //__Anki_Cozmo_Basestation_GroundplaneClassifier_h
