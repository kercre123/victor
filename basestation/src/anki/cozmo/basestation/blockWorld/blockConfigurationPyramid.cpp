/**
 * File: blockConfigurationPyramid.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Defines the Pyramid and PyramidBase configurations:
 *
 * A PyramidBase is any two blocks that are close enough to adjacent to each other
 * that a top block could be stacked on top of them by Cozmo
 * Currently this is defined as cubes that are within 1/2 the block size of each others centers
 * on at least one axis and are both on the ground
 *
 * A PyramidBase has what are termed a StaticBlock and a BaseBlock
 * these are interchangable, but the names are derived from how Cozmo builds a
 * base - bringing a base block over to a static block and placing the base adjacent to the static
 *
 * A Pyramid is a PyramidBase that already has a top block on top of it
 * A top block has to be offset from the static block in the same direction
 * as the base block
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/observableObject.h"

#include "util/math/constantsAndMacros.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
namespace BlockConfigurations{
  
namespace {
using PyramidIterator = std::vector<PyramidWeakPtr>::iterator;
const float kOnGroundTolerencePyramidOnly = 2*ON_GROUND_HEIGHT_TOL_MM;
const float kTopBlockOverhangMin_mm = 5;
}

// ---------------------------------------------------------
// Pyramid Base
// ---------------------------------------------------------
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PyramidBase::PyramidBase(const ObjectID& staticBlockID, const ObjectID& baseBlockID)
: BlockConfiguration(ConfigurationType::PyramidBase)
{
  _staticBlockID = staticBlockID;
  _baseBlockID = baseBlockID;
  
  if(_staticBlockID == _baseBlockID){
    PRINT_NAMED_WARNING("PyramidBase.PyramidBase.InvalidBlocksPassedIn","Attempted to create a base with two blocks with ID:%d", (int)_staticBlockID);
    ClearBase();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::operator==(const PyramidBase& other) const
{
  // base vs static distinction is arbitrary, so either one matching counts as the same PyramidBase
  return _staticBlockID != _baseBlockID &&
        (GetStaticBlockID() == other.GetStaticBlockID() || GetStaticBlockID() == other.GetBaseBlockID()) &&
        (GetBaseBlockID() == other.GetBaseBlockID() || GetBaseBlockID() == other.GetStaticBlockID());
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::GetCurrentBlockOffsetProperties(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock,
                                              float& xAxisOffset, float& yAxisOffset, float&  maxXAxisOffset, float& maxYAxisOffset)
{
  if((staticBlock == nullptr || baseBlock == nullptr) ||
      staticBlock->GetID() == baseBlock->GetID()){
    return false;
  }
  
  Pose3d basePoseWRTStatic;
  if(baseBlock->GetPose().GetWithRespectTo(staticBlock->GetPose(), basePoseWRTStatic)){
    
    //Radians baseBlockOffsetAngle = std::atan2(basePoseWRTStatic.GetTranslation().y(), basePoseWRTStatic.GetTranslation().x());
    
    Radians baseBlockOffsetAngle = std::atan2(baseBlock->GetPose().GetTranslation().y() - staticBlock->GetPose().GetTranslation().y(),
                                              baseBlock->GetPose().GetTranslation().x() - staticBlock->GetPose().GetTranslation().x());
    
    
    baseBlockOffsetAngle = -(staticBlock->GetPose().GetRotation().GetAngleAroundZaxis() - baseBlockOffsetAngle);


    // figure out the axis the block is along, and then remove the size of the blocks
    // from the offset to account for the seperation of their centers
    bool alongXAxis = false;
    
    if(baseBlockOffsetAngle < M_PI_4 && baseBlockOffsetAngle >= -M_PI_4) {
      alongXAxis = true;
    }
    else if(baseBlockOffsetAngle < 3*M_PI_4 && baseBlockOffsetAngle >= M_PI_4) {
      alongXAxis = false;
    }
    else if(baseBlockOffsetAngle < -M_PI_4 && baseBlockOffsetAngle >= -3*M_PI_4) {
      alongXAxis = false;
    }
    else {
      assert(baseBlockOffsetAngle < -3*M_PI_4 || baseBlockOffsetAngle > 3*M_PI_4);
      alongXAxis = true;
    }
    
    maxXAxisOffset = staticBlock->GetSize().x()/2;
    maxYAxisOffset = staticBlock->GetSize().y()/2;

    xAxisOffset = basePoseWRTStatic.GetTranslation().x();
    yAxisOffset = basePoseWRTStatic.GetTranslation().y();
    
    // lessen the offset to account for the space from the center of the block to the edge
    // and extend the max offset since we can be more lenienet along that axis
    const float kAdditionalHorizontalToleranceMultiplier = 0.25;
    if(alongXAxis){
      xAxisOffset -= (staticBlock->GetSize().x()/2 + baseBlock->GetSize().x()/2);
      maxXAxisOffset += baseBlock->GetSize().x()*kAdditionalHorizontalToleranceMultiplier;
    }else{
      yAxisOffset -= (staticBlock->GetSize().y()/2 + baseBlock->GetSize().y()/2);
      maxYAxisOffset += baseBlock->GetSize().y()*kAdditionalHorizontalToleranceMultiplier;
    }
    
    
    PRINT_CH_INFO("Behaviors", "PyramidBase.GetCurrentBlockOffsetProperties.BlockOffsetAngle",
                      "Base block ID:%d at x:%f y:%f, Static staticID:%d at x:%f y:%f offset deg:%f - Along XAxis:%d",
                      baseBlock->GetID().GetValue(), baseBlock->GetPose().GetTranslation().x(), baseBlock->GetPose().GetTranslation().y(),
                      staticBlock->GetID().GetValue(), staticBlock->GetPose().GetTranslation().x(), staticBlock->GetPose().GetTranslation().y(),
                      RAD_TO_DEG_F32(baseBlockOffsetAngle.ToFloat()), alongXAxis);
    
    return true;
  }
  
  return false;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::BlocksFormPyramidBase(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock)
{
  float xAxisOffset;
  float yAxisOffset;
  float maxXAxisOffset;
  float maxYAxisOffset;
  if(GetCurrentBlockOffsetProperties(robot, staticBlock, baseBlock, xAxisOffset, yAxisOffset, maxXAxisOffset, maxYAxisOffset)){
    return FLT_LT(std::abs(xAxisOffset), maxXAxisOffset) && FLT_LT(std::abs(yAxisOffset), maxYAxisOffset);
  }
  
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool PyramidBase::GetBaseRelStaticInfo(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock, bool& alongXAxis, bool& isPositive)
{
  if(!staticBlock || !baseBlock){
    PRINT_NAMED_WARNING("PyramidBase.GetBaseBlockOffset.NullObject",
                      "Static block or base block is null: sb:%p, bb:%p", staticBlock, baseBlock);
    return false;
  }
  
  float xAxisOffset;
  float yAxisOffset;
  float maxXAxisOffset;
  float maxYAxisOffset;
  if(GetCurrentBlockOffsetProperties(robot, staticBlock, baseBlock, xAxisOffset, yAxisOffset, maxXAxisOffset, maxYAxisOffset)){
    // the axis with the smaller max offset is assumed to be the denominant axis
    alongXAxis = maxXAxisOffset > maxYAxisOffset;
    if(alongXAxis){
      isPositive = FLT_GT(xAxisOffset, 0);
    }else{
      isPositive = FLT_GT(yAxisOffset, 0);
    }
    return true;
  }
  
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool PyramidBase::GetBaseRelStaticInfo(const Robot& robot, bool& alongXAxis, bool& isPositive) const
{
  const ObservableObject* const baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  const ObservableObject* const staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  return GetBaseRelStaticInfo(robot, staticBlock, baseBlock, alongXAxis, isPositive);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Point2f PyramidBase::GetBaseBlockIdealOffsetValues(const Robot& robot) const
{
  Point2f offset = Point2f(0,0);
  bool alongXAxis = false;
  bool isPositive = false;
  const ObservableObject* const staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  if(staticBlock != nullptr && GetBaseRelStaticInfo(robot, alongXAxis, isPositive)){
    const float staticBlockSizeX = staticBlock->GetSize().x();
    const float staticBlockSizeY = staticBlock->GetSize().y();
    const float signMultiplier = isPositive ? 1 : -1;
    if(alongXAxis){
      offset = Point2f(signMultiplier*staticBlockSizeX, 0);
    }else{
      offset = Point2f(0, signMultiplier*staticBlockSizeY);
    }
  }else{
    // unable to get offset
    if(staticBlock == nullptr){
      PRINT_CH_INFO("Behaviors","PyramidBase.GetBaseBlockOffsetValues.StaticNullptr",
                        "The static blockID for this pyramid is not valid");
    }else{
      PRINT_CH_INFO("Behaviors","PyramidBase.GetBaseBlockOffsetValues.NoBaseOffset",
                    "Unable to get a base block offset for a pyramid base");
    }
    
  }
    
  return offset;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool PyramidBase::ObjectIsOnTopOfBase(const Robot& robot, const ObservableObject* const object) const
{
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_staticBlockID);
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(_baseBlockID);
  if(!staticBlock || !baseBlock || !object){
    return false;
  }
  
  bool ableToComparePoses = true;
  
  Pose3d objectPoseWRTStatic;
  ableToComparePoses = ableToComparePoses && object->GetPose().GetWithRespectTo(staticBlock->GetPose(), objectPoseWRTStatic);
  Pose3d basePoseWRTStatic;
  ableToComparePoses = ableToComparePoses && baseBlock->GetPose().GetWithRespectTo(staticBlock->GetPose(), basePoseWRTStatic);
  
  if(!ableToComparePoses){
    return false;
  }
  
  // Check that the block is above the static block
  const Pose3d staticWrtOrigin = staticBlock->GetPose().GetWithRespectToOrigin();
  const Point3f rotatedBtmSize(staticWrtOrigin.GetRotation() * staticBlock->GetSize());
  const float staticBlockTopZ = staticWrtOrigin.GetTranslation().z() + (0.5f * std::abs(rotatedBtmSize.z()));
  
  const Pose3d topWrtOrigin = object->GetPose().GetWithRespectToOrigin();
  const Point3f rotatedTopSize(topWrtOrigin.GetRotation() * object->GetSize());
  const float topBlockBottomZ = topWrtOrigin.GetTranslation().z() - (0.5f * std::abs(rotatedTopSize.z()));
  
  const bool topAtAppropriateHeight = Util::IsNear(topBlockBottomZ, staticBlockTopZ, STACKED_HEIGHT_TOL_MM);
  if(!topAtAppropriateHeight){
    return false;
  }
  
  bool alongXAxis = false;
  bool isPositive = false;
  if(!GetBaseRelStaticInfo(robot, alongXAxis, isPositive)){
    return false;
  }
  
  // Check to make sure the top block is between the bottom and stack blocks
  
  const float xTopBlockOverhangTolerence_mm = staticBlock->GetSize().x()/2 - kTopBlockOverhangMin_mm;
  const float yTopBlockOverhangTolerence_mm = staticBlock->GetSize().y()/2 - kTopBlockOverhangMin_mm;
  const float arbitrarilyHighZ = 100.f; // this has already been checked
  const Point3f distanceTolerence = Point3f(xTopBlockOverhangTolerence_mm, yTopBlockOverhangTolerence_mm, arbitrarilyHighZ);

  // Calculate the edge of the static block's offset
  float staticEdgeX = 0;
  float staticEdgeY = 0;
  const float offsetMultiplyer = isPositive ? 1 : -1;
  if(alongXAxis){
    staticEdgeX += offsetMultiplyer * staticBlock->GetSize().x()/2;
  }else{
    staticEdgeY += offsetMultiplyer * staticBlock->GetSize().y()/2;
  }
  
  
  Pose3d topBlockIdealCenter = Pose3d(0.f, Z_AXIS_3D(), {staticEdgeX, staticEdgeY, 0.f}, &object->GetPose());
  
  return object->GetPose().IsSameAs(topBlockIdealCenter, distanceTolerence, M_PI);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PyramidBase::ClearBase()
{
  _baseBlockID.SetToUnknown();
  _staticBlockID.SetToUnknown();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<ObjectID> PyramidBase::GetAllBlockIDsOrdered() const
{
  std::vector<ObjectID> allBlocks;

  // always order static vs block in value order for valid comparisons
  if(_baseBlockID > _staticBlockID){
    allBlocks.push_back(_staticBlockID);
    allBlocks.push_back(_baseBlockID);
  }else{
    allBlocks.push_back(_baseBlockID);
    allBlocks.push_back(_staticBlockID);
  }
  
  return allBlocks;
}
  
// ---------------------------------------------------------
// Pyramid
// ---------------------------------------------------------

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Pyramid::Pyramid(const PyramidBase& base, const ObjectID& topBlockID)
: BlockConfiguration(ConfigurationType::Pyramid)
, _base(base)
, _topBlockID(topBlockID)
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Pyramid::Pyramid(const ObjectID& staticBlockID, const ObjectID& baseBlockID, const ObjectID& topBlockID)
: BlockConfiguration(ConfigurationType::Pyramid)
, _base(PyramidBase(staticBlockID, baseBlockID))
, _topBlockID(topBlockID)
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Pyramid::operator==(const Pyramid& other) const
{
  return GetPyramidBase() == other.GetPyramidBase() &&
         GetTopBlockID() == other.GetTopBlockID();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<const PyramidBase*> Pyramid::BuildAllPyramidBasesForBlock(const Robot& robot, const ObservableObject* object)
{
  std::vector<const PyramidBase*> bases;
  
  std::vector<const ObservableObject*> blocksOnGround;
  
  {
    BlockWorldFilter bottomBlockFilter;
    bottomBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    bottomBlockFilter.AddFilterFcn([](const ObservableObject* blockPtr)
                                   {
                                     if(blockPtr->IsPoseStateUnknown()){
                                       return false;
                                     }
                                     
                                     if(!blockPtr->IsRestingAtHeight(0, kOnGroundTolerencePyramidOnly)){
                                       return false;
                                     }
                                     
                                     if(!blockPtr->IsRestingFlat()){
                                       return false;
                                     }
                                     
                                     return true;
                                   });
    
    robot.GetBlockWorld().FindMatchingObjects(bottomBlockFilter, blocksOnGround);
  }
  
  // check to see if there are enough blocks on the ground for a pyramid to exist
  if(blocksOnGround.size() < 2){
    return bases;
  }
  
  auto objectOnGroundIter = std::find(blocksOnGround.begin(), blocksOnGround.end(), object);
  const bool isTargetObjectOnGround = objectOnGroundIter != blocksOnGround.end();
  
  if(isTargetObjectOnGround){
    blocksOnGround.erase(objectOnGroundIter);
    // check all other blocks on the ground to see if they form a pyramid base with the target object
    for(const auto& baseBlock: blocksOnGround){
      const bool blocksFormBase = PyramidBase::BlocksFormPyramidBase(robot, object, baseBlock);
      if(blocksFormBase){
        bases.push_back(new PyramidBase(object->GetID(), baseBlock->GetID()));
      }// end if(blocksFormBase)
    }// end for(blocksOnGround)
  }
  
  return bases;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<const Pyramid*> Pyramid::BuildAllPyramidsForBlock(const Robot& robot, const ObservableObject* object)
{
  std::vector<const Pyramid*> pyramids;
  if(object == nullptr){
    return pyramids;
  }
  
  std::vector<PyramidBasePtr> allPyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();

  // if the block is on the ground, check if it is part of a pyramid base
  // and then check all possible top blocks to see if they are on top of it
  if(object->IsRestingAtHeight(0, kOnGroundTolerencePyramidOnly)){
    std::vector<const ObservableObject*> potentialTopBlocks;
    // Get all potential top blocks
    {
      BlockWorldFilter topBlocksFilter;
      topBlocksFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
      topBlocksFilter.AddFilterFcn([](const ObservableObject* blockPtr)
                                   {
                                     if(!blockPtr->IsPoseStateKnown()){
                                       return false;
                                     }
                                     
                                     if(blockPtr->IsRestingAtHeight(0, kOnGroundTolerencePyramidOnly)){
                                       return false;
                                     }
                                     
                                     return true;
                                   });
      robot.GetBlockWorld().FindMatchingObjects(topBlocksFilter, potentialTopBlocks);
    }
    
    // check to see if the target block is part of any pyramid bases
    for(const auto& basePtr: allPyramidBases){
      if(basePtr->ContainsBlock(object->GetID())){
        // The target is part of a pyramid base
        // check each topBlock against this base - if it's on top add it to the pyramid set
        for(const auto& topBlock: potentialTopBlocks){
          if(basePtr->ObjectIsOnTopOfBase(robot, topBlock)){
            pyramids.push_back(new Pyramid(basePtr->GetStaticBlockID(), basePtr->GetBaseBlockID(), topBlock->GetID()));
          }
        }
      }
    }
  }else{
    // check  to see if the object is a top block for any bases
    for(const auto& basePtr: allPyramidBases){
      if(basePtr->ObjectIsOnTopOfBase(robot, object)){
        pyramids.push_back(new Pyramid(basePtr->GetStaticBlockID(), basePtr->GetBaseBlockID(), object->GetID()));
        break;
      }
    }
  }
  
  return pyramids;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Pyramid::ClearPyramid()
{
  _base.ClearBase();
  _topBlockID.SetToUnknown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<ObjectID> Pyramid::GetAllBlockIDsOrdered() const
{
  std::vector<ObjectID> allBlocks = _base.GetAllBlockIDsOrdered();
  allBlocks.push_back(_topBlockID);
  return allBlocks;
}
  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki
