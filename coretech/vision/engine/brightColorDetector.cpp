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

#include <dlib/array2d.h>
#include <dlib/image_transforms/label_connected_blobs.h>

#include <array>
#include <vector>

namespace Anki {
namespace Vision {

namespace {

/**
 * @brief Container struct for storing information about contiguous regions of bright colors
 */
struct BrightColorRegion
{
  std::vector<float> hues;
  std::vector<float> scores;
  std::vector<cv::Point2f> points;
};

/**
 * @brief Functor that returns true if the pixels are within an epsilon of each other
 */
struct IsConnected
{
  IsConnected(float epsilon=Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT) : epsilon(epsilon) {}

  template <typename image_view_type>
  bool operator() (const image_view_type& img, const dlib::point& a, const dlib::point& b) const
  {
      return Util::IsNear(img[a.y()][a.x()], img[b.y()][b.x()], epsilon);
  }

private:
  float epsilon;
};

/**
 * @brief Functor that returns true if the pixel value is less than 0
 */
struct IsBackground
{
    template <typename image_view_type>
    bool operator() (const image_view_type& img, const dlib::point& p) const
    {
      return img[p.y()][p.x()] < 0.f;
    }
};

} /* anonymous namespace */

struct BrightColorDetector::Parameters
{
  bool isInitialized;

  //! Number of cells in the grid horizontally
  u32 gridWidth;

  //! Number of cells in the grid vertically
  u32 gridHeight;

  //! Threshold that bright colors must exceed to be considered bright [0,109+]
  float brightnessScoreThreshold;

  //! Group salient points into larger salient points based on whether they are similar hues
  bool groupGridPoints;

  //! The epsilon for deciding whether two hues in range [0,1] are the same or similar.
  float colorConnectednessEpsilon;

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

