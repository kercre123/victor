/**
 * File: colorDetector.cpp
 *
 * Author: Patrick Doran
 * Date: 2018/11/12
 */

#include "coretech/vision/engine/colorDetector.h"

#include "coretech/vision/engine/image.h"

#include <iostream>

namespace Anki
{
namespace Vision
{

namespace
{
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Convert an ImageRGB to an Nx3 matrix where each row is one pixel in RGB order
 */
void ImageToSamples(const ImageRGB& inputImage, cv::Mat& samples)
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
 * @brief
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

} /* anonymous namespace */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorDetector::ColorDetector (const Json::Value& config)
{
  bool loaded = Load(config);
  std::cerr<<"Loaded: "<<loaded<<std::endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorDetector::~ColorDetector ()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ColorDetector::Load (const Json::Value& config)
{
  Json::Value jsonLabels = config.get("labels",Json::Value::null);
  if (jsonLabels.isNull() || !jsonLabels.isArray())
  {
    std::cerr<<"failure 0"<<std::endl;
    return false; // fail;
  }

  std::vector<Label> labels;
  for (const Json::Value& jsonLabel : jsonLabels)
  {
    if (!jsonLabel.isObject())
    {
      std::cerr<<"failure 1"<<std::endl;
      return false;
    }

    Json::Value jsonName = jsonLabel.get("name",Json::Value::null);
    if (jsonName.isNull() || !jsonName.isString())
    {
      std::cerr<<"failure 2"<<std::endl;
      return false;
    }
    std::string name = jsonName.asString();

    Json::Value jsonColor = jsonLabel.get("color",Json::Value::null);
    if (jsonColor.isNull() || !jsonColor.isArray() || jsonColor.size() != 3)
    {
      std::cerr<<"failure 3"<<std::endl;
      return false;
    }
    PixelRGB color(jsonColor[0].asUInt(), jsonColor[1].asUInt(), jsonColor[2].asUInt());

    labels.emplace_back(Label{name, color});
  }

  // TOOD: Load the labels;

  Json::Value jsonClassifier = config.get("classifier",Json::Value::null);
  if (jsonClassifier.isNull() || !jsonClassifier.isObject())
  {
    std::cerr<<"failure 4"<<std::endl;
    return false;
  }

  ColorClassifier classifier;
  if (!classifier.Load(jsonClassifier))
  {
    std::cerr<<"failure 5"<<std::endl;
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
Result ColorDetector::Detect (const ImageRGB& inputImage, ImageRGB& outputImage)
{
  // TODO: Make smaller image size configurable
  ImageRGB smallerImage(100,100);
  inputImage.Resize(smallerImage, ResizeMethod::NearestNeighbor);

  cv::Mat samples;
  ImageToSamples(smallerImage, samples);

  cv::Mat labelings(samples.rows,1,CV_32SC1);

  _classifier.Classify(samples,labelings);

  labelings = labelings.reshape(1,{smallerImage.GetNumRows(), smallerImage.GetNumCols()});

  // Color the image
  PixelRGB unknown(0,0,0);

  // TODO: Move this transform somewhere, it needs only be done once. Or pass _labels vector to LabelToImage function
  auto xform = [](const Label& lbl) -> PixelRGB {
    return lbl.color;
  };
  std::vector<PixelRGB> colors;
  std::transform(_labels.begin(),_labels.end(), std::back_inserter(colors), xform);

  LabelToImage(labelings, colors, PixelRGB(0,0,0), smallerImage);

#if 0
  {
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::minMaxLoc(labelings, &minVal, &maxVal, &minLoc, &maxLoc);
    std::cerr<<"Min: "<<minVal<<std::endl;
    std::cerr<<"Max: "<<maxVal<<std::endl;
  }
#endif

  outputImage = ImageRGB(inputImage.GetNumRows(), inputImage.GetNumCols());
  smallerImage.Resize(outputImage, ResizeMethod::NearestNeighbor);

  return RESULT_OK;
}



} /* namespace Vision */
} /* namespace Anki */
