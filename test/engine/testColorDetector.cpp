/**
 * File: testColorDetector.cpp
 *
 * Author: Patrick Doran
 * Date: 2018/11/13
 */

#include "gtest/gtest.h"


#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/colorDetector.h"
#include "coretech/vision/engine/image.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"

#include "util/fileUtils/fileUtils.h"

#include "json/json.h"

using Anki::Vision::ColorDetector;
using Anki::Vision::ImageRGB;
using Anki::Vision::PixelRGB;
using Anki::Vision::SalientPoint;
using Anki::Poly2f;
using Anki::Point2f;
using Anki::ColorRGBA;

extern Anki::Vector::CozmoContext* cozmoContext;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(ColorDetector, LabelImages)
{
  // Load the configuration
  Json::Value config;
  ASSERT_TRUE(cozmoContext->GetDataPlatform()->readAsJson(Anki::Util::Data::Scope::Resources,
      "config/engine/vision/colorDetector/colorDetector.json",
      config));

  ColorDetector detector(config);
  ASSERT_EQ(detector.Init(), Anki::Result::RESULT_OK);

  // Find the test images
  ImageRGB inputImage, outputImage;
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(
      Anki::Util::Data::Scope::Resources,
      "test/colorTests/test");

  std::vector<std::string> filenames;
  {
    bool useFullPath = true;
    bool recursive = true;
    filenames = Anki::Util::FileUtils::FilesInDirectory(dataPath,useFullPath,"jpg",recursive);
  }
  std::sort(filenames.begin(), filenames.end());

  for (const auto& filename : filenames){
    ASSERT_EQ(inputImage.Load(filename), Anki::Result::RESULT_OK);
    inputImage.CopyTo(outputImage);

    std::list<std::pair<std::string,ImageRGB>> debugImageRGBs;
    std::list<SalientPoint> salientPoints;

    Anki::Result result = detector.Detect(inputImage, salientPoints, debugImageRGBs);

    ASSERT_EQ(result, Anki::Result::RESULT_OK);
    for (auto& dbg : debugImageRGBs)
    {
      dbg.second.Display(dbg.first.c_str(), 1);
    }
    for (const auto& salientPoint : salientPoints)
    {
      Poly2f poly;
      for (const auto& pt : salientPoint.shape)
      {
        poly.emplace_back({pt.x*inputImage.GetNumCols(),pt.y*inputImage.GetNumRows()});
      }
      inputImage.DrawPoly(poly, ColorRGBA(salientPoint.color_rgba), 2, true);
    }

    inputImage.Display("Input", 0);
  }
}
