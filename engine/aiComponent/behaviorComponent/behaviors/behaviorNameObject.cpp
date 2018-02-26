/**
 * File: BehaviorNameObject.cpp
 *
 * Author: ross
 * Created: 2018-02-25
 *
 * Description: the robot will name the object
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorNameObject.h"

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/encodedImage.h"
#include "engine/externalInterface/externalInterface.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/objectDetector.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/boundedWhile.h"

#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorNameObject::BehaviorNameObject(const Json::Value& config)
  : ICozmoBehavior(config)
  , _encodedImage( new EncodedImage() )
  , _image( new Vision::ImageRGB )
{
  AddWaitForUserIntent( USER_INTENT(name_object) );
  
  
  
  
  
  
  SubscribeToTags({
    ExternalInterface::MessageEngineToGameTag::ImageChunk
  });
  
}
  
  void BehaviorNameObject::InitBehavior()
  {
    _thread.reset( new std::thread(&BehaviorNameObject::ClassifierThread, this) );
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorNameObject::~BehaviorNameObject()
{
  {
    std::lock_guard<std::mutex> lock( _mutex );
    _runThread = false;
    _condition.notify_one();
  } // unlock
  
  if( _thread ) {
    _thread->join();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorNameObject::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorNameObject::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorNameObject::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorNameObject::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  // todo: fgigure out how dvars can hold non-movable data or data shared between threads
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    _runThread = true;
    _imageAvailable = false;
  }
  _madeAttempt = false;
  
  PRINT_NAMED_WARNING("Result", "REQUESTING IMAGE CHUNK!");
  
  ExternalInterface::SetRobotImageSendMode m{ImageSendMode::SingleShot};
  ExternalInterface::MessageGameToEngine message;
  message.Set_SetRobotImageSendMode(m);
  GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( std::move(message) );
  
  // this doesnt seem to be used
//  ExternalInterface::ImageRequest m;
//  m.mode = ImageSendMode::SingleShot;
//  ExternalInterface::MessageGameToEngine message;
//  message.Set_ImageRequest(m);
//  GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( std::move(message) );
  
}
 
  
void BehaviorNameObject::HandleWhileActivated(const EngineToGameEvent& event)
{
  if( _madeAttempt ) {
    return;
  }
  
  if( event.GetData().GetTag() != ExternalInterface::MessageEngineToGameTag::ImageChunk ) {
    return;
  }
  
  PRINT_NAMED_WARNING("Result", "GOT IMAGE CHUNK!");
  
  ImageChunk msg = event.GetData().Get_ImageChunk();
  const bool isImageReady = _encodedImage->AddChunk(msg);
  
  if(isImageReady) {
    Vision::ImageRGB img; // maybe should instead copy directly?
    Result result = _encodedImage->DecodeImageRGB(img);
    PRINT_NAMED_WARNING("Result", "FINISHED IMAGE CHUNK!");
    if( !ANKI_VERIFY(RESULT_OK == result, "","") ) {
      return;
    }
    
    _madeAttempt = true;
    // todo: turn off sending of images? or maybe not needed with singleshot
    
    PRINT_NAMED_WARNING("Result", "NOTFYING");
    {
      std::lock_guard<std::mutex> lock{ _mutex };
      PRINT_NAMED_WARNING("Result", "Copying");
      img.CopyTo( *_image.get() );
      PRINT_NAMED_WARNING("Result", "done Copying");
      _imageAvailable = true;
      PRINT_NAMED_WARNING("Result", "now true");
      _condition.notify_one();
    } // unlock
    
    PRINT_NAMED_WARNING("Result", "should be done");
    
    
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorNameObject::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  std::string detectionResult;
  
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    
    if( _detectionResult.empty() ) {
      return;
    }
    detectionResult = _detectionResult;
    _detectionResult.clear();
  } // unlock
  
  
  PRINT_NAMED_WARNING("Result", "Result='%s'",detectionResult.c_str());
  // there's a result in detectionResult
  
  CompoundActionSequential* finalAnimation = new CompoundActionSequential();
  
  {
    // say the word
    SayTextAction* sayNameAction1 = new SayTextAction(detectionResult, SayTextIntent::Unprocessed);

    // todo: is this line needed?
    sayNameAction1->SetAnimationTrigger(AnimationTrigger::MeetCozmoFirstEnrollmentSayName);

    finalAnimation->AddAction(sayNameAction1);
  }
  
  {
    // celebration
    TriggerAnimationAction* celebrateAction = new TriggerAnimationAction(AnimationTrigger::MeetCozmoFirstEnrollmentCelebration);
    finalAnimation->AddAction(celebrateAction);
  }
  
  DelegateIfInControl( finalAnimation, [this](ActionResult result){
    if(ActionResult::SUCCESS != result) {
      PRINT_NAMED_WARNING("BehaviorEnrollFace.TransitionToSayingName.FinalAnimationFailed", "");
    }
    CancelSelf();
  });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorNameObject::ClassifierThread()
{
  Vision::ObjectDetector detector;
  
  auto* context = GetBEI().GetRobotInfo().GetContext();
  const std::string modelPath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,"config/engine/mobilenet");//std::string(getenv("TEST_DATA_PATH")) + " test/dnn_models";
  const std::string testImagePath = context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,"test/images"); //std::string(getenv("TEST_DATA_PATH"))
  
  
  if(Util::FileUtils::DirectoryDoesNotExist(modelPath))
  {
    PRINT_NAMED_WARNING("Result", "Model path not found: %s", modelPath.c_str());
    return;
  } else {
    PRINT_NAMED_WARNING("Result", "model path exists");
  }
  
  Json::Value config;
  config["graph"] = "mobilenet_v1_for_dnn_softmax.pb";// "mobilenet_v1_1.0_224_opencvdnn.pb";
  config["labels"] = "labels.txt"; //mobilenet_labels.txt";
  config["mode"] = "classification";
  config["input_width"] = 224;
  config["input_height"] = 224;
  config["do_crop"] = false;
  config["input_mean_R"] = 0;
  config["input_mean_G"] = 0;
  config["input_mean_B"] = 0;
  config["input_std"] = 255;
  config["input_layer"] = "input";
  config["output_scores_layer"] = "MobilenetV1/Predictions/Softmax";
  config["top_K"] = 1;
  config["min_score"] = 0.1f;
  
  Result result = detector.Init(modelPath, config);
  
  if( !ANKI_VERIFY( result == RESULT_OK, "","") ) {
    PRINT_NAMED_WARNING("Result", "bad init");
    return;
  }
  
  PRINT_NAMED_WARNING("Result", "init complete");
  
  while( true ) {
    
    Vision::ImageRGB image;
    
    {
      std::unique_lock<std::mutex> lock{ _mutex };
      _condition.wait(lock, [this]() {
        return _imageAvailable || !_runThread;
      });
      
      PRINT_NAMED_WARNING("Result", "got image!");
      
      if( !_runThread ) {
        break;
      }
      
      _imageAvailable = false;
      
      _image->CopyTo(image);
    } // unlock
    
  
    Vision::ImageCache imageCache;
    std::vector<std::string> objectNames;
    
    if( image.IsEmpty() ) {
      PRINT_NAMED_WARNING("Result", "Empty image (unit tests)");
    } else {
      imageCache.Reset( image );
      result = DetectionHelper(detector, imageCache, objectNames);
    }
    
    {
      std::lock_guard<std::mutex> lock{ _mutex };
      
      if( !_runThread ) {
        break;
      }
      
      const std::string idk = "I dont know"; // todo: does TTS say this correctly?
      if( !ANKI_VERIFY(result == RESULT_OK, "","") ) {
        _detectionResult = idk;
      }
      
      for( const auto& name : objectNames ) {
        PRINT_NAMED_WARNING("Result", "Found %s in image\n", name.c_str());
      }
      
      if( ANKI_DEV_CHEATS ) {
        
        Json::Value toWebViz;
        
        toWebViz["objects"] = Json::arrayValue;
        auto& webVizNames = toWebViz["objects"];
        for( const auto& name : objectNames ) {
          webVizNames.append( name );
        }
        
        float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        toWebViz["time"] = currTime;
        const auto* webService = GetBEI().GetRobotInfo().GetContext()->GetWebService();
        const std::string moduleName = "ObjectsSeen";
        webService->SendToWebViz( moduleName, toWebViz );
      }
      
      if( !objectNames.empty() ) {
        _detectionResult = objectNames.front();
      } else {
        _detectionResult = idk;
      }
      PRINT_NAMED_WARNING("Result", "result=%s", _detectionResult.c_str());
    } // unlock
  }
}
  
Result BehaviorNameObject::DetectionHelper(Vision::ObjectDetector& detector,
                                           Vision::ImageCache& imageCache,
                                           std::vector<std::string>& objectNames) const
{
  const bool started = detector.StartProcessingIfIdle(imageCache);
  if( !started ){
    PRINT_NAMED_WARNING("Result","didnt start");
    return RESULT_FAIL;
  }
  
  objectNames.clear();
  std::list<Vision::ObjectDetector::DetectedObject> objects;
  
  bool gotResult = false;
  BOUNDED_WHILE( 500, (gotResult = detector.GetObjects(objects)) == false ) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  
  if( !gotResult ) {
    PRINT_NAMED_WARNING("Result","didnt get result");
    return RESULT_FAIL;
  }
  
  std::transform( objects.begin(),
                  objects.end(),
                  std::back_inserter(objectNames), [](const auto& obj)
  {
    return obj.name.substr( obj.name.find(":") + 1 );
  });
  
  return RESULT_OK;
}

} // namespace
} // namespace
