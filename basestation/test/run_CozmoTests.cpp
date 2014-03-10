#include <array>
#include <fstream>

#include "gtest/gtest.h"
#include "json/json.h"

#include "anki/common/types.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/robot/matlabInterface.h"

TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}

// This test object allows us to reuse the TEST_P below with different
// Json filenames as a parameter
class BlockWorldTest : public ::testing::TestWithParam<const char*>
{
  
}; // class BlockWorldTest

#define DISPLAY_ERRORS 1

// This is the parameterized test, instantied with a list of Json files below
TEST_P(BlockWorldTest, BlockAndRobotLocalization)
{
  using namespace Anki;
  using namespace Cozmo;
  
  // TODO: Tighten/loosen thresholds?
  const float   blockPoseDistThresholdFraction = 0.03f; // within 3% of actual distance
  const Radians blockPoseAngleThreshold    = DEG_TO_RAD(10.f); // TODO: make dependent on distance?
  const float   robotPoseDistThreshold_mm  = 5.f;
  const Radians robotPoseAngleThreshold    = DEG_TO_RAD(3.f);
  
  Json::Reader reader;
  Json::Value jsonRoot;
  std::vector<std::string> jsonFileList;
  
  const std::string subPath("basestation/test/blockWorldTests/");
  const std::string jsonFilename = PREPEND_SCOPED_PATH(Test, subPath + GetParam());
  
  fprintf(stdout, "\n\nLoading JSON file '%s'\n", jsonFilename.c_str());
  
  std::ifstream jsonFile(jsonFilename);
  bool jsonParseResult = reader.parse(jsonFile, jsonRoot);
  ASSERT_TRUE(jsonParseResult);

  BlockWorld blockWorld;
  Robot robot(0, 0, &blockWorld, 0);    // TODO: Support multiple robots

  
  ASSERT_TRUE(jsonRoot.isMember("CameraCalibration"));
  Vision::CameraCalibration calib(jsonRoot["CameraCalibration"]);
  robot.set_camCalibration(calib);
  
  bool checkRobotPose;
  ASSERT_TRUE(JsonTools::GetValueOptional(jsonRoot, "CheckRobotPose", checkRobotPose));

  // Everything before this is per-world, not per-pose
  
  ASSERT_TRUE(jsonRoot.isMember("Poses"));
  
  const int NumPoses = jsonRoot["Poses"].size();
  
#if DISPLAY_ERRORS
  struct ErrorStruct {
    Vec3f T_robot, T_blockTrue, T_blockObs;
    ErrorStruct(const Vec3f& a, const Vec3f& b, const Vec3f& c)
    : T_robot(a), T_blockTrue(b), T_blockObs(c)
    {
      
    }
  };
  std::vector<ErrorStruct> errorVsDist;
#endif
  
  for(int i_pose=0; i_pose<NumPoses; ++i_pose)
  {
    const Json::Value& jsonData = jsonRoot["Poses"][i_pose];
    
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
      Vision::ObservedMarker marker(msg.markerType, corners, camera);
      
      // Give this vision marker to BlockWorld for processing
      blockWorld.QueueObservedMarker(marker);
      
    } // for each VisionMarker in the jsonFile
    
    
    Pose3d trueRobotPose;
    ASSERT_TRUE(JsonTools::GetPoseOptional(jsonData, "RobotPose", trueRobotPose));
    
    float headAngle;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData["RobotPose"], "HeadAngle", headAngle));
    robot.set_headAngle(headAngle);
    
    
    if(checkRobotPose) {
      // Use all the VisionMarkers to update the blockworld's pose estimates for
      // all the robots
      // TODO: loop over all robots
      ASSERT_TRUE(blockWorld.UpdateRobotPose(&robot));
      
      // Make sure the estimated robot pose matches the ground truth pose
      Pose3d P_diff;
      const bool robotPoseMatches = trueRobotPose.IsSameAs(robot.get_pose(),
                                                           robotPoseDistThreshold_mm,
                                                           robotPoseAngleThreshold, P_diff);
      
      fprintf(stdout, "X/Y error in robot pose = %.2fmm, Z error = %.2fmm\n",
              sqrtf(P_diff.get_translation().x()*P_diff.get_translation().x() +
                    P_diff.get_translation().y()*P_diff.get_translation().y()),
              P_diff.get_translation().z());
      
      EXPECT_TRUE(robotPoseMatches);
      
      
      if(not robotPoseMatches) {
        // Use ground truth pose so we can continue
        robot.set_pose(trueRobotPose);
      }
    }
    else {
      // Just set the robot's pose to the ground truth in the JSON file
      robot.set_pose(trueRobotPose);
    }
    
    // Use the rest of the VisionMarkers to update the blockworld's pose
    // estimates for the blocks
    uint32_t numBlocksObserved = blockWorld.UpdateBlockPoses();
    
    // Toss unused markers
    uint32_t numUnusedMarkers = blockWorld.ClearObservedMarkers();
    if(numUnusedMarkers > 0) {
      fprintf(stdout, "%u observed markers went unused for block/robot "
              "localization.\n", numUnusedMarkers);
    }
    
    if(jsonRoot.isMember("Blocks"))
    {
      const Json::Value& jsonBlocks = jsonRoot["Blocks"];
      const int numBlocksTrue = jsonBlocks.size();
      
      EXPECT_EQ(numBlocksObserved, numBlocksTrue);
      
      //if(numBlocksObserved != numBlocksTrue)
      //  break;
      
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
        ASSERT_TRUE(JsonTools::GetPoseOptional(jsonBlocks[i_block], "BlockPose", blockPose));
        groundTruthBlock->SetPose(blockPose);
        
        // Make sure this ground truth block was seen and its estimated pose
        // matches the ground truth pose
        auto observedBlocks = blockWorld.GetExistingBlocks(groundTruthBlock->GetType());
        int matchesFound = 0;
        
        // The threshold will vary with how far away the block actually is
        const float blockPoseDistThreshold_mm = (blockPoseDistThresholdFraction *
                                                 (groundTruthBlock->GetPose().get_translation() -
                                                  trueRobotPose.get_translation()).length());
        
        for(auto & observedBlock : observedBlocks)
        {
          Pose3d P_diff;
          if(groundTruthBlock->IsSameAs(*observedBlock.second,
                                        blockPoseDistThreshold_mm,
                                        blockPoseAngleThreshold, P_diff))
          {
            if(matchesFound > 0) {
              // We just found multiple matches for this ground truth block
              fprintf(stdout, "Match #%d found for one ground truth block. "
                      "T_diff = %.2fmm (vs. %.2fmm), Angle_diff = %.1fdeg\n",
                      matchesFound+1, P_diff.get_translation().length(),
                      blockPoseDistThreshold_mm,
                      P_diff.get_rotationAngle().getDegrees());
              groundTruthBlock->IsSameAs(*observedBlock.second,
                                         blockPoseDistThreshold_mm,
                                         blockPoseAngleThreshold, P_diff);
            }
#if DISPLAY_ERRORS
            if(matchesFound == 0) {
              const Vec3f& T_true = groundTruthBlock->GetPose().get_translation();
              
              Vec3f T_dir(T_true - trueRobotPose.get_translation());
              const float distance = T_dir.makeUnitLength();
              
              const Vec3f T_error(T_true - observedBlock.second->GetPose().get_translation());
              /*
               fprintf(stdout, "Block position error = %.1fmm at a distance of %.1fmm\n",
               (T_true - observedBlock.second->GetPose().get_translation()).length(),
               ().length());
               */
              //errorVsDist.push_back(std::make_pair( distance, dot(T_error, T_dir) ));
              //errorVsDist.push_back(std::make_pair(distance, (T_true - observedBlock.second->GetPose().get_translation()).length()));
              errorVsDist.emplace_back(trueRobotPose.get_translation(),
                                       groundTruthBlock->GetPose().get_translation(), observedBlock.second->GetPose().get_translation());
            }
#endif
            ++matchesFound;
          }
        } // for each observed block
        
        EXPECT_EQ(matchesFound, 1); // Exactly one observed block should match
        
        delete groundTruthBlock;
        
      } // for each ground truth block
      
    } // IF there are blocks
    else {
      EXPECT_EQ(0, numBlocksObserved) <<
      "No blocks are defined in the JSON file, but some were observed.";
    }
  
  } // FOR each pose
  
#if DISPLAY_ERRORS
  // Plot the error vs distance in Matlab
  //Anki::Embedded::Matlab matlab(false);
  fprintf(stdout, "Paste this into Matlab to get an error vs. distance plot:\n");
  fprintf(stdout, "errorVsDist = [");
  for(auto measurement : errorVsDist) {
    fprintf(stdout, "%f %f %f   %f %f %f   %f %f %f;\n",
            measurement.T_robot.x(), measurement.T_robot.y(), measurement.T_robot.z(),
            measurement.T_blockTrue.x(), measurement.T_blockTrue.y(), measurement.T_blockTrue.z(),
            measurement.T_blockObs.x(), measurement.T_blockObs.y(), measurement.T_blockObs.z());
            
  }
  fprintf(stdout, "];\n");
#endif
  
} // TEST_P(BlockWorldTest, BlockAndRobotLocalization)


// This is the list of JSON files containing vision test worlds:
// TODO: automatically get all available tests from some directory?
const char *visionTestJsonFiles[] = {
  "visionTest_VaryingDistance.json",
  "visionTest_MatPoseTest.json",
  "visionTest_TwoBlocksOnePose.json",
  "visionTest_RepeatedBlock.json",
  "visionTest_OffTheMat.json"
};


// This actually creates the set of tests, one for each filename above.
INSTANTIATE_TEST_CASE_P(JsonFileBased, BlockWorldTest,
                        ::testing::ValuesIn(visionTestJsonFiles));


int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}