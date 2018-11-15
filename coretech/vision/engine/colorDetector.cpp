/**
 * File: colorDetector.cpp
 *
 * Author: Patrick Doran
 * Date: 2018/11/12
 */

#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/colorDetector.h"
#include "coretech/vision/engine/image.h"
#include "util/math/math.h"


#include <dlib/opencv.h>
#include <dlib/image_transforms/label_connected_blobs.h>

#include <tuple>

// TODO: remove this
#include <iostream>

namespace Anki
{
namespace Vision
{

namespace
{

class value_pixels_are_background
{
public:
  value_pixels_are_background(s32 value) : _value(value) {}
  template <typename image_view_type>
  bool operator() (const image_view_type& img, const dlib::point& p) const
  {
    return img[p.y()][p.x()] == _value;
  }
private:
  s32 _value;

};

struct ColorRegion
{
  ColorRegion() : color(), count(0), points() {}

  PixelRGB color;

  /**
   * @brief Number of pixels classified into this region
   */
  u32 count;

  std::vector<cv::Point2i> points;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Convert an ImageRGB to an Nx3 matrix where each row is one pixel in RGB order
 */
void ImageToSamplesRGB(const ImageRGB& inputImage, cv::Mat& samples)
{
  cv::Mat red(inputImage.GetNumRows(),inputImage.GetNumCols(),CV_8U);
  cv::Mat green(red.rows,red.cols,CV_8U);
  cv::Mat blue(red.rows,red.cols,CV_8U);
  cv::split(inputImage.get_CvMat_(), std::vector<cv::Mat>{red, green, blue});

  // Fill out as rows first (should be faster memory copying) and then transpose so that each row is R,G,B

  int rows = inputImage.GetNumRows() * inputImage.GetNumCols();
  cv::Mat merged = cv::Mat::zeros(3, rows, CV_8UC1);
  red.reshape(1,1).copyTo(merged.row(0));
  green.reshape(1,1).copyTo(merged.row(1));
  blue.reshape(1,1).copyTo(merged.row(2));
  merged = merged.t();

  cv::Mat converted;
  merged.convertTo(converted,CV_64FC1);

  samples = converted;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Convert an ImageRGB to an Nx2 matrix where each row is one pixel in UV (of YUV) order
 */
void ImageToSamplesYUV(const ImageRGB& inputImage, cv::Mat& samples)
{
  cv::Mat red(inputImage.GetNumRows(),inputImage.GetNumCols(),CV_8U);
  cv::Mat green(red.rows,red.cols,CV_8U);
  cv::Mat blue(red.rows,red.cols,CV_8U);
  cv::split(inputImage.get_CvMat_(), std::vector<cv::Mat>{red, green, blue});

  // Fill out as rows first (should be faster memory copying) and then transpose so that each row is R,G,B

  int rows = inputImage.GetNumRows() * inputImage.GetNumCols();
  cv::Mat merged = cv::Mat::zeros(3, rows, CV_8UC1);
  red.reshape(1,1).copyTo(merged.row(0));
  green.reshape(1,1).copyTo(merged.row(1));
  blue.reshape(1,1).copyTo(merged.row(2));
  merged = merged.t();

  // Convert to YUV
  for (auto row = 0; row < merged.rows; ++row)
  {
    PixelYUV yuv;
    yuv.FromPixelRGB(PixelRGB(merged.at<u8>(row,0), merged.at<u8>(row,1), merged.at<u8>(row,2)));
    merged.at<u8>(row,0) = yuv.y();
    merged.at<u8>(row,1) = yuv.u();
    merged.at<u8>(row,2) = yuv.v();
  }

  cv::Mat converted;
  merged.convertTo(converted,CV_64FC1);

  // Drop the Y and copy to the output variable
  samples = converted.colRange(1,3).clone();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Convert a label image (labelings) to an ImageRGB
 * @todo: Really don't need an unknown value, just set all values of image to that value
 */
void LabelToImage(const cv::Mat& labelings,
                  const std::vector<PixelRGB>& colors,
                  const PixelRGB& unknown,
                  ImageRGB& image)
{
  int rows = image.GetNumRows();
  int cols = image.GetNumCols();
  for (auto row = 0; row < rows; ++row)
  {
    for (auto col = 0; col < cols; ++col)
    {
      s32 index = labelings.at<s32>(row,col);
      image.get_CvMat_().at<PixelRGB>(row,col) = (index >= 0 && index < colors.size())? colors[index] : unknown;
    }
  }
}

/**
 * @brief Turn a label image (labelings) into a list of salient points
 */
void LabelToSalientPoints(const ImageRGB& inputImage,
                          const cv::Mat& labelings,
                          const std::vector<PixelRGB>& colors,
                          std::list<SalientPoint>& salientPoints)
{
  // Label the connected blobs. There may be multiple blobs of the same color

  dlib::array2d<u32> blobs(labelings.rows, labelings.cols);
  unsigned long numBlobs = dlib::label_connected_blobs(dlib::cv_image<s32>(labelings),
                                                       value_pixels_are_background(-1),
                                                       dlib::neighbors_8(),
                                                       dlib::connected_if_equal(),
                                                       blobs);

  // Aggregate the data for each blob

  std::vector<ColorRegion> regions(numBlobs);
  for (u32 row = 0; row < blobs.nr(); ++row)
  {
    for (u32 col = 0; col < blobs.nc(); ++col)
    {
      const s32 label = labelings.at<s32>(row,col);

      // Skip background pixels
      if (label == -1)
      {
        continue;
      }

      const u32 blob = blobs[row][col];
      ColorRegion& region = regions[blob];

      region.count++;

      region.color = colors[label];

      // Add four corners of the location to make sure we can always get a polygon around the grid location
      region.points.emplace_back(col,   row);
      region.points.emplace_back(col+1, row);
      region.points.emplace_back(col+1, row+1);
      region.points.emplace_back(col,   row+1);
    }
  }

  // TODO: Can someone explain why the first one doesn't compile?
  // Compiler error:
  //      no known conversion from 'const std::__1::__wrap_iter<cv::Point_<int> *>' to 'const cv::Point2i' (aka 'const Point_<int>')

  // Create the salient point from each region
  // blob 0 is the background, so skip it.
  // SalientPoint polygon is the bounding rectangle, could also do convex hull or the contour
  const u32 timestamp = inputImage.GetTimestamp();
  const Vision::SalientPointType type = Vision::SalientPointType::BrightColors;
  const char* typeString = EnumToString(type);
  const float kWidthScale = 1.f/labelings.cols;
  const float kHeightScale = 1.f/labelings.rows;

  for (u32 blob = 1; blob < numBlobs; ++blob){
    ColorRegion& region = regions[blob];

    // TODO: move this number to configuration
    if (region.count < 4)
    {
      continue; // skip small regions, assume it's noise.
    }
    f32 left, top, right, bottom;
    {
      cv::Rect2f rect = cv::boundingRect(region.points);

      // TODO: Not sure why I have to shift everything left and up by one to get the intended result.
      left = rect.x;
      right = left + (rect.width - 1);
      top = rect.y;
      bottom = top + (rect.height - 1);
    }

    left = Util::Clamp(left * kWidthScale, 0.f, 1.f);
    right = Util::Clamp(right * kWidthScale, 0.f, 1.f);
    top = Util::Clamp(top * kHeightScale, 0.f, 1.f);
    bottom = Util::Clamp(bottom * kHeightScale, 0.f, 1.f);

    std::vector<Anki::CladPoint2d> poly;
    poly.emplace_back(left,top);
    poly.emplace_back(right,top);
    poly.emplace_back(right,bottom);
    poly.emplace_back(left,bottom);

    f32 centerX = (right+left)/2.f;
    f32 centerY = (bottom+top)/2.f;

    // Coordinates are in [[0,0] - [1,1]] so the area_fraction is just the bounding rectangle's size
    f32 area_fraction = (right-left)*(bottom-top);

    // TODO: Replace with something from the probability of each blob's label.
    f32 score = 1.f;

    u32 rgba = ColorRGBA(region.color.r(),region.color.g(),region.color.b(),255).AsRGBA();

    salientPoints.emplace_back(timestamp,
                               centerX, centerY,
                               score,
                               area_fraction,
                               type,
                               typeString,
                               poly,
                               rgba);
  }
}

} /* anonymous namespace */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorDetector::ColorDetector (const Json::Value& config)
{
  DEV_ASSERT(Load(config),"Failed to load ColorDetector config");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorDetector::~ColorDetector ()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ColorDetector::Load (const Json::Value& config)
{
  Json::Value jsonModelKey = config.get("model",Json::Value::null);
  if (jsonModelKey.isNull() || !jsonModelKey.isString())
  {
    return false;
  }

  Json::Value jsonModels = config.get("models",Json::Value::null);
  if (jsonModels.isNull() || !jsonModels.isObject())
  {
    return false;
  }

  Json::Value jsonModel = jsonModels.get(jsonModelKey.asString(),Json::Value::null);
  if (jsonModel.isNull() || !jsonModel.isObject())
  {
    return false;
  }

  Json::Value jsonLabels = jsonModel.get("labels",Json::Value::null);
  if (jsonLabels.isNull() || !jsonLabels.isArray())
  {
    return false;
  }

  Json::Value jsonFormat = jsonModel.get("format",Json::Value::null);
  if (jsonFormat.isNull() || !jsonFormat.isString())
  {
    return false;
  }
  _format = jsonFormat.asString();

  std::vector<Label> labels;
  for (const Json::Value& jsonLabel : jsonLabels)
  {
    if (!jsonLabel.isObject())
    {
      return false;
    }

    Json::Value jsonName = jsonLabel.get("name",Json::Value::null);
    if (jsonName.isNull() || !jsonName.isString())
    {
      return false;
    }
    std::string name = jsonName.asString();

    Json::Value jsonColor = jsonLabel.get("color",Json::Value::null);
    if (jsonColor.isNull() || !jsonColor.isArray() || jsonColor.size() != 3)
    {
      return false;
    }
    PixelRGB color(jsonColor[0].asUInt(), jsonColor[1].asUInt(), jsonColor[2].asUInt());

    labels.emplace_back(Label{name, color});
  }

  // TODO: Replace this with loading a data file (e.g. classifier_rgb.json)
  Json::Value jsonClassifier = jsonModel.get("classifier",Json::Value::null);
  if (jsonClassifier.isNull() || !jsonClassifier.isObject())
  {
    return false;
  }

  ColorClassifier classifier;
  if (!classifier.Load(jsonClassifier))
  {
    return false;
  }

  _labels = labels;
  _classifier = classifier;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ColorDetector::Init ()
{
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ColorDetector::Detect (const ImageRGB& inputImage,
                              std::list<SalientPoint>& salientPoints,
                              std::list<std::pair<std::string, ImageRGB>>& debugImageRGBs)
{
  // TODO: Make smaller image size configurable
  ImageRGB smallerImage(16,16);
  inputImage.Resize(smallerImage, ResizeMethod::NearestNeighbor);

  cv::Mat samples;
  if (_format == "RGB")
  {
    ImageToSamplesRGB(smallerImage, samples);
  }
  else if (_format == "YUV")
  {
    ImageToSamplesYUV(smallerImage, samples);
  }
  else
  {
    // TODO: never get to this point
    return RESULT_FAIL;
  }

  cv::Mat labelings(samples.rows,1,CV_32SC1);

  _classifier.Classify(samples,labelings);

  labelings = labelings.reshape(1,{smallerImage.GetNumRows(), smallerImage.GetNumCols()});

  // Color the image
  PixelRGB unknown(0,0,0);

  // TODO: Move this transform somewhere, it needs only be done once. Or pass _labels vector to LabelToImage function
  auto xform = [](const Label& lbl) -> PixelRGB { return lbl.color; };
  std::vector<PixelRGB> colors;
  std::transform(_labels.begin(),_labels.end(), std::back_inserter(colors), xform);

  LabelToImage(labelings, colors, PixelRGB(0,0,0), smallerImage);

#if 1
  ImageRGB labelImage(inputImage.GetNumRows(), inputImage.GetNumCols());
  smallerImage.Resize(labelImage, ResizeMethod::NearestNeighbor);
  debugImageRGBs.emplace_back(std::make_pair("ColorLabel",labelImage));
#elif
  debugImageRGBs.emplace_back(std::make_pair("ColorLabel",smallerImage));
#endif

  // Create salient points
  LabelToSalientPoints(smallerImage, labelings, colors, salientPoints);

  return RESULT_OK;
}



} /* namespace Vision */
} /* namespace Anki */
