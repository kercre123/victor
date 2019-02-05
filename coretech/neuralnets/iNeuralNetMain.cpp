/**
 * File: iNeuralNetMain.cpp
 *
 * Author: Andrew Stein
 * Date:   9/14/2018
 *
 * Description: Interface class to create a standalone process to run forward inference using a neural network.
 *
 *              Currently uses the file system as a poor man's IPC to communicate with the "messenger"
 *              NeuralNetRunner::Model implementation in vic-engine's Vision System. See also VIC-2686.
 *
 * Copyright: Anki, Inc. 2018
 **/


#if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
#  include "coretech/neuralnets/neuralNetModel_tensorflow.h"
#elif defined(ANKI_NEURALNETS_USE_CAFFE2)
#  include "coretech/neuralnets/objectDetector_caffe2.h"
#elif defined(ANKI_NEURALNETS_USE_OPENCV_DNN)
#  include "coretech/neuralnets/objectDetector_opencvdnn.h"
#elif defined(ANKI_NEURALNETS_USE_TFLITE)
#  include "coretech/neuralnets/neuralNetModel_tflite.h"
#else
#  error One of ANKI_NEURALNETS_USE_{TENSORFLOW | CAFFE2 | OPENCVDNN | TFLITE} must be defined
#endif

#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/scopedTicToc.h"
#include "coretech/neuralnets/iNeuralNetMain.h"
#include "coretech/neuralnets/neuralNetFilenames.h"
#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"
#include "coretech/vision/engine/image_impl.h"
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace NeuralNets {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Must be implemented here (in .cpp) due to use of unique_ptr with a forward declaration
INeuralNetMain::INeuralNetMain() = default;
INeuralNetMain::~INeuralNetMain() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void INeuralNetMain::CleanupAndExit(Result result)
{
  LOG_INFO("INeuralNetMain.CleanupAndExit", "result:%d", result);
  Util::gLoggerProvider = nullptr;
  
  DerivedCleanup();
  
  sync();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result INeuralNetMain::Init(const std::string& configFilename,
                            const std::string& modelPath,
                            const std::string& cachePath,
                            const std::string& imageFileToProcess)
{
  _isInitialized = false;
  
  Util::gLoggerProvider = GetLoggerProvider();
  
  if(nullptr == Util::gLoggerProvider)
  {
    // Having no logger shouldn't kill us but probably isn't what we intended, so issue a warning
    LOG_WARNING("INeuralNetMain.Init.NullLogger", "");
  }
  
  Result result = RESULT_OK;
  
  // Make sure the config file and model path are valid
  // NOTE: cachePath need not exist yet as it will be created by the NeuralNetRunner
  if(!Util::FileUtils::FileExists(configFilename))
  {
    LOG_ERROR("INeuralNetMain.Init.BadConfigFile", "ConfigFile:%s", configFilename.c_str());
    result = RESULT_FAIL;
  }
  if(!Util::FileUtils::DirectoryExists(modelPath))
  {
    LOG_ERROR("INeuralNetMain.Init.BadModelPath", "ModelPath:%s", modelPath.c_str());
    result = RESULT_FAIL;
  }
  if(RESULT_OK != result)
  {
    CleanupAndExit(result);
  }
  LOG_INFO("INeuralNetMain.Init.Starting", "Config:%s ModelPath:%s CachePath:%s",
           configFilename.c_str(), modelPath.c_str(), cachePath.c_str());
  
  _cachePath = cachePath;
  
  // Read config file
  Json::Value config;
  {
    Json::Reader reader;
    std::ifstream file(configFilename);
    const bool success = reader.parse(file, config);
    file.close();
    if(!success)
    {
      LOG_ERROR("INeuralNetMain.Init.ReadConfigFailed",
                        "Could not read config file: %s", configFilename.c_str());
      CleanupAndExit(RESULT_FAIL);
    }
    
    if(!config.isMember(JsonKeys::NeuralNets))
    {
      LOG_ERROR("INeuralNetMain.Init.MissingConfigKey", "Config file missing '%s' field", JsonKeys::NeuralNets);
      CleanupAndExit(RESULT_FAIL);
    }
    
    config = config[JsonKeys::NeuralNets];
  }
  
  _imageFilename = imageFileToProcess;
  
  // Initialize the detectors
  if(!config.isMember(JsonKeys::Models))
  {
    LOG_ERROR("INeuralNetMain.Init.MissingModelsField", "No 'models' specified in config file");
    CleanupAndExit(RESULT_FAIL);
  }

  const Json::Value& modelsConfig = config[JsonKeys::Models];
  bool anyVerbose = false;
  for(auto & modelConfig : modelsConfig)
  {
    if(!modelConfig.isMember(JsonKeys::NetworkName))
    {
      LOG_ERROR("INeuralNetMain.Init.MissingNameField", "No 'name' specified in model spec in config");
      CleanupAndExit(RESULT_FAIL);
    }

    const std::string name = modelConfig[JsonKeys::NetworkName].asString();
    std::unique_ptr<INeuralNetModel> model;
    
    if(!modelConfig.isMember(JsonKeys::ModelType))
    {
      LOG_ERROR("INeuralNetModel.CreateFromTypeConfig.MissingConfig", "%s", JsonKeys::ModelType);
      CleanupAndExit(RESULT_FAIL);
    }

    const std::string& modelTypeString = modelConfig[JsonKeys::ModelType].asString();
    if(NeuralNets::JsonKeys::TFLiteModelType == modelTypeString)
    {
      model.reset( new NeuralNets::TFLiteModel() );
    }
    else if(NeuralNets::JsonKeys::OffboardModelType == modelTypeString)
    {
      model.reset( new NeuralNets::OffboardModel(_cachePath) );
    }
    else
    {
      LOG_ERROR("NeuralNetRunner.Init.UnknownModelType", "%s", modelTypeString.c_str());
      CleanupAndExit(RESULT_FAIL);
    }
    
    auto insertionResult = _neuralNets.emplace(name, std::move(model));
    if(!insertionResult.second)
    {
      LOG_ERROR("INeuralNetMain.Init.DuplicateModelName", "More than one model named '%s'", name.c_str());
      CleanupAndExit(RESULT_FAIL);
    }
    
    std::unique_ptr<INeuralNetModel>& neuralNet = insertionResult.first->second;
    ScopedTicToc ticToc("LoadModel", LOG_CHANNEL);
    result = neuralNet->LoadModel(modelPath, modelConfig);
    
    if(RESULT_OK != result)
    {
      LOG_ERROR("INeuralNetMain.Init.LoadModelFail", "Failed to load model '%s' from path: %s",
                name.c_str(), modelPath.c_str());
      CleanupAndExit(result);
    }
    
    // Make sure output directory for this model exists
    const std::string outputDir = Util::FileUtils::FullFilePath({_cachePath, name});
    const bool success = Util::FileUtils::CreateDirectory(outputDir);
    if(!success)
    {
      LOG_ERROR("INeuralNetMain.Init.CreateOutputDirFailed", "Tried: %s", outputDir.c_str());
      CleanupAndExit(RESULT_FAIL);
    }
    
    anyVerbose |= neuralNet->IsVerbose();
    _pollPeriod_ms = std::min(_pollPeriod_ms, GetPollPeriod_ms(modelConfig));
  
  }
  ScopedTicToc::Enable(anyVerbose);
  
  if(_pollPeriod_ms >= std::numeric_limits<int>::max())
  {
    LOG_ERROR("INeuralNetMain.Init.MissingPollingPeriod", "");
    CleanupAndExit(RESULT_FAIL);
  }
  
  _isInitialized = true;
  
  LOG_INFO("INeuralNetMain.Init.DetectorInitialized", "Loaded %zu model(s). Polling for images every %dms",
           _neuralNets.size(), _pollPeriod_ms);
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result INeuralNetMain::Run()
{
  if(!_isInitialized)
  {
    LOG_ERROR("INeuralNetMain.Run.NotInitialized", "");
    return RESULT_FAIL;
  }
  
  Result result = RESULT_OK;
  
  const bool imageFileProvided = !_imageFilename.empty();
  
  bool anyFailures = false;
  while(!anyFailures && !ShouldShutdown())
  {
    for(auto & model : _neuralNets)
    {
      const std::string& networkName = model.first;
      std::unique_ptr<NeuralNets::INeuralNetModel>& neuralNet = model.second;
      
      // Is there an image file available in the cache?
      const std::string fullImagePath = (imageFileProvided ?
                                         _imageFilename :
                                         Util::FileUtils::FullFilePath({_cachePath, networkName, Filenames::Image}));
      
      const bool isImageAvailable = Util::FileUtils::FileExists(fullImagePath);
      
      if(isImageAvailable)
      {
        if(neuralNet->IsVerbose())
        {
          LOG_INFO("INeuralNetMain.Run.FoundImage", "%s", fullImagePath.c_str());
        }
        
        // Get the image
        Vision::ImageRGB img;
        {
          ScopedTicToc ticToc("GetImage", LOG_CHANNEL);
          
          const std::string timestampFilename = Util::FileUtils::FullFilePath({_cachePath, networkName, Filenames::Timestamp});
          
          GetImage(fullImagePath, timestampFilename, img);
          
          if(img.IsEmpty())
          {
            LOG_ERROR("INeuralNetMain.Run.ImageReadFailed", "Error while loading image %s", fullImagePath.c_str());
            if(imageFileProvided)
            {
              // If we loaded in image file specified on the command line, we are done
              anyFailures = true;
              break;
            }
            else
            {
              // Remove the corrupted image image to show we are ready for a new one
              if(neuralNet->IsVerbose())
              {
                LOG_INFO("INeuralNetMain.Run.DeletingCorruptedImageFile", "%s", fullImagePath.c_str());
              }
              Util::FileUtils::DeleteFile(fullImagePath);
              continue; // no need to stop the process, it was just a bad image, won't happen again
            }
          }
        }
        
        // Detect what's in it
        std::list<Vision::SalientPoint> salientPoints;
        {
          ScopedTicToc ticToc("Detect", LOG_CHANNEL);
          result = neuralNet->Detect(img, salientPoints);
          
          if(RESULT_OK != result)
          {
            LOG_ERROR("INeuralNetMain.Run.DetectFailed", "");
            result = RESULT_OK; // keep trying (?)
          }
        }
        
        // Convert the results to JSON
        Json::Value detectionResults;
        ConvertSalientPointsToJson(salientPoints, neuralNet->IsVerbose(), detectionResults);
        
        // Write out the Json
        {
          if(!imageFileProvided)
          {
            // Remove the image file now that we're done working with it
            if(neuralNet->IsVerbose())
            {
              LOG_INFO("INeuralNetMain.Run.DeletingImageFile", "%s", fullImagePath.c_str());
            }
            Util::FileUtils::DeleteFile(fullImagePath);
          }
          
          ScopedTicToc ticToc("WriteJSON", LOG_CHANNEL);
          const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, networkName, "tempResult.json"});
          if(neuralNet->IsVerbose())
          {
            LOG_INFO("INeuralNetMain.Run.WritingTempResults", "%s", tempFilename.c_str());
          }
          
          bool success = WriteResults(tempFilename, detectionResults);
          if(!success)
          {
            anyFailures = true;
            break;
          }
          
          const std::string jsonFilename = Util::FileUtils::FullFilePath({_cachePath, networkName, Filenames::Result});
          if(neuralNet->IsVerbose())
          {
            LOG_INFO("INeuralNetMain.Run.MovingToFinalResults", "%s", jsonFilename.c_str());
          }
          success = Util::FileUtils::MoveFile(jsonFilename, tempFilename);
          if(!success)
          {
            anyFailures = true;
            break;
          }
        }
      }
      else if(imageFileProvided)
      {
        LOG_ERROR("INeuralNetMain.Run.ImageFileDoesNotExist", "%s", _imageFilename.c_str());
        anyFailures = true;
        break;
      }
      else if(neuralNet->IsVerbose())
      {
        const int kVerbosePrintFreq_ms = 1000;
        static int count = 0;
        if(count++ * _pollPeriod_ms >= kVerbosePrintFreq_ms)
        {
          LOG_INFO("INeuralNetMain.Run.WaitingForImage", "%s", fullImagePath.c_str());
          count = 0;
        }
      }
      
    } // FOR each model
    
    if(imageFileProvided)
    {
      // No polling needed when a specific image to process is provided, so finish
      break;
    }
    
    Step(_pollPeriod_ms);
    
  } // WHILE should not shutdown
  
  CleanupAndExit( anyFailures ? RESULT_FAIL : RESULT_OK );
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void INeuralNetMain::GetImage(const std::string& imageFilename, const std::string& timestampFilename, Vision::ImageRGB& img)
{
  using namespace Anki;
  const Result loadResult = img.Load(imageFilename);

  if(RESULT_OK != loadResult)
  {
    PRINT_NAMED_ERROR("INeuralNetMain.GetImage.EmptyImageRead", "%s", imageFilename.c_str());
    return;
  }
  
  if(Util::FileUtils::FileExists(timestampFilename))
  {
    std::ifstream file(timestampFilename);
    std::string line;
    std::getline(file, line);
    const TimeStamp_t timestamp = uint(std::stol(line));
    img.SetTimestamp(timestamp);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void INeuralNetMain::ConvertSalientPointsToJson(const std::list<Vision::SalientPoint>& salientPoints,
                                                const bool isVerbose,
                                                Json::Value& detectionResults)
{
  std::string salientPointsStr;
  
  Json::Value& salientPointsJson = detectionResults["salientPoints"];
  for(auto const& salientPoint : salientPoints)
  {
    salientPointsStr += salientPoint.description + "[" + std::to_string((int)round(100.f*salientPoint.score)) + "] ";
    const Json::Value json = salientPoint.GetJSON();
    salientPointsJson.append(json);
  }
  
  if(isVerbose && !salientPoints.empty())
  {
    LOG_INFO("INeuralNetMain.ConvertSalientPointsToJson",
             "Detected %zu objects: %s", salientPoints.size(), salientPointsStr.c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool INeuralNetMain::WriteResults(const std::string& jsonFilename, const Json::Value& detectionResults)
{
  // Write to a temporary file, then move into place once the write is complete (poor man's "lock")
  const std::string tempFilename = jsonFilename + ".lock";
  Json::StyledStreamWriter writer;
  std::fstream fs;
  fs.open(tempFilename, std::ios::out);
  if (!fs.is_open()) {
    LOG_ERROR("INeuralNetMain.WriteResults.OutputFileOpenFailed", "%s", jsonFilename.c_str());
    return false;
  }
  writer.write(fs, detectionResults);
  fs.close();
  
  const bool success = Util::FileUtils::MoveFile(jsonFilename, tempFilename);
  if (!success)
  {
    LOG_ERROR("INeuralNetMain.WriteResults.RenameFail",
              "%s -> %s", tempFilename.c_str(), jsonFilename.c_str());
    return false;
  }
  
  return true;
}
  
} // namespace NeuralNets
} // namespace Anki

