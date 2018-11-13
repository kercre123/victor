/*
 * File: testBrightColorDetector.cpp
 *
 *  Author: Patrick Doran
 *  Created: 2018-10-30
 */

#include "gtest/gtest.h"

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/brightColorDetector.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/markerDetector.h"
#include "coretech/vision/engine/visionMarker.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"

#include "util/fileUtils/fileUtils.h"

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>

#include "json/json.h"

#include <fstream>

using Anki::CladPoint2d;
using Anki::ColorRGBA;
using Anki::Point2f;
using Anki::Poly2f;
using Anki::Quad2f;
using Anki::Vision::BrightColorDetector;
using Anki::Vision::Camera;
using Anki::Vision::Image;
using Anki::Vision::ImageRGB;
using Anki::Vision::MarkerDetector;
using Anki::Vision::ObservedMarker;
using Anki::Vision::PixelHSV;
using Anki::Vision::PixelRGB;
using Anki::Vision::SalientPoint;

extern Anki::Vector::CozmoContext* cozmoContext;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Struct for containing label information
 */
struct LabelRegion
{
  std::string label;
  Anki::Rectangle<s32> rectangle;
};

struct LabelStats
{
  std::string label;
  std::vector<cv::Vec3f> samples;
  cv::Vec3f mean;
  cv::Matx33f cov;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Helper function for drawing debugging output
 */
void drawSalientPoints(const std::list<SalientPoint>& salientPoints, ImageRGB& image)
{
  // Get text height
  f32 textScale = 0.5f;
  int textThickness = 1;
  f32 textHeight = 1.1*cv::getTextSize("hello", CV_FONT_NORMAL, textScale, textThickness, nullptr).height;

  auto LessByX = [](const CladPoint2d& lhs, const CladPoint2d& rhs)->bool{ return lhs.x < rhs.x; };
  auto LessByY = [](const CladPoint2d& lhs, const CladPoint2d& rhs)->bool{ return lhs.y < rhs.y; };
  for (const auto& salientPoint : salientPoints)
  {
    Point2f topLeft;
    {
      auto iterA = std::min_element(salientPoint.shape.begin(),salientPoint.shape.end(),LessByX);
      topLeft.x() = iterA->x * image.GetNumCols();
      auto iterB = std::min_element(salientPoint.shape.begin(),salientPoint.shape.end(),LessByY);
      topLeft.y() = iterB->y * image.GetNumRows();
    }

    // Draw polygons
    {
      Poly2f poly(salientPoint.shape);
      for (u32 i = 0; i < poly.size(); ++i){
        poly[i].x() *= image.GetNumCols();
        poly[i].y() *= image.GetNumRows();
      }
      image.DrawPoly(poly, ColorRGBA(salientPoint.color_rgba), 3, true);
    }

    // Draw text
    {
      ColorRGBA rgba(salientPoint.color_rgba);
      PixelHSV hsv(PixelRGB(rgba.r(),rgba.g(),rgba.b()));
      Point2f center(salientPoint.x_img*image.GetNumCols(), salientPoint.y_img*image.GetNumRows());
      {
        std::stringstream ss;
        ss << "Hue:   "<< static_cast<s32>(std::roundf(hsv.h() * 360));
        std::string text = ss.str();
        Point2f location = topLeft;
        location.y() += textHeight;
        image.DrawText(location,text,0xFFFFFFFF,textScale,false,textThickness,false);
      }
      {
        std::stringstream ss;
        ss << "Score: "<<static_cast<s32>(std::roundf(salientPoint.score));
        std::string text = ss.str();
        Point2f location = topLeft;
        location.y() += 2*textHeight;
        image.DrawText(location,text,0xFFFFFFFF,textScale,false,textThickness,false);
      }

    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Get the white value from a marker
 */
PixelRGB GetWhite(const ImageRGB& inputImage, MarkerDetector& markerDetector)
{
  Image grayImage = inputImage.ToGray();

  // Scale the image down because we'll run out of memory otherwise
  grayImage.Resize(0.5,Anki::Vision::ResizeMethod::Cubic);

  std::list<ObservedMarker> observedMarkers;
  markerDetector.Detect(grayImage,observedMarkers);
  if (observedMarkers.size() != 1)
  {
    // Skip any images with 0 or multiple markers; we want exactly 1
    return PixelRGB();
  }

  ObservedMarker marker = observedMarkers.front();

  // Get white pixels
  PixelRGB white;
  {
    cv::Size size(100,100);
    ImageRGB extractedImage(size.height,size.width);

    Quad2f quad = marker.GetImageCorners();
    // Scale the coordinates back to the input size
    std::vector<cv::Point2f> src {
      cv::Point2f(quad.GetTopLeft().x()*2, quad.GetTopLeft().y()*2),
      cv::Point2f(quad.GetTopRight().x()*2, quad.GetTopRight().y()*2),
      cv::Point2f(quad.GetBottomRight().x()*2, quad.GetBottomRight().y()*2)
    };
    std::vector<cv::Point2f> dst {
      cv::Point2f(0,0),
      cv::Point2f(size.width,0),
      cv::Point2f(size.width,size.height),
    };
    cv::Mat xform = cv::getAffineTransform(src, dst);

    cv::warpAffine(inputImage.get_CvMat_(), extractedImage.get_CvMat_(), xform, size);

    // TODO: Get the white/black by modifying the marker type to include it
    std::vector<PixelRGB> whiteSamples = {
      extractedImage.get_CvMat_()(50,5),
      extractedImage.get_CvMat_()(5,50),
      extractedImage.get_CvMat_()(95,50),
      extractedImage.get_CvMat_()(50,95)
    };
    cv::Scalar meanSample = cv::mean(whiteSamples);
    white = PixelRGB(meanSample.val[0], meanSample.val[1], meanSample.val[2]);

//    extractedImage.Display("Extracted Marker", 0);
  }
  return white;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Rotate the image colors such that reference white and neutral white are aligned.
 * @details Given a reference white from the image, compute the rotation between neutral white and the reference white.
 * Then apply this rotation to all the RGB pixels. This effectively undoes the white balance from gains in a post
 * processing step as an additional white balancing.
 */
bool RotateImageOntoWhite(const ImageRGB& inputImage, const PixelRGB& white, ImageRGB& outputImage)
{
  inputImage.CopyTo(outputImage);

  cv::Vec3f neutral(1,1,1);
  neutral /= cv::norm(neutral);
  cv::Vec3f measured(white.r(),white.g(),white.b());
  measured /= cv::norm(measured);

  // If dot product of two vectors is 1 then the cross product is 0
  // Divide by zero protection. This means that white is already aligned with the netural white.
  // Examples of this happening are even white balance or the white being completely blown out.
  if (Anki::Util::IsNear(measured.dot(neutral),1))
    return false;


  cv::Vec3f axis = measured.cross(neutral);
  cv::Vec3f dbgAxis = axis;
  axis /= cv::norm(axis);
  float angle = acosf(measured.dot(neutral));

  cv::Matx33f rotation;
  cv::Rodrigues(angle*axis, rotation);

#if 0
  std::cerr<<"White:      "<<static_cast<u32>(white.r())<<", "
                           <<static_cast<u32>(white.g())<<", "
                           <<static_cast<u32>(white.b())<<std::endl;
  std::cerr<<"Normalized: "<<measured<<std::endl;
  std::cerr<<"Angle:      "<<angle<<std::endl;
  std::cerr<<"Axis:       "<<axis<<std::endl;
  std::cerr<<"Axis (dbg): "<<dbgAxis<<std::endl;
  std::cerr<<"Rotation:   "<<std::endl<<rotation<<std::endl;
#endif

  cv::Mat_<PixelRGB>& image = outputImage.get_CvMat_();
  for (s32 row = 0; row < outputImage.GetNumRows(); ++row){
    for (s32 col = 0; col < outputImage.GetNumCols(); ++col){
      PixelRGB pixel = image(row,col);
      cv::Matx31f rgb(pixel.r(),pixel.g(),pixel.b());
      rgb = rotation * rgb;
      pixel.r() = static_cast<u8>(Anki::Util::Clamp(roundf(rgb(0)), 0.f, 255.f));
      pixel.g() = static_cast<u8>(Anki::Util::Clamp(roundf(rgb(1)), 0.f, 255.f));
      pixel.b() = static_cast<u8>(Anki::Util::Clamp(roundf(rgb(2)), 0.f, 255.f));
      image(row,col) = pixel;
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Load labels from a Labelme json file
 */
bool LoadLabels(const std::string& filename, std::unordered_map<std::string,std::vector<LabelRegion>>& labels)
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
    LabelRegion newLabel;

    Json::Value shape = shapes[ii];

    Json::Value type = shape.get("shape_type",Json::Value::null);
    if (type.isNull() || !type.isString() || type.asString() != "rectangle")
    {
      continue;
    }

    Json::Value label = shape.get("label",Json::Value::null);
    if (label.isNull() || !label.isString())
    {
      continue;
    }
    newLabel.label = label.asString();

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

    newLabel.rectangle = Anki::Rectangle<s32>(x0, y0, x1-x0, y1-y0);

    auto iter = labels.find(newLabel.label);
    if (iter == labels.end())
    {
      labels.emplace(newLabel.label, std::vector<LabelRegion>());
    }
    labels[newLabel.label].push_back(newLabel);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
TEST(BrightColorDetector, EvaluateImages)
{
  Camera camera;
  BrightColorDetector detector(camera);
  ASSERT_EQ(detector.Init(), Anki::Result::RESULT_OK);

  ImageRGB inputImage, outputImage;

  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(
      Anki::Util::Data::Scope::Resources,
      "test/brightColorTests/");

  std::vector<std::string> filenames;
  {
    bool useFullPath = true;
    bool recursive = true;
    filenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"jpg",recursive);
  }
  std::sort(filenames.begin(), filenames.end());

  std::string windowName = "BrightColors";
  for (const auto& filename : filenames){
    ASSERT_EQ(inputImage.Load(filename), Anki::Result::RESULT_OK);
    inputImage.CopyTo(outputImage);

    std::list<SalientPoint> salientPoints;
    Anki::Result result = detector.Detect(inputImage, salientPoints);

    ASSERT_EQ(result, Anki::Result::RESULT_OK);
    drawSalientPoints(salientPoints,outputImage);
    outputImage.Display(windowName.c_str(), 0);
  }
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
TEST(BrightColorDetector, WhiteBalance)
{
  Camera camera;
  BrightColorDetector brightColorDetector(camera);
  MarkerDetector markerDetector(camera);
  ASSERT_EQ(brightColorDetector.Init(), Anki::Result::RESULT_OK);
  ASSERT_EQ(markerDetector.Init(1280,720), Anki::Result::RESULT_OK);

  ImageRGB inputImage, outputImage, colorRotatedImage;
  Image grayImage;

  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(
      Anki::Util::Data::Scope::Resources,
      "test/brightColorTests/");

  std::vector<std::string> filenames;
  {
    bool useFullPath = true;
    bool recursive = true;
    filenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"jpg",recursive);
  }
  std::sort(filenames.begin(), filenames.end());

  std::string windowName = "BrightColors with White Balance";
  for (const auto& filename : filenames){

    ASSERT_EQ(inputImage.Load(filename), Anki::Result::RESULT_OK);
    inputImage.CopyTo(outputImage);

    // Get the reference white color from the image
    PixelRGB white = GetWhite(inputImage, markerDetector);

    // Rotate a copy of the input image into the white direction
    bool rotated = RotateImageOntoWhite(inputImage, white, colorRotatedImage);

#if 0
    if (!rotated)
    {
      std::cerr<<"White is already on axis"<<std::endl;
    }
#endif

    inputImage.Display("Input Image", 0);
    colorRotatedImage.Display("Color Adjusted Image", 0);

    // Detect bright colors on the input image
    std::list<SalientPoint> originalImageSalientPoints;
    Anki::Result result = brightColorDetector.Detect(inputImage, originalImageSalientPoints);

    ASSERT_EQ(result, Anki::Result::RESULT_OK);

    std::list<SalientPoint> modifiedImageSalientPoints;
    result = brightColorDetector.Detect(colorRotatedImage, modifiedImageSalientPoints);

    ASSERT_EQ(result, Anki::Result::RESULT_OK);

    drawSalientPoints(originalImageSalientPoints, outputImage);
    outputImage.Display("Output Image", 0);
    drawSalientPoints(modifiedImageSalientPoints, colorRotatedImage);
    colorRotatedImage.Display("Color Adjusted Output Image", 0);
  }
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void GetSamples(const ImageRGB& image, LabelRegion& region, LabelStats& stats)
{
  ImageRGB subimage = image.GetROI(region.rectangle);
  for (int row = 0; row < subimage.GetNumRows(); ++row)
  {
    for (int col = 0; col < subimage.GetNumCols(); ++col)
    {
      PixelRGB& pixel = subimage(row,col);
      cv::Vec3f rgb(pixel.r() / 255.f, pixel.g() / 255.f, pixel.b() / 255.f);
      stats.samples.push_back(rgb);
    }
  }
}

void FillOutStats(LabelStats& stats)
{
  cv::Mat covar, mean;
  int flags = CV_COVAR_NORMAL | CV_COVAR_ROWS;
  int ctype = CV_32FC1;
  // TODO figure out why the channels don't work
  cv::calcCovarMatrix(cv::Mat((int)stats.samples.size(),3,CV_32F,stats.samples.data()), covar,mean,flags, ctype);

  stats.mean = mean;
  stats.cov = covar;

  cv::Mat w,u,vt;
  cv::SVD::compute(covar, w, u, vt, cv::SVD::FULL_UV);

  std::cerr<<"w: "<<std::endl<<w<<std::endl;
  std::cerr<<"u: "<<std::endl<<u<<std::endl;
  std::cerr<<"vt: "<<std::endl<<vt<<std::endl;
}

#if 1
TEST(BrightColorDetector, ColorStats)
{
  Camera camera;
  MarkerDetector markerDetector(camera);
  ASSERT_EQ(markerDetector.Init(1280,720), Anki::Result::RESULT_OK);

  ImageRGB inputImage, outputImage, colorRotatedImage;
  Image grayImage;

  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(
      Anki::Util::Data::Scope::Resources,
      "test/brightColorTests/");

  std::vector<std::string> imageFilenames;
  std::vector<std::string> labelFilenames;
  {
    bool useFullPath = true;
    bool recursive = true;
    imageFilenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"jpg",recursive);
    labelFilenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"json",recursive);
    std::sort(imageFilenames.begin(), imageFilenames.end());
    std::sort(labelFilenames.begin(), labelFilenames.end());
    ASSERT_EQ((int)imageFilenames.size(), (int)labelFilenames.size());
  }

  std::string windowName = "BrightColors with White Balance";
  std::unordered_map<std::string, LabelStats> inputImageStats;
  std::unordered_map<std::string, LabelStats> rotatedImageStats;

  for (auto index = 0; index < imageFilenames.size(); ++index)
  {
    const std::string& imageFilename = imageFilenames[index];
    const std::string& labelFilename = labelFilenames[index];

    ASSERT_EQ(inputImage.Load(imageFilename), Anki::Result::RESULT_OK);
    inputImage.CopyTo(outputImage);

    // Get the reference white color from the image
    PixelRGB white = GetWhite(inputImage, markerDetector);

    // Rotate a copy of the input image into the white direction
    bool rotated = RotateImageOntoWhite(inputImage, white, colorRotatedImage);
    if (!rotated)
    {
      // Skip ones that don't get rotated
      continue;
    }

    std::unordered_map<std::string,std::vector<LabelRegion>> labels;
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
      std::vector<LabelRegion>& regions = kv.second;

      // Add stats if they don't exist

      {
        auto iter = inputImageStats.find(label);
        if (iter == inputImageStats.end())
        {
          LabelStats stats;
          stats.label = label;
          inputImageStats.emplace(label,stats);
        }
      }

      {
        auto iter = rotatedImageStats.find(label);
        if (iter == rotatedImageStats.end())
        {
          LabelStats stats;
          stats.label = label;
          rotatedImageStats.emplace(label,stats);
        }
      }


      for (auto& region : regions)
      {
        GetSamples(inputImage,region,inputImageStats[label]);
        GetSamples(colorRotatedImage, region, rotatedImageStats[label]);
      }
    }
  }

  // We now have all the samples inside the image stats for each label


  // Collect all the labels in a list
  std::vector<std::string> labels;
  std::transform(inputImageStats.begin(), inputImageStats.end(), std::back_inserter(labels),
                 [](const auto& pair){ return pair.first; });

  for (const auto& label : labels)
  {
    // Input Image Stats
    {
      LabelStats& stats = inputImageStats.at(label);
      FillOutStats(stats);
    }

    // Rotated Image Stats
    {
      LabelStats& stats = rotatedImageStats.at(label);
      FillOutStats(stats);
    }
  }


  std::cerr<<"--- --- --- - Input - --- --- ---"<<std::endl;
  for (const auto& label : labels)
  {
    LabelStats& stats = inputImageStats.at(label);
    std::cerr<<"--- "<<stats.label<<" --- "<<std::endl
        <<"mean: "<<stats.mean<<std::endl
        <<"covar: "<<std::endl<<stats.cov<<std::endl;
  }

  std::cerr<<"--- --- --- - Rotated - --- --- ---"<<std::endl;
  for (const auto& label : labels)
  {
    LabelStats& stats = rotatedImageStats.at(label);
    std::cerr<<"--- "<<stats.label<<" --- "<<std::endl
        <<"mean: "<<stats.mean<<std::endl
        <<"covar: "<<std::endl<<stats.cov<<std::endl;
  }

#if 0
  cv::Mat rgb[] = {
      cv::Mat(subimage.GetNumRows(),subimage.GetNumCols(),CV_8U),
      cv::Mat(subimage.GetNumRows(),subimage.GetNumCols(),CV_8U),
      cv::Mat(subimage.GetNumRows(),subimage.GetNumCols(),CV_8U)
  };

  cv::split(subimage.get_CvMat_(), rgb);
  rgb[0].reshape(1);
  rgb[1].reshape(1);
  rgb[2].reshape(1);


  cv::Mat combined;
  cv::hconcat(rgb, 3, combined);

  cv::Mat covar(3,3,CV_32F);
  cv::Mat mean(3,1,CV_32F);
  int flags = CV_COVAR_NORMAL;

  cv::calcCovarMatrix(combined, covar, mean, flags, CV_32F);

  std::cerr<<"PDORAN "<<"GetStats"<<std::endl;
#endif
}

#endif
