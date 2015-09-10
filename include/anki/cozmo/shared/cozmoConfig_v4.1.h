/***************************************************************************
 *
 *                          Physical Robot Geometry
 *
 **************************************************************************/
/*
// The x-offset from robot origin that the robot's drive center is
// located for the treaded robot when not carrying a block.
// (If you were to model the treaded robot as a two-wheel robot,
// the drive center is the location between the two wheels)
const f32 DRIVE_CENTER_OFFSET = -17.f;

// The height of the lift (specifically upper wrist joint) at various configurations
// Actual limit in proto is closer to 20.4mm, but there is a weird
// issue with moving the lift when it is at a limit. The lift arm
// flies off of the robot and comes back! So for now, we just don't
// drive the lift down that far. We also skip calibration in sim.

// Cozmo v3.2, with clutch-less lift and stronger motor
// These are heights of the wrist joint.
const f32 LIFT_HEIGHT_LOWDOCK  = 33.f;
const f32 LIFT_HEIGHT_HIGHDOCK = 83.f; // 76.f;
const f32 LIFT_HEIGHT_CARRY    = 97.f; // 92.f;
const f32 LIFT_HEIGHT_LOW_ROLL = 73.f; // 68.f;

// Distance between the lift shoulder joint and the lift "wrist" joint where arm attaches to fork assembly
const f32 LIFT_ARM_LENGTH = 66.f;

// Height of the lifter front assembly above the gripper, used to compute
// the overall height of the robot when the lift is up (i.e. the height is
// is the lift height plus this)
const f32 LIFT_HEIGHT_ABOVE_GRIPPER = 5.f; // approximate

// The lift height is defined as the height of the upper lift arm's wrist joint plus this offset.
const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0;

// The height of the top of the lift crossbar with respect to the wrist joint
const f32 LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT = -20.f;

// The distance along the x axis from the wrist joint to the front of the lifter plate
const f32 LIFT_FRONT_WRT_WRIST_JOINT = 4.f;

  // Neck joint relative to robot origin
const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 34.5f + WHEEL_RAD_TO_MM};

// camera relative to neck joint
const f32 HEAD_CAM_POSITION[3]   = {17.7f, 0.f, -8.f};

// Upper shoulder joint relative to robot origin
const f32 LIFT_BASE_POSITION[3]  = {-41.0f, 0.f, 31.3f + WHEEL_RAD_TO_MM}; // relative to robot origin

// IMU position relative to neck joint
const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};
*/

