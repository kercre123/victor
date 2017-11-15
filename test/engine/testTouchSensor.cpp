/**
 * File: testTouchSensor.cpp
 *
 * Author: Arjun Menon
 * Created: 7/10/2017
 *
 * Description: Unit/regression tests for Victor's Touch Sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 * --gtest_filter=TouchSensor*
 **/

// core testing focus
#include "engine/components/sensors/touchSensorComponent.h"

// victor boilerplate
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/signals/simpleSignal_fwd.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/events/ankiEvent.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"

// test infra
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeGTest.h"

// system includes
#include <stdio.h>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Cozmo;

const std::string kTestResourcesDir = "test/touchSensorTests/";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// helper class to track messaging events for test purposes
class TestTouchSensorMessageMonitor
{
public:

  TestTouchSensorMessageMonitor(const CozmoContext* context)
  : _touchButtonEventCount(0)
  , _lastTouchButtonEventPressed(false)
  {
    using namespace Anki::Cozmo::ExternalInterface;

    IExternalInterface* extInterface = context->GetExternalInterface();

    auto countTouchButtonEventCb = 
    [this](const AnkiEvent<MessageEngineToGame>& event) {
      this->CountTouchButtonEvent(event.GetData().Get_TouchButtonEvent().isPressed);
    };

    _signalHandles.push_back( 
      extInterface->Subscribe(
        MessageEngineToGameTag::TouchButtonEvent , 
        countTouchButtonEventCb));
  }

  size_t GetCountTouchButtonEvent() const { return _touchButtonEventCount; }
  void ResetCountTouchButtonEvent()       { _touchButtonEventCount = 0;    }

protected:

  void CountTouchButtonEvent(bool isPressed)
  {
    if(_lastTouchButtonEventPressed && !isPressed) {
      _touchButtonEventCount++;
    }
    _lastTouchButtonEventPressed = isPressed;
  }

private:

  std::vector<Signal::SmartHandle> _signalHandles;

  size_t _touchButtonEventCount;

