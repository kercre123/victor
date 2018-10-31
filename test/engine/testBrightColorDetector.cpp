/*
 * File: testBrightColorDetector.cpp
 *
 *  Author: Patrick Doran
 *  Created: 2018-10-30
 */

#include "gtest/gtest.h"

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/brightColorDetector.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/point_impl.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"

#include "util/fileUtils/fileUtils.h"

#include <opencv2/highgui.hpp>

using Anki::Vision::BrightColorDetector;
using Anki::Vision::Camera;
using Anki::Vision::ImageRGB;
using Anki::Vision::PixelHSV;
using Anki::Vision::PixelRGB;
using Anki::Vision::SalientPoint;
using Anki::ColorRGBA;
using Anki::Poly2f;
using Anki::Point2f;
using Anki::CladPoint2d;

extern Anki::Vector::CozmoContext* cozmoContext;

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
        topLeft.x() = iterA->x * outputImage.GetNumCols();
        auto iterB = std::min_element(salientPoint.shape.begin(),salientPoint.shape.end(),LessByY);
        topLeft.y() = iterB->y * outputImage.GetNumRows();
      }

      // Draw polygons
      {
        Poly2f poly(salientPoint.shape);
        for (u32 i = 0; i < poly.size(); ++i){
          poly[i].x() *= outputImage.GetNumCols();
          poly[i].y() *= outputImage.GetNumRows();
        }
        outputImage.DrawPoly(poly, ColorRGBA(salientPoint.color_rgba), 3, true);
      }


      // Draw text
      {
        ColorRGBA rgba(salientPoint.color_rgba);
        PixelHSV hsv(PixelRGB(rgba.r(),rgba.g(),rgba.b()));
        Point2f center(salientPoint.x_img*outputImage.GetNumCols(), salientPoint.y_img*outputImage.GetNumRows());
        {
          std::stringstream ss;
          ss << "Hue:   "<< static_cast<s32>(std::roundf(hsv.h() * 360));
          std::string text = ss.str();
          Point2f location = topLeft;
          location.y() += textHeight;
          outputImage.DrawText(location,text,0xFFFFFFFF,textScale,false,textThickness,false);
        }
        {
          std::stringstream ss;
          ss << "Score: "<<static_cast<s32>(std::roundf(salientPoint.score));
          std::string text = ss.str();
          Point2f location = topLeft;
          location.y() += 2*textHeight;
          outputImage.DrawText(location,text,0xFFFFFFFF,textScale,false,textThickness,false);
        }

      }
    }
    outputImage.Display(windowName.c_str(), 0);
  }
}