  if (_params->groupGridPoints){
    return GetSalientPointsWithGrouping(smallerImage,salientPoints);
  } else {
    return GetSalientPointsWithoutGrouping(smallerImage, salientPoints);
  }
}

Result BrightColorDetector::GetSalientPointsWithoutGrouping(const ImageRGB& inputImage,
                                                         std::list<SalientPoint>& salientPoints)
{
  const u32 timestamp = inputImage.GetTimestamp();
  const Vision::SalientPointType type = Vision::SalientPointType::BrightColors;
  const char* typeString = EnumToString(type);

  const s32 cellHeight = inputImage.GetNumRows()/_params->gridHeight;
  const s32 cellWidth = inputImage.GetNumCols()/_params->gridWidth;

  const float kHeightScale = 1.f / static_cast<float>(inputImage.GetNumRows());
  const float kWidthScale  = 1.f / static_cast<float>(inputImage.GetNumCols());

  const float kHeightStep = cellHeight * kHeightScale;
  const float kWidthStep = cellWidth * kWidthScale;
  const float kHeightStep2 = kHeightStep/2.f;
  const float kWidthStep2 = kWidthStep/2.f;

  for (s32 row = 0; row < _params->gridHeight; ++row)
  {
    const s32 imageRow = row * cellHeight;
    for (s32 col = 0; col < _params->gridWidth; ++col)
    {
      const s32 imageCol = col * cellWidth;
      Rectangle<s32> roi(imageCol,imageRow,cellHeight,cellWidth);

      const float score = GetScore(inputImage.GetROI(roi));

      const bool isBright = (score > _params->brightnessScoreThreshold);

      if (isBright){
        // Compute average color for region:
        // 1) Grab the mean color for the region.
        // 2) Convert to HSV space
        // 3) Push saturation and value up to the max
        // 4) Convert back to RGB
        // 5) Pack into a uint32
        // Note that this means if the region has two colors (e.g. red, yellow),
        // you'll get the average of the two (e.g. orange).

        const cv::Scalar_<double> color = cv::mean(inputImage.GetROI(roi).get_CvMat_());
        PixelHSV hsv(PixelRGB(color[0],color[1],color[2]));
        hsv.s() = 1.f;
        hsv.v() = 1.f;
        const PixelRGB pixel = hsv.ToPixelRGB();
        const u32 rgba = ColorRGBA(pixel.r(),pixel.g(),pixel.b(),255).AsRGBA();

        // Provide polygon for the ROI at [0,1] scale
        const float x = Util::Clamp(imageCol * kWidthScale, 0.f, 1.f);
        const float y = Util::Clamp(imageRow * kHeightScale, 0.f, 1.f);

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

Result BrightColorDetector::GetSalientPointsWithGrouping(const ImageRGB& inputImage,
                                                         std::list<SalientPoint>& salientPoints)
{
  const u32 timestamp = inputImage.GetTimestamp();
  const Vision::SalientPointType type = Vision::SalientPointType::BrightColors;
  const char* typeString = EnumToString(type);

  const s32 cellHeight = inputImage.GetNumRows()/_params->gridHeight;
  const s32 cellWidth = inputImage.GetNumCols()/_params->gridWidth;

  const float kHeightScale = 1.f / static_cast<float>(inputImage.GetNumRows());
  const float kWidthScale  = 1.f / static_cast<float>(inputImage.GetNumCols());

  dlib::array2d<float> gridScores(_params->gridHeight,_params->gridWidth);
  dlib::array2d<float> gridHues(gridScores.nr(), gridScores.nc());

  // Generate BrightColorGrid

  for (s32 row = 0; row < _params->gridHeight; ++row)
  {
    const s32 imageRow = row * cellHeight;
    for (s32 col = 0; col < _params->gridWidth; ++col)
    {
      const s32 imageCol = col * cellWidth;
      Rectangle<s32> roi(imageCol,imageRow,cellHeight,cellWidth);

      const float score = GetScore(inputImage.GetROI(roi));

      const bool isBright = (score > _params->brightnessScoreThreshold);

      if (isBright){

        const cv::Scalar_<double> color = cv::mean(inputImage.GetROI(roi).get_CvMat_());
        const PixelHSV hsv(PixelRGB(color[0],color[1],color[2]));

        gridHues[row][col] = hsv.h();
        gridScores[row][col] = score;
      } else {
        gridHues[row][col] = -1.f;
        gridScores[row][col]= 0.f;
      }
    }
  }

  // Label the image

  dlib::array2d<u32> labels(gridScores.nr(), gridScores.nc());

  unsigned long numLabels = dlib::label_connected_blobs(gridHues,
                                                        IsBackground(),
                                                        dlib::neighbors_4(),
                                                        IsConnected(_params->colorConnectednessEpsilon),
                                                        labels);

  // Grow region from list of salient points and their hues

  // NOTE: Using a convex hull is an easy way to do this, though it is not a tight contour around the polygon.
  // That means areas that were not labeled as part of the same color can overlap.

  std::vector<BrightColorRegion> regions(numLabels);
  for (u32 row = 0; row < labels.nr(); ++row)
  {
    for (u32 col = 0; col < labels.nc(); ++col)
    {
      const u32 label = labels[row][col];

      // Skip background
      if (label == 0)
      {
        continue;
      }

      float hue = gridHues[row][col];
      float score = gridScores[row][col];

      BrightColorRegion& region = regions[label];
      region.hues.push_back(hue);
      region.scores.push_back(score);

      // Add four corners of the location to make sure we can always get a polygon around the grid location
      region.points.emplace_back(col,   row);
      region.points.emplace_back(col+1, row);
      region.points.emplace_back(col+1, row+1);
      region.points.emplace_back(col,   row+1);
    }
  }

  // Create Vision::SalientPoints
  // Skip label 0, it's the background pixels (i.e. not bright colors)
  for (u32 label = 1; label < numLabels; ++label){
    BrightColorRegion& region = regions[label];

    // Get the score
    float score = cv::mean(region.scores).val[0];

    // Get the convex hull of the points
    std::vector<cv::Point2f> hull;
    cv::convexHull(region.points, hull,true,true);

    // Get the center of the convex hull
    cv::Point2f center;
    {
      // Get the average point, shift it half a unit, then scale to image coordinates in [0,1]
      cv::Mat mean(cv::Mat::zeros(1,2,CV_32FC1));
      cv::reduce(hull, mean, 1, CV_REDUCE_AVG);
      center.x = Util::Clamp((mean.at<float>(0,0) + 0.5f) * cellWidth * kWidthScale,   0.f, 1.f);
      center.y = Util::Clamp((mean.at<float>(0,1) + 0.5f) * cellHeight * kHeightScale, 0.f, 1.f);
    }

    // Convert the hull to a polygon in [0,1] image coordinates
    std::vector<Anki::CladPoint2d> poly;
    for (auto& pt : hull)
    {
      // Convert points to image coordinates in [0,1]
      const float x = Util::Clamp(pt.x * cellWidth * kWidthScale,   0.f, 1.f);
      const float y = Util::Clamp(pt.y * cellHeight * kHeightScale, 0.f, 1.f);
      poly.emplace_back(x,y);
    }

    // Convert from HSV to RGBA with maximized saturation and value
    u32 rgba;
    {
      float hue = cv::mean(region.hues).val[0];
      PixelRGB rgb = PixelHSV(hue,1.f,1.f).ToPixelRGB();
      rgba = ColorRGBA(rgb.r(),rgb.g(),rgb.b(),255).AsRGBA();
    }

    // Get the area_fraction. The convexh hull is in [0,1] coordinates so the area is also the area fraction.
    float area_fraction = cv::contourArea(hull);

    // Create the Vision::SalientPoint for this region
    salientPoints.emplace_back(timestamp,
                               center.x, center.y,
                               score,
                               area_fraction,
                               type,
                               typeString,
                               poly,
                               rgba);
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
, gridWidth(10)
, gridHeight(10)
, brightnessScoreThreshold(59.f)
, groupGridPoints(false)
, colorConnectednessEpsilon(5.f/360.f) // N/360 degrees in either direction
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BrightColorDetector::Parameters::Initialize()
{
  isInitialized = true;
}

} /* namespace Vision */
} /* namespace Anki */
