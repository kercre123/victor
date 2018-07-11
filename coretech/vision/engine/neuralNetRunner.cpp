/**
 * File: objectDetector.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "coretech/vision/engine/neuralNetRunner.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <cstdio>
#include <list>
#include <fstream>

// TODO: put this back if/when we start supporting other NeuralNetRunnerModels
//#if USE_TENSORFLOW
//#  ifndef TENSORFLOW_USE_AOT
//#    error Expecting TENSORFLOW_USE_AOT to be defined by cmake!
//#  elif TENSORFLOW_USE_AOT==1
//#    include "objectDetectorModel_tensorflow_AOT.cpp"
//#  else
//#    include "objectDetectorModel_tensorflow.cpp"
//#  endif
//#elif USE_TENSORFLOW_LITE
//#  include "objectDetectorModel_tensorflow_lite.cpp"
//#elif USE_OPENCV_DNN
//#  include "neuralNetRunner_opencvdnnModel.cpp"
//#else
#include "neuralNetRunner_messengerModel.cpp"
//#endif

namespace Anki {
namespace Vision {
 
// Log channel name currently expected to be defined by one of the model cpp files...
// static const char * const kLogChannelName = "VisionSystem";
 
namespace {
#define CONSOLE_GROUP "Vision.NeuralNetRunner"

  CONSOLE_VAR(f32,   kObjectDetection_Gamma,       CONSOLE_GROUP, 1.0f); // set to 1.0 to disable

  // Save images sent to the model for processing to:
  //   <cachePath>/saved_images/{full|resized}/<timestamp>.png
  // 0: off
  // 1: save resized images
  // 2: save full images
  CONSOLE_VAR(s32,   kNeuralNetRunner_SaveImages,  CONSOLE_GROUP, 0);

#undef CONSOLE_GROUP
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetRunner::NeuralNetRunner()
: _profiler("NeuralNetRunner")
, _model(new Model(_profiler))
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetRunner::~NeuralNetRunner()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetRunner::Init(const std::string& modelPath, const std::string& cachePath, const Json::Value& config)
{
  Result result = RESULT_OK;

  _isInitialized = false;
  _cachePath = cachePath;
  
  _profiler.Tic("LoadModel");
  result = _model->LoadModel(modelPath, cachePath, config);
  _profiler.Toc("LoadModel");
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.Init.LoadModelFailed", "");
    return result;
  }
  
  // Get the input height/width so we can do the resize and only need to share/copy/write as
  // small an image as possible for the standalone CNN process to pick up
  if(false == JsonTools::GetValueOptional(config, "inputHeight", _processingHeight))
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.Init.MissingConfig", "inputHeight");
    return RESULT_FAIL;
  }
  
  if(false == JsonTools::GetValueOptional(config, "inputWidth", _processingWidth))
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.Init.MissingConfig", "inputWidth");
    return RESULT_FAIL;
  }

  PRINT_NAMED_INFO("NeuralNetRunner.Init.LoadModelTime", "Loading model from '%s' took %.1fsec",
                   modelPath.c_str(), Util::MilliSecToSec(_profiler.AverageToc("LoadModel")));

  _profiler.SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  _profiler.SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());

  // Clear the cache of any stale images/results:
  Util::FileUtils::RemoveDirectory(_cachePath);
  Util::FileUtils::CreateDirectory(_cachePath);

  // Note: right now we should assume that we only will be running
  // one model. This is definitely going to change but unitl
  // we know how we want to handle a bit more let's not worry about it.
  _visualizationDirectory = config.get("visualizationDirectory", "").asString();
  if (_visualizationDirectory != "") {
    Util::FileUtils::CreateDirectory(Util::FileUtils::FullFilePath(
                                     {_cachePath, _visualizationDirectory}));
  }

  _isInitialized = true;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetRunner::ApplyGamma(ImageRGB& img)
{
  if(Util::IsFltNear(kObjectDetection_Gamma, 1.f))
  {
    return;
  }
  
  auto ticToc = _profiler.TicToc("Gamma");
  
  if(!Util::IsFltNear(kObjectDetection_Gamma, _currentGamma))
  {
    _currentGamma = kObjectDetection_Gamma;
    const f32 gamma = 1.f / _currentGamma;
    const f32 divisor = 1.f / 255.f;
    for(s32 value=0; value<256; ++value)
    {
      _gammaLUT[value] = std::round(255.f * std::powf((f32)value * divisor, gamma));
    }
  }
  
  s32 nrows = img.GetNumRows();
  s32 ncols = img.GetNumCols();
  if(img.IsContinuous()) {
    ncols *= nrows;
    nrows = 1;
  }
  for(s32 i=0; i<nrows; ++i)
  {
    Vision::PixelRGB* img_i = img.GetRow(i);
    for(s32 j=0; j<ncols; ++j)
    {
      Vision::PixelRGB& pixel = img_i[j];
      pixel.r() = _gammaLUT[pixel.r()];
      pixel.g() = _gammaLUT[pixel.g()];
      pixel.b() = _gammaLUT[pixel.b()];
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NeuralNetRunner::StartProcessingIfIdle(ImageCache& imageCache)
{
  if(!_isInitialized)
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.StartProcessingIfIdle.NotInitialized", "");
    return false;
  }
  
  // If we're not already processing an image with a "future", create one to process this image asynchronously.
  if(!_future.valid())
  {
    // Require color data
    if(!imageCache.HasColor())
    {
      PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "NeuralNetRunner.StartProcessingIfIdle.NeedColorData", "");
      return false;
    }
  
    if(kNeuralNetRunner_SaveImages == 2)
    {
      const Vision::ImageRGB& img = imageCache.GetRGB(ImageCache::Size::Full);
      const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "full",
        std::to_string(img.GetTimestamp()) + ".png"});
      img.Save(saveFilename);
    }

    // Resize to processing size
    _imgBeingProcessed.Allocate(_processingHeight, _processingWidth);
    const ImageCache::Size kImageSize = ImageCache::Size::Full;
    const Vision::ResizeMethod kResizeMethod = Vision::ResizeMethod::Linear;
    const Vision::ImageRGB& imgOrig = imageCache.GetRGB(kImageSize);
    imgOrig.Resize(_imgBeingProcessed, kResizeMethod);
    
    // Apply gamma (no-op if gamma is set to 1.0)
    ApplyGamma(_imgBeingProcessed);
    
    if(kNeuralNetRunner_SaveImages == 1)
    {
      const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "resized",
        std::to_string(_imgBeingProcessed.GetTimestamp()) + ".png"});
      _imgBeingProcessed.Save(saveFilename);
    }

    // Store its size relative to original size so we can rescale object detections later
    _heightScale = (f32)imgOrig.GetNumRows();
    _widthScale  = (f32)imgOrig.GetNumCols();
    
    if(_model->IsVerbose())
    {
      PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.StartProcessingIfIdle.ProcessingImage",
                    "Detecting salient points in %dx%d image t=%u",
                    _imgBeingProcessed.GetNumCols(), _imgBeingProcessed.GetNumRows(), _imgBeingProcessed.GetTimestamp());
    }
    
    _future = std::async(std::launch::async, [this]() {
      std::list<SalientPoint> salientPoints;
  
      _profiler.Tic("Model.Run");
      Result result = _model->Run(_imgBeingProcessed, salientPoints);
      _profiler.Toc("Model.Run");
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("NeuralNetRunner.StartProcessingIfIdle.AsyncLambda.ModelRunFailed", "");
      }
    
      return salientPoints;
    });
    
    // We did start processing the given image
    return true;
  }
  
  // We were not idle, so did not start processing the new image
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NeuralNetRunner::GetDetections(std::list<SalientPoint>& salientPoints)
{
  // Always clear the output list, so it's definitely only populated once the
  // future is ready below
  salientPoints.clear();

  if(_future.valid())
  {
    // Check the future's status and keep waiting until it's ready.
    // Once it's ready, return the result.
    const auto kWaitForTime = std::chrono::microseconds(500);
    const std::future_status futureStatus = _future.wait_for(kWaitForTime);
    if(std::future_status::ready == futureStatus)
    {
      salientPoints = _future.get();
      DEV_ASSERT(!_future.valid(), "NeuralNetRunner.GetDetections.FutureStillValid");
      
      if(ANKI_DEV_CHEATS && _model->IsVerbose())
      {
        if(salientPoints.empty())
        {
          PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.GetDetections.NoSalientPoints",
                        "t=%ums", _imgBeingProcessed.GetTimestamp());
        }
        for(auto const& salientPoint : salientPoints)
        {
          PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.GetDetections.FoundSalientPoint",
                        "t=%ums Name:%s Score:%.3f",
                        _imgBeingProcessed.GetTimestamp(), salientPoint.description.c_str(), salientPoint.score);
        }
      }
      
      return true;
    }
  }
  
  return false;
}
  
} // namespace Vision
} // namespace Anki
