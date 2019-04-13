/**
 * File: asyncRunnerInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   4/1/2019
 *
 * Description: See header file.
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "coretech/common/engine/jsonTools.h"

#include "coretech/vision/engine/asyncRunnerInterface.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/image.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "NeuralNets" // TODO: Logging to NeuralNets for historical reasons, should probably be elsewhere

namespace Anki {
namespace Vision {
  
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

const char* const IAsyncRunner::JsonKeys::InputHeight = "inputHeight";
const char* const IAsyncRunner::JsonKeys::InputWidth = "inputWidth";

constexpr static const int kSaveResizedImage   = 1;
constexpr static const int kSaveFullSizedImage = 2;

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
  if(false == JsonTools::GetValueOptional(config, JsonKeys::InputHeight, _processingHeight))
  {
    LOG_ERROR("IAsyncRunner.Init.MissingConfig", "%s", JsonKeys::InputHeight);
    return RESULT_FAIL;
  }
  
  if(false == JsonTools::GetValueOptional(config, JsonKeys::InputWidth, _processingWidth))
  {
    LOG_ERROR("IAsyncRunner.Init.MissingConfig", "%s", JsonKeys::InputWidth);
    return RESULT_FAIL;
  }
  
  _profiler.SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  _profiler.SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());
  
  // Clear the cache of any stale images/results:
  Util::FileUtils::RemoveDirectory(_cachePath);
  Util::FileUtils::CreateDirectory(_cachePath);

  result = InitInternal(cachePath, config);
  if(ANKI_VERIFY(RESULT_OK == result, "IAsyncRunner.Init.InitInternalFailed", ""))
  {
    _isInitialized = true;
  }
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::StartProcessingHelper()
{
  DEV_ASSERT(!_imgOrig.IsEmpty(), "IAsyncRunner.StartProcessingHelper.EmptyImage");
  
  if(kIAsyncRunner_SaveImages == kSaveFullSizedImage)
  {
    const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "full",
      std::to_string(_imgOrig.GetTimestamp()) + ".png"});
    _imgOrig.Save(saveFilename);
  }
  
  // Resize to processing size
  _imgBeingProcessed.Allocate(_processingHeight, _processingWidth);
  const Vision::ResizeMethod kResizeMethod = Vision::ResizeMethod::Linear;
  _imgOrig.Resize(_imgBeingProcessed, kResizeMethod);
  
  if(kIAsyncRunner_SaveImages == kSaveResizedImage)
  {
    const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath, "resized",
      std::to_string(_imgBeingProcessed.GetTimestamp()) + ".png"});
    _imgBeingProcessed.Save(saveFilename);
  }
  
  if(IsVerbose())
  {
    LOG_INFO("IAsyncRunner.StartProcessingHelper.ProcessingImage",
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
  // Require color data
  if(!imageCache.HasColor())
  {
    LOG_PERIODIC_DEBUG(30, "IAsyncRunner.StartProcessingIfIdle.NeedColorData", "");
    return false;
  }
  const ImageCacheSize kOrigImageSize = imageCache.GetSize(kIAsyncRunner_OrigImageSubsample);
  return StartProcessingIfIdle(imageCache.GetRGB(kOrigImageSize));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAsyncRunner::StartProcessingIfIdle(const Vision::ImageRGB& img)
{
  if(!_isInitialized)
  {
    // This will spam the log, but only in the LOG_CHANNEL channel, plus it helps make it more obvious to a
    // developer that something is wrong since it's easy to miss a model load failure (and associated error
    // in the log) at startup.
    //
    // If you do see this error, it is likely one of two things:
    //  1. Your model configuration in vision_config.json is wrong (look for other errors on load)
    //  2. Git LFS has failed you. See: https://ankiinc.atlassian.net/browse/VIC-13455
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

} // namespace Vision
} // namespace Anki

