
namespace Anki {

// Parts of robot state that do not change (are not effected by relocalization).
struct StaticRobotState {
  StaticRobotState(const Anki::RobotState &state) {
    lwheel_speed_mmps = state.lwheel_speed_mmps;
    rwheel_speed_mmps = state.rwheel_speed_mmps;
    headAngle = state.headAngle;
    liftAngle = state.liftAngle;
    accel = state.accel;
    gyro = state.gyro;
    imuData = state.imuData;
    batteryVoltage = state.batteryVoltage;
    chargerVoltage = state.chargerVoltage;
    status = state.status;
    cliffDataRaw = state.cliffDataRaw;
    proxDataRaw = state.proxData;
    backpackTouchSensorRaw = state.backpackTouchSensorRaw;
    cliffDetectedFlags = state.cliffDetectedFlags;
    whiteDetectedFlags = state.whiteDetectedFlags;
    battTemp_C = state.battTemp_C;
    currPathSegment = state.currPathSegment;
  }
  float lwheel_speed_mmps;
  float rwheel_speed_mmps;
  float headAngle;
  float liftAngle;
  Anki::Vector::AccelData accel;
  Anki::Vector::GyroData gyro;
  Anki::Vector::IMUDataFrame imuData[6];
  float batteryVoltage;
  float chargerVoltage;
  uint32_t status;
  uint16_t cliffDataRaw[4];
  Anki::Vector::ProxSensorDataRaw proxDataRaw;
  uint16_t backpackTouchSensorRaw;
  uint8_t cliffDetectedFlags;
  uint8_t whiteDetectedFlags;
  uint8_t battTemp_C;
  int8_t currPathSegment;
};

struct HistoricalRobotState {
  RobotTimestamp_t timestamp;
  StaticRobotState robot_state;
  ProxSensorData prox_data;
  Pose3d pose;
};

HistoricalRobotState Interpolate(const HistoricalRobotState &state_1,
                                 const HistoricalRobotState &state_2,
                                 f32 fraction);

} // namespace Anki
