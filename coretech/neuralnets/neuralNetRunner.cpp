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

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/jsonTools.h"

#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"
#include "coretech/neuralnets/neuralNetModel_tflite.h"
#include "coretech/neuralnets/neuralNetRunner.h"

#include "util/fileUtils/fileUtils.h"
#include "util/threading/threadPriority.h"

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
namespace NeuralNets {
 
// Log channel name currently expected to be defined by one of the model cpp files...
// static const char * const kLogChannelName = "VisionSystem";

  
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
      _model.reset(new TFLiteModel());
    }
    else if(NeuralNets::JsonKeys::OffboardModelType == modelTypeString)
    {
      _model.reset(new OffboardModel(GetCachePath()));
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
  
  // Note: right now we should assume that we only will be running
  // one model. This is definitely going to change but until
  // we know how we want to handle a bit more let's not worry about it.
  if(JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::VisualizationDir, _visualizationDirectory))
  {
    Util::FileUtils::CreateDirectory(Util::FileUtils::FullFilePath({cachePath, _visualizationDirectory}));
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
std::list<Vision::SalientPoint> NeuralNetRunner::Run(Vision::ImageRGB& img)
{
  Util::SetThreadName(pthread_self(), _model->GetName());
  
  std::list<Vision::SalientPoint> salientPoints;
  
  GetProfiler().Tic("Model.Detect");
  Result result = _model->Detect(img, salientPoints);
  GetProfiler().Toc("Model.Detect");
  
  if(RESULT_OK != result)
  {
    LOG_WARNING("IAsyncRunner.RunModel.ModelDetectFailed", "");
  }
  
  return salientPoints;
}
  
} // namespace NeuralNets
} // namespace Anki
