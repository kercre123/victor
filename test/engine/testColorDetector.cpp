/**
 * File: testColorDetector.cpp
 *
 * Author: Patrick Doran
 * Date: 2018/11/08
 */

#include "gtest/gtest.h"

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/myClassifier.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/image.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"

#include "util/fileUtils/fileUtils.h"

#include "json/json.h"

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

#include <fstream>
#include <unordered_map>
#include <vector>

using Anki::CladPoint2d;
using Anki::ColorRGBA;
using Anki::Point2f;
using Anki::Poly2f;
using Anki::Quad2f;
using Anki::Vision::Camera;
using Anki::Vision::Image;
using Anki::Vision::ImageRGB;
using Anki::Vision::PixelHSV;
using Anki::Vision::PixelRGB;

using Anki::Vision::MyClassifier;

extern Anki::Vector::CozmoContext* cozmoContext;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Load labels from a Labelme json file
 */
bool LoadLabels(const std::string& filename, std::unordered_map<std::string,std::vector<Anki::Rectangle<s32>>>& labels)
{
  Json::CharReaderBuilder rbuilder;
  rbuilder["collectComments"] = false;
  std::string errs;
  std::ifstream is(filename);
  Json::Value root;

  if (!Json::parseFromStream(rbuilder, is, &root, &errs))
  {
    return false;
  }

  Json::Value shapes = root.get("shapes",Json::Value::null);
  if (shapes.isNull() || !shapes.isArray())
  {
    return false;
  }

  for (auto ii = 0; ii < shapes.size(); ++ii)
  {

    Json::Value shape = shapes[ii];

    Json::Value type = shape.get("shape_type",Json::Value::null);
    if (type.isNull() || !type.isString() || type.asString() != "rectangle")
    {
      continue;
    }

    Json::Value lbl = shape.get("label",Json::Value::null);
    if (lbl.isNull() || !lbl.isString())
    {
      continue;
    }
    std::string label = lbl.asString();

    Json::Value points = shape.get("points",Json::Value::null);
    if (points.isNull() || !points.isArray() || points.size() != 2)
    {
      continue;
    }
    Json::Value p0 = points[0];
    Json::Value p1 = points[1];
    if (!p0.isArray() || p0.size() != 2 || !p1.isArray() || p1.size() != 2)
    {
      continue;
    }

    float x0 = p0[0].asInt();
    float y0 = p0[1].asInt();
    float x1 = p1[0].asInt();
    float y1 = p1[1].asInt();

    Anki::Rectangle<s32> rectangle = Anki::Rectangle<s32>(x0, y0, x1-x0, y1-y0);

    auto iter = labels.find(label);
    if (iter == labels.end())
    {
      labels.emplace(label, std::vector<Anki::Rectangle<s32>>());
    }
    labels[label].push_back(rectangle);
  }

  return true;
}


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

//  converted /= 255.f;

  samples = converted;
}

void ColorLabelImage(const cv::Mat& labelings,
                     const std::vector<std::string>& labels,
                     const std::unordered_map<std::string, PixelRGB>& colors,
                     ImageRGB& image)
{
  int rows = image.GetNumRows();
  int cols = image.GetNumCols();
  for (auto row = 0; row < rows; ++row)
  {
    for (auto col = 0; col < cols; ++col)
    {
      s32 index = labelings.at<s32>(row,col);
      std::string label = (index >= 0 && index < labels.size())? labels[index] : "UNKNOWN";
      auto iter = colors.find(label);
      PixelRGB color = (iter == colors.end())? PixelRGB(255,255,255) : iter->second;
      image.get_CvMat_().at<PixelRGB>(row,col) = color;
    }
  }
}

