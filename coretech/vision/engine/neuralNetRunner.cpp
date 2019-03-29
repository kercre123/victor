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

#include "coretech/common/shared/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"
#include "coretech/neuralnets/neuralNetModel_tflite.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"
#include "util/threading/threadPriority.h"

#include <cstdio>
#include <list>
#include <fstream>

// TODO: put this back if/when we start supporting other IAsyncRunnerModels
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
//#include "neuralNetRunner_messengerModel.cpp"
//#endif

#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace Vision {
 
// Log channel name currently expected to be defined by one of the model cpp files...
// static const char * const kLogChannelName = "VisionSystem";
 
namespace {
#define CONSOLE_GROUP "Vision.IAsyncRunner"

  // Save images sent to the model for processing to:
  //   <cachePath>/saved_images/{full|resized}/<timestamp>.png
  // 0: off
  // 1: save resized images
  // 2: save full images
  CONSOLE_VAR_ENUM(s32,   kIAsyncRunner_SaveImages,  CONSOLE_GROUP, 0, "Off,Save Resized,Save Original Size");
  
  // 1: Full size, 2: Half size
  CONSOLE_VAR_RANGED(s32, kIAsyncRunner_OrigImageSubsample, CONSOLE_GROUP, 1, 1, 2);

#undef CONSOLE_GROUP
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAsyncRunner::IAsyncRunner()
: _profiler("IAsyncRunner")
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAsyncRunner::~IAsyncRunner()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IAsyncRunner::Init(const std::string& cachePath, const Json::Value& config)
{
  Result result = RESULT_OK;

  _isInitialized = false;
  _cachePath = cachePath;
  
  // Get the input height/width so we can do the resize and only need to share/copy/write as
  // small an image as possible for the standalone CNN process to pick up
  if(false == JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::InputHeight, _processingHeight))
  {
    LOG_ERROR("IAsyncRunner.Init.MissingConfig", "%s", NeuralNets::JsonKeys::InputHeight);
    return RESULT_FAIL;
  }
  
