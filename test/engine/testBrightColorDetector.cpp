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

    // Rotate the image color
    if (!rotated)
    {
#if 0
      std::cerr<<"White is already on axis"<<std::endl;
#endif
    }

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

    // Adjust the white balance of the original image based on what we know 'white' to be
  }
}
