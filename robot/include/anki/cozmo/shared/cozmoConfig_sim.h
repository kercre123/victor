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
const f32 DRIVE_CENTER_OFFSET = -15.f;

// The height of the lift at various configurations
// Actual limit in proto is closer to 20.4mm, but there is a weird
// issue with moving the lift when it is at a limit. The lift arm
// flies off of the robot and comes back! So for now, we just don't
// drive the lift down that far. We also skip calibration in sim.

// Need a different height in simulation because of the position of the connectors
const f32 LIFT_HEIGHT_LOWDOCK  = 29.f;
const f32 LIFT_HEIGHT_HIGHDOCK = 73.f;
const f32 LIFT_HEIGHT_CARRY    = 88.f;
const f32 LIFT_HEIGHT_LOW_ROLL = 68.f;

// Distance between the lift shoulder joint and the lift "wrist" joint where arm attaches to fork assembly
const f32 LIFT_ARM_LENGTH = 64.f;

// Height of the lifter front assembly above the gripper, used to compute
// the overall height of the robot when the lift is up (i.e. the height is
// is the lift height plus this)
const f32 LIFT_HEIGHT_ABOVE_GRIPPER = 5.f; // approximate

// The lift height is defined as the height of the upper lift arm's wrist joint plus this offset.
const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0;

// The height of the top of the lift crossbar with respect to the wrist joint
const f32 LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT = -22.f;

// The distance along the x axis from the wrist joint to the front of the lifter plate
const f32 LIFT_FRONT_WRT_WRIST_JOINT = 3.5f;

// The height of the "fingers"
const f32 LIFT_FINGER_HEIGHT = 3.8f;

const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 33.5f + WHEEL_RAD_TO_MM}; // relative to robot origin

//const f32 HEAD_CAM_POSITION[3]   = {8.8f, 0.f, -6.f}; // lens face relative to neck joint
const f32 HEAD_CAM_POSITION[3]   = {4.8f, 0.f, -6.f}; // camera/PCB interface relative to neck joint

const f32 LIFT_BASE_POSITION[3]  = {-40.0f, 0.f, 29.5f + WHEEL_RAD_TO_MM}; // relative to robot origin

const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};  // relative to neck joint
*/

/***************************************************************************
 *
 *                          Timing (non-comms)
 *
 **************************************************************************/

// Simulated camera's frame rate, specified by its period in milliseconds
const s32 VISION_TIME_STEP = 65; // This should be a multiple of the world's basic time step!
