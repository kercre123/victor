
/***************************************************************************
 *
 *                          Physical Robot Geometry
 *
 **************************************************************************/

const f32 WHEEL_DIAMETER_MM  = 29.f;
const f32 HALF_WHEEL_CIRCUM  = WHEEL_DIAMETER_MM * M_PI_2;
const f32 WHEEL_RAD_TO_MM    = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
const f32 WHEEL_DIST_MM      = 47.7f; // approx distance b/w the center of the front treads
const f32 WHEEL_DIST_HALF_MM = WHEEL_DIST_MM / 2.f;
const f32 WHEEL_BASE_MM      = 48.f;


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

// Distance between the robot origin and the distance along the robot's x-axis
// to the lift when it is in the low docking position.
const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 20.f;
const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 17.f;
const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 20.f;
const f32 ORIGIN_TO_LOW_ROLL_DIST_MM = 13.f;

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



/***************************************************************************
 *
 *                          Camera Calibration
 *
 **************************************************************************/

static const u8 NUM_RADIAL_DISTORTION_COEFFS = 4;

// Calibration values from Sept 1, 2015 - on 4.1 robot headboard with SSID 3a97
const u16 HEAD_CAM_CALIB_WIDTH  = 400;
const u16 HEAD_CAM_CALIB_HEIGHT = 296;
const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 278.065116921f;
const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 278.867229568f;
const f32 HEAD_CAM_CALIB_CENTER_X       = 197.801561858f;
const f32 HEAD_CAM_CALIB_CENTER_Y       = 151.672492176f;
const f32 HEAD_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] = {
  0.11281163f,
  -0.31673507f,
  -0.00226334f,
  0.00200109f
};

