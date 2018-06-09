/**
 * File: nightVisionAccumulator.cpp
 *
 * Author: Humphrey Hu
 * Date:   2018-06-08
 *
 * Description: Helper class for averaging images together and contrast adjusting them.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/vision/nightVisionAccumulator.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/imageBrightnessHistogram.h"

namespace Anki {
namespace Cozmo {

const char* kMinAccImagesKey = "min_num_images";
const char* kHistSubsampleKey = "hist_subsample";
const char* kTargetPercentileKey = "target_percentile";
const char* kTargetValueKey = "target_value";

u32 CastPixel(const u8& p) { return (u32) p; }

u8 DividePixel(const u32& p, const u32& count)
{
  u32 out = p / count;
  if( out > 255 ) { out = 255; }
  return static_cast<u8>( out );
}

u8 ScalePixel(u8 p, const f32& k)
{
  u32 pK = static_cast<u32>( p * k );
  if( pK > 255 ) { pK = 255; }
  return static_cast<u8>( pK );
}

NightVisionAccumulator::NightVisionAccumulator(const Json::Value& config)
{
  _minNumImages = JsonTools::ParseUInt32( config, kMinAccImagesKey, "Vision.NightVisionAccumulator.Constructor" );
  _histSubsample = JsonTools::ParseInt32( config, kHistSubsampleKey, "Vision.NightVisionAccumulator.Constructor" );
  _contrastTargetPercentile = JsonTools::ParseFloat( config, kTargetPercentileKey, "Vision.NightVisionAccumulator.Constructor" );
  _contrastTargetValue = JsonTools::ParseUint8( config, kTargetValueKey, "Vision.NightVisionAccumulator.Constructor" );

  Reset();
}

void NightVisionAccumulator::Reset()
{
  _numAccImages = 0;
  _accumulator.Clear();
}

void NightVisionAccumulator::AddImage( const Vision::Image& img, 
                                       const VisionPoseData& poseData )
{
  // If robot moved at all, reset and bail
  if(poseData.histState.WasCameraMoving() || poseData.histState.WasPickedUp() ||
     poseData.histState.WasLiftMoving())
  {
    Reset();
    return;
  }
  
  // If first image, allocate accumulator and reset
  if( _numAccImages == 0 )
  {
    _accumulator.Allocate( img.GetNumRows(), img.GetNumCols() );
    _accumulator.FillWith( 0 );
  }

  // Cast img to u32 type to add
  static ImageAcc castImg;
  castImg.Allocate( img.GetNumRows(), img.GetNumCols() );
  std::function<u32(const u8&)> castOp = std::bind(&CastPixel, std::placeholders::_1);
  img.ApplyScalarFunction( castOp, castImg );

  _accumulator += castImg;
  _numAccImages++;
}

bool NightVisionAccumulator::GetOutput( Vision::Image& out ) const
{
  if( _numAccImages < _minNumImages )
  {
    return false;
  }

  // Divide by the number of images
  out.Allocate( _accumulator.GetNumRows(), _accumulator.GetNumCols() );
  std::function<u8(const u32&)> divOp = std::bind(&DividePixel, std::placeholders::_1, _numAccImages);
  _accumulator.ApplyScalarFunction(divOp, out);

  // Compute image histogram and scale contrast
  static Vision::ImageBrightnessHistogram hist;
  hist.Reset();
  hist.FillFromImage( out, _histSubsample );
  u8 val = hist.ComputePercentile( _contrastTargetPercentile );
  f32 scale = static_cast<f32>(_contrastTargetValue) / val;
  std::function<u8(const u8)> scaleOp = std::bind(&ScalePixel, std::placeholders::_1, scale);
  out.ApplyScalarFunction(scaleOp);
  return true;
}

} // end namespace Cozmo
} // end namespace Anki