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

// note: uncomment this line to build the parameter sweeping code
// #define ENABLE_PROTOTYPING_ANALYSIS_TESTS

#ifdef ENABLE_PROTOTYPING_ANALYSIS_TESTS
#define private public
#define protected public
#endif

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
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeGTest.h"

// system includes
#include <iomanip>
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

#ifdef ENABLE_PROTOTYPING_ANALYSIS_TESTS
TEST_F(TouchSensorTest, DISABLED_ParameterSweepTouchDetect)
{
  using namespace Anki::Cozmo::ExternalInterface;

  const std::string basePath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, kTestResourcesDir);
  const std::string configFile = "config.json";

  std::vector<std::vector<size_t> > readingsList;
  std::vector<Annotation> annotations;
  LoadAnnotationsFromFile(basePath, configFile, readingsList, annotations);

  // progress tracker helper lambda
  struct ProgressTracker
  {
    int count;
    int total;
    int stride;
    int lastPercent;
    
    ProgressTracker(int total, int stride)
    : count(0) 
    , total(total)
    , stride(stride)
    , lastPercent(0)
    {
    }


    void Tick()
    {
      int percent = std::floor(float(100*(++count))/total);
      if( (lastPercent+stride) == (percent) ) {
        lastPercent = percent;
        PRINT_NAMED_INFO("TestTouchSensor","%d%%",percent);
      }
    }
  };

  // parameters to be sweeped:
  // - debouncer limit (low)
  // - debouncer limit (high)
  // - stdev factor for classification post-calib
  //
  // create a table to be plotted and inspected later
  
  int debounceLimLo;
  int debounceLimHi;
  float stdevFactor;

  struct ParameterEntry
  {
    int dbLimLo;
    int dbLimHi;
    float sdFactor;
  };
  std::vector<ParameterEntry> paramTable;

  struct ResultScores
  {
    float hardTouchPercent;
    float softTouchPercent;
    bool didCalibrate;
    bool noFalsePositiveTouch;
    
    // calibration values
    float calibMean;
    float calibStdev;
  };

  // helper lambda -- feed in touch input values in the interval
  auto helperFeedTouchInputs = [&](const std::vector<size_t>& rst,
                                  TouchSensorComponent& tscomp,
                                  IntervalType interval)
  {
    for(size_t j=interval.first; j<interval.second; ++j) {
      uint16_t touch = rst[j];
      Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
      msg.backpackTouchSensorRaw = touch;
      tscomp.Update(msg);
    }
  };

  std::stringstream parameterSS;

  for(debounceLimLo = 2; debounceLimLo < 6; debounceLimLo++) {
    for(debounceLimHi = 2; debounceLimHi < 6; debounceLimHi++) {
      for(stdevFactor = 1.25f; stdevFactor < 1.75f; stdevFactor += 0.05f) {
        paramTable.push_back({debounceLimLo, debounceLimHi, stdevFactor});
        parameterSS << debounceLimLo << ", " << debounceLimHi << ", "
                    << std::setprecision(3) << std::setw(4)
                    << stdevFactor << std::endl;
      }
    }
  }
  
  Util::FileUtils::WriteFile(basePath+"parameters.csv", parameterSS.str());

  // main test body
  
  ProgressTracker progress(int(readingsList.size() * paramTable.size()), 5);
  std::vector<ResultScores> resultScoresList;
  for(int i=0; i<readingsList.size(); ++i)
  {
    for(int j=0; j<paramTable.size(); ++j)
    {
      TouchSensorComponent tscomp(*_robot);
      
      tscomp._debouncer._loLimit = paramTable[j].dbLimLo;
      tscomp._debouncer._hiLimit = paramTable[j].dbLimHi;
      tscomp._touchDetectStdevFactor = paramTable[j].sdFactor;
      
      const auto& rawTouchSensorReadings = readingsList[i];
      const auto& anot                   = annotations[i];

      // calibrate first
      _msgMonitor->ResetCountTouchButtonEvent();
      helperFeedTouchInputs(rawTouchSensorReadings, tscomp, anot.idle);
      bool noFalsePositiveTouch = _msgMonitor->GetCountTouchButtonEvent()==0;
      
      // compute the "score" for each parameter sweep, using the factors:
      // 
      // - hard strokes percent
      // - soft strokes percent
      // - whether the system is calibrated after the sweep
      // - whether there are spurious touch detects in the idle data
       
      _msgMonitor->ResetCountTouchButtonEvent();
      helperFeedTouchInputs(rawTouchSensorReadings, tscomp, anot.hard);
      float hardPercent = float(_msgMonitor->GetCountTouchButtonEvent())/anot.hardCount;

      _msgMonitor->ResetCountTouchButtonEvent();
      helperFeedTouchInputs(rawTouchSensorReadings, tscomp, anot.soft);
      float softPercent = float(_msgMonitor->GetCountTouchButtonEvent())/anot.softCount;

      resultScoresList.push_back({hardPercent,
                                  softPercent, 
                                  tscomp.IsCalibrated(), 
                                  noFalsePositiveTouch,
                                  tscomp._baselineCalib.GetFilteredTouchMean(),
                                  tscomp._baselineCalib.GetFilteredTouchStdev()});
      
      progress.Tick();
    }
  }

  std::stringstream resultSS;
  for(const auto& scores : resultScoresList)
  {
    resultSS  << std::setprecision(4) << scores.hardTouchPercent << ","
              << std::setprecision(4) << scores.softTouchPercent << ","
              << scores.didCalibrate  << ","
              << scores.noFalsePositiveTouch << ","
              << std::setprecision(4) << scores.calibMean << ","
              << std::setprecision(4) << scores.calibStdev << std::endl;
  }
  Util::FileUtils::WriteFile(basePath+"results.csv", resultSS.str());
}

// An analysis script for parsing multiple logs of touch-sensor data
// from DVT1 robots produced for Victor//
// NOTE: All touch sensor streams are indicating idle touch (no contact)
//
// Inputs:
// Json dumps of touch sensor data, annotated by what the robot was doing
// at the time of data collection. The robot could either be:
// - on charger and stationary
// - off charger and stationary
// - off charger and mobile
//
// Outputs:
// + a csv with a line item for each trial (a trial consists of robot+playpen attempt)
//    where the column headers for the csv are:
//      * robot condition or scenario (charger/mobility state)
//      * mean touch sensor reading for the whole range
//      * standard deviation for the touch sensor readings
//      * the calibrated mean when using the stream as input to the touch sensor
//      * the calibration standard deviation when using the stream as above
//      * index in the touch sensor stream when the calibration completes
//      * whether or not any false positive touches were detected during calib
//
// + a sanity check which calibrates the robot with one touch-sensor stream
//    and then looks for false positives in a differently conditioned touch-sensor
//    stream (on the same robot).
//
//    This check is framed as an aggregate accross all trials where it outputs
//    the percentage of trials that correctly "do not result in false positives"
TEST_F(TouchSensorTest, DISABLED_AnalyzePlayPenTestsDec17)
{
  using namespace Anki::Cozmo::ExternalInterface;
  
  const std::string mfgDataDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,"dvt_dec17");
  
  PRINT_NAMED_INFO("AnalyzePlayPenTests","%s",mfgDataDir.c_str());
  
  ASSERT_TRUE( Util::FileUtils::DirectoryExists(mfgDataDir) );
  
  enum EScenarioType : uint8_t
  {
    OffChargerStationary,
    OnChargerStationary,
    OffChargerMobile,
    Count
  };
  
  typedef std::vector<uint16_t> TouchSensorStream;
  
  // helper struct -- each trial has a condition and touch sensor reading set
  struct PlaypenTrialEntry
  {
    TouchSensorStream readingsByScenario[EScenarioType::Count];
  };
  
  // helper lambda -- converts Json::Array into list of integers
  const auto convertJsonToVector = [](const Json::Value& json)->TouchSensorStream
  {
    TouchSensorStream ss;
    
    for(Json::Value::const_iterator it = json.begin(); it!=json.end(); ++it) {
      ss.push_back(it->asUInt());
    }
    
    return ss;
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // analysis: csv of characteristics of each robot trial, for the purposes of
  //            plotting and visualization for detecting trends and confirming
  //            assumptions.
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  std::vector<std::string> jsonFileList = Util::FileUtils::FilesInDirectory(mfgDataDir,
                                                                            true,
                                                                            "json",
                                                                            true);
  
  PRINT_NAMED_INFO("AnalyzePlayPenTests", "Found %zd jsons", jsonFileList.size());
  
  // load all the data from disk
  std::vector<PlaypenTrialEntry> allTouchSensorTrialData;
  for(const auto& jsonFilename : jsonFileList) {
    PRINT_NAMED_INFO("AnalyzePlayPenTests", "Processing '%s'",
                     Util::FileUtils::GetFileName(jsonFilename).c_str());
    std::string fileContents{ Util::FileUtils::ReadFile(jsonFilename) };
    
    Json::Value trialJsonData;
    Json::Reader reader;
    bool success = reader.parse(fileContents, trialJsonData);
    ASSERT_TRUE(success);
    
    Json::Value& offChrgStat = trialJsonData["TouchSensor_PlaypenSoundCheck"];
    Json::Value& onChrgStat = trialJsonData["TouchSensor_PlaypenDriftCheck"];
    Json::Value& offChrgMobl = trialJsonData["TouchSensor_PlaypenPickupCube"];
    
    ASSERT_TRUE(offChrgStat.isArray());
    ASSERT_TRUE(onChrgStat.isArray());
    ASSERT_TRUE(offChrgMobl.isArray());
    
    allTouchSensorTrialData.push_back(
                                      PlaypenTrialEntry{
                                        .readingsByScenario = {
                                          convertJsonToVector(offChrgStat),
                                          convertJsonToVector(onChrgStat),
                                          convertJsonToVector(offChrgMobl)
                                        }
                                      });
  }
  
  PRINT_NAMED_INFO("AnalyzePlayPenTests", "Parsed %zd touch sensor streams", allTouchSensorTrialData.size());
  
  // compute and organize the data for the CSV dump
  std::stringstream resultStream;
  for(const auto& trial : allTouchSensorTrialData) {
    
    for(uint8_t i=0; i<EScenarioType::Count; ++i) {
      
      auto condition = (EScenarioType)i;
      auto readings = trial.readingsByScenario[condition];
      
      float resultFullMean = 0.0f;
      float resultFullStdev = 0.0f;
      for(const auto& reading : readings) {
        resultFullMean += reading;
      }
      resultFullMean /= readings.size();
      
      for(const auto& reading : readings) {
        const float tmp = reading - resultFullMean;
        resultFullStdev += tmp*tmp;
      }
      resultFullStdev /= readings.size() - 1;
      resultFullStdev = sqrt(resultFullStdev);
      
      
      float resultCalibMean;
      float resultCalibStdev;
      int resultIndexWhenCalib = -1;
      int resultCountFalsePosCalib = 0;
      {
        _msgMonitor->ResetCountTouchButtonEvent();
        TouchSensorComponent tscomp(*_robot);
        
        resultIndexWhenCalib = -1;
        for(int i=0; i<readings.size(); ++i) {
          const auto reading = readings[i];
          Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
          msg.backpackTouchSensorRaw = reading;
          tscomp.Update(msg);
          
          if(resultIndexWhenCalib<0 && tscomp._baselineCalib.IsCalibrated()) {
            resultIndexWhenCalib = i;
          } else if(resultIndexWhenCalib>=0) {
            EXPECT_TRUE(tscomp._baselineCalib.IsCalibrated());
          }
        }
        
        resultCalibMean = tscomp._baselineCalib.GetFilteredTouchMean();
        resultCalibStdev = tscomp._baselineCalib.GetFilteredTouchStdev();
        resultCountFalsePosCalib = _msgMonitor->GetCountTouchButtonEvent();
      }
      
      
      resultStream << resultFullMean << ", ";
      resultStream << resultFullStdev << ", ";
      resultStream << resultCalibMean << ", ";
      resultStream << resultCalibStdev << ", ";
      resultStream << resultIndexWhenCalib << ", ";
      resultStream << resultCountFalsePosCalib << std::endl;
    }
  }
  
  Util::FileUtils::WriteFile(mfgDataDir+"/results.csv", resultStream.str());
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // analysis: sanity check the false positive rate for the condition when
  //  calibrated under different conditions than when testing
  //
  // i.e. when calibrated on the charger, but now receiving off-charger input
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // helper lambda -- convert scenario type to string
  const auto scenToString = [](const EScenarioType t)->std::string{
    const std::string map[EScenarioType::Count] =
    {
      "OffChrgSt",
      "OnChrgSt",
      "OffChrgMob"
    };
    return map[t];
  };
  
  // helper struct -- testing table for no false positives
  struct TestingEntry
  {
    EScenarioType calibCond; // calibrate under this condition
    EScenarioType testCond; // test under this condition
  };
  
  // table of reasonable testing skews that could be
  // representative of real world conditions
  std::vector<TestingEntry> testTable =
  {
    {OffChargerStationary,  OnChargerStationary   },
    {OffChargerStationary,  OffChargerMobile      },
    {OnChargerStationary,   OffChargerStationary  },
    {OnChargerStationary,   OffChargerMobile      },
  };
  
  // compute an aggregate score (the false positive rate) for
  // each testing skew across all the robot trials
  for(int i=0; i<testTable.size(); ++i) {
    size_t numFalsePositives = 0;
    for(const auto& trial : allTouchSensorTrialData) {
      const auto& calibrationReadings = trial.readingsByScenario[ testTable[i].calibCond ];
      const auto& testingReadings = trial.readingsByScenario[ testTable[i].testCond ];
      
      TouchSensorComponent tscomp(*_robot);
      
      // (1) calibrate
      for(int i=0; i<calibrationReadings.size(); ++i) {
        const auto reading = calibrationReadings[i];
        Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw = reading;
        tscomp.Update(msg);
      }
      
      ASSERT_TRUE(tscomp._baselineCalib.IsCalibrated());
      
      // (2) check for false positive touch detections
      _msgMonitor->ResetCountTouchButtonEvent();
      for(int i=0; i<testingReadings.size(); ++i) {
        const auto reading = testingReadings[i];
        Anki::Cozmo::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw = reading;
        tscomp.Update(msg);
      }
      
      numFalsePositives += _msgMonitor->GetCountTouchButtonEvent()>0;
    }
    float successRate = 100.0f*float(allTouchSensorTrialData.size() - numFalsePositives)/allTouchSensorTrialData.size();
    PRINT_NAMED_INFO("AnalyzePlayPenTests",
                     "%10s -> %10s (%2zd / %2zd) = %5.2f %%",
                     scenToString(testTable[i].calibCond).c_str(),
                     scenToString(testTable[i].testCond).c_str(),
                     allTouchSensorTrialData.size() - numFalsePositives,
                     allTouchSensorTrialData.size(),
                     successRate);
  }
}
#endif // ifdef ENABLE_PROTOTYPING_ANALYSIS_TESTS
