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

#include <array>

namespace Anki {
namespace Vision {

struct BrightColorDetector::Parameters
{
  bool isInitialized;

  //! Number of cells in the grid horizontally
  u32 gridWidth;

  //! Number of cells in the grid vertically
  u32 gridHeight;

  //! Threshold that bright colors must exceed to be considered bright [0,109+]
  float brightnessScoreThreshold;

  //! Group connected regions into a single polygon
  bool groupRegions;

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
  // TODO: Get a smaller input image
  ImageRGB smallerImage(100,100);
  inputImage.Resize(smallerImage,ResizeMethod::NearestNeighbor);
  float imageHeightScale = inputImage.GetNumRows()/static_cast<float>(smallerImage.GetNumRows());
  float imageWidthScale = inputImage.GetNumCols()/static_cast<float>(smallerImage.GetNumCols());

  const u32 timestamp = smallerImage.GetTimestamp();
  const Vision::SalientPointType type = Vision::SalientPointType::BrightColors;
  const char* typeString = EnumToString(type);
//  const s32 rows = smallerImage.GetNumRows();
//  const s32 cols = smallerImage.GetNumCols();
  const s32 numStepsRow = _params->gridHeight;
  const s32 stepRow = smallerImage.GetNumRows()/numStepsRow;
  const s32 numStepsCol = _params->gridWidth;
  const s32 stepCol = smallerImage.GetNumCols()/numStepsCol;

  const float kWidthScale  = 1.f / static_cast<float>(inputImage.GetNumCols());
  const float kHeightScale = 1.f / static_cast<float>(inputImage.GetNumRows());
  const float kHeightStep = stepRow * kHeightScale * imageHeightScale;
  const float kWidthStep = stepCol * kWidthScale * imageWidthScale;
  const float kHeightStep2 = kHeightStep/2.f;
  const float kWidthStep2 = kWidthStep/2.f;

  // TODO replace std::shared_ptr with std::optional so we can just move the object rather than copy
#if 0
  std::array<std::array<std::shared_ptr<SalientPoint>,_params->gridWidth>,_params->gridHeight> gridSalientPoints;
#endif

  // TODO: Add ignore regions argument

  for (s32 row = 0; row < _params->gridHeight; ++row){
    s32 iRow = row * stepRow;
    for (s32 col = 0; col < _params->gridWidth; ++col) {
      s32 iCol = col * stepCol;
      Rectangle<s32> roi(iCol,iRow,stepCol,stepRow);

      float score = GetScore(smallerImage.GetROI(roi));

      bool isBright = (score > _params->brightnessScoreThreshold);

      if (isBright){
        // Compute average color for region:
        // 1) Grab the mean color for the region.
        // 2) Convert to HSV space
        // 3) Push saturation and value up to the max
        // 4) Convert back to RGB
        // 5) Pack into a uint32
        // Note that this means if the region has two colors (e.g. red, yellow),
        // you'll get the average of the two (e.g. orange).

        cv::Scalar_<double> color = cv::mean(smallerImage.GetROI(roi).get_CvMat_());
        PixelHSV hsv(PixelRGB(color[0],color[1],color[2]));
        hsv.s() = 1.f;
        hsv.v() = 1.f;
        PixelRGB pixel = hsv.ToPixelRGB();
        u32 rgba = ColorRGBA(pixel.r(),pixel.g(),pixel.b(),255).AsRGBA();

        // Provide polygon for the ROI at [0,1] scale
        float x = Util::Clamp(iCol*kWidthScale, 0.f, 1.f);
        float y = Util::Clamp(iRow*kHeightScale, 0.f, 1.f);

        std::vector<Anki::CladPoint2d> poly;
        poly.emplace_back(Util::Clamp(x,            0.f,1.f),
                          Util::Clamp(y,            0.f,1.f));
        poly.emplace_back(Util::Clamp(x+kWidthStep, 0.f,1.f),
                          Util::Clamp(y,            0.f,1.f));
        poly.emplace_back(Util::Clamp(x+kWidthStep, 0.f,1.f),
                          Util::Clamp(y+kHeightStep,0.f,1.f));
        poly.emplace_back(Util::Clamp(x,            0.f,1.f),
                          Util::Clamp(y+kHeightStep,0.f,1.f));

        if (_params->groupRegions){
#if 0
          gridSalientPoints[row][col] = std::make_shared<SalientPoint>(timestamp,
                                                                       x+kWidthStep2,y+kHeightStep2,
                                                                       score,
                                                                       (kWidthScale*kHeightScale),
                                                                       type,
                                                                       typeString,
                                                                       poly,
                                                                       rgba);
#endif
        }
        else
        {
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
  }


#if 0
  if (_params->groupRegions)
  {
    std::array<std::array<bool,_params->gridWidth>,_params->gridHeight> visited;
    for (s32 row = 0; row < _params->gridHeight; ++row)
    {
      for (s32 col = 0; col < _params->gridHeight; ++col)
      {
        visited[row][col] = false;
      }
    }

    auto flood = [&](s32 row, s32 col) -> void {
      visited[row][col] = true;
      flood(
    };

    for (s32 row = 0; row < _params->gridHeight; ++row)
    {
      for (s32 col = 0; col < _params->gridHeight; ++col)
      {
        if (gridSalientPoints[row][col] == nullptr)
        {
          visited[row][col] = true;
          continue;
        }
        else
        {
          // TODO: flood
          visited[row][col] = true;
        }
      }
    }
    // TODO: Join regions and put them all in salientPoints
  }
#endif
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
, gridWidth(10)
, gridHeight(10)
, brightnessScoreThreshold(59.f)
, groupRegions(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrightColorDetector::Parameters::Initialize()
{
  isInitialized = true;
}

} /* namespace Vision */
} /* namespace Anki */
