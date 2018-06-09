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
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/imageBrightnessHistogram.h"

namespace Anki {
namespace Cozmo {

const char* kMinAccImagesKey     = "MinNumImages";
const char* kHistSubsampleKey    = "HistSubsample";
const char* kTargetPercentileKey = "TargetPercentile";
const char* kTargetValueKey      = "TargetValue";

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

NightVisionAccumulator::NightVisionAccumulator()
: _contrastHist( new Vision::ImageBrightnessHistogram )
{
  Reset();
}

Result NightVisionAccumulator::Init( const Json::Value& config )
{
  #define PARSE_PARAM(conf, key, var) \
  if( !JsonTools::GetValueOptional( conf, key, var ) ) \
  { \
    PRINT_NAMED_ERROR( "NightVisionAccumulator.Init.MissingParameter", "Could not parse parameter: %s", key ); \
    return RESULT_FAIL; \
  }
  
  PARSE_PARAM( config, kMinAccImagesKey, _minNumImages );
  PARSE_PARAM( config, kHistSubsampleKey, _histSubsample );
  PARSE_PARAM( config, kTargetPercentileKey, _contrastTargetPercentile );
  PARSE_PARAM( config, kTargetValueKey, _contrastTargetValue );
  Reset();
  return RESULT_OK;
}

void NightVisionAccumulator::Reset()
{
  _numAccImages = 0;
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

  if( img.GetNumRows() != _accumulator.GetNumRows() ||
      img.GetNumCols() != _accumulator.GetNumCols() )
  {
    PRINT_NAMED_ERROR("NightVisionAccumulator.AddImage.SizeError", "");
    Reset();
    return;
  }

  // Cast img to u16 type to add
  _castImage.Allocate( img.GetNumRows(), img.GetNumCols() );
  std::function<u16(const u8&)> castOp = std::bind(&CastPixel, std::placeholders::_1);
  img.ApplyScalarFunction( castOp, _castImage );
  _accumulator += _castImage;
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
  std::function<u8(const u16&)> divOp = std::bind(&DividePixel, std::placeholders::_1, _numAccImages);
  _accumulator.ApplyScalarFunction(divOp, out);

  // Compute image histogram and scale contrast
  _contrastHist->Reset();
  _contrastHist->FillFromImage( out, _histSubsample );
  u8 val = _contrastHist->ComputePercentile( _contrastTargetPercentile );
  f32 scale = static_cast<f32>(_contrastTargetValue) / val;
  std::function<u8(const u8)> scaleOp = std::bind(&ScalePixel, std::placeholders::_1, scale);
  out.ApplyScalarFunction(scaleOp);
  return true;
}

} // end namespace Cozmo
} // end namespace Anki