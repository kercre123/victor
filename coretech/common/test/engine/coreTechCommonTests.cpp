#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/poseOriginList.h"

#pragma mark --- Pose Tests ---

TEST(PoseTest, ConstructionAndAssignment)
{
  using namespace Anki;

  Pose3d::AllowUnownedParents(true);
  
  const Pose3d origin;
  Pose3d P_orig(DEG_TO_RAD(30), Z_AXIS_3D(), {100.f, 200.f, 300.f}, origin, "Original");
  P_orig.SetID(42);
  EXPECT_EQ(1, P_orig.GetNodeOwnerCount());
  
  // Test copy constructor
  {
    Pose3d P_copy(P_orig);
    EXPECT_EQ(P_orig.GetTranslation(), P_copy.GetTranslation());
    EXPECT_EQ(P_orig.GetRotation(), P_copy.GetRotation());
    EXPECT_TRUE(P_copy.HasSameRootAs(P_orig));
    EXPECT_NE(P_orig.GetID(), P_copy.GetID()); // IDs are NOT copied!
    EXPECT_EQ(P_orig.GetName() + "_COPY", P_copy.GetName());
    EXPECT_EQ(1, P_copy.GetNodeOwnerCount());
    EXPECT_EQ(1, P_orig.GetNodeOwnerCount());
  }
  
  // Test assignment
  {
    Pose3d P_copy;
    P_copy = P_orig;
    EXPECT_EQ(P_orig.GetTranslation(), P_copy.GetTranslation());
    EXPECT_EQ(P_orig.GetRotation(), P_copy.GetRotation());
    EXPECT_TRUE(P_copy.HasSameRootAs(P_orig));
    EXPECT_NE(P_orig.GetID(), P_copy.GetID()); // IDs are NOT copied!
    EXPECT_EQ(P_orig.GetName() + "_COPY", P_copy.GetName());
    EXPECT_EQ(1, P_copy.GetNodeOwnerCount());
    EXPECT_EQ(1, P_orig.GetNodeOwnerCount());
  }
  
// COZMO-10891: Put these back if we discover Rvalue construction/assignment don't have anything to do with this ticket
//  // Test rvalue constructor
//  {
//    Pose3d P_orig2(P_orig); // copy (which we've tested above) so we can move and still reuse P_orig below
//    P_orig2.SetName("Original2");
//    P_orig2.SetID(123);
//    Pose3d P_move(std::move(P_orig2));
//    EXPECT_EQ(P_orig.GetTranslation(), P_move.GetTranslation());
//    EXPECT_EQ(P_orig.GetRotation(), P_move.GetRotation());
//    EXPECT_TRUE(P_move.HasSameRootAs(P_orig));
//    EXPECT_EQ(123, P_move.GetID()); // IDs *are* moved!
//    EXPECT_EQ("Original2", P_move.GetName());
//    EXPECT_EQ(1, P_move.GetNodeOwnerCount());
//  }
//  
//  // Test rvalue assignment
//  {
//    Pose3d P_orig2(P_orig);
//    P_orig2.SetName("Original2");
//    P_orig2.SetID(123);
//    Pose3d P_move;
//    P_move = std::move(P_orig2);
//    EXPECT_EQ(P_orig.GetTranslation(), P_move.GetTranslation());
//    EXPECT_EQ(P_orig.GetRotation(), P_move.GetRotation());
//    EXPECT_TRUE(P_move.HasSameRootAs(P_orig));
//    EXPECT_EQ(123, P_move.GetID()); // IDs *are* moved!
//    EXPECT_EQ("Original2", P_move.GetName());
//    EXPECT_EQ(1, P_move.GetNodeOwnerCount());
//  }
  
}

TEST(PoseTest, RotateBy)
{
  using namespace Anki;
  
  // pi/7 radians around [0.6651 0.7395 0.1037] axis
  const RotationMatrix3d Rmat1 = {
    0.944781277013538,   0.003728131819389,   0.327680392513508,
    0.093691220482485,   0.955123218207869,  -0.281001055593651,
    -0.314022760017760,   0.296185312048692,   0.902033240583432
  };
  
  // 0.6283 radians around [-0.8190   -0.5670   -0.0875] axis
  const RotationMatrix3d Rmat2 = {
    0.937130929836233,   0.140108185633360,  -0.319617453626684,
    0.037286544290262,   0.870425010004489,   0.490886968225451,
    0.346980307739744,  -0.471942791318199,   0.810478048909173
  };
  
  // Matrix product as computed in Matlab, R1*R2
  const RotationMatrix3d Rprod = {
    0.999221409206381,  -0.019029749384142,  -0.034560729477129,
    0.025912352001530,   0.977106466215908,   0.211167004271049,
    0.029751057079763,  -0.211898141373248,   0.976838805681470
  };
  
  // Rotate by a rotation matrix
  {
    Pose3d P(Rmat2, {1.f, 0.f, 0.f});
    P.RotateBy(Rmat1);
    EXPECT_TRUE(IsNearlyEqual(P.GetRotationMatrix(), Rprod, 1e-6f));
    EXPECT_TRUE(IsNearlyEqual(P.GetTranslation(), Rmat1.GetColumn(0), 1e-6f));
  }
  
  // Rotate by a rotation vector
  {
    Pose3d P(Rmat2, {0.f, 1.f, 0.f});
    P.RotateBy(RotationVector3d(Rmat1));
    EXPECT_TRUE(IsNearlyEqual(P.GetRotationMatrix(), Rprod, 1e-6f));
    EXPECT_TRUE(IsNearlyEqual(P.GetTranslation(), Rmat1.GetColumn(1), 1e-6f));
  }
  
  // Rotate by quaternion
  {
    Pose3d P(Rmat2, {0.f, 0.f, 1.f});
    P.RotateBy(Rotation3d(Rmat1));
    EXPECT_TRUE(IsNearlyEqual(P.GetRotationMatrix(), Rprod, 1e-6f));
    EXPECT_TRUE(IsNearlyEqual(P.GetTranslation(), Rmat1.GetColumn(2), 1e-6f));
  }
}

TEST(PoseTest, PoseTreeValidity)
{
  using namespace Anki;
  
  // So we can restore later
  const bool originalAllowUnownedParents = Pose3d::AreUnownedParentsAllowed();
  
  Pose3d poseOrigin("Origin");
  PoseOriginID_t uniqueID = 1;
  poseOrigin.SetID(uniqueID++);
  ASSERT_EQ(1, poseOrigin.GetNodeOwnerCount());
  
  Pose3d child1("Child1");
  child1.SetParent(poseOrigin);
  child1.SetID(uniqueID++);
  ASSERT_EQ(1, child1.GetNodeOwnerCount());
  
  {
    // This will wrpa the same underlying PoseTreeNode as the origin, but
    // is a separate wrapper. Thus the owner count increases by 1.
    Pose3d tempParent = child1.GetParent();
    ASSERT_EQ(2, poseOrigin.GetNodeOwnerCount());
  }
  // Once tempParent destructs, we're back to a single wrapper around the origin's underlying node.
  ASSERT_EQ(1, poseOrigin.GetNodeOwnerCount());
  
  ASSERT_TRUE(child1.IsChildOf(poseOrigin));
  ASSERT_TRUE(poseOrigin.IsParentOf(child1));
  ASSERT_FALSE(child1.IsParentOf(poseOrigin));
  ASSERT_FALSE(poseOrigin.IsChildOf(child1));
  ASSERT_NE(child1.GetID(), poseOrigin.GetID());
  ASSERT_EQ(child1.GetParent().GetID(), poseOrigin.GetID());
  
  // Make a copy of child1. Copy should still be poseOrigin's child, but should be a separate pose.
  Pose3d child2(child1);
  ASSERT_TRUE(child2.IsChildOf(poseOrigin));
  ASSERT_TRUE(poseOrigin.IsParentOf(child2));
  ASSERT_EQ(child1.GetName(), child2.GetName().substr(0, child1.GetName().length()));
  ASSERT_NE(child1.GetID(), child2.GetID()); // ID should *not* be preserved
  ASSERT_EQ(0, child2.GetID());
  
  // Same, but copy via assignment
  Pose3d child3;
  ASSERT_FALSE(child3.IsChildOf(poseOrigin));
  ASSERT_FALSE(poseOrigin.IsParentOf(child3));
  child3 = child1;
  ASSERT_TRUE(child3.IsChildOf(poseOrigin));
  ASSERT_TRUE(poseOrigin.IsParentOf(child3));
  ASSERT_EQ(child1.GetName(), child3.GetName().substr(0, child1.GetName().length()));
  ASSERT_NE(child1.GetID(), child3.GetID()); // ID should *not* be preserved
  ASSERT_EQ(0, child3.GetID());
  
  ASSERT_TRUE(child1.HasSameParentAs(child2));
  ASSERT_TRUE(child2.HasSameParentAs(child1));
  ASSERT_TRUE(child3.HasSameParentAs(child1));
  ASSERT_TRUE(child2.HasSameParentAs(child3));
  ASSERT_TRUE(child3.HasSameParentAs(child2));
  ASSERT_TRUE(child1.HasSameParentAs(child1));
  ASSERT_FALSE(child1.HasSameParentAs(poseOrigin));
  
  ASSERT_EQ(poseOrigin.GetID(), child1.GetRootID());
  ASSERT_EQ(poseOrigin.GetID(), child2.GetRootID());
  ASSERT_EQ(poseOrigin.GetID(), child3.GetRootID());
  
  // Go one level deeper, by adding a child of a child
  Pose3d grandChild1("GrandKid1");
  grandChild1.SetParent(child1);
  ASSERT_TRUE(grandChild1.HasSameRootAs(poseOrigin));
  ASSERT_FALSE(grandChild1.IsChildOf(poseOrigin));
  ASSERT_TRUE(grandChild1.HasSameRootAs(child1));
  ASSERT_TRUE(grandChild1.HasSameRootAs(child2));
  ASSERT_EQ(poseOrigin.GetID(), grandChild1.GetRootID());
  
  {
    // This will wrpa the same underlying PoseTreeNode as the origin, but
    // is a separate wrapper. Thus the owner count increases by 1.
    const Pose3d tempRoot = grandChild1.FindRoot();
    ASSERT_EQ(2, poseOrigin.GetNodeOwnerCount());
  }
  // Once tempRoot destructs, we're back to a single wrapper around the origin's underlying node.
  ASSERT_EQ(1, poseOrigin.GetNodeOwnerCount());
  
  PoseID_t tempID = 0;
  Pose3d* tempPosePtr = nullptr;
  Pose3d grandChild2("GrandKid2");
  {
    // Create temp pose parented to origin, with grandChild2 as its child.
    // When this temp pose goes out of scope grandChild2 should still have a path to origin
    // *iff* unowned parents are allowed
    Pose3d tempPose("Temp");
    tempPose.SetID(uniqueID++);
    tempID = tempPose.GetID();
    tempPose.SetParent(poseOrigin);
    grandChild2.SetParent(tempPose);
    tempPosePtr = &tempPose;
    Pose3d::AllowUnownedParents(true); // Allow unowned to allow the tempPose going out of scope not to cause an abort
  }
  Pose3d::AllowUnownedParents(false); // GetParent call should abort b/c parent is unowned
  ASSERT_DEATH(grandChild2.GetParent(), "");
  
  Pose3d::AllowUnownedParents(true); // Allow unowned parents for the purposes of the rest of this test
  ASSERT_TRUE(Pose3d::AreUnownedParentsAllowed());
  
  memset((void*)tempPosePtr, 0, sizeof(Pose3d)); // Forcibly nuke tempPose's memory just to be extra nasty
  ASSERT_TRUE(grandChild2.HasSameRootAs(poseOrigin));
  ASSERT_EQ(grandChild2.GetParent().GetName(), "Temp");
  ASSERT_EQ(tempID, grandChild2.GetParent().GetID());
  ASSERT_EQ(poseOrigin.GetID(), grandChild2.GetRootID());
  
  ASSERT_EQ(1, poseOrigin.GetNodeOwnerCount());
  
  // Verify parents are hooked up as expected with GetWRT and SetParent calls
  {
    Pose3d::AllowUnownedParents(false);
    Pose3d otherPose("Other");
    otherPose.SetParent(child1.GetParent());
    ASSERT_EQ(otherPose.GetParent().GetID(), poseOrigin.GetID());
    
    child2.SetID(uniqueID++);
    ASSERT_TRUE(child3.GetWithRespectTo(child2, otherPose));
    ASSERT_EQ(otherPose.GetParent().GetID(), child2.GetID());
    
    ASSERT_TRUE(child3.GetWithRespectTo(child2.GetParent(), otherPose));
    ASSERT_EQ(otherPose.GetParent().GetID(), poseOrigin.GetID());
    
    ASSERT_TRUE(child3.GetWithRespectTo(grandChild1.FindRoot(), otherPose));
    ASSERT_EQ(otherPose.GetParent().GetID(), poseOrigin.GetID());
    
    ASSERT_TRUE(child3.GetWithRespectTo(grandChild1.GetParent().GetParent(), otherPose));
    ASSERT_EQ(otherPose.GetParent().GetID(), poseOrigin.GetID());
    
    otherPose = grandChild1.GetWithRespectToRoot();
    ASSERT_EQ(otherPose.GetParent().GetID(), poseOrigin.GetID());
    
    // Let the poses with IDs destruct without complaint
    Pose3d::AllowUnownedParents(true);
  }
  
  // Make sure we restore to whatever the setting was at the start
  Pose3d::AllowUnownedParents(originalAllowUnownedParents);
}

TEST(PoseOriginList, IDs)
{
  using namespace Anki;
  
  PoseOriginList originList;
  
  const PoseID_t id1 = originList.AddNewOrigin();
  const PoseID_t id2 = originList.AddNewOrigin();
  
  ASSERT_NE(PoseOriginList::UnknownOriginID, id1);
  ASSERT_NE(id1, id2);
  
  // Duplicating an ID will abort with DEV_ASSERT in Debug, or return false in Release/Shipping
# if defined(NDEBUG)
  ASSERT_FALSE(originList.AddOriginWithID(id1));
  ASSERT_FALSE(originList.AddOriginWithID(id2));
# else 
  ASSERT_DEATH(originList.AddOriginWithID(id1), "");
  ASSERT_DEATH(originList.AddOriginWithID(id2), "");
# endif
  
  const PoseID_t id3 = id2 + 1;
  ASSERT_TRUE(originList.AddOriginWithID(id3));
  
  ASSERT_EQ(3, originList.GetSize());
}

TEST(PoseOriginList, Lookup)
{
  using namespace Anki;
  
  PoseOriginList originList;
  
  const PoseID_t originID1 = originList.AddNewOrigin();
  ASSERT_NE(PoseOriginList::UnknownOriginID, originID1);
  
  const Pose3d& origin1 = originList.GetOriginByID(originID1);
  ASSERT_EQ(originID1, origin1.GetID());
  ASSERT_EQ(originID1, originList.GetCurrentOriginID());
  ASSERT_EQ(originID1, originList.GetCurrentOrigin().GetID());
  ASSERT_TRUE(originList.ContainsOriginID(originID1));
  ASSERT_FALSE(originList.ContainsOriginID(101));
  
  // After adding new origin (which should have a unique ID), the origin above should still exist in the list and still
  // with different ID.
  const PoseID_t originID2 = originList.AddNewOrigin();
  ASSERT_NE(originID1, originID2);
  ASSERT_EQ(originID2, originList.GetCurrentOriginID());
  ASSERT_TRUE(originList.ContainsOriginID(originID1));
  ASSERT_TRUE(originList.ContainsOriginID(originID2));
}

#pragma mark --- Matrix Tests ---

GTEST_TEST(Matrix, Transpose)
{
  
}



#pragma mark --- Rotation Tests ---


TEST(CoreTech_Common, EmptyTest)
{
  printf("This is a test\n");
}

