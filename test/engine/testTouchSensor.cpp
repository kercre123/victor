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
#include "engine/robotComponents_fwd.h"

// test infra
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeGTest.h"
#include "util/string/stringUtils.h"

// system includes
#include <iomanip>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>

extern Anki::Vector::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Vector;

namespace {
  
  const std::string kTestResourcesDir = "test/touchSensorTests/";
  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// helper class to track messaging events for test purposes
class TestTouchSensorMessageMonitor
{
public:

  TestTouchSensorMessageMonitor(const CozmoContext* context)
  : _touchButtonEventCount(0)
  , _lastTouchButtonEventPressed(false)
  {
    using namespace Anki::Vector::ExternalInterface;

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
    _robot = new Robot(0, cozmoContext);
    _msgMonitor = new TestTouchSensorMessageMonitor(_robot->GetContext());
  }

  virtual void TearDown() override
  {
    Util::SafeDelete(_msgMonitor);
    Util::SafeDelete(_robot);
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
  Robot*             _robot;

  TestTouchSensorMessageMonitor* _msgMonitor;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
TEST_F(TouchSensorTest, DISABLED_CalibrateAndClassifyMultirobotTests)
{
  using namespace Anki::Vector::ExternalInterface;

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
    
    TouchSensorComponent testTouchSensorComponent;
    RobotCompMap empty;
    testTouchSensorComponent.InitDependent(_robot, empty);
    auto feedAndTestHelper = [&](std::pair<size_t,size_t> interval,
                                 const std::function<void(void)>& testBody)
    {
      const size_t begIdx = interval.first;
      const size_t endIdx = interval.second;

      for(size_t j=begIdx; j<endIdx; ++j) {
        uint16_t touch = rawTouchSensorReadings[j];
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(touch);
        testTouchSensorComponent.NotifyOfRobotState(msg);
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
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(touch);
        testTouchSensorComponent.NotifyOfRobotState(msg);

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

namespace {
  
  // for the purpose of the tests, this number is an upperbound
  // on the number of readings required by the implementation in
  // touchSensorComponent.cpp
  const int kMinNumReadingsToCalibrate = 91;
  
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
  using namespace Anki::Vector::ExternalInterface;
  
  RobotCompMap dummyCompMap;
  
  const std::string mfgDataDir = "";
  // const std::string mfgDataDir = "/Users/arjunm/Code/data/touch_sensor/playpen_dvt1/copy";
  
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
  
  // remove the following named ones: they were identified in a prior pass as invalid
  {
    std::vector<std::string> invalidFiles = {
      "1e1918f5-0001.json",
      "1d58880c-0004.json",
      "1d58881a-0005.json",
      "1e58d9e6-0008.json",
    };
    for(auto s : invalidFiles) {
      auto got = std::find_if(jsonFileList.begin(), jsonFileList.end(),
                              [s=s](std::string& other)->bool {
                                return s.compare( Util::FileUtils::GetFileName(other) )==0;
                              }
                              );
      if(got!=jsonFileList.end()) {
        jsonFileList.erase(got);
      }
    }
  }
  
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
      
      
      _msgMonitor->ResetCountTouchButtonEvent();
      TouchSensorComponent tscomp;
      tscomp.InitDependent(_robot,dummyCompMap);
      
      DEV_ASSERT(readings.size()>=kMinNumReadingsToCalibrate, "NotEnoughReadingsToCalibrate");
      
      for(int i=0; i<kMinNumReadingsToCalibrate; ++i) {
        const auto reading = readings[i];
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(reading);
        tscomp.NotifyOfRobotStateInternal(msg);
      }
      
      resultStream << (int)i << ", ";
      resultStream << resultFullMean << ", ";
      resultStream << resultFullStdev << ", ";
      resultStream << tscomp.IsCalibrated() << std::endl;
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
    size_t total = 0;
    size_t numDidCalib = 0;
    size_t numFalsePositives = 0;
    for(const auto& trial : allTouchSensorTrialData) {
      total++;
      const auto& calibrationReadings = trial.readingsByScenario[ testTable[i].calibCond ];
      const auto& testingReadings = trial.readingsByScenario[ testTable[i].testCond ];
      
      TouchSensorComponent tscomp;
      
      tscomp.InitDependent(_robot,dummyCompMap);
      
      // (1) calibrate
      DEV_ASSERT(calibrationReadings.size()>=kMinNumReadingsToCalibrate, "NotEnoughReadingsToCalibrate");
      for(int i=0; i<kMinNumReadingsToCalibrate; ++i) {
        const auto reading = calibrationReadings[i];
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(reading);
        tscomp.NotifyOfRobotStateInternal(msg);
      }
      
      if( tscomp.IsCalibrated() ) {
        numDidCalib++;
      }
      
      // (2) check for false positive touch detections
      _msgMonitor->ResetCountTouchButtonEvent();
      for(int i=0; i<testingReadings.size(); ++i) {
        const auto reading = testingReadings[i];
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(reading);
        tscomp.NotifyOfRobotStateInternal(msg);
      }
      
      numFalsePositives += _msgMonitor->GetCountTouchButtonEvent()>0;
    }
    const auto pct = [](size_t n, size_t t)->float {
      return 100.0f * n / t;
    };
    float calibRate = pct(numDidCalib, total);
    float noFalsePosRate = pct(numDidCalib-numFalsePositives,numDidCalib);
    PRINT_NAMED_INFO("AnalyzePlayPenTests",
                     "%10s -> %10s | %6.2f%% = %3zd / %3zd | %6.2f%% = %3zd / %3zd",
                     scenToString(testTable[i].calibCond).c_str(),
                     scenToString(testTable[i].testCond).c_str(),
                     calibRate, numDidCalib, total,
                     noFalsePosRate, numDidCalib-numFalsePositives, numDidCalib);
  }
}

// this test takes as inputs:
// - a dataset of touch sensor readings for multiple robots, under various
//   electrical environment conditions (motoring and charger status) and
//   for various forms of touch (ranging between no-touch, and full palm)
// - an answerkey that labels each of the touch sensor readings with the
//   expected number of touches to be seen per file
//
// So an example data set might contain for robot1, a sample of readings
// taken when the robot was idle (not moving motors) and on the charger
// and it was pet 9 times (softly).
//
// For the initial run of this test, data was collected the skews of:
//  - 7 robots
//  - idle vs moving motors
//  - on charger vs off charger
//  - experiencing 4 types of touch (no-touch, petting soft, petting-hard
//    and sustained contact)
//
// With this data, the tests are constructed to first calibrate and count
// the number of successful calibrations. After calibration, then the test
// passes in a set of readings, and uses the answer key to determine if
// the accuracy is high
//
// Aggregating this data across all the robots, and touch types, and across
// various testing conditions, this test is helpful in determining the
// efficacy of the touch detection algorithm
TEST_F(TouchSensorTest, DISABLED_AnalyzeManualDataCollection)
{
  using namespace std;
  
  // answer key is formatted:
  // ID_MOTORSTATE_CHARGERSTATE_TOUCHTYPE, numTouches
  //
  // so the following line item reads
  // 1d182069_Idle_OffCharger_No_Touch, 0
  // ID           = 1d182069
  // MOTORSTATE   = Idle
  // CHARGERSTATE = OffCharger
  // TOUCHTYPE    = No_Touch
  
  const std::string dataDir = "/Users/arjunm/Code/data/touch_sensor/manual_dvt1/touchDataCollection/rename";
  
  // parse the answerkey into a map
  std::string answerKeyFile = dataDir + "/answerkey";
  std::string answerKeyContents { Util::FileUtils::ReadFile(answerKeyFile) };
  std::vector<std::string> lines = Anki::Util::StringSplit(answerKeyContents,'\n');
  std::map<std::string,int> answerKey;
  for(const auto& line : lines) {
    std::vector<std::string> items = Anki::Util::StringSplit(line,',');
    answerKey[items[0]] = std::stoi(items[1]);
  }
  PRINT_NAMED_INFO("AnalyzeManualDataCollection", "Loaded Answer Key (%zd entries)", answerKey.size());
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // all possible signals are listed here
  const std::vector<std::string> robotIDs =
  {
    "1d182069",
    "1e99ea7b",
    "1ed88710",
    "1ed8889d",
    "1f19f8bf",
    "50156d80",
    "d3ef4932",
  };
  
  const std::vector<std::string> motorStates =
  {
    "Idle",
    "Moving"
  };
  
  const std::vector<std::string> chargerStates =
  {
    "OffCharger",
    "OnCharger"
  };
  
  const std::vector<std::string> touchTypes =
  {
    "No_Touch",
    "Sustained_Contact",
    "Petting_Soft",
    "Petting_Hard"
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Test infrastructure
  struct Score {
    int total;
    int numCalib;
    int numCorrect; // meets minimum percent correct
    
    int numTouchExpected;
    int numTouchDetected;
    
    int numFalsePos;
    
    Score()
    : total(0)
    , numCalib(0)
    , numCorrect(0)
    , numTouchExpected(0)
    , numTouchDetected(0)
    , numFalsePos(0)
    {
    }
    
    Score(int tot, int cal, int cor, int exp, int det, int fal)
    : total(tot)
    , numCalib(cal)
    , numCorrect(cor)
    , numTouchExpected(exp)
    , numTouchDetected(det)
    , numFalsePos(fal)
    {
    }
    
    Score operator+(const Score& rhs) const {
      return Score(
                   total+rhs.total,
                   numCalib+rhs.numCalib,
                   numCorrect+rhs.numCorrect,
                   numTouchExpected+rhs.numTouchExpected,
                   numTouchDetected+rhs.numTouchDetected,
                   numFalsePos+rhs.numFalsePos
                   );
    }
  };
  
  // helper lambda
  const auto LoadReadingsFromFile = [](const std::string& filename)->std::vector<int> {
    std::string contents{ Anki::Util::FileUtils::ReadFile(filename) };
    std::vector<std::string> items = Anki::Util::StringSplit(contents,'\n');
    std::vector<int> ret;
    std::for_each(items.begin(), items.end(), [&ret](const std::string& s)->void { ret.push_back(std::stoi(s)); });
    return ret;
  };
  
  RobotCompMap dummyCompMap;
  
  typedef function<Score(const vector<int>&, const vector<int>&, const int, const float)> TestFuncType;
  
  TestFuncType MainCalibAndTestScheme = [this,&dummyCompMap]
  (const std::vector<int>& calib,
   const std::vector<int>& test,
   const int expectedTouchCount,
   const float minPercentCorrect)
  -> Score
  {
    _msgMonitor->ResetCountTouchButtonEvent();
    TouchSensorComponent tscomp;
    tscomp.InitDependent(_robot,dummyCompMap);
    Score result;
    

    DEV_ASSERT(calib.size()>=kMinNumReadingsToCalibrate, "NotEnoughReadingsToCalibrate");
    for(int i=0; i<kMinNumReadingsToCalibrate; ++i) {
      const auto reading = calib[i];
      Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
      msg.backpackTouchSensorRaw.fill(reading);
      tscomp.NotifyOfRobotStateInternal(msg);
    }
    
    result.total++;
    
    if(tscomp.IsCalibrated()) {
      result.numCalib++;
      
      result.numTouchExpected += expectedTouchCount;
      
      _msgMonitor->ResetCountTouchButtonEvent();
      for(int i=0; i<test.size(); ++i) {
        const auto reading = test[i];
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw.fill(reading);
        tscomp.NotifyOfRobotStateInternal(msg);
      }
      
      result.numTouchDetected = (int)_msgMonitor->GetCountTouchButtonEvent();
      
      float percentDetected = float(result.numTouchDetected)/expectedTouchCount;
      
      if(expectedTouchCount == 0) {
        result.numFalsePos += _msgMonitor->GetCountTouchButtonEvent();
      }
      
      result.numCorrect += percentDetected > minPercentCorrect;
    }
    
    return result;
  };
  
  const auto printScore = [](const Score& s)->std::string {
    std::stringstream ss;
    ss << std::setw(4) << s.numTouchDetected << ", ";
    ss << std::setw(4) << s.numTouchExpected;
    return ss.str();
  };
  
  const auto getFileKey = [](const string& robot,
                             const string& motorS,
                             const string& chargerS,
                             const string& touchT)->string {
    return string{robot + "_" + motorS + "_" + chargerS + "_" + touchT};
  };
  
  // XXX(agm) switch this to test a different mode
  auto func = MainCalibAndTestScheme;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // testing skews
  // - same motor+charger state for calib and test
  // - different motor+charger state for calib and test
  
  // === SAME MOTOR & CHARGER STATE ===
  
  struct MotorChargerState
  {
    int motorIdx;
    int chargerIdx;
  };
  
  std::vector<MotorChargerState> sameMotorChargerStateSkews =
  {
    {0,0},
    {0,1},
  };

  const float kMinPercent = 0.5;
  std::stringstream output;
  
  for(const auto& cfg : sameMotorChargerStateSkews) {
    output << endl << endl << endl;
    std::string motorState = motorStates[cfg.motorIdx];
    std::string chargerState = chargerStates[cfg.chargerIdx];
    output << "Calib Condition: " << motorState << " " << chargerState << endl;
    for(const auto& touchType : vector<string>{"Petting_Soft","Petting_Hard"}) {
      output << "ACCURACY TESTING (" << touchType << ")" << endl;
      
      // header
      output << "\t" << std::setw(10) << "";
      output << std::setw(4) << "DET" << std::setw(2) << "";
      output << std::setw(4) << "EXP" << std::endl;
      
      Score score_tot;
      for(const auto& robot : robotIDs) {
        Score score_rob;
        
        // calibrate
        std::string calibKey = getFileKey(robot,motorState,chargerState,"No_Touch");
        std::vector<int> calib = LoadReadingsFromFile(dataDir+"/"+calibKey);
        
        // test and score
        std::string testKey = getFileKey(robot,motorState,chargerState,touchType);
        std::vector<int> test = LoadReadingsFromFile(dataDir+"/"+testKey);
        score_rob = func(calib, test, answerKey[testKey], kMinPercent);
        score_tot = score_tot + score_rob;
        output << "\t" << std::setw(10) << robot << printScore(score_rob) << endl;
      }
      output << "\t" << std::setw(10) << "all" << printScore(score_tot) << endl;
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // === DIFF MOTOR & CHARGER STATE ===
  
  
  struct CalibAndTestSkew
  {
    MotorChargerState calib;
    MotorChargerState test;
  };
  
  std::vector<CalibAndTestSkew> diffMotorChargerStateSkews =
  {
    {{0,0},{0,1}}, //IdleOff -> IdleOn
    {{0,0},{1,0}}, //IdleOff -> MotorsOff
    
    {{0,1},{0,0}}, //IdleOn -> IdleOff
    {{0,1},{1,0}}, //IdleOn -> MotorsOff
  };
  
  output << std::endl << std::endl << std::endl;
  output << "FALSE POSITIVE TESTING" << endl;
  
  for(const auto& skew : diffMotorChargerStateSkews) {
    const auto& motorStateCalib = motorStates[skew.calib.motorIdx];
    const auto& chargerStateCalib = chargerStates[skew.calib.chargerIdx];
    const auto& motorStateTest = motorStates[skew.test.motorIdx];
    const auto& chargerStateTest = chargerStates[skew.test.chargerIdx];
    
    output << "Calib Condition: " << motorStateCalib << " " << chargerStateCalib << endl;
    output << "Test Condition: " << motorStateTest << " " << chargerStateTest << endl;
    
    Score score_tot;
    
    for(const auto& robot : robotIDs) {
      output << "\tRobot " << robot << " ";
      std::vector<int> calib = LoadReadingsFromFile(dataDir+"/"+getFileKey(robot,motorStateCalib,chargerStateCalib,"No_Touch"));
      const auto testKey = getFileKey(robot,motorStateTest,chargerStateTest,"No_Touch");
      std::vector<int> test = LoadReadingsFromFile(dataDir+"/"+testKey);
      
      Score score_rob = func(calib, test, answerKey[testKey], kMinPercent);
      score_tot = score_tot + score_rob;
      
      output << (int)score_rob.numFalsePos << endl;
    }
    
    output << "\tRobot all " << (int)score_tot.numFalsePos << endl;
  }
  
  std::cout << output.str();
}

// this test harness uses data collected from real robots
// and sweeps through the parameter space of the touch
// detection algorithm and ouputs a table whose rows consist
// of the following:
//
// * params used in the algorithm (referred to as a skew)
// * whether calibration succeeded (boolean)
// * accuracy on real touches detected
// * num false positive touches detected
// * num extra detections in the true positive scenario
//
// since there are multiple robots tested in this sweep
// the above settings:
// * MULTIPLIED across all robots, for calibration and accuracy
// * SUMMED across all robots, for falsePositives and extraDetects
//
// the data files are collected using BehaviorDevTouchDataCollection
// and the harness assumes you touch the robot EXACTLY 20 TIMES
// this is how the accuracy is computed
TEST_F(TouchSensorTest, DISABLED_TouchSensorTuningPVT)
{
  // IMPORTANT:
  // in order to save effort in writing complex code to score
  // the performance of any one parameter skew, we assume that
  // across all robots, all touch session where there was
  // a real contact consist of exactly 20 true touches.
  const int numTrueTouchesPerInstance = 20;
  
  // helper lambda
  const auto LoadReadingsFromFile = [](const std::string& filename)->std::vector<uint16_t> {
    std::string contents{ Anki::Util::FileUtils::ReadFile(filename) };
    std::vector<std::string> items = Anki::Util::StringSplit(contents,'\n');
    std::vector<uint16_t> ret;
    std::for_each(items.begin(), items.end(), [&ret](const std::string& s)->void { ret.push_back((uint16_t)std::stoi(s)); });
    return ret;
  };
  
  // - create parameter skews (minus box)
  // - for each skew:
  //   - for each box filter size:
  //     - for each robot:
  //       - create the filtered signals
  //       - compute the result metrics
  //       - sum the skew+box across robots = total
  //       - create result entry
  // - output the results to disk
  // - check the results, itemized per skew => select best
  //
  // TODO:
  //  + add file disk caching
  
  std::string workspace = "/Users/arjun/Code/data/touch_sensor/pvt_dvt4p5/workspace";
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  struct TestResultScore
  {
    bool didCalib = false;
    
    int numTouchesGot = -1;
    int numFalsePos = -1;
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  std::vector<TouchSensorComponent::FilterParameters> skewTable;
  skewTable.reserve(7560);
  for(int box=35; box<=60; box+=5) {
    for(int gap=1; gap<=6; ++gap) {
      for(int fod=0; fod<=5; ++fod) {
        for(int fou=0; fou<=5; ++fou) {
          for(int mfou=6; mfou<=10; ++mfou) {
            for(int mfod=20; mfod<=20; mfod+=5) { // manual testing reveals this is irrelevant
              for(int mccu=6; mccu<=6; ++mccu) { // manual testing reveals this is irrelevant
                skewTable.push_back(
                  TouchSensorComponent::FilterParameters{
                    box,
                    gap,
                    fod,
                    fou,
                    mfod,
                    mfou,
                    mccu
                  });
              }
            }
          }
        }
      }
    }
  }
  PRINT_NAMED_INFO("TouchSensorTuningPVT","%zd skews created", skewTable.size());
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  std::vector<std::string> robots =
  {
    "e1005b",
    "e20077",
    "e2014d",
    "e200d4",
    "e20155"
  };
  
  struct SensorReadings
  {
    std::vector<uint16_t> idleOff_noTouch;
    std::vector<uint16_t> idleOn_noTouch;
    std::vector<uint16_t> idleOff_pettingSoft;
    std::vector<uint16_t> idleOn_pettingSoft;
  };
  
  std::vector<SensorReadings> readingSet;
  
  for(auto& robot : robots) {
    SensorReadings sr;
    sr.idleOff_noTouch = LoadReadingsFromFile(workspace + "/" + robot + "_Idle_OffCharger_No_Touch");
    sr.idleOn_noTouch = LoadReadingsFromFile(workspace + "/" + robot + "_Idle_OnCharger_No_Touch");
    sr.idleOff_pettingSoft = LoadReadingsFromFile(workspace + "/" + robot + "_Idle_OffCharger_Petting_Soft");
    sr.idleOn_pettingSoft = LoadReadingsFromFile(workspace + "/" + robot + "_Idle_OnCharger_Petting_Soft");
    readingSet.push_back(sr);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  RobotCompMap dummyCompMap;
  
  typedef std::function<TestResultScore(const SensorReadings&,
    const TouchSensorComponent::FilterParameters& skew)> TestFuncType;
  
  TestFuncType TestMain = [this,&dummyCompMap]
  (const SensorReadings& sensorReadings,
   const TouchSensorComponent::FilterParameters& skew)
  -> TestResultScore
  {
    _msgMonitor->ResetCountTouchButtonEvent();
    TouchSensorComponent tscomp;
    tscomp.InitDependent(_robot,dummyCompMap);
    tscomp.UnitTestOnly_SetFilterParameters(skew);
    
    TestResultScore result;
    
    const auto applyReadings = [&](const std::vector<uint16_t>& readings) {
      size_t last = readings.size();
      last -= (readings.size()%6);
      for(int i=240; i<last; i+=6) {
        Anki::Vector::RobotState msg = _robot->GetDefaultRobotState();
        msg.backpackTouchSensorRaw = {
          {readings[i],
            readings[i+1],
            readings[i+2],
            readings[i+3],
            readings[i+4],
            readings[i+5]},
        };
        tscomp.NotifyOfRobotStateInternal(msg);
      }
    };
    
    applyReadings(sensorReadings.idleOff_noTouch);
    
    result.didCalib = tscomp.IsCalibrated();
    if(tscomp.IsCalibrated()) {
      result.numFalsePos = (int)_msgMonitor->GetCountTouchButtonEvent();
      
      _msgMonitor->ResetCountTouchButtonEvent();
      applyReadings(sensorReadings.idleOff_pettingSoft);
      applyReadings(sensorReadings.idleOn_pettingSoft);
      result.numTouchesGot = (int)_msgMonitor->GetCountTouchButtonEvent();
      
      _msgMonitor->ResetCountTouchButtonEvent();
      applyReadings(sensorReadings.idleOn_noTouch);
      result.numFalsePos += (int)_msgMonitor->GetCountTouchButtonEvent();
    }
    
    return result;
  };
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // meta computation variables to track progress
  // in sweeping through all the parameter skews
  clock_t last_progress_update_time = clock();
  clock_t per_comp_time;
  float sum_time = 0.0f;
  int iteration_num = 0;
  
  std::stringstream resultStream;
  
  for(auto& skew : skewTable) {
    
    resultStream << skew.boxFilterSize << ",";
    resultStream << skew.dynamicThreshGap << ",";
    resultStream << skew.dynamicThreshDetectFollowOffset << ",";
    resultStream << skew.dynamicThreshUndetectFollowOffset << ",";
    resultStream << skew.dynamicThreshMaximumDetectOffset << ",";
    resultStream << skew.dynamicThreshMininumUndetectOffset << ",";
    resultStream << skew.minConsecCountToUpdateThresholds << ",";
    
    clock_t t = clock();
    
    bool allCalib = true;
    float aggPctCorrect = 100.0f;
    int aggNumFalsePos = 0;
    int aggExtraDetects = 0;
    for(auto& readings : readingSet) {
      TestResultScore ret = TestMain(readings, skew);
      
      allCalib &= ret.didCalib;
      aggNumFalsePos += ret.numFalsePos;
      if(ret.numTouchesGot >= numTrueTouchesPerInstance) {
        aggExtraDetects += ret.numTouchesGot - numTrueTouchesPerInstance;
      } else {
        aggPctCorrect *= float(ret.numTouchesGot)/numTrueTouchesPerInstance;
      }
    }
    
    per_comp_time = clock() - t;
    sum_time += per_comp_time;
    iteration_num++;
    
    resultStream << allCalib << ",";
    resultStream << aggPctCorrect << ",";
    resultStream << aggNumFalsePos << ",";
    resultStream << aggExtraDetects << std::endl;
    
    // periodically output remaining time estimate
    auto durSinceProgressUpdate = float( clock()-last_progress_update_time )/CLOCKS_PER_SEC;
    if( durSinceProgressUpdate > 3.0 ) {
      float average_time_per_comp = sum_time / iteration_num;
      float estimated_time_remain = average_time_per_comp *
      float(skewTable.size()-iteration_num) /
      iteration_num;
      printf("%6.2f minutes remaining\n", estimated_time_remain/60.0f );
      last_progress_update_time = clock();
    }
  }
  
  std::string contents = resultStream.str();
  Util::FileUtils::WriteFile(workspace+"/results.csv", contents);
}

#endif // ifdef ENABLE_PROTOTYPING_ANALYSIS_TESTS
