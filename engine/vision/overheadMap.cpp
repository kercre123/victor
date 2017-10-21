/**
 * File: overHeadMap.cpp
 *
 * Author: Lorenzo Riano
 * Created: 10/9/17
 *
 * Description: This class maintains an overhead map of the robot's surrounding. The map has images taken
 * from the ground plane in front of the robot.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/



#include "overheadMap.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/quad_impl.h" // include this to avoid linking error in android
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

#define DEBUG_VISUALIZE false

namespace {

const char* kLogChannelName = "VisionSystem";

// In some circumstances the bounding box is slightly larger than the one returned by stock openCV.
// Need to test this function, I'm not sure it's working 100%
__attribute__((used)) cv::Rect OptimizedBoundingBox(const cv::RotatedRect& rect)
{

  float degree = rect.angle*3.1415/180;
  float majorAxe = rect.size.width/2;
  float minorAxe = rect.size.height/2;
  float x = rect.center.x;
  float y = rect.center.y;
  float c_degree = cos(degree);
  float s_degree = sin(degree);
  float t1 = atan(-(majorAxe*s_degree)/(minorAxe*c_degree));
  float c_t1 = cos(t1);
  float s_t1 = sin(t1);
  float w1 = majorAxe*c_t1*c_degree;
  float w2 = minorAxe*s_t1*s_degree;
  float maxX = x + w1-w2;
  float minX = x - w1+w2;

  t1 = atan((minorAxe*c_degree)/(majorAxe*s_degree));
  c_t1 = cos(t1);
  s_t1 = sin(t1);
  w1 = minorAxe*s_t1*c_degree;
  w2 = majorAxe*c_t1*s_degree;
  float maxY = y + w1+w2;
  float minY = y - w1-w2;
  if (minY > maxY)
  {
    float temp = minY;
    minY = maxY;
    maxY = temp;
  }
  if (minX > maxX)
  {
    float temp = minX;
    minX = maxX;
    maxX = temp;
  }
  cv::Rect r(minX,minY,maxX-minX+1,maxY-minY+1);
  return r;
}

} // namespace


OverheadMap::OverheadMap(int numrows, int numcols)
: _overheadMap(numrows, numcols)
{

}

OverheadMap::OverheadMap(const Json::Value& config)
{
  int numRows, numCols;
  if (! JsonTools::GetValueOptional(config, "NumRows", numRows)) {
    PRINT_NAMED_ERROR("OverheadMap.MissingJsonParameter", "Size");
    return;
  }
  if (! JsonTools::GetValueOptional(config, "NumCols", numCols)) {
    PRINT_NAMED_ERROR("OverheadMap.MissingJsonParameter", "Size");
    return;
  }

  _overheadMap.Allocate(numRows, numCols);
}


Result OverheadMap::Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                           DebugImageList <Vision::ImageRGB>& debugImageRGBs)
{

  // nothing to do here if there's no ground plane visible
  if (! poseData.groundPlaneVisible) {
    PRINT_CH_DEBUG(kLogChannelName, "OverheadMap.Update.Groundplane", "Ground plane is not visible");
    return RESULT_OK;
  }

  const Matrix_3x3f& H = poseData.groundPlaneHomography;

  const GroundPlaneROI& roi = poseData.groundPlaneROI;

  Quad2f imgGroundQuad;
  roi.GetImageQuad(H, image.GetNumCols(), image.GetNumRows(), imgGroundQuad);

  // For each point on the ground plane, project it back into the image to get the color
  // Then add it to the overhead image
  s32 pointsInMap = 0;
  for(s32 i=0; i<roi.GetWidthFar(); ++i) {
    const u8* mask_i = roi.GetOverheadMask().GetRow(i);
    // i is an index, but given that in the overhead mask 1 pixel = 1mm, i is also a distance
    const f32 ground_y = static_cast<f32>(i) - 0.5f*roi.GetWidthFar(); // Zero is at the center
    for(s32 j=0; j<roi.GetLength(); ++j) {

      if(mask_i[j] == 0) { // skip if not in the mask
        continue;
      }

      // Project ground plane point in robot frame to image
      const f32 ground_x = static_cast<f32>(j) + roi.GetDist();
      Point3f imgPoint = H * Point3f(ground_x,ground_y,1.f);
      DEV_ASSERT(imgPoint.z() > 0.f, "OverheadMap.Update.NegativePointZ");
      const f32 divisor = 1.f / imgPoint.z();
      imgPoint.x() *= divisor;
      imgPoint.y() *= divisor;
      const s32 x_img = (s32)std::round(imgPoint.x());
      const s32 y_img = (s32)std::round(imgPoint.y());

      // if the ground point doesn't fall beyond the image plane
      if(x_img >= 0 && y_img >= 0 &&
         x_img < image.GetNumCols() && y_img < image.GetNumRows())
      {
        const Vision::PixelRGB value = image(y_img, x_img);

        // Get corresponding map point in world coords
        Point3f mapPoint = poseData.histState.GetPose() * Point3f(ground_x,ground_y,0.f);
        // World map is assumed to have the origin of the world in the center, hence (rows/2, cols/2)
        const s32 x_map = (s32)std::round( mapPoint.x() + static_cast<f32>(_overheadMap.GetNumCols())*0.5f);
        const s32 y_map = (s32)std::round(-mapPoint.y() + static_cast<f32>(_overheadMap.GetNumRows())*0.5f);
        if(x_map >= 0 && y_map >= 0 &&
           x_map < _overheadMap.GetNumCols() && y_map < _overheadMap.GetNumRows())
        {
          pointsInMap++;

          // Replace the old value here rather than blending,
          // want to make sure the map is up-to-date and it doesn't have spurious values
          _overheadMap(y_map, x_map) = value;
        }
      }
    }
  }
  PRINT_CH_DEBUG(kLogChannelName, "OverheadMap.Update.UpdatedPixels", "Number of pixels updated in overhead map: %c, "
                 "number of pixels outside: %d",
                 pointsInMap, int(imgGroundQuad.ComputeArea()) - pointsInMap);

  // only update here for debug and sending the images over
  if (DEBUG_VISUALIZE) {
    const Vision::ImageRGB robotFootPrint = GetImageCenteredOnRobot(poseData.histState.GetPose(), debugImageRGBs);
    debugImageRGBs.emplace_back("RobotFootprint", robotFootPrint);
  }

  _overheadMap.SetTimestamp(poseData.timeStamp);
  return RESULT_OK;
}

Vision::ImageRGB OverheadMap::GetImageCenteredOnRobot(const Pose3d& robotPose,
                                                      DebugImageList<Anki::Vision::ImageRGB>& debugImageRGBs) const
{

  // To extract the pixels underneath the robot, the (cropped) overhead map is rotated by the
  // the current robot angle and then the region underneath the footprint is extracted

  cv::RotatedRect footprintRect;
  // Calculate the footprint as a cv::RotatedRect. To get the correct size the footprint is aligned
  // with the axis
  {
    // save the rotation angle, as we need to zero it in the footprint
    // the angle needs to be flipped here. Only Euclid knows why
    const Radians R( - robotPose.GetRotation().GetAngleAroundZaxis());

    Pose3d alignedPose(robotPose);
    // set rotation to 0, i.e. aligned with the axis
    alignedPose.SetRotation(0, {1, 0, 0});
    // get the footprint aligned with the coordinate system
    Quad2f robotFootprint = Robot::GetBoundingQuadXY(alignedPose);

    // The robot footprint is not centered on the robot centroid, but the center of the front axis :(
    robotFootprint += Point2f(16.9f * std::cos(R.ToFloat()), 16.9f * std::sin(R.ToFloat()));

    // Move the footprint to the image coordinate system, with the center in top left and the y axis pointing down
    for (auto& point: robotFootprint) {
      point.x() = point.x() + static_cast<f32>(_overheadMap.GetNumCols()) * 0.5f;
      point.y() = -point.y() + static_cast<f32>(_overheadMap.GetNumRows()) * 0.5f;
    }

    // get the data from the robot footprint
    s32 robotMinX = s32(std::round(robotFootprint.GetMinX()));
    Util::Clamp(robotMinX, 0, _overheadMap.GetNumCols());
    s32 robotMaxX = s32(std::round(robotFootprint.GetMaxX()));
    Util::Clamp(robotMinX, 0, _overheadMap.GetNumCols());
    s32 robotMinY = s32(std::round(robotFootprint.GetMinY()));
    Util::Clamp(robotMinX, 0, _overheadMap.GetNumCols());
    s32 robotMaxY = s32(std::round(robotFootprint.GetMaxY()));
    Util::Clamp(robotMinX, 0, _overheadMap.GetNumCols());

    const Point2f centroid = robotFootprint.ComputeCentroid();
    // This is the robot footprint with the right angle
    footprintRect = cv::RotatedRect(cv::Point2f(centroid.x(), centroid.y()),
                                    cv::Size2f(robotMaxX - robotMinX, robotMaxY - robotMinY),
                                    R.getDegrees());
  }

  // get the image to rotate
  const cv::Mat& src = _overheadMap.get_CvMat_();
  // take only the region that would be rotate
  const cv::Rect boundingRect = footprintRect.boundingRect();
  cv::Mat croppedOverhead = src(boundingRect);
  // create the rotation matirx
  float angle = footprintRect.angle;
  // With the cropped image the center of rotation is the center of the image
  cv::Mat rotatationMatrix = cv::getRotationMatrix2D({float(croppedOverhead.cols)/2.0f,
                                                      float(croppedOverhead.rows)/2.0f},
                                                     angle, 1.0);

  cv::Mat rotatedOverhead;
  // rotate the image
  try {
    cv::warpAffine(croppedOverhead, rotatedOverhead, rotatationMatrix, croppedOverhead.size(), cv::INTER_LINEAR);
  }
  catch (cv::Exception& e) {
    PRINT_NAMED_ERROR("OverheadMap.GetImageCenteredOnRobot.ErrorOnWarp"," Error while warping image: %s",
                      e.what());
    return Vision::ImageRGB();
  }

  const cv::Size footprintSize = footprintRect.size;
  // crop the resulting image
  Vision::ImageRGB toRet;
  cv::getRectSubPix(rotatedOverhead, footprintSize,
                    {float(rotatedOverhead.cols)*0.5f, float(rotatedOverhead.rows)*0.5f}, // since we cropped the image take the center
                    toRet.get_CvMat_());

  if (toRet.GetNumCols() == 0 || toRet.GetNumRows()==0) {
    PRINT_NAMED_ERROR("OverheadMap.GetImageCenteredOnRobot.EmptyImage","Error: result image has %d rows and %d cols",
                      toRet.GetNumRows(), toRet.GetNumCols());
    return Vision::ImageRGB();
  }

  if (DEBUG_VISUALIZE)
  {
    Vision::ImageRGB toDisplay;
    _overheadMap.CopyTo(toDisplay);
    // create the footprint visualization
    {
      cv::Point2f vertices[4];
      footprintRect.points(vertices);
      for (int i = 0; i < 4; i++) {
        const Point2f start(vertices[i]);
        const Point2f end(vertices[(i + 1) % 4]);
        toDisplay.DrawLine(start, end, ColorRGBA(u8(0), u8(255), u8(0)));
      }
    }
    // create the bigger bounding box
    {
      const Rectangle<f32> rect(boundingRect);
      toDisplay.DrawRect(rect, ColorRGBA(u8(0), u8(0), u8(255)));
    }
    debugImageRGBs.emplace_back("OverheadMap", toDisplay);
  }

  return toRet;

  // Note: see https://stackoverflow.com/a/13152420/1047543 to calculate the points in the rotated rectangle
  // without having to do the image. This approach allows for visualization though

}

const Vision::ImageRGB& OverheadMap::GetOverheadMap() const
{
  return _overheadMap;
}

} //namespace Cozmo
} //namespace Anki
