#ifndef __OPTO_H
#define __OPTO_H

#include "messages.h"

namespace Opto {
  extern FailureCode failure;

  void start(void);
  void stop(void);
  void tick(void);
  void transmit(BodyToHead *payload);
  bool sensorsValid(void);
}

// Cliff sensor registers
static const uint8_t DROP_SENSOR_ADDRESS = 0xA6;

static const uint8_t MAIN_CTRL = 0x00;
static const uint8_t PS_LED = 0x01;
static const uint8_t PS_PULSES = 0x02;
static const uint8_t PS_MEAS_RATE = 0x03;
static const uint8_t PART_ID = 0x06;
static const uint8_t MAIN_STATUS = 0x07;
static const uint8_t PS_DATA_0 = 0x08;
static const uint8_t PS_DATA_1 = 0x09;
static const uint8_t INT_CFG = 0x19;
static const uint8_t INT_PST = 0x1A;
static const uint8_t PS_THRES_UP_0 = 0x1B;
static const uint8_t PS_THRES_UP_1 = 0x1C;
static const uint8_t PS_THRES_LOW_0 = 0x1D;
static const uint8_t PS_THRES_LOW_1 = 0x1E;
static const uint8_t PS_CAN_0 = 0x1F;
static const uint8_t PS_CAN_1 = 0x20;

// Time of flight sensor registers
static const uint8_t TOF_SENSOR_ADDRESS = 0x52;

static const uint8_t SYSRANGE_START = 0x00;

static const uint8_t SYSTEM_THRESH_HIGH = 0x0C;
static const uint8_t SYSTEM_THRESH_LOW = 0x0E;

static const uint8_t SYSTEM_SEQUENCE_CONFIG = 0x01;
static const uint8_t SYSTEM_RANGE_CONFIG = 0x09;
static const uint8_t SYSTEM_INTERMEASUREMENT_PERIOD = 0x04;

static const uint8_t SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A;

static const uint8_t GPIO_HV_MUX_ACTIVE_HIGH = 0x84;

static const uint8_t SYSTEM_INTERRUPT_CLEAR = 0x0B;

static const uint8_t RESULT_INTERRUPT_STATUS = 0x13;
static const uint8_t RESULT_RANGE_STATUS = 0x14;

static const uint8_t RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN = 0xBC;
static const uint8_t RESULT_CORE_RANGING_TOTAL_EVENTS_RTN = 0xC0;
static const uint8_t RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF = 0xD0;
static const uint8_t RESULT_CORE_RANGING_TOTAL_EVENTS_REF = 0xD4;
static const uint8_t RESULT_PEAK_SIGNAL_RATE_REF = 0xB6;

static const uint8_t ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x28;

static const uint8_t I2C_SLAVE_DEVICE_ADDRESS = 0x8A;

static const uint8_t MSRC_CONFIG_CONTROL = 0x60;

static const uint8_t PRE_RANGE_CONFIG_MIN_SNR = 0x27;
static const uint8_t PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56;
static const uint8_t PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57;
static const uint8_t PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64;

static const uint8_t FINAL_RANGE_CONFIG_MIN_SNR = 0x67;
static const uint8_t FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47;
static const uint8_t FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48;
static const uint8_t FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44;

static const uint8_t PRE_RANGE_CONFIG_SIGMA_THRESH_HI = 0x61;
static const uint8_t PRE_RANGE_CONFIG_SIGMA_THRESH_LO = 0x62;

static const uint8_t PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50;
static const uint8_t PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51;
static const uint8_t PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x52;

static const uint8_t SYSTEM_HISTOGRAM_BIN = 0x81;
static const uint8_t HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT = 0x33;
static const uint8_t HISTOGRAM_CONFIG_READOUT_CTRL = 0x55;

static const uint8_t FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70;
static const uint8_t FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71;
static const uint8_t FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x72;
static const uint8_t CROSSTALK_COMPENSATION_PEAK_RATE_MCPS = 0x20;

static const uint8_t MSRC_CONFIG_TIMEOUT_MACROP = 0x46;

static const uint8_t SOFT_RESET_GO2_SOFT_RESET_N = 0xBF;
static const uint8_t IDENTIFICATION_MODEL_ID = 0xC0;
static const uint8_t IDENTIFICATION_REVISION_ID = 0xC2;

static const uint8_t OSC_CALIBRATE_VAL = 0xF8;

static const uint8_t GLOBAL_CONFIG_VCSEL_WIDTH = 0x32;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xB0;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_1 = 0xB1;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_2 = 0xB2;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_3 = 0xB3;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_4 = 0xB4;
static const uint8_t GLOBAL_CONFIG_SPAD_ENABLES_REF_5 = 0xB5;

static const uint8_t GLOBAL_CONFIG_REF_EN_START_SELECT = 0xB6;
static const uint8_t DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4E;
static const uint8_t DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4F;
static const uint8_t POWER_MANAGEMENT_GO1_POWER_FORCE = 0x80;

static const uint8_t VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV = 0x89;

static const uint8_t ALGO_PHASECAL_LIM = 0x30;
static const uint8_t ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30;

#endif