  if(false == JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::InputWidth, _processingWidth))
  {
    LOG_ERROR("IAsyncRunner.Init.MissingConfig", "%s", NeuralNets::JsonKeys::InputWidth);
    return RESULT_FAIL;
  }

  _profiler.SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  _profiler.SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());

  // Clear the cache of any stale images/results:
  Util::FileUtils::RemoveDirectory(_cachePath);
  Util::FileUtils::CreateDirectory(_cachePath);

  // Note: right now we should assume that we only will be running
  // one model. This is definitely going to change but until
  // we know how we want to handle a bit more let's not worry about it.
  if(JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::VisualizationDir, _visualizationDirectory))
  {
    Util::FileUtils::CreateDirectory(Util::FileUtils::FullFilePath({_cachePath, _visualizationDirectory}));
  }

  _isInitialized = true;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::StartProcessingHelper()
{
  DEV_ASSERT(!_imgOrig.IsEmpty(), "IAsyncRunner.StartProcessingHelper.EmptyImage");
  
  if(kIAsyncRunner_SaveImages == 2)
  {
    const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "half",
      std::to_string(_imgOrig.GetTimestamp()) + ".png"});
    _imgOrig.Save(saveFilename);
  }
  
  // Resize to processing size
  _imgBeingProcessed.Allocate(_processingHeight, _processingWidth);
  const Vision::ResizeMethod kResizeMethod = Vision::ResizeMethod::Linear;
  _imgOrig.Resize(_imgBeingProcessed, kResizeMethod);
  
  if(kIAsyncRunner_SaveImages == 1)
  {
    const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "resized",
      std::to_string(_imgBeingProcessed.GetTimestamp()) + ".png"});
    _imgBeingProcessed.Save(saveFilename);
  }
  
  if(IsVerbose())
  {
    LOG_INFO("IAsyncRunner.StartProcessingIfIdle.ProcessingImage",
             "Detecting salient points in %dx%d image t=%u",
             _imgBeingProcessed.GetNumCols(), _imgBeingProcessed.GetNumRows(), _imgBeingProcessed.GetTimestamp());
  }
  
  _future = std::async(std::launch::async, [this]() { return Run(_imgBeingProcessed); });
  
  // We did start processing the given image
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::StartProcessingIfIdle(ImageCache& imageCache)
{
  if(!_isInitialized)
  {
    // This will spam the log, but only in the NeuralNets channel, plus it helps make it more obvious to a
    // developer that something is wrong since it's easy to miss a model load failure (and associated error
    // in the log) at startup.
    //
    // If you do see this error, it is likely one of two things:
    //  1. Your model configuration in vision_config.json is wrong (look for other errors on load)
    //  2. Git LFS has failed you. See: https://ankiinc.atlassian.net/browse/VIC-13455
    LOG_INFO("IAsyncRunner.StartProcessingIfIdle.FromCache.NotInitialized", "t:%ums", imageCache.GetTimeStamp());
    return false;
  }
  
  // If we're not already processing an image with a "future", create one to process this image asynchronously.
  if(!_future.valid())
  {
    // Require color data
    if(!imageCache.HasColor())
    {
      LOG_PERIODIC_DEBUG(30, "IAsyncRunner.StartProcessingIfIdle.NeedColorData", "");
      return false;
    }
    
    const ImageCacheSize kOrigImageSize = imageCache.GetSize(kIAsyncRunner_OrigImageSubsample);
    
    imageCache.GetRGB(kOrigImageSize).CopyTo(_imgOrig);
    
    return StartProcessingHelper();
  }
  
  // We were not idle, so did not start processing the new image
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::StartProcessingIfIdle(const Vision::ImageRGB& img)
{
  if(!_isInitialized)
  {
    // See note above for same !initialized case in StartProcessingIfIdle(imageCache)
    LOG_INFO("IAsyncRunner.StartProcessingIfIdle.FromImage.NotInitialized", "t:%ums", img.GetTimestamp());
    return false;
  }
  
  // If we're not already processing an image with a "future", create one to process this image asynchronously.
  if(!_future.valid())
  {
    img.CopyTo(_imgOrig);
    
    return StartProcessingHelper();
  }
  
  // We were not idle, so did not start processing the new image
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::GetDetections(std::list<SalientPoint>& salientPoints)
{
  if(_future.valid())
  {
    // Check the future's status and keep waiting until it's ready.
    // Once it's ready, return the result.
    const std::future_status futureStatus = _future.wait_for(std::chrono::milliseconds{0});
    if(std::future_status::ready == futureStatus)
    {
      auto newSalientPoints = _future.get();
      std::copy(newSalientPoints.begin(), newSalientPoints.end(), std::back_inserter(salientPoints));

      DEV_ASSERT(!_future.valid(), "IAsyncRunner.GetDetections.FutureStillValid");
      
      if(ANKI_DEV_CHEATS && IsVerbose())
      {
        if(salientPoints.empty())
        {
          LOG_INFO("IAsyncRunner.GetDetections.NoSalientPoints",
                   "t=%ums", _imgBeingProcessed.GetTimestamp());
        }
        for(auto const& salientPoint : salientPoints)
        {
          LOG_INFO("IAsyncRunner.GetDetections.FoundSalientPoint",
                   "t=%ums Name:%s Score:%.3f",
                   _imgBeingProcessed.GetTimestamp(), salientPoint.description.c_str(), salientPoint.score);
        }
      }
      
      return true;
    }
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetRunner::NeuralNetRunner(const std::string& modelPath)
: _modelPath(modelPath)
{
  GetProfiler().SetProfileGroupName("NeuralNetRunner");
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetRunner::~NeuralNetRunner()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetRunner::InitInternal(const std::string& cachePath, const Json::Value& config)
{
  std::string modelTypeString;
  if(JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::ModelType, modelTypeString))
  {
    if(NeuralNets::JsonKeys::TFLiteModelType == modelTypeString)
    {
      _model.reset(new NeuralNets::TFLiteModel());
    }
    else if(NeuralNets::JsonKeys::OffboardModelType == modelTypeString)
    {
      _model.reset(new NeuralNets::OffboardModel(GetCachePath()));
    }
    else
    {
      LOG_ERROR("IAsyncRunner.Init.UnknownModelType", "%s", modelTypeString.c_str());
      return RESULT_FAIL;
    }
  }
  else
  {
    LOG_ERROR("IAsyncRunner.Init.MissingConfig", "%s", NeuralNets::JsonKeys::ModelType);
    return RESULT_FAIL;
  }
  
  GetProfiler().Tic("LoadModel");
  Result result = _model->LoadModel(_modelPath, config);
  GetProfiler().Toc("LoadModel");
  
  if(RESULT_OK != result)
  {
    LOG_ERROR("IAsyncRunner.Init.LoadModelFailed", "");
    return result;
  }
  
  PRINT_NAMED_INFO("IAsyncRunner.Init.LoadModelTime", "Loading model from '%s' took %.1fsec",
                   _modelPath.c_str(), Util::MilliSecToSec(GetProfiler().AverageToc("LoadModel")));
  
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NeuralNetRunner::IsVerbose() const
{
  return _model->IsVerbose();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::list<SalientPoint> NeuralNetRunner::Run(ImageRGB& img)
{
  Util::SetThreadName(pthread_self(), _model->GetName());
  
  std::list<SalientPoint> salientPoints;
  
  GetProfiler().Tic("Model.Detect");
  Result result = _model->Detect(img, salientPoints);
  GetProfiler().Toc("Model.Detect");
  
  if(RESULT_OK != result)
  {
    LOG_WARNING("IAsyncRunner.RunModel.ModelDetectFailed", "");
  }
  
  return salientPoints;
}
  
} // namespace Vision
} // namespace Anki
