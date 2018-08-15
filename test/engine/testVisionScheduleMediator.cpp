/**
 * File: testVisionScheduleMediator.cpp
 * 
 * Author: Sam Russell
 * Created: 2018-02-21
 * 
 * Description: Unit tests for the VisionScheduleMediator
 * 
 * Copyright: Anki, Inc. 2018
 * 
 * --gtest_filter=VisionScheduleMediator
 * 
 **/

#include "gtest/gtest.h"

// access VSM internals for test
#define private public
#define protected public

#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/components/visionScheduleMediator/iVisionModeSubscriber.h"
#include "engine/vision/visionModeSchedule.h"
#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/dependencyManagedEntity.h"

using namespace Anki;
using namespace Anki::Vector;

namespace {

void CreateVSMConfig(const std::string& jsonString, Json::Value& config) 
{
  Json::Reader reader;
  const bool parsedOK = reader.parse(jsonString, config, false);
  ASSERT_TRUE(parsedOK);
}

class TestSubscriber : IVisionModeSubscriber
{
public:
  explicit TestSubscriber(VisionScheduleMediator* vsm, std::set<VisionModeRequest> visionModes)
  : _vsm(vsm)
  , _visionModes(visionModes)
  {
  }

  void Subscribe()
  {
    _vsm->SetVisionModeSubscriptions(this, _visionModes);
  }

  void Unsubscribe()
  {
    _vsm->ReleaseAllVisionModeSubscriptions(this);
  }

private:
  VisionScheduleMediator* _vsm;
  std::set<VisionModeRequest> _visionModes;
};

} // namespace 

