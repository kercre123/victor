/**
 * File: nightVisionFilter.cpp
 *
 * Author: Humphrey Hu
 * Date:   2018-06-08
 *
 * Description: Helper class for averaging images together to minimize noise.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/vision/nightVisionFilter.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

const char* kMinAccImagesKey     = "MinNumImages";
const char* kHistSubsampleKey    = "HistSubsample";
const char* kBodyAngleThreshKey  = "BodyAngleThreshold";
const char* kBodyPoseThreshKey   = "BodyPoseThreshold";
const char* kHeadAngleThreshKey  = "HeadAngleThreshold";

u16 CastPixel(const u8& p) { return (u16) p; }

u8 DividePixel(const u16& p, const u16& count)
{
  u16 out = p / count;
  if( out > 255 ) { out = 255; }
  return static_cast<u8>( out );
}

u8 ScalePixel(u8 p, const f32& k)
{
  u16 pK = static_cast<u16>( p * k );
  if( pK > 255 ) { pK = 255; }
  return static_cast<u8>( pK );
}

NightVisionFilter::NightVisionFilter()
{
  Reset();
}

Result NightVisionFilter::Init( const Json::Value& config )
{
  #define PARSE_PARAM(conf, key, var) \
  if( !JsonTools::GetValueOptional( conf, key, var ) ) \
  { \
    PRINT_NAMED_ERROR( "NightVisionFilter.Init.MissingParameter", "Could not parse parameter: %s", key ); \
    return RESULT_FAIL; \
  }
  #define PARSE_PARAM_ANGLE(conf, key, var) \
  if( !JsonTools::GetAngleOptional( conf, key, var, true ) ) \
  { \
    PRINT_NAMED_ERROR( "NightVisionFilter.Init.MissingParameter", "Could not parse parameter: %s", key ); \
    return RESULT_FAIL; \
  }
  
  PARSE_PARAM( config, kMinAccImagesKey, _minNumImages );
  PARSE_PARAM( config, kHistSubsampleKey, _histSubsample );
  PARSE_PARAM_ANGLE( config, kBodyAngleThreshKey, _bodyAngleThresh );
  PARSE_PARAM( config, kBodyPoseThreshKey, _bodyPoseThresh );
  PARSE_PARAM_ANGLE( config, kHeadAngleThreshKey, _headAngleThresh );
  Reset();
  return RESULT_OK;
}

void NightVisionFilter::Reset()
{
  _numAccImages = 0;
}

void NightVisionFilter::AddImage( const Vision::Image& img, 
                                  const VisionPoseData& poseData )
{
  // If first image, allocate accumulator and reset
  if( _numAccImages == 0 )
  {
    _accumulator.Allocate( img.GetNumRows(), img.GetNumCols() );
    _accumulator.FillWith( 0 );
  }
  // Otherwise see if robot moved, since filter can only run when stationary
  else if( HasMoved( poseData ) )
  {
    Reset();
    return;
  }
  _lastPoseData = poseData;

  // Sanity check
  if( img.GetNumRows() != _accumulator.GetNumRows() ||
      img.GetNumCols() != _accumulator.GetNumCols() )
  {
    PRINT_NAMED_ERROR("NightVisionFilter.AddImage.SizeError", "");
    Reset();
    return;
  }

  const cv::Mat_<u8>& imgMat = img.get_CvMat_();
  cv::Mat_<u16>& accMat = _accumulator.get_CvMat_();
  cv::add( accMat, imgMat, accMat, cv::noArray(), CV_16U );

  _lastTimestamp = img.GetTimestamp();
  _numAccImages++;
}

bool NightVisionFilter::HasMoved( const VisionPoseData& poseData )
{
  // Some of these are not set to true if robot moved by human
  const bool robotMoved = poseData.histState.WasCameraMoving() ||
                          poseData.histState.WasPickedUp() ||
                          poseData.histState.WasLiftMoving();
  // Should always catch case when robot moved by human
  const bool isStill = _lastPoseData.IsBodyPoseSame( poseData, _bodyAngleThresh, _bodyPoseThresh ) &&
                       _lastPoseData.IsHeadAngleSame( poseData, _headAngleThresh );
  return robotMoved || !isStill;
}

bool NightVisionFilter::GetOutput( Vision::Image& out ) const
{
  if( _numAccImages < _minNumImages )
  {
    return false;
  }

  // Divide by the number of images
  out.Allocate( _accumulator.GetNumRows(), _accumulator.GetNumCols() );
  out.SetTimestamp( _lastTimestamp );
  
  const cv::Mat_<u16>& accMat = _accumulator.get_CvMat_();
  cv::Mat_<u8>& outMat = out.get_CvMat_();
  // NOTE No way to get around conversion to double and back, since OpenCV doesn't seem to support
  // native integer division
  accMat.convertTo( outMat, CV_8U, 1.0 / _numAccImages );
  return true;
}

} // end namespace Cozmo
} // end namespace Anki