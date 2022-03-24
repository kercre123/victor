#ifndef ENC_TEST_H
#define ENC_TEST_H

const uint ONBOARD_LED_PIN = 25;

const uint DETECT_PIN = 28;
const uint DETECT_ADC = 2;
const uint ENC_A_PIN = 27;
const uint ENC_A_ADC = 1;
const uint ENC_B_PIN = 26;
const uint ENC_B_ADC = 0;

const uint ENC_LED_PIN = 15;

const uint MOTOR_ENABLE_PIN = 14;

const uint LED_RED_PIN = 11;
const uint LED_GREEN_PIN = 12;
const uint LED_BLUE_PIN = 13;

const int DELTA_LOW = 40;
const int DELTA_HIGH = 80;

#define sample_size 1000

typedef enum {
  RED,
  GREEN,
  BLUE,
  WHITE,
  YELLOW,
  MAGENTA,
  CYAN,
  OFF
} color_t;

typedef enum {
  ANALYZE_NOT_INITIALIZED,
  ANALYZE_INITIALIZED,
  ANALYZE_HIGH_DONE,
  ANALYZE_LOW_DONE
} analyze_state_t;

struct analyze_data {
  int a_high_tick;
  int b_high_tick;
  int a_low_tick;
  int b_low_tick;
  float a_min_volt;
  float b_min_volt;
  float a_max_volt;
  float b_max_volt;
  int high_delta_us;
  int low_delta_us;
  
};

typedef enum {
  SUCCESS,
  ERROR_NO_HIGH,
  ERROR_NO_LOW,
  ERROR_DELTA_TOO_LOW,
  ERROR_DELTA_TOO_HIGH,
  ERROR_LED_SHOULD_BE_OFF,
  ERROR_WIRES_REVERSED
} test_status_t;

#endif