TEST(VisionScheduleMediator, Interleaving)
{
  
  std::string configString = R"json(
  {
    "VisionModeSettings" :
    [
      {
        "mode"         : "DetectingMarkers",
        "low"          : 4,
        "med"          : 2,
        "high"         : 1,
        "standard"     : 2,
        "relativeCost" : 16
      },
      {
        "mode"         : "DetectingFaces",
        "low"          : 4,
        "med"          : 2,
        "high"         : 1,
        "standard"     : 2,
        "relativeCost" : 16
      },
      {
        "mode"         : "DetectingPets",
        "low"          : 8,
        "med"          : 4,
        "high"         : 2,
        "standard"     : 8,
        "relativeCost" : 10
      },
      {
        "mode"         : "DetectingMotion",
        "low"          : 4,
        "med"          : 2,
        "high"         : 1,
        "standard"     : 2,
        "relativeCost" : 4
      }
    ]
  })json";
  Json::Value config;
  CreateVSMConfig(configString, config);
  VisionComponent visionComponent;
  VisionScheduleMediator vsm;
  vsm.Init(config);
  std::set<VisionMode> enabledModes;

  TestSubscriber lowMarkerSubscriber(&vsm, { { VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low } });
  TestSubscriber medMarkerSubscriber(&vsm, { { VisionMode::DetectingMarkers, EVisionUpdateFrequency::Med } });
  TestSubscriber highMarkerSubscriber(&vsm, { { VisionMode::DetectingMarkers, EVisionUpdateFrequency::High } });

  TestSubscriber lowFaceSubscriber(&vsm, { { VisionMode::DetectingFaces, EVisionUpdateFrequency::Low } });
  TestSubscriber medFaceSubscriber(&vsm, { { VisionMode::DetectingFaces, EVisionUpdateFrequency::Med } });
  TestSubscriber highFaceSubscriber(&vsm, { { VisionMode::DetectingFaces, EVisionUpdateFrequency::High } });

  TestSubscriber lowPetSubscriber(&vsm, { { VisionMode::DetectingPets, EVisionUpdateFrequency::Low } });
  TestSubscriber medPetSubscriber(&vsm, { { VisionMode::DetectingPets, EVisionUpdateFrequency::Med } });
  TestSubscriber highPetSubscriber(&vsm, { { VisionMode::DetectingPets, EVisionUpdateFrequency::High } });
  
  TestSubscriber lowMotionSubscriber(&vsm, { { VisionMode::DetectingMotion, EVisionUpdateFrequency::Low } });
  TestSubscriber medMotionSubscriber(&vsm, { { VisionMode::DetectingMotion, EVisionUpdateFrequency::Med } });
  TestSubscriber highMotionSubscriber(&vsm, { { VisionMode::DetectingMotion, EVisionUpdateFrequency::High } });

  // Test basic Subscription
  lowFaceSubscriber.Subscribe();
  lowMarkerSubscriber.Subscribe();
  lowPetSubscriber.Subscribe();
  lowMotionSubscriber.Subscribe();
  vsm.UpdateVisionSchedule(visionComponent, nullptr);
  AllVisionModesSchedule::ModeScheduleList scheduleList = vsm.GenerateBalancedSchedule(visionComponent);

  for(auto& modeSchedule : scheduleList){
    switch(modeSchedule.first){
      case VisionMode::DetectingFaces:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, false, true, false})));
        break;
      case VisionMode::DetectingMarkers:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, false, false, true})));
        break;
      case VisionMode::DetectingPets:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, true, false, false,
                                                                         false, false, false, false})));
        break;
      case VisionMode::DetectingMotion:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({true, false, false, false})));
        break;
      default:
        // If we get here... the unit test has been broken and should be fixed.
        EXPECT_TRUE(false) << "TestVisionScheduleMediator.UnhandledVisionMode";
        break;
    }
  }

  // Test Layered Subscription
  medFaceSubscriber.Subscribe();
  medMarkerSubscriber.Subscribe();
  medPetSubscriber.Subscribe();
  medMotionSubscriber.Subscribe();
  vsm.UpdateVisionSchedule(visionComponent, nullptr);
  scheduleList = vsm.GenerateBalancedSchedule(visionComponent);

  for(auto& modeSchedule : scheduleList){
    switch(modeSchedule.first){
      case VisionMode::DetectingFaces:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({true, false})));
        break;
      case VisionMode::DetectingMarkers:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, true})));
        break;
      case VisionMode::DetectingPets:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, true, false, false})));
        break;
      case VisionMode::DetectingMotion:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({true, false})));
        break;
      default:
        // If we get here... the unit test has been broken and should be fixed.
        EXPECT_TRUE(false) << "TestVisionScheduleMediator.UnhandledVisionMode";
        break;
    }
  }

  // Test basic Unsubscribe
  medFaceSubscriber.Unsubscribe();
  medMarkerSubscriber.Unsubscribe();
  medPetSubscriber.Unsubscribe();
  medMotionSubscriber.Unsubscribe();
  vsm.UpdateVisionSchedule(visionComponent, nullptr);
  scheduleList = vsm.GenerateBalancedSchedule(visionComponent);

  for(auto& modeSchedule : scheduleList){
    switch(modeSchedule.first){
      case VisionMode::DetectingFaces:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, false, true, false})));
        break;
      case VisionMode::DetectingMarkers:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, false, false, true})));
        break;
      case VisionMode::DetectingPets:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({false, true, false, false,
                                                                         false, false, false, false})));
        break;
      case VisionMode::DetectingMotion:
        EXPECT_TRUE((modeSchedule.second._schedule == std::vector<bool>({true, false, false, false})));
        break;
      default:
        // If we get here... the unit test has been broken and should be fixed.
        EXPECT_TRUE(false) << "TestVisionScheduleMediator.UnhandledVisionMode";
        break;
    }
  }

  // Test full Unsubscribe
  lowFaceSubscriber.Unsubscribe();
  lowMarkerSubscriber.Unsubscribe();
  lowPetSubscriber.Unsubscribe();
  lowMotionSubscriber.Unsubscribe();
  vsm.UpdateVisionSchedule(visionComponent, nullptr);
  scheduleList = vsm.GenerateBalancedSchedule(visionComponent);

  // Verify default modes are still scheduled after unsubscribing
  EXPECT_TRUE(scheduleList.size() == 1);
  EXPECT_TRUE(scheduleList.front().first == VisionMode::DetectingMarkers);
  EXPECT_TRUE(scheduleList.front().second._schedule == std::vector<bool>({true, false, false, false}));

}
