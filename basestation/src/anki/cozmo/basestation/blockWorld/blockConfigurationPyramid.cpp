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

#include "util/math/math.h"


namespace Anki {
namespace Cozmo {
namespace BlockConfigurations{
  
namespace {
using PyramidIterator = std::vector<PyramidWeakPtr>::iterator;
const float kOnGroundTolerencePyramidOnly = 2*ON_GROUND_HEIGHT_TOL_MM;
const float kTopBlockOverhangMin_mm = 5;
// Distance of 60mm used - 30 mm from corner to center plus max distance seperation of 30mm
const float kDistMaxCornerToCenter_mm = 60;
const float kDistMaxCornerToCenter_mm_sqr = kDistMaxCornerToCenter_mm * kDistMaxCornerToCenter_mm;
}

// ---------------------------------------------------------
// Pyramid Base
// ---------------------------------------------------------
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PyramidBase::PyramidBase(const ObjectID& baseBlock1, const ObjectID& baseBlock2)
: BlockConfiguration(ConfigurationType::PyramidBase)
{
  
  _baseBlockID = baseBlock1 < baseBlock2 ? baseBlock1 : baseBlock2;
  _staticBlockID = baseBlock1 < baseBlock2 ? baseBlock2 : baseBlock1;
  
  if(_staticBlockID == _baseBlockID){
    PRINT_NAMED_WARNING("PyramidBase.PyramidBase.InvalidBlocksPassedIn","Attempted to create a base with two blocks with ID:%d", (int)_staticBlockID);
    ClearBase();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::operator==(const PyramidBase& other) const
{
  return _staticBlockID != _baseBlockID &&
          GetStaticBlockID() == other.GetStaticBlockID() &&
          GetBaseBlockID() == other.GetBaseBlockID();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::BlocksFormPyramidBase(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock)
{
  if(staticBlock == nullptr || baseBlock == nullptr){
    return false;
  }
  
  Pose3d staticCorner1;
  Pose3d staticCorner2;
  Pose3d baseCorner1;
  Pose3d baseCorner2;
  if(GetBaseInteriorCorners(robot, staticBlock, baseBlock, staticCorner1, staticCorner2) &&
     GetBaseInteriorCorners(robot, baseBlock, staticBlock, baseCorner1, baseCorner2)){
    const Pose3d& basePoseCenterAtTop = baseBlock->GetZRotatedPointAboveObjectCenter(0.5f);
    const Pose3d& staticPoseCenterAtTop = staticBlock->GetZRotatedPointAboveObjectCenter(0.5f);
    
    float staticDist1;
    float staticDist2;
    float baseDist1;
    float baseDist2;
    if(ComputeDistanceSQBetween(staticCorner1, basePoseCenterAtTop, staticDist1) &&
       ComputeDistanceSQBetween(staticCorner2, basePoseCenterAtTop, staticDist2) &&
       ComputeDistanceSQBetween(baseCorner1, staticPoseCenterAtTop, baseDist1) &&
       ComputeDistanceSQBetween(baseCorner2, staticPoseCenterAtTop, baseDist2)){
      return staticDist1 < kDistMaxCornerToCenter_mm_sqr &&
             staticDist2 < kDistMaxCornerToCenter_mm_sqr &&
             baseDist1 < kDistMaxCornerToCenter_mm_sqr &&
             baseDist2 < kDistMaxCornerToCenter_mm_sqr;
    }
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::GetBaseInteriorCorners(const Robot& robot,
                                         const ObservableObject* const targetCube,
                                         const ObservableObject* const otherCube,
                                         Pose3d& corner1, Pose3d& corner2)
{
  std::vector<Point2f> allPoints;
  std::multimap<float,Pose3d> poseMap;
  
  // Get all corners of the
  auto boundingQuad = targetCube->GetBoundingQuadXY(0);
  allPoints.push_back(boundingQuad.GetBottomRight());
  allPoints.push_back(boundingQuad.GetTopRight());
  allPoints.push_back(boundingQuad.GetBottomLeft());
  allPoints.push_back(boundingQuad.GetTopLeft());

  const float zSize = targetCube->GetDimInParentFrame<'Z'>();
  
  // calculate distance from corners to center of other block
  for(const auto& point: allPoints){
    Pose3d pointAsPose = Pose3d(0, Z_AXIS_3D(),
                                {point.x(), point.y(), zSize},
                                robot.GetWorldOrigin());
    float dist;
    if(!ComputeDistanceSQBetween(pointAsPose, otherCube->GetPose(), dist)){
      return false;
    }
    poseMap.insert(std::make_pair(dist,std::move(pointAsPose)));
  }
  
  // find the two closest points - the map is automatically sorted by distance
  auto mapIter = poseMap.begin();
  corner1 = (*mapIter).second.GetWithRespectToOrigin();
  ++mapIter;
  corner2 = (*mapIter).second.GetWithRespectToOrigin();
  
  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBase::GetBaseInteriorMidpoint(const Robot& robot,
                                          const ObservableObject* const targetCube,
                                          const ObservableObject* const otherCube,
                                          Pose3d& midPoint){
  Pose3d corner1;
  Pose3d corner2;
  const bool gotCorners = GetBaseInteriorCorners(robot, targetCube, otherCube, corner1, corner2);
  if(!gotCorners){
    return false;
  }
  
  corner1 = corner1.GetWithRespectToOrigin();
  corner2 = corner2.GetWithRespectToOrigin();
  
  const float edgeMiddleX = 0.5f * (corner1.GetTranslation().x() + corner2.GetTranslation().x());
  const float edgeMiddleY = 0.5f * (corner1.GetTranslation().y() + corner2.GetTranslation().y());
  
  const float zSize = targetCube->GetDimInParentFrame<'Z'>();
  
  midPoint = Pose3d(0, Z_AXIS_3D(), {edgeMiddleX, edgeMiddleY, zSize},
                    &robot.GetPose().FindOrigin());
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool PyramidBase::ObjectIsOnTopOfBase(const Robot& robot, const ObservableObject* const targetObject) const
{
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetLocatedObjectByID(_staticBlockID);
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetLocatedObjectByID(_baseBlockID);
  if(staticBlock == nullptr || baseBlock == nullptr || targetObject == nullptr){
    return false;
  }
  
  bool ableToComparePoses = true;
  
  Pose3d objectPoseWRTStatic;
  ableToComparePoses = ableToComparePoses && targetObject->GetPose().GetWithRespectTo(staticBlock->GetPose(), objectPoseWRTStatic);
  Pose3d basePoseWRTStatic;
  ableToComparePoses = ableToComparePoses && baseBlock->GetPose().GetWithRespectTo(staticBlock->GetPose(), basePoseWRTStatic);
  
  if(!ableToComparePoses){
    return false;
  }
  
  // Check that the block is above the static block
  const Pose3d  staticWrtOrigin = staticBlock->GetPose().GetWithRespectToOrigin();
  const Point3f rotatedBtmSize  = staticBlock->GetSizeInParentFrame(staticWrtOrigin);
  const float   staticBlockTopZ = staticWrtOrigin.GetTranslation().z() + (0.5f * std::abs(rotatedBtmSize.z()));
  
  const Pose3d  topWrtOrigin    = targetObject->GetPose().GetWithRespectToOrigin();
  const float   rotatedTopZSize = targetObject->GetDimInParentFrame<'Z'>(topWrtOrigin);
  const float   topBlockBottomZ = topWrtOrigin.GetTranslation().z() - (0.5f * rotatedTopZSize);
  
  const bool topAtAppropriateHeight = Util::IsNear(topBlockBottomZ, staticBlockTopZ, STACKED_HEIGHT_TOL_MM);
  if(!topAtAppropriateHeight){
    return false;
  }
  
  Pose3d staticBlockIdealCenter;
  Pose3d baseBlockIdealCenter;
  if(!GetBaseInteriorMidpoint(robot, staticBlock, baseBlock, staticBlockIdealCenter) ||
     !GetBaseInteriorMidpoint(robot, baseBlock, staticBlock, baseBlockIdealCenter)){
    return false;
  }
  
  const float xTopBlockOverhangTolerence_mm = (0.5f * rotatedBtmSize.x()) - kTopBlockOverhangMin_mm;
  const float yTopBlockOverhangTolerence_mm = (0.5f * rotatedBtmSize.y()) - kTopBlockOverhangMin_mm;
  const float arbitrarilyHighZ = 100.f; // this has already been checked
  const Point3f distanceTolerence = Point3f(xTopBlockOverhangTolerence_mm, yTopBlockOverhangTolerence_mm, arbitrarilyHighZ);
  
  const Vec3f& targetTrans = targetObject->GetPose().GetTranslation();
  Pose3d targetObjectUnrotatedCenter(0, Z_AXIS_3D(),
                                     {targetTrans.x(),
                                      targetTrans.y(),
                                      0},
                                     &staticBlock->GetPose().FindOrigin());
  
  const bool topBlockCentered =
               targetObjectUnrotatedCenter.IsSameAs(staticBlockIdealCenter, distanceTolerence, M_PI) &&
               targetObjectUnrotatedCenter.IsSameAs(baseBlockIdealCenter, distanceTolerence, M_PI);
  
  
  return topBlockCentered;
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
  allBlocks.push_back(_baseBlockID);
  allBlocks.push_back(_staticBlockID);

  
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
Pyramid::Pyramid(const ObjectID& baseBlock1, const ObjectID& baseBlock2, const ObjectID& topBlockID)
: BlockConfiguration(ConfigurationType::Pyramid)
, _base(PyramidBase(baseBlock1, baseBlock2))
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
                                     if(!blockPtr->IsRestingAtHeight(0, kOnGroundTolerencePyramidOnly)){
                                       return false;
                                     }
                                     
                                     if(!blockPtr->IsRestingFlat()){
                                       return false;
                                     }
                                     
                                     return true;
                                   });
    
    robot.GetBlockWorld().FindLocatedMatchingObjects(bottomBlockFilter, blocksOnGround);
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
      robot.GetBlockWorld().FindLocatedMatchingObjects(topBlocksFilter, potentialTopBlocks);
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
