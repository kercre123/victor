#include <array>
#include <fstream>

#include "gtest/gtest.h"
#include "json/json.h"

#include "anki/common/types.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"

#include "anki/common/robot/matlabInterface.h"

#include "messageHandler.h"
#include "pathPlanner.h"

TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}

// This test object allows us to reuse the TEST_P below with different
// Json filenames as a parameter
class BlockWorldTest : public ::testing::TestWithParam<const char*>
{
  
}; // class BlockWorldTest

#define DISPLAY_ERRORS 0

// This is the parameterized test, instantied with a list of Json files below
TEST_P(BlockWorldTest, BlockAndRobotLocalization)
{
  using namespace Anki;
  using namespace Cozmo;
  
  // TODO: Tighten/loosen thresholds?
  //const float   blockPoseDistThresholdFraction = 0.05f; // within 5% of actual distance
  const float blockPoseDistThreshold_mm    = 12.f;
  const Radians blockPoseAngleThreshold    = DEG_TO_RAD(15.f); // TODO: make dependent on distance?
  const float   robotPoseDistThreshold_mm  = 10.f;
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

  // Create the modules we need (and stubs of those we don't)
  RobotManager        robotMgr;
  BlockWorld          blockWorld;
  MessageHandlerStub  msgHandler;
  PathPlannerStub     pathPlanner;
  
  blockWorld.Init(&robotMgr);
  robotMgr.Init(&msgHandler, &blockWorld, &pathPlanner);
  
  robotMgr.AddRobot(0);
  Robot& robot = *robotMgr.GetRobotByID(0);
  
//  Robot robot(0, 0, &blockWorld, 0);    // TODO: Support multiple robots

  ASSERT_TRUE(jsonRoot.isMember("CameraCalibration"));
  Vision::CameraCalibration calib(jsonRoot["CameraCalibration"]);
  robot.SetCameraCalibration(calib);
  
  // Fake a robot state update (i think we have to have received one before
  // we can start adding vision-based poses to history)
  MessageRobotState msg;
  {
    msg.timestamp = 0;
    msg.pose_frame_id = 0;
    msg.pose_x = 0;
    msg.pose_y = 0;
    msg.pose_z = 0;
    msg.pose_angle = 0;
    msg.headAngle = 0;
    msg.liftAngle = 0;
    
    ASSERT_EQ(robot.AddRawOdomPoseToHistory(msg.timestamp,
                                            msg.pose_frame_id,
                                            msg.pose_x, msg.pose_y, msg.pose_z,
                                            msg.pose_angle,
                                            msg.headAngle,
                                            msg.liftAngle,
                                            robot.GetPoseOrigin()), RESULT_OK);
    
    ASSERT_TRUE(robot.UpdateCurrPoseFromHistory());
  }
  
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
    TimeStamp_t currentTimeStamp = (i_pose+1)*100;
    
    const Json::Value& jsonData = jsonRoot["Poses"][i_pose];
    
    // Put the robot's head at the right angle *before* queueing up the markers
    float headAngle;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData["RobotPose"], "HeadAngle", headAngle));
    robot.SetHeadAngle(headAngle);
    
    Pose3d trueRobotPose;
    ASSERT_TRUE(JsonTools::GetPoseOptional(jsonData, "RobotPose", trueRobotPose));
    
    // Create a fake robot state message to do a fake RawOdometryPose update.
    // I think we need to make sure we have one for each matPose VisionMarker
    // message.
    MessageRobotState msg;
    msg.timestamp = currentTimeStamp;
    msg.pose_frame_id = 0;
    msg.headAngle  = headAngle;
    msg.liftAngle  = 0;
    
    // If we're not going to be checking the robot's pose, we need to set it
    // to the ground truth now, *before* queueing up the markers
    if(!checkRobotPose) {
      msg.pose_x = trueRobotPose.get_translation().x();
      msg.pose_y = trueRobotPose.get_translation().y();
      msg.pose_z = trueRobotPose.get_translation().z();
      msg.pose_angle = trueRobotPose.get_rotationAngle<'Z'>().ToFloat();
    }
    
    ASSERT_EQ(robot.AddRawOdomPoseToHistory(msg.timestamp,
                                            msg.pose_frame_id,
                                            msg.pose_x, msg.pose_y, msg.pose_z,
                                            msg.pose_angle,
                                            msg.headAngle, msg.liftAngle,
                                            robot.GetPoseOrigin()), RESULT_OK);
    
    ASSERT_TRUE(robot.UpdateCurrPoseFromHistory());

    int NumMarkers;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData, "NumMarkers", NumMarkers));
    
    ASSERT_TRUE(jsonData.isMember("VisionMarkers"));
    Json::Value jsonMessages = jsonData["VisionMarkers"];
    ASSERT_EQ(NumMarkers, jsonMessages.size());
    
    std::vector<MessageVisionMarker> messages;
    messages.reserve(jsonMessages.size());
    
    for(auto & jsonMsg : jsonMessages) {

      // Kludge to convert the string MarkerType stored in the JSON back to an
      // enum value so we can use the (auto-generated) JSON constructor for
      // messages.
      CORETECH_ASSERT(jsonMsg.isMember("markerType"));
      jsonMsg["markerType"] = Vision::StringToMarkerType.at(jsonMsg["markerType"].asString());
      
      MessageVisionMarker msg(jsonMsg);
      msg.timestamp = currentTimeStamp;
      
      ASSERT_EQ(blockWorld.QueueObservedMarker(msg, robot), RESULT_OK);
      
    } // for each VisionMarker in the jsonFile
    
    
    // Process all the markers we've queued
    uint32_t numBlocksObserved = 0;
    blockWorld.Update(numBlocksObserved);
    
    if(checkRobotPose) {
      // TODO: loop over all robots
      
      // Make sure the estimated robot pose matches the ground truth pose
      Pose3d P_diff;
      const bool robotPoseMatches = trueRobotPose.IsSameAs(robot.GetPose(),
                                                           robotPoseDistThreshold_mm,
                                                           robotPoseAngleThreshold, P_diff);
      
      fprintf(stdout, "X/Y error in robot pose = %.2fmm, Z error = %.2fmm\n",
              sqrtf(P_diff.get_translation().x()*P_diff.get_translation().x() +
                    P_diff.get_translation().y()*P_diff.get_translation().y()),
              P_diff.get_translation().z());
      
      // If the robot's pose is not correct, we can't continue, because
      // all the blocks' poses will also be incorrect
      ASSERT_TRUE(robotPoseMatches);
      
    }
    
    // Use the rest of the VisionMarkers to update the blockworld's pose
    // estimates for the blocks
    //uint32_t numBlocksObserved = blockWorld.UpdateBlockPoses();
    
    
    if(jsonRoot.isMember("Blocks"))
    {
      const Json::Value& jsonBlocks = jsonRoot["Blocks"];
      const int numBlocksTrue = jsonBlocks.size();
      
      EXPECT_GE(numBlocksObserved, numBlocksTrue); // TODO: Should this be EXPECT_EQ?
      
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
        
        // Make sure this ground truth block was seen and its estimated pose
        // matches the ground truth pose
        auto observedBlocks = blockWorld.GetExistingBlocks(groundTruthBlock->GetType());
        int matchesFound = 0;
        
        // The threshold will vary with how far away the block actually is
        /*const float blockPoseDistThreshold_mm = (blockPoseDistThresholdFraction *
                                                 (blockPose.get_translation() -
                                                  trueRobotPose.get_translation()).Length());*/
        
        for(auto & observedBlock : observedBlocks)
        {
          Pose3d P_diff;
          
          blockPose.set_parent(&observedBlock.second->GetPose().FindOrigin());
          groundTruthBlock->SetPose(blockPose);
          
          if(groundTruthBlock->IsSameAs(*observedBlock.second,
                                        blockPoseDistThreshold_mm,
                                        blockPoseAngleThreshold, P_diff))
          {
            if(matchesFound > 0) {
              // We just found multiple matches for this ground truth block
              fprintf(stdout, "Match #%d found for one ground truth block. "
                      "T_diff = %.2fmm (vs. %.2fmm), Angle_diff = %.1fdeg (vs. %.1fdeg)\n",
                      matchesFound+1, P_diff.get_translation().Length(),
                      blockPoseDistThreshold_mm,
                      P_diff.get_rotationAngle().getDegrees(),
                      blockPoseAngleThreshold.getDegrees());
              
              groundTruthBlock->IsSameAs(*observedBlock.second,
                                         blockPoseDistThreshold_mm,
                                         blockPoseAngleThreshold, P_diff);
            }

            if(matchesFound == 0) {
              fprintf(stdout, "Match found for observed block with "
                      "T_diff = %.2fmm (vs. %.2fmm), Angle_diff = %.1fdeg (vs %.1fdeg)\n",
                      P_diff.get_translation().Length(),
                      blockPoseDistThreshold_mm,
                      P_diff.get_rotationAngle().getDegrees(),
                      blockPoseAngleThreshold.getDegrees());
#if DISPLAY_ERRORS
              const Vec3f& T_true = groundTruthBlock->GetPose().get_translation();
              
              Vec3f T_dir(T_true - trueRobotPose.get_translation());
              const float distance = T_dir.makeUnitLength();
              
              const Vec3f T_error(T_true - observedBlock.second->GetPose().get_translation());
              /*
               fprintf(stdout, "Block position error = %.1fmm at a distance of %.1fmm\n",
               (T_true - observedBlock.second->GetPose().get_translation()).length(),
               ().length());
               */
              //errorVsDist.push_back(std::make_pair( distance, DotProduct(T_error, T_dir) ));
              //errorVsDist.push_back(std::make_pair(distance, (T_true - observedBlock.second->GetPose().get_translation()).length()));
              errorVsDist.emplace_back(trueRobotPose.get_translation(),
                                       groundTruthBlock->GetPose().get_translation(), observedBlock.second->GetPose().get_translation());
#endif
            }

            ++matchesFound;
          } else {
            fprintf(stdout, "Observed type-%d block %d at (%.2f,%.2f,%.2f) does not match "
                    "type-%d ground truth at (%.2f,%.2f,%.2f). T_diff = %2fmm (vs. %.2fmm), "
                    "Angle_diff = %.1fdeg (vs. %.1fdeg)\n",
                    observedBlock.second->GetType(),
                    observedBlock.second->GetID(),
                    observedBlock.second->GetPose().get_translation().x(),
                    observedBlock.second->GetPose().get_translation().y(),
                    observedBlock.second->GetPose().get_translation().z(),
                    groundTruthBlock->GetType(),
                    groundTruthBlock->GetPose().get_translation().x(),
                    groundTruthBlock->GetPose().get_translation().y(),
                    groundTruthBlock->GetPose().get_translation().z(),
                    P_diff.get_translation().Length(),
                    blockPoseDistThreshold_mm,
                    P_diff.get_rotationAngle().getDegrees(),
                    blockPoseAngleThreshold.getDegrees());
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