TEST(ColorClassifier, LabelImages)
{
  ImageRGB inputImage, outputImage;

  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(
      Anki::Util::Data::Scope::Resources,
      "test/brightColorTests/");

  std::vector<std::string> trainingImageFilenames;
  std::vector<std::string> trainingLabelFilenames;
  {
    bool useFullPath = true;
    bool recursive = true;
    trainingImageFilenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"jpg",recursive);
    trainingLabelFilenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"json",recursive);
    std::sort(trainingImageFilenames.begin(), trainingImageFilenames.end());
    std::sort(trainingLabelFilenames.begin(), trainingLabelFilenames.end());
    ASSERT_EQ((int)trainingImageFilenames.size(), (int)trainingLabelFilenames.size());
  }

  std::unordered_map<std::string, cv::Mat> inputSamples;
  for (auto index = 0; index < trainingImageFilenames.size(); ++index)
  {
    const std::string& imageFilename = trainingImageFilenames[index];
    const std::string& labelFilename = trainingLabelFilenames[index];

    ASSERT_EQ(inputImage.Load(imageFilename), Anki::Result::RESULT_OK);

    std::unordered_map<std::string,std::vector<Anki::Rectangle<s32>>> labels;
    bool loaded = LoadLabels(labelFilename, labels);
    if (!loaded || labels.empty())
    {
      // Skip the ones without labels
      continue;
    }

    // Get the samples for all the label regions
    for (auto& kv : labels)
    {
      const std::string& label = kv.first;
      std::vector<Anki::Rectangle<s32>>& regions = kv.second;

      // Add stats if they don't exist
      auto iter = inputSamples.find(label);
      if (iter == inputSamples.end())
      {
        inputSamples.emplace(label,std::vector<cv::Vec3f>());
      }

      for (auto& region : regions)
      {
        cv::Mat samples;
        ImageToSamples(inputImage.GetROI(region), samples);
        if (inputSamples[label].empty())
        {
          inputSamples[label] = samples;
        }
        else
        {
          cv::Mat tmp;
          cv::vconcat(inputSamples[label], samples, tmp);
          inputSamples[label] = tmp;
        }
      }
    }
  }

  std::vector<std::string> labels;
  std::transform(inputSamples.begin(), inputSamples.end(), std::back_inserter(labels),
                 [](const auto& pair){ return pair.first; });

  //-------------
  //- TRANSFORM -
  //-------------

  // Transform the training data into how we want to use it with the classifier
  // At this point it is an Nx3 matrix where each column is R,G,B floats in scale 0,255
  // Make sure to do the same transformation to the samples during testing


  //------------
  //- TRAINING -
  //------------

  // Train the classifier

  MyClassifier classifier;
  {
    std::vector<cv::Mat> trainingSamples;
    for (auto& label : labels)
    {
      trainingSamples.push_back(inputSamples[label]);
    }
    classifier.Train(trainingSamples);
  }

  classifier.Save("/tmp/myclassifier.json");

  //-----------
  //- TESTING -
  //-----------

  // TODO: Get colors-to-labels map from somewhere
  std::unordered_map<std::string, PixelRGB> colors = {
      { "UNKNOWN", PixelRGB(0,0,0) },
      { "red", PixelRGB(255,0,0) },
      { "green", PixelRGB(0,255,0) },
      { "blue", PixelRGB(0,0,255) }
  };

  // TODO: Get separate test data from a different directory
  std::vector<std::string> testImageFilenames = trainingImageFilenames;

  std::string windowName = "Color Classifier";

  for (auto index = 0; index < trainingImageFilenames.size(); ++index)
  {
    const std::string& imageFilename = trainingImageFilenames[index];

    ASSERT_EQ(inputImage.Load(imageFilename), Anki::Result::RESULT_OK);
    inputImage.Resize(0.5f);
    inputImage.CopyTo(outputImage);

    cv::Mat samples;
    ImageToSamples(inputImage, samples);

    cv::Mat labelings(samples.rows,1,CV_32SC1);
    classifier.Classify(samples,labelings);

    labelings = labelings.reshape(1,{inputImage.GetNumRows(), inputImage.GetNumCols()});

    ColorLabelImage(labelings, labels, colors, outputImage);

    inputImage.Display("Input Image", 1);
    outputImage.Display("Color Classifier", 0);
  }
}


