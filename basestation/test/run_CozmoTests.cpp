#include <array>
#include <fstream>

#include "gtest/gtest.h"
#include "json/json.h"

#include "anki/common/types.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/cozmo/basestation/blockWorld.h"

TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}


TEST(Cozmo, BlockWorldVisionTest)
{
  using namespace Anki;
  using namespace Cozmo;
  
  // TODO: Tighten/loosen thresholds?
  const float   blockPoseDistThresholdFraction = 0.05f; // within 5% of actual distance
  const Radians blockPoseAngleThreshold    = DEG_TO_RAD(3.f);
  const float   robotPoseDistThreshold_mm  = 5.f;
  const Radians robotPoseAngleThreshold    = DEG_TO_RAD(3.f);
  
  BlockWorld blockWorld;
  
  Json::Reader reader;
  Json::Value jsonData;
  std::vector<std::string> jsonFileList;
  
  // TODO: get all available tests from some directory?
  
  std::string jsonFilePath = PlatformPathManager::getInstance()->PrependPath(PlatformPathManager::Test, "basestation/test/blockWorldTests/");
  
  jsonFileList.emplace_back(jsonFilePath + "visionTestWorld1_Pose0.json");
  
  for(auto & jsonFilename : jsonFileList)
  {
    std::ifstream jsonFile(jsonFilename);
    bool jsonParseResult = reader.parse(jsonFile, jsonData);
    ASSERT_TRUE(jsonParseResult);
   
    // TODO: Support multiple robots

    Pose3d robotPose;
    ASSERT_TRUE(JsonTools::GetPoseOptional(jsonData["RobotPose"], robotPose));
    
    // TODO: Also grab robot head angle from Json and set it
    float headAngle;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData["RobotPose"], "HeadAngle", headAngle));
    
    Robot robot(0, &blockWorld);
    robot.set_headAngle(headAngle);
    
    ASSERT_TRUE(jsonData.isMember("CameraCalibration"));
    Vision::CameraCalibration calib(jsonData["CameraCalibration"]);
    robot.set_camCalibration(calib);
    
    int NumMarkers;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData, "NumMarkers", NumMarkers));
    
    ASSERT_TRUE(jsonData.isMember("VisionMarkers"));
    Json::Value jsonMessages = jsonData["VisionMarkers"];
    ASSERT_EQ(NumMarkers, jsonMessages.size());

    std::vector<MessageVisionMarker> messages;
    messages.reserve(jsonMessages.size());
    
    for(auto const& jsonMsg : jsonMessages) {
      MessageVisionMarker msg(jsonMsg);
      
      Quad2f corners;
      corners[Quad::TopLeft].x()     = msg.x_imgUpperLeft;
      corners[Quad::TopLeft].y()     = msg.y_imgUpperLeft;
      
      corners[Quad::TopRight].x()    = msg.x_imgUpperRight;
      corners[Quad::TopRight].y()    = msg.y_imgUpperRight;
      
      corners[Quad::BottomLeft].x()  = msg.x_imgLowerLeft;
      corners[Quad::BottomLeft].y()  = msg.y_imgLowerLeft;
      
      corners[Quad::BottomRight].x() = msg.x_imgLowerRight;
      corners[Quad::BottomRight].y() = msg.y_imgLowerRight;
      
      // TODO: get the camera of the robot corresponding to the one that saw this VisionMarker
      const Vision::Camera& camera = robot.get_camHead();
      Vision::ObservedMarker marker(&(msg.code[0]), corners, camera);
      
      // Give this vision marker to BlockWorld for processing
      blockWorld.QueueObservedMarker(marker);
      
    } // for each VisionMarker in the jsonFile
    
    // Use all the VisionMarkers to update the blockworld's pose estimates for
    // all the robots
    // TODO: loop over all robots
    ASSERT_TRUE(blockWorld.UpdateRobotPose(&robot));
    
    // Make sure the estimated robot pose matches the ground truth pose
    EXPECT_TRUE(robotPose.IsSameAs(robot.get_pose(),
                                   robotPoseDistThreshold_mm,
                                   robotPoseAngleThreshold));
    
    // Use the rest of the VisionMarkers to update the blockworld's pose
    // estimates for the blocks
    uint32_t numBlocksObserved = blockWorld.UpdateBlockPoses();
    
    // TODO: In the case that the robot's pose is not estimated correctly (or
    //       at all, e.g. due to no visible mat marker), check block poses
    //       relative to robot, instead of relative to absolute world coords?
    
    ASSERT_TRUE(jsonData.isMember("Blocks"));
    const Json::Value& jsonBlocks = jsonData["Blocks"];
    const int numBlocksTrue = jsonBlocks.size();
    
    ASSERT_EQ(numBlocksObserved, numBlocksTrue);
    
    // Check to see that we found and successfully localized each ground truth
    // block
    for(int i_block=0; i_block<numBlocksTrue; ++i_block)
    {
      ObjectType_t blockType;
      ASSERT_TRUE(JsonTools::GetValueOptional(jsonBlocks[i_block], "Type", blockType));
      
      const Vision::ObservableObject* block = blockWorld.GetBlockLibrary().GetObjectWithType(blockType);
      
      // The ground truth block type should be known to the block world
      ASSERT_TRUE(block != NULL);
      Block *groundTruthBlock = dynamic_cast<Block*>(block->Clone());
      
      // Set its pose to what is listed in the json file
      Pose3d blockPose;
      ASSERT_TRUE(jsonBlocks[i_block].isMember("BlockPose"));
      ASSERT_TRUE(JsonTools::GetPoseOptional(jsonBlocks[i_block]["BlockPose"], blockPose));
      groundTruthBlock->SetPose(blockPose);
      
      // Make sure this ground truth block was seen and its estimated pose
      // matches the ground truth pose
      auto observedBlocks = blockWorld.GetExistingBlocks(groundTruthBlock->GetType());
      int matchesFound = 0;
      
      for(auto & observedBlock : observedBlocks)
      {
        // The threshold will vary with how far away the block actually is
        float blockPoseDistThreshold_mm = (blockPoseDistThresholdFraction *
                                           (groundTruthBlock->GetPose().get_translation() -
                                            robotPose.get_translation()).length());

        if(groundTruthBlock->IsSameAs(*observedBlock.second,
                                      blockPoseDistThreshold_mm,
                                      blockPoseAngleThreshold))
        {
          ++matchesFound;
        }
      } // for each observed block
      EXPECT_EQ(matchesFound, 1); // Exactly one observed block should match
      
      delete groundTruthBlock;
      
    } // for each ground truth block
    
  } // for each jsonFile
  
} // TEST(BlockWorldVisionTest)


int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}