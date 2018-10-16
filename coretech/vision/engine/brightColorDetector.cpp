/**
 * File: brightColorDetector.cpp
 *
 * Author: Patrick Doran
 * Date: 10/22/2018
 */
#include "brightColorDetector.h"
#include "util/math/math.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/common/engine/math/rect_impl.h"

namespace Anki {
namespace Vision {

struct BrightColorDetector::Parameters
{
  bool isInitialized;
  Parameters();
  void Initialize();
};

class BrightColorDetector::Memory
{
public:
  Memory(){}
};

BrightColorDetector::BrightColorDetector (const Camera& camera)
  : _camera(camera)
  , _params(new Parameters)
  , _memory(new Memory)
{
  // To pass unused variable warning
  _camera.GetID();
}

BrightColorDetector::~BrightColorDetector () {

}

Result BrightColorDetector::Init ()
{
  _params->Initialize();
  return RESULT_OK;
}

Result BrightColorDetector::Detect (const ImageRGB& inputImage,
                                    std::list<SalientPoint>& salientPoints)
{
  const u32 timestamp = inputImage.GetTimestamp();
  const Vision::SalientPointType type = Vision::SalientPointType::BrightColors;
  const char* typeString = EnumToString(type);
  const s32 rows = inputImage.GetNumRows();
  const s32 cols = inputImage.GetNumCols();
  const float kWidthScale  = 1.f / static_cast<float>(inputImage.GetNumCols());
  const float kHeightScale = 1.f / static_cast<float>(inputImage.GetNumRows());
  const s32 numStepsRow = 3;
  const s32 stepRow = inputImage.GetNumRows()/numStepsRow;
  const s32 numStepsCol = 3;
  const s32 stepCol = inputImage.GetNumCols()/numStepsCol;

  const float kHeightStep = stepRow * kHeightScale;
  const float kWidthStep = stepCol * kWidthScale;
  const float kHeightStep2 = kHeightStep/2.f;
  const float kWidthStep2 = kWidthStep/2.f;

  // TODO: Turn this into a parameter
  const float kScoreThreshold = 59.f;

  // TODO: Add ignore regions argument

  for (s32 row = 0; row < rows; row += stepRow){
    for (s32 col = 0; col < cols; col += stepCol) {
      Rectangle<s32> roi(col,row,stepCol,stepRow);

      float score = GetScore(inputImage.GetROI(roi));

      bool isBright = (score > kScoreThreshold);

      if (isBright){
        // Compute average color for region:
        // 1) Grab the mean color for the region.
        // 2) Convert to HSV space
        // 3) Push saturation and value up to the max
        // 4) Convert back to RGB
        // 5) Pack into a uint32
        // Note that this means if the region has two colors (e.g. red, yellow),
        // you'll get the average of the two (e.g. orange).

        cv::Scalar_<double> color = cv::mean(inputImage.GetROI(roi).get_CvMat_());
        PixelHSV hsv(PixelRGB(color[0],color[1],color[2]));
        hsv.s() = 1.f;
        hsv.v() = 1.f;
        PixelRGB pixel = hsv.ToPixelRGB();
        u32 rgba = ColorRGBA(pixel.r(),pixel.g(),pixel.b(),255).AsRGBA();

        // Provide polygon for the ROI at [0,1] scale
        float x = Util::Clamp(col*kWidthScale, 0.f, 1.f);
        float y = Util::Clamp(row*kHeightScale, 0.f, 1.f);

        std::vector<Anki::CladPoint2d> poly;
        poly.emplace_back(Util::Clamp(x,            0.f,1.f),
                          Util::Clamp(y,            0.f,1.f));
        poly.emplace_back(Util::Clamp(x+kWidthStep, 0.f,1.f),
                          Util::Clamp(y,            0.f,1.f));
        poly.emplace_back(Util::Clamp(x+kWidthStep, 0.f,1.f),
                          Util::Clamp(y+kHeightStep,0.f,1.f));
        poly.emplace_back(Util::Clamp(x,            0.f,1.f),
                          Util::Clamp(y+kHeightStep,0.f,1.f));

        salientPoints.emplace_back(timestamp,
                                   x+kWidthStep2,y+kHeightStep2,
                                   score,
                                   (kWidthScale*kHeightScale),
                                   type,
                                   typeString,
                                   poly,
                                   rgba);
      }
    }
  }
  return RESULT_OK;
}

float BrightColorDetector::GetScore(const ImageRGB& image)
{
  // TODO Do this without allocating images for each channels.

  cv::Mat red(image.GetNumRows(),image.GetNumCols(),CV_8U);
  cv::Mat green(red.rows,red.cols,CV_8U);
  cv::Mat blue(red.rows,red.cols,CV_8U);
  cv::split(image.get_CvMat_(), std::vector<cv::Mat>{red, green, blue});

  // OpenCV uses double internally in cv::meanStdDev
  cv::Scalar_<double> meanRG;
  cv::Scalar_<double> stdRG;
  cv::Scalar_<double> meanYB;
  cv::Scalar_<double> stdYB;
  double meanRoot;
  double stdRoot;

  cv::meanStdDev(cv::abs(red-green), meanRG, stdRG);
  cv::meanStdDev(cv::abs((red+green)/2.f - blue), meanYB, stdYB);

  meanRoot = sqrt( (meanRG.val[0] * meanRG.val[0]) + (meanYB.val[0] * meanYB.val[0]) );
  stdRoot = sqrt( (stdRG.val[0] * stdRG.val[0]) + (stdYB.val[0] * stdYB.val[0]) );

  return stdRoot + (0.3f * meanRoot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BrightColorDetector::Parameters::Parameters()
: isInitialized(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrightColorDetector::Parameters::Initialize()
{
  isInitialized = true;
}

} /* namespace Vision */
} /* namespace Anki */