  bool _lastTouchButtonEventPressed;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
std::pair<size_t, size_t> ParseTupleFromJsonArray(const Json::Value& v)
{
  const size_t beg = JsonTools::GetValue<size_t>(v[0]);
  const size_t end = JsonTools::GetValue<size_t>(v[1]);
  return {beg, end};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// helper struct to organize the data from each annotation

typedef std::pair<size_t, size_t> IntervalType;

struct Annotation
{
  IntervalType idle;
  IntervalType hold;
  IntervalType soft;
  size_t softCount;
  IntervalType hard;
  size_t hardCount;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
class TouchSensorTest : public testing::Test
{
public:

  virtual void SetUp() override
  {
    _handler = new UiMessageHandler(0, nullptr);
    _messagingContext = new CozmoContext(nullptr, _handler);
    _robot = new Robot(0, _messagingContext);
    _msgMonitor = new TestTouchSensorMessageMonitor(_messagingContext);
  }

  virtual void TearDown() override
  {
    Util::SafeDelete(_msgMonitor);
    Util::SafeDelete(_robot);
    Util::SafeDelete(_messagingContext);
    Util::SafeDelete(_handler);
  }

  // file-io helper to load the annotations from disk
  void LoadAnnotationsFromFile(const std::string& basePath,
                               const std::string& configFile,
                               std::vector< std::vector<size_t> >& rawTouchSensorReadingsList,
                               std::vector<Annotation>& annotations)
  {
    annotations.clear();
    rawTouchSensorReadingsList.clear();

    const std::string fileContents{Util::FileUtils::ReadFile(basePath+configFile)};

    Json::Value configs;
    Json::Reader reader;
    bool success = reader.parse(fileContents, configs);
    ASSERT_TRUE(success);

    for(const auto& testConfig : configs) {
      const std::string rawFilename = JsonTools::ParseString(testConfig, "filename", "");
      ASSERT_TRUE(!rawFilename.empty());

      // read all touch sensor values
      const std::string rawFilepath{basePath + rawFilename};
      const std::string rawFileContents{Util::FileUtils::ReadFile(rawFilepath)};

      std::vector<size_t> fileTouchSensorReadings;
      std::istringstream is(rawFileContents);
      std::string item;
      while(getline(is,item,'\n')) {
        fileTouchSensorReadings.push_back( std::stoi(item) );
      }

      PRINT_NAMED_INFO("TestTouchSensor", "Read %zd values from file (%s)",
          fileTouchSensorReadings.size(),
          rawFilepath.c_str());

      // load annotation info
      Annotation anot;
      const Json::Value& intervalsJson = testConfig["interval_descriptions"];
      ASSERT_TRUE(intervalsJson.isArray());
      for(const auto& intervalDesc : intervalsJson) {
        const std::string tag = JsonTools::ParseString(intervalDesc, "tag", "");
        const Json::Value& range = intervalDesc["range"];
        if(tag.compare("idle")==0) {
          anot.idle = ParseTupleFromJsonArray(range);
        } else if(tag.compare("hold")==0) {
          anot.hold = ParseTupleFromJsonArray(range);
        } else if(tag.compare("soft")==0) {
          anot.soft = ParseTupleFromJsonArray(range);
          anot.softCount = JsonTools::GetValue<size_t>( (intervalDesc)["count"] );
        } else if(tag.compare("hard")==0) {
          anot.hard = ParseTupleFromJsonArray(range);
          anot.hardCount = JsonTools::GetValue<size_t>( (intervalDesc)["count"] );
        } else {
          PRINT_NAMED_ERROR("TestTouchSensor","Not a valid tag type (%s)", tag.c_str());
          ASSERT_TRUE(false);
        }
      }

      annotations.push_back(anot);
      rawTouchSensorReadingsList.push_back( fileTouchSensorReadings );
    }
  }

public: 
  UiMessageHandler*  _handler;

  CozmoContext*      _messagingContext;

  Robot*             _robot;

  TestTouchSensorMessageMonitor* _msgMonitor;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
TEST_F(TouchSensorTest, CalibrateAndClassifyMultirobotTests)
{
  using namespace Anki::Cozmo::ExternalInterface;

  const std::string basePath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, kTestResourcesDir);
  const std::string configFile = "config.json";

  // messages are every 30ms, so time to calibrate is capped at 9s
  // XXX: this is currently too high so should be corrected in VIC-648
  constexpr size_t kMaxMessagesTilCalib = 300;
  
  std::vector<std::vector<size_t> > readingsList;
  std::vector<Annotation> annotations;
  LoadAnnotationsFromFile(basePath, configFile, readingsList, annotations);

  for(int i=0; i<readingsList.size(); ++i) {

    const auto& rawTouchSensorReadings = readingsList[i];
    const auto& anot                   = annotations[i];

    // administer the test:
    // - given raw touch sensor data
    // - annotations on the raw touch sensor data
    // by checking the following properties:
    // - calibration is completed after feeding in the "idle" intervals
    // - the hold gesture is detected after feeding in hold interval
    // - the stroking gesture is detected after feeding in hard-stroke data
    // - the number of detected touches matches for hard-strokes
    // - the number of detected touches matches for soft-strokes (within reason)
    
    TouchSensorComponent testTouchSensorComponent(*_robot);

    auto feedAndTestHelper = [&](std::pair<size_t,size_t> interval,
                                 const std::function<void(void)>& testBody)
    {
      const size_t begIdx = interval.first;
      const size_t endIdx = interval.second;

      for(size_t j=begIdx; j<endIdx; ++j) {
        uint16_t touch = rawTouchSensorReadings[j];
        Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw = touch;
        testTouchSensorComponent.Update(msg);
      }
      testBody();
    };

    // test idle data -> calibration
    {
      const size_t begIdx = anot.idle.first;
      const size_t endIdx = anot.idle.second;

      size_t indexWhenCalib = std::numeric_limits<size_t>::max();

      for(size_t j=begIdx; j<endIdx; ++j) {
        uint16_t touch = rawTouchSensorReadings[j];
        Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw = touch;
        testTouchSensorComponent.Update(msg);

        if(testTouchSensorComponent.IsCalibrated()) {
          indexWhenCalib = j;
          break;
        }
      }

      // check that we calibrate before the worst expected time to calibrate
      EXPECT_TRUE(indexWhenCalib < kMaxMessagesTilCalib);
      
      // reset the counts to measure that the remainder of the touch input
      _msgMonitor->ResetCountTouchButtonEvent();

      for(size_t j=indexWhenCalib; j<endIdx; ++j) {
        EXPECT_TRUE(testTouchSensorComponent.IsCalibrated());
      }

      EXPECT_EQ(0, _msgMonitor->GetCountTouchButtonEvent());
      _msgMonitor->ResetCountTouchButtonEvent();
    }


    // test hold data -> detects a hold event
    std::function<void(void)> testHoldCase = [&](){
      EXPECT_EQ(1, _msgMonitor->GetCountTouchButtonEvent());
    };
    feedAndTestHelper(anot.hold, testHoldCase);
    _msgMonitor->ResetCountTouchButtonEvent();


    // test stroke data
    // IMPORTANT:
    // we send a touch button event, every time the cap sensor is considered
    // pressed AND released
    // therefore for each stroke, we have two messages
    // hence the factor for multiplication

    // HARD presses
    std::function<void(void)> testHardCase = [&](){
      EXPECT_EQ(anot.hardCount, _msgMonitor->GetCountTouchButtonEvent());
      EXPECT_TRUE(testTouchSensorComponent.IsCalibrated());
    };
    feedAndTestHelper(anot.hard, testHardCase);
    _msgMonitor->ResetCountTouchButtonEvent();

    // SOFT presses
    // XXX: currently this test is disabled temporarily because soft-touches are
    //      extremely adversarial, and until we can improve the reliability of 
    //      detecting soft touches without increasing the number of false 
    //      positives.
    //      VIC-648 seeks to address this issue
    std::function<void(void)> testSoftCase = [&](){
      // const float percentStrokeDetected = (float)_msgMonitor->GetCountTouchButtonEvent()/(anot.softCount);
      // EXPECT_GE(percentStrokeDetected, 0.125-Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
      EXPECT_TRUE(testTouchSensorComponent.IsCalibrated());
    };
    feedAndTestHelper(anot.soft, testSoftCase);
    _msgMonitor->ResetCountTouchButtonEvent();
  }
}
