#ifndef HARDWARE_H
#define HARDWARE_H

#define NRF_BAUD(x) (int)(x * 268.435456) // 2^28/1MHz
#define CYCLES_MS(ms) (int)(32768 * 256.0f * ms / 1000.0f)

//#define DEBUG_MESSAGES
//#define RADIO_TIMING_TEST

// Updated for EP1
#define ROBOT_EP1_BODY

enum e_nrf_gpio {
  // Encoders
  PIN_ENCODER_LEFT    = 13, // ENC1
  PIN_ENCODER_RIGHT   = 4,  // ENC2
  PIN_ENCODER_HEADA   = 17, // ENC3
  PIN_ENCODER_HEADB   = 14,
  PIN_ENCODER_LIFTA   = 1,  // ENC4
  PIN_ENCODER_LIFTB   = 0,
  

  // Motors + charge OK signal
  PIN_LEFT_P          = 19,
  PIN_LEFT_N1         = 18,
  PIN_LEFT_N2         = 16,
  PIN_RIGHT_P         = 2,
  PIN_RIGHT_N1        = 27,
  PIN_RIGHT_N2        = 29,
  PIN_HEAD_P          = 3, 
  PIN_HEAD_N1         = 28,
  PIN_HEAD_N2         = 25,
  PIN_LIFT_P          = 30,
  PIN_LIFT_N1         = 24,
  PIN_LIFT_N2         = 23,

  // Backpack leds
  PIN_LED1            = 8,
  PIN_LED2            = 9,
  PIN_LED3            = 10,
  PIN_LED4            = 11,

  // Charging
  PIN_CHARGE_EN       = 20,  // TODO
  PIN_nCHGOK          = 2,   // TODO

  // Power
  PIN_PWR_EN          = 15,
  PIN_VDDs_EN         = 7,
  PIN_V_BAT_SENSE     = 5,
  PIN_V_EXT_SENSE     = 6,

  // IR drop sensor
  PIN_CLIFF_SENSE     = 26,
  PIN_IR_DROP         = 21,
  PIN_IR_FORWARD      = 22,   // TODO

  // Spine
  PIN_TX_HEAD         = 12,
  PIN_TX_VEXT         = 6,    // This is the debug port on the battery charger
};

enum AnalogInput {
  ANALOG_CLIFF_SENSE = ADC_CONFIG_PSEL_AnalogInput0,  // P26
  ANALOG_V_EXT_SENSE = ADC_CONFIG_PSEL_AnalogInput7,  // P6
  ANALOG_V_BAT_SENSE = ADC_CONFIG_PSEL_AnalogInput6   // P5
};


enum e_ppi_channel {
  PPI_MOTOR_CHANNEL_0,
  PPI_MOTOR_CHANNEL_1,
  PPI_MOTOR_CHANNEL_2,
  PPI_MOTOR_CHANNEL_3,
  PPI_MOTOR_CHANNEL_4,
  PPI_MOTOR_CHANNEL_5,
  PPI_MOTOR_CHANNEL_6,
  PPI_MOTOR_CHANNEL_7
};


#endif
