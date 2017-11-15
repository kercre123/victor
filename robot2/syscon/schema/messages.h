/** Clad definition file for messages sent between Application processor and Hardware processor.
 *
 * All structures must be fixed size.
*/

/* All spine messages follow the following format:
    <SYNC><TYPE><WORDCOUNT><PAYLOAD><CHECKSUM>
    SYNC is 4-byte fixed string for sync:
        "\xAAB2H" for body messages
        "\xAAH2B" for head messages
    TYPE uint16: payload type.
    Huge majority of messages will be
        "DF" for primary DataFrame
       Others to be added later to suport init, dfu, etc as needed.
       *Important*: Payload Type can not contain any of the Sync characters.
                    This allows us to avoid rescanning them when we get invalid ones.
    BYTECOUNT = uint16: bytes  to follow (excluding checksum).
    PAYLOAD one of the message types below
    CHECKSUM is uint32 bytewise sum of payload.
*/

#ifndef __SCHEMA_MESSAGES_H__
#define __SCHEMA_MESSAGES_H__

#include <stdbool.h>
#include <stdint.h>

// ENUM SpineSync
enum {
  SYNC_BODY_TO_HEAD = 0x483242aa,
  SYNC_HEAD_TO_BODY = 0x423248aa,
};
typedef uint32_t SpineSync;

// ENUM RobotMotor
enum {
  MOTOR_LEFT  = 0,
  MOTOR_RIGHT = 1,
  MOTOR_LIFT  = 2,
  MOTOR_HEAD  = 3,
  MOTOR_COUNT = 4,
};
typedef uint32_t RobotMotor;

// ENUM DropSensor
enum {
  DROP_SENSOR_FRONT_LEFT  = 0,
  DROP_SENSOR_FRONT_RIGHT = 1,
  DROP_SENSOR_BACK_LEFT   = 2,
  DROP_SENSOR_BACK_RIGHT  = 3,
  DROP_SENSOR_COUNT       = 4,
};
typedef uint32_t DropSensor;

// ENUM PayloadId
enum {
  PAYLOAD_DATA_FRAME  = 0x6466,
  PAYLOAD_CONT_DATA   = 0x6364,
  PAYLOAD_MODE_CHANGE = 0x6d64,
  PAYLOAD_VERSION     = 0x7276,
  PAYLOAD_ACK         = 0x6b61,
  PAYLOAD_ERASE       = 0x7878,
  PAYLOAD_VALIDATE    = 0x7374,
  PAYLOAD_DFU_PACKET  = 0x6675,
  PAYLOAD_TYPE_COUNT  = 8,
};
typedef uint16_t PayloadId;

// ENUM Ack
enum {
  ACK_PAYLOAD       = 1,
  ACK_BOOTED        = 2,
  ACK_APPLICATION   = 3,
  NACK_CRC_FAILED   = -1,
  NACK_NOT_ERASED   = -2,
  NACK_FLASH_FAILED = -3,
  NACK_CERT_FAILED  = -4,
  NACK_BAD_ADDRESS  = -5,
  NACK_SIZE_ALIGN   = -6,
  NACK_BAD_COMMAND  = -7,
  NACK_NOT_VALID    = -8,
};
typedef int32_t Ack;

// ENUM BatteryFlags
enum {
  isCharging  = 0x1,
  isOnCharger = 0x2,
  chargerOOS  = 0x4,
};
typedef uint32_t BatteryFlags;

// ENUM PowerState
enum {
  POWER_STATE_OFF               = 0,
  POWER_STATE_OFF_WAKE_ON_RADIO = 1,
  POWER_STATE_ON                = 2,
  POWER_STATE_IDLE              = 3,
  POWER_STATE_FORCE_RECOVERY    = 4,
  POWER_STATE_OTA_MODE          = 5,
  POWER_STATE_CHARGER_TEST_MODE = 6,
  POWER_STATE_DTM_MODE          = 7,
};
typedef uint32_t PowerState;

// ENUM LedIndexes
enum {
  LED0_RED    = 0,
  LED0_GREEN  = 1,
  LED0_BLUE   = 2,
  LED1_RED    = 3,
  LED1_GREEN  = 4,
  LED1_BLUE   = 5,
  LED2_RED    = 6,
  LED2_GREEN  = 7,
  LED2_BLUE   = 8,
  LED3_RED    = 9,
  LED3_GREEN  = 10,
  LED3_BLUE   = 11,
  LED_UNUSED1 = 12,
  LED_UNUSED2 = 13,
  LED_UNUSED3 = 14,
  LED_COUNT   = 15,
};
typedef uint32_t LedIndexes;
#define LED_CHANEL_CT 3  //RGB

struct MotorPower
{
  int16_t leftWheel;
  int16_t rightWheel;
  int16_t liftMotor;
  int16_t headMotor;
};

struct BatteryState
{
  int32_t battery;
  int32_t charger;
  BatteryFlags flags;
};

struct ButtonState
{
  uint16_t level;
};

struct RangeData
{
  uint8_t rangeStatus;
  uint8_t spare1;
  uint16_t rangeMM;
  uint16_t signalRate;
  uint16_t ambientRate;
  uint16_t spadCount;
  uint16_t spare2;
  uint32_t calibrationResult;
};

struct ProcessorStatus
{
  uint16_t watchdogCount;
  uint16_t statusBits;
};

struct MotorState
{
  int32_t position;
  int32_t delta;
  uint32_t time;
};

struct SpineMessageHeader
{
  SpineSync sync_bytes;
  PayloadId payload_type;
  uint16_t bytes_to_follow;
};

struct SpineMessageFooter
{
  uint32_t checksum;
};

// TODO(Al/Lee): Put back once mics and camera can co-exist
#define MICDATA_ENABLED 1
#define MICDATA_SAMPLES_COUNT 480 // 120 samples per channel * 4 channels
/// Start Packets
struct BodyToHead
{
  uint32_t framecounter;
  PowerState powerState;
  struct MotorState motor[4];
  uint16_t cliffSense[4];
  struct BatteryState battery;
  struct RangeData proximity;
  uint16_t touchLevel[2];
#if MICDATA_ENABLED
  int16_t audio[MICDATA_SAMPLES_COUNT];
#endif
};

struct ContactData
{
  uint8_t data[32];
};

struct HeadToBody
{
  uint32_t framecounter;
  PowerState powerState;
  int16_t motorPower[4];
  uint8_t ledColors[16];
};

struct AckMessage
{
  Ack status;
};

struct WriteDFU
{
  uint16_t address;
  uint16_t wordCount;
  uint32_t data[256];
};

struct VersionInfo
{
  uint32_t hw_revision;
  uint32_t hw_model;
  uint8_t ein[16];
  uint8_t app_version[16];
};

#endif
