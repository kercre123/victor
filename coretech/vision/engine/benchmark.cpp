/**
 * File: benchmark.cpp
 *
 * Author: Andrew Stein
 * Date:   11-30-2017
 *
 * Description: Vision system component for benchmarking operations.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "coretech/common/engine/array2d_impl.h"

#include "coretech/vision/engine/benchmark.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCache.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vision {
  
static const char * const kLogChannelName = "Benchmark";
  
namespace {
#define CONSOLE_GROUP "Vision.Benchmark"
  
CONSOLE_VAR(s32,  kVisionBenchmark_PrintFrequency_ms, CONSOLE_GROUP, 3000);
CONSOLE_VAR(bool, kVisionBenchmark_DisplayImages,     CONSOLE_GROUP, false); // Only works if running synchronously
CONSOLE_VAR(s32,  kVisionBenchmark_ScaleMultiplier,   CONSOLE_GROUP, 1);
  
// Toggle corresponding modes at runtime
#if REMOTE_CONSOLE_ENABLED
CONSOLE_VAR(std::underlying_type<Benchmark::Mode>::type, kVisionBenchmark_ToggleMode, CONSOLE_GROUP, 0);
CONSOLE_VAR(bool, kVisionBenchmark_EnableAllModes,  CONSOLE_GROUP, false);
CONSOLE_VAR(bool, kVisionBenchmark_DisableAllModes, CONSOLE_GROUP, false);
#endif // REMOTE_CONSOLE_ENABLED
  
#undef CONSOLE_GROUP
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Benchmark::Benchmark()
: Profiler("Vision.Benchmark")
{
  if(ANKI_DEV_CHEATS)
  {
    // Enable all modes by default
    _enabledModes.SetFlags(std::numeric_limits<ModeType>::max());
    
    SetPrintChannelName(kLogChannelName);
    SetPrintFrequency(kVisionBenchmark_PrintFrequency_ms);
    
    CreateNormalizeLUT();
    CreateRatioLUT();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::Update(ImageCache& imageCache)
{
  if(!ANKI_DEV_CHEATS)
  {
    return RESULT_OK;
  }
  
#if REMOTE_CONSOLE_ENABLED
  // Update modes based on any console vars that are set:
  if(kVisionBenchmark_ToggleMode != 0)
  {
    Mode whichMode = static_cast<Mode>(kVisionBenchmark_ToggleMode);
    const bool enable = (_enabledModes.IsBitFlagSet(whichMode) ? false : true);
    PRINT_CH_INFO(kLogChannelName, "Benchmark.Update.TogglingMode", "%s mode %hhu",
                  (enable ? "Enabling" : "Disabling"), whichMode);
    EnableMode(whichMode, enable);
    kVisionBenchmark_ToggleMode = 0;
  }
  if(kVisionBenchmark_EnableAllModes)
  {
    PRINT_CH_INFO(kLogChannelName, "Benchmark.Update.EnablingAllModes", "");
    _enabledModes.SetFlags(std::numeric_limits<ModeType>::max());
    kVisionBenchmark_EnableAllModes = false;
  }
  if(kVisionBenchmark_DisableAllModes)
  {
    PRINT_CH_INFO(kLogChannelName, "Benchmark.Update.DisablingAllModes", "");
    _enabledModes.ClearFlags();
    kVisionBenchmark_DisableAllModes = false;
  }
#endif // REMOTE_CONSOLE_ENABLED
  
  Result result = RESULT_OK;
  bool anyFailures = false;
  
# define RUN_IF_ENABLED(__method__) \
  if(_enabledModes.IsBitFlagSet(Mode::Do##__method__)) \
  { \
    result = __method__(image); \
    anyFailures |= (result != RESULT_OK); \
  }
  
  const ImageCacheSize whichSize = ImageCache::GetSize(kVisionBenchmark_ScaleMultiplier, ResizeMethod::NearestNeighbor);
  const ImageRGB& image = imageCache.GetRGB(whichSize);
  
  {
    auto ticToc = TicToc("TotalBenchmarkUpdate"); // excluding the GetRGB call (which could include downsampling)
    
    RUN_IF_ENABLED(ScalarSubtraction);
    
    RUN_IF_ENABLED(SquaredDiffFromScalar);
    
    RUN_IF_ENABLED(DiffWithPrevious);
    
    RUN_IF_ENABLED(RatioWithPrevious);
    
    RUN_IF_ENABLED(NormalizeRGB);
  }
  
# undef RUN_IF_ENABLED
  
  return (anyFailures ? RESULT_FAIL : RESULT_OK);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::ScalarSubtraction(const ImageRGB& image)
{
  const bool kDoChecks = false;
  
  ImageRGB diffImage(image.GetNumRows(), image.GetNumCols());
  const PixelRGB scalar(37, 100, 251);
  
  {
    auto ticToc = TicToc("ScalarSubtraction_ForLoop");
    
    s32 nrows = image.GetNumRows();
    s32 ncols = image.GetNumCols();
    if(image.IsContinuous())
    {
      ncols *= nrows;
      nrows = 1;
    }
    for(s32 i=0; i<nrows; ++i)
    {
      const PixelRGB* image_i = image.GetRow(i);
      PixelRGB*       diffImage_i = diffImage.GetRow(i);
      
      for(s32 j=0; j<ncols; ++j)
      {
        PixelRGB& diffPixel = diffImage_i[j];
        diffPixel = image_i[j];
        diffPixel -= scalar;
      }
    }
  }
  
  PixelRGB checkValue(image(1,1));
  checkValue -= scalar;

  if(kDoChecks && diffImage(1,1) != checkValue)
  {
    PRINT_NAMED_ERROR("Benchmark.ScalarSubtraction.ForLoopFailed", "Expected:(%d,%d,%d) Got:(%d,%d,%d)",
                      checkValue.r(), checkValue.g(), checkValue.b(),
                      diffImage(1,1).r(), diffImage(1,1).g(), diffImage(1,1).b());
    return RESULT_FAIL;
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    diffImage.Display("ScalarSubtraction_ForLoop");
  }
  
  diffImage.FillWith(0);
  {
    auto ticToc = TicToc("ScalarSubtraction_ApplyScalarFunction");
   
    image.CopyTo(diffImage);
    std::function<PixelRGB(const PixelRGB&)> diffFunc = [&scalar](const PixelRGB& p)
    {
      PixelRGB diff(p);
      diff -= scalar;
      return diff;
    };
    
    diffImage.ApplyScalarFunction(diffFunc);
  }
  
  if(kDoChecks && diffImage(1,1) != checkValue)
  {
    PRINT_NAMED_ERROR("Benchmark.ScalarSubtraction.ApplyScalarFuncFailed", "Expected:(%d,%d,%d) Got:(%d,%d,%d)",
                      checkValue.r(), checkValue.g(), checkValue.b(),
                      diffImage(1,1).r(), diffImage(1,1).g(), diffImage(1,1).b());
    return RESULT_FAIL;
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    diffImage.Display("ScalarSubtraction_ApplyScalarFunction");
  }
  
  diffImage.FillWith(0);
  {
    auto ticToc = TicToc("ScalarSubtraction_OpenCV");
    cv::subtract(image.get_CvMat_(), cv::Scalar(scalar.r(),scalar.g(),scalar.b()), diffImage.get_CvMat_());
  }
  
  if(diffImage.GetNumRows() != image.GetNumRows() || diffImage.GetNumCols() != image.GetNumCols())
  {
    PRINT_NAMED_ERROR("Benchmark.ScalarSubtraction.BadResultSize",
                      "OpenCV subtraction yielded %dx%d image instead of %dx%d",
                      diffImage.GetNumCols(), diffImage.GetNumRows(), image.GetNumCols(), image.GetNumRows());
    return RESULT_FAIL_INVALID_SIZE;
  }
  
  if(kDoChecks && diffImage(1,1) != checkValue)
  {
    PRINT_NAMED_ERROR("Benchmark.ScalarSubtraction.OpenCVFailed", "Expected:(%d,%d,%d) Got:(%d,%d,%d)",
                      checkValue.r(), checkValue.g(), checkValue.b(),
                      diffImage(1,1).r(), diffImage(1,1).g(), diffImage(1,1).b());
    return RESULT_FAIL;
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    diffImage.Display("ScalarSubtraction_OpenCV");
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::DiffWithPrevious(const ImageRGB& image)
{
  if(_prevImage.GetNumRows() == image.GetNumRows() &&
     _prevImage.GetNumCols() == image.GetNumCols())
  {
    Image diffImage(image.GetNumRows(), image.GetNumCols());
    
    {
      auto ticToc = TicToc("DiffWithPrevious_ForLoop");
      
      s32 nrows = image.GetNumRows();
      s32 ncols = image.GetNumCols();
      if(image.IsContinuous() && _prevImage.IsContinuous())
      {
        ncols *= nrows;
        nrows = 1;
      }
      for(s32 i=0; i<nrows; ++i)
      {
        const PixelRGB* image_i = image.GetRow(i);
        const PixelRGB* prevImage_i = _prevImage.GetRow(i);
        u8* diffImage_i = diffImage.GetRow(i);
        
        for(s32 j=0; j<ncols; ++j)
        {
          const PixelRGB& pCrnt = image_i[j];
          const PixelRGB& pPrev = prevImage_i[j];
          
          diffImage_i[j] = std::max(std::abs(pCrnt.r() - pPrev.r()),
                                    std::max(std::abs(pCrnt.g() - pPrev.g()), std::abs(pCrnt.b() - pPrev.b())));
        }
      }
    }
    
    if(kVisionBenchmark_DisplayImages)
    {
      diffImage.Display("DiffWithPrevious_ForLoop");
    }
    
    diffImage.FillWith(0);
    {
      auto ticToc = TicToc("DiffWithPrevious_OpenCV");

      cv::Mat diffImageRGB;
      cv::absdiff(image.get_CvMat_(), _prevImage.get_CvMat_(), diffImageRGB);
      cv::reduce(diffImageRGB.reshape(1, image.GetNumElements()),
                 diffImage.get_CvMat_().reshape(1, image.GetNumElements()), 1, CV_REDUCE_MAX);
    }
    
    if(kVisionBenchmark_DisplayImages)
    {
      diffImage.Display("DiffWithPrevious_OpenCV");
    }
  }
  
  image.CopyTo(_prevImage);
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline f32 ratioTestHelper(u8 value1, u8 value2)
{
  // NOTE: not checking for divide-by-zero here because kMotionDetection_MinBrightness (DEV_ASSERTed to be > 0 in
  //  the constructor) prevents values of zero from getting to this helper
  if(value1 > value2) {
    return static_cast<f32>(value1) / std::max(1.f, static_cast<f32>(value2));
  } else {
    return static_cast<f32>(value2) / std::max(1.f, static_cast<f32>(value1));
  }
}

void Benchmark::CreateRatioLUT()
{
  auto ticToc = TicToc("CreateRatioLUT");
  
  _ratioLUT.resize(256);
  
  for(u16 a=0; a<=255; ++a)
  {
    _ratioLUT[a].resize(256);
    
    for(u16 b=0; b<=255; ++b)
    {
      if(a==0 || b==0)
      {
        _ratioLUT[a][b] = 0.f;
      }
      else
      {
        if(a > b) {
          _ratioLUT[a][b] = (f32)a / (f32)b;
        } else {
          _ratioLUT[a][b] = (f32)b / (f32)a;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::RatioWithPrevious(const ImageRGB& image)
{
  if(_prevImage.GetNumRows() == image.GetNumRows() &&
     _prevImage.GetNumCols() == image.GetNumCols())
  {
    const u8 kMinBrightness = 10;
    const u8 kDisplayMultiplier = 100;
    
    Image ratioImage(image.GetNumRows(), image.GetNumCols());
    
    {
      auto ticToc = TicToc("RatioWithPrevious_ForLoop");
     
      s32 nrows = image.GetNumRows();
      s32 ncols = image.GetNumCols();
      if(image.IsContinuous() && _prevImage.IsContinuous())
      {
        ncols *= nrows;
        nrows = 1;
      }
      for(s32 i=0; i<nrows; ++i)
      {
        const PixelRGB* image_i = image.GetRow(i);
        const PixelRGB* prevImage_i = _prevImage.GetRow(i);
        u8* ratioImage_i = ratioImage.GetRow(i);
        
        for(s32 j=0; j<ncols; ++j)
        {
          const PixelRGB& p1 = image_i[j];
          const PixelRGB& p2 = prevImage_i[j];
          
          if(p1.IsBrighterThan(kMinBrightness) && p2.IsBrighterThan(kMinBrightness))
          {
            const f32 ratioR = ratioTestHelper(p1.r(), p2.r());
            const f32 ratioG = ratioTestHelper(p1.g(), p2.g());
            const f32 ratioB = ratioTestHelper(p1.b(), p2.b());
            ratioImage_i[j] = kDisplayMultiplier*(std::max(ratioR, std::max(ratioG, ratioB))-1.f);
          }
          else
          {
            ratioImage_i[j] = 0;
          }
        }
      }
    }
    
    if(kVisionBenchmark_DisplayImages)
    {
      ratioImage.Display("RatioWithPrevious_ForLoop");
    }
    
    ratioImage.FillWith(0);
    {
      auto ticToc = TicToc("RatioWithPrevious_ForLoopLUT");
      
      s32 nrows = image.GetNumRows();
      s32 ncols = image.GetNumCols();
      if(image.IsContinuous() && _prevImage.IsContinuous())
      {
        ncols *= nrows;
        nrows = 1;
      }
      for(s32 i=0; i<nrows; ++i)
      {
        const PixelRGB* image_i = image.GetRow(i);
        const PixelRGB* prevImage_i = _prevImage.GetRow(i);
        u8* ratioImage_i = ratioImage.GetRow(i);
        
        for(s32 j=0; j<ncols; ++j)
        {
          const PixelRGB& p1 = image_i[j];
          const PixelRGB& p2 = prevImage_i[j];
          
          if(p1.IsBrighterThan(kMinBrightness) && p2.IsBrighterThan(kMinBrightness))
          {
            ratioImage_i[j] = kDisplayMultiplier*(std::max(_ratioLUT[p1.r()][p2.r()],
                                                           std::max(_ratioLUT[p1.g()][p2.g()],
                                                                    _ratioLUT[p1.b()][p2.b()])) - 1.f);
          }
          else
          {
            ratioImage_i[j] = 0;
          }
          
        }
      }
    }
    
    if(kVisionBenchmark_DisplayImages)
    {
      ratioImage.Display("RatioWithPrevious_ForLoopLUT");
    }
  }
  
  image.CopyTo(_prevImage);
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::SquaredDiffFromScalar(const ImageRGB& image)
{
  const PixelRGB_<s32> scalar(12,34,56);
  
  Image diffSq(image.GetNumRows(), image.GetNumCols());
  
  diffSq.FillWith(0);
  {
    auto ticToc = TicToc("SquaredDiffFromScalar_ForLoop");
    
    s32 nrows = image.GetNumRows();
    s32 ncols = image.GetNumCols();
    if(image.IsContinuous())
    {
      ncols *= nrows;
      nrows = 1;
    }
    for(s32 i=0; i<nrows; ++i)
    {
      const PixelRGB* image_i = image.GetRow(i);
      u8* diffImage_i = diffSq.GetRow(i);
      
      for(s32 j=0; j<ncols; ++j)
      {
        const PixelRGB& pixel = image_i[j];
        
        s32 channelDiff = (s32)pixel.r() - scalar.r();
        s32 d = channelDiff*channelDiff;
        
        channelDiff = (s32)pixel.g() - scalar.g();
        d += channelDiff*channelDiff;
        
        channelDiff = (s32)pixel.b() - scalar.b();
        d += channelDiff*channelDiff;
        
        d /= 3;
        
        diffImage_i[j] = static_cast<u8>(d >> 8);
      }
    }
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    diffSq.Display("SquaredDiffFromScalar_ForLoop");
  }
  
  diffSq.FillWith(0);
  {
    auto ticToc = TicToc("SquaredDiffFromScalar_OpenCV");

    cv::Mat squared;
    cv::subtract(image.get_CvMat_(), cv::Scalar(scalar.r(), scalar.g(), scalar.b()), squared,
                 cv::noArray(), CV_32F);
    
    squared = squared.mul(squared);
    squared = squared.reshape(1, squared.rows * squared.cols);
    cv::reduce(squared, squared, 1, CV_REDUCE_AVG);
    squared = squared.reshape(1, diffSq.GetNumRows());
    squared.convertTo(diffSq.get_CvMat_(), CV_8U, 1.0/256.0);
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    diffSq.Display("SquaredDiffFromScalar_OpenCV");
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Benchmark::CreateNormalizeLUT()
{
  auto ticToc = TicToc("CreateNormalizeLUT");
  
  _normLUT.resize(256);
  
  for(u16 numer=0; numer<=255; ++numer) // Note: need u16 because ++255 rolls back to zero, making this "while(true)"!
  {
    _normLUT[numer].resize(3*256);
    
    for(u16 denom=0; denom<=3*255; ++denom)
    {
      if(denom == 0 || denom < numer)
      {
        _normLUT[numer][denom] = 0;
      }
      else
      {
        _normLUT[numer][denom] = (numer*255)/denom;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Benchmark::NormalizeRGB(const ImageRGB& image)
{
  ImageRGB imageNorm(image.GetNumRows(), image.GetNumCols());
  
  {
    auto ticToc = TicToc("NormalizeRGB_ForLoop");
    
    s32 nrows = image.GetNumRows();
    s32 ncols = image.GetNumCols();
    if(image.IsContinuous())
    {
      ncols *= nrows;
      nrows = 1;
    }
    for(s32 i=0; i<nrows; ++i)
    {
      const PixelRGB* image_i = image.GetRow(i);
      PixelRGB* imageNorm_i = imageNorm.GetRow(i);
      
      for(s32 j=0; j<ncols; ++j)
      {
        const PixelRGB& pixel = image_i[j];
        PixelRGB& normPixel = imageNorm_i[j];
        
        s32 sum = (s32)pixel.r() + (s32)pixel.g() + (s32)pixel.b();
        if(sum == 0)
        {
          normPixel = 0;
          continue;
        }
        
        normPixel.r() = static_cast<u8>((((s32)pixel.r())*255) / sum);
        normPixel.g() = static_cast<u8>((((s32)pixel.g())*255) / sum);
        normPixel.b() = static_cast<u8>((((s32)pixel.b())*255) / sum);
      }
    }
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    imageNorm.Display("NormalizeRGB_ForLoop");
  }
  
  imageNorm.FillWith(0);
  {
    auto ticToc = TicToc("NormalizeRGB_ForLoopLUT");
    
    s32 nrows = image.GetNumRows();
    s32 ncols = image.GetNumCols();
    if(image.IsContinuous())
    {
      ncols *= nrows;
      nrows = 1;
    }
    for(s32 i=0; i<nrows; ++i)
    {
      const PixelRGB* image_i = image.GetRow(i);
      PixelRGB* imageNorm_i = imageNorm.GetRow(i);
      
      for(s32 j=0; j<ncols; ++j)
      {
        const PixelRGB& pixel = image_i[j];
        PixelRGB& normPixel = imageNorm_i[j];
        
        const u16 sum = (u16)pixel.r() + (u16)pixel.g() + (u16)pixel.b();
        normPixel.r() = _normLUT[pixel.r()][sum];
        normPixel.g() = _normLUT[pixel.g()][sum];
        normPixel.b() = _normLUT[pixel.b()][sum];
      }
    }
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    imageNorm.Display("NormalizeRGB_ForLoopLUT");
  }
  
  imageNorm.FillWith(0);
  {
    auto ticToc = TicToc("NormalizeRGB_OpenCV");
    image.GetNormalizedColor(imageNorm);
  }
  
  if(kVisionBenchmark_DisplayImages)
  {
    imageNorm.Display("NormalizeRGB_OpenCV");
  }
  
  return RESULT_OK;
}


} // namespace Vision
} // namespace Anki
