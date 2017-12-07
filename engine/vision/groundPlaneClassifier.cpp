/**
 * File: GroundPlaneClassifier.cpp
 *
 * Author: Lorenzo Riano
 * Created: 11/29/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "groundPlaneClassifier.h"

#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/logisticRegression.h" // TODO this is temporary only for calculateError
#include "basestation/utils/data/dataPlatform.h"
#include "engine/cozmoContext.h"
#include "engine/groundPlaneROI.h"
#include "engine/overheadEdge.h"
#include "util/fileUtils/fileUtils.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#define DEBUG_DISPLAY_IMAGES true

namespace Anki {
namespace Cozmo {

GroundPlaneClassifier::GroundPlaneClassifier(const Json::Value& config, const CozmoContext *context)
: _context(context)
{

  DEV_ASSERT(context != nullptr, "GroundPlaneClassifier.ContextCantBeNULL");

  // TODO the classifier should be loaded from a file, not created here
  {
    _classifier.reset(new DTDrivingSurfaceClassifier(config, context));
    const std::string path = _context->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                         "config/vision/groundclassification");
//    const std::string path = "/Users/lorenzori/tmp/images_training/simulated_images";
    PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.pathToData", "the path to data is: %s",
                    path.c_str());

    const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
    const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

    bool result = _classifier->TrainFromFiles(positivePath.c_str(), negativePath.c_str());

    DEV_ASSERT(result, "GroundPlaneClassifier.TrainingFromFiles");

    // Error on whole training set
    {
      cv::Mat trainingSamples, trainingLabels;
      _classifier->GetTrainingData(trainingSamples, trainingLabels);

      // building a std::vector<Vision::PixelRGB>
      std::vector<Anki::Vision::PixelRGB> pixels;
      {
        pixels.reserve(trainingSamples.rows);
        const cv::Vec3f* data = trainingSamples.ptr<cv::Vec3f>(0);
        for (int i = 0; i < trainingSamples.rows; ++i) {
          const cv::Vec3f& vec = data[i];
          pixels.emplace_back(vec[0], vec[1], vec[2]);
        }
      }

      // calculate error
      std::vector<uchar> responses = _classifier->PredictClass(pixels);
      cv::Mat responsesMat(responses);

      const float error = Anki::calculateError(responsesMat, trainingLabels);
      PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.Train.ErrorLevel", "Error after training is: %f", error);
      DEV_ASSERT(error < 0.1, "GroundPLaneClassifier.TrainingErrorTooHigh");
    }

  }
}

Result GroundPlaneClassifier::Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                                     DebugImageList <Vision::ImageRGB>& debugImageRGBs,
                                     std::list<OverheadEdgeFrame>& outEdges)
{

  // nothing to do here if there's no ground plane visible
  if (! poseData.groundPlaneVisible) {
    PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.Update.GroundPlane", "Ground plane is not visible");
    return RESULT_OK;
  }

  // TODO Probably STEP 1 and 2 and 3 can be combined in a single pass

  // STEP 1: Obtain the overhead ground plane image
  GroundPlaneROI groundPlaneROI;
  const Matrix_3x3f& H = poseData.groundPlaneHomography;
  const Vision::ImageRGB groundPlaneImage = groundPlaneROI.GetOverheadImage(image, H);

  // STEP 2: Classify the overhead image
  Vision::Image classifiedMask(groundPlaneImage.GetNumRows(), groundPlaneImage.GetNumCols(), u8(0));
  _classifier->classifyImage(groundPlaneImage, classifiedMask);

  // STEP 3: Find leading edge in the classified mask (i.e. closest edge of obstacle to robot)
  OverheadEdgeFrame edgeFrame;
  edgeFrame.timestamp = image.GetTimestamp();
  edgeFrame.groundPlaneValid = true;
  OverheadEdgeChainVector& candidateChains = edgeFrame.chains;
  OverheadEdgePoint edgePoint;
  const Vision::Image& overheadMask = groundPlaneROI.GetOverheadMask();
  for(s32 i=0; i<classifiedMask.GetNumRows(); ++i)
  {
    const u8* overheadMask_i   = overheadMask.GetRow(i);
    const u8* classifiedMask_i = classifiedMask.GetRow(i);
    
    // Walk along the row and look for the first transition from not
    // TODO: this is not gonna work in the presence of heavy noise. Will need to clean up the classification with post-processing first.
    const Point2f& overheadOrigin = groundPlaneROI.GetOverheadImageOrigin();
    for(s32 j=1; j<classifiedMask.GetNumCols(); ++j)
    {
      // *Both* pixels must be *in* the overhead mask so we don't get leading edges of the boundary of
      // the ground plane quad itself!
      const bool bothInMask = ((overheadMask_i[j-1] > 0) && (overheadMask_i[j] > 0));
      if(bothInMask)
      {
        // To be a leading edge of an obstacle according to the classifier, first pixel should be "on" (drivable) and
        // second pixel should be "off" (not drivable)
        const bool isLeadingEdge = ((classifiedMask_i[j-1]>0) && (classifiedMask_i[j]==0));
        if(isLeadingEdge)
        {
          // Note that rows in ground plane image are robot y, and cols are robot x.
          // Just need to offset them to the right origin.
          edgePoint.position.x() = static_cast<f32>(j) + overheadOrigin.x();
          edgePoint.position.y() = -1*(static_cast<f32>(i) + overheadOrigin.y());
          edgePoint.gradient = 0.f; // TODO: Do we need the gradient for anything?
          
          candidateChains.AddEdgePoint(edgePoint, true);
          
          // No need to keep looking at this row as soon as a leading edge is found
          break;
        }
      }
    }
  }
  
  const u32 kMinChainLength_mm = 5;
  candidateChains.RemoveChainsShorterThan(kMinChainLength_mm);
  
  if(DEBUG_DISPLAY_IMAGES)
  {
    debugImageRGBs.emplace_back("OverheadImage", groundPlaneImage);
    
    Vision::ImageRGB leadingEdgeDisp(classifiedMask);
    
    static const std::vector<ColorRGBA> lineColorList = {
      NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
      NamedColors::ORANGE, NamedColors::CYAN, NamedColors::YELLOW,
    };
    auto color = lineColorList.begin();
    
    const Point2f& overheadOrigin = groundPlaneROI.GetOverheadImageOrigin();
    
    for (const auto &chain : candidateChains.GetVector())
    {
      // Draw line segments between all pairs of points in this chain
      Anki::Point2f startPoint(chain.points[0].position);
      startPoint -= overheadOrigin;
      
      for (s32 i = 1; i < chain.points.size(); ++i) {
        Anki::Point2f endPoint(chain.points[i].position);
        endPoint -= overheadOrigin;
        
        leadingEdgeDisp.DrawLine(startPoint, endPoint, *color, 3);
        std::swap(endPoint, startPoint);
      }
      
      // Switch colors for next segment
      ++color;
      if (color == lineColorList.end()) {
        color = lineColorList.begin();
      }
    }
    
    debugImageRGBs.emplace_back("LeadingEdges", std::move(leadingEdgeDisp));
  
    // Draw Ground plane on the image and display it
    Vision::ImageRGB toDisplay;
    image.CopyTo(toDisplay);
    Quad2f quad;
    if (poseData.groundPlaneVisible) {
      groundPlaneROI.GetImageQuad(H,
                                  toDisplay.GetNumCols(),
                                  toDisplay.GetNumRows(),
                                  quad);

      toDisplay.DrawQuad(quad, NamedColors::WHITE, 3);
      debugImageRGBs.emplace_back("GroundQuadImage", toDisplay);
    }
  }

  // Actually return the resulting edges in the provided list
  outEdges.emplace_back(std::move(edgeFrame));
  
  return RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
