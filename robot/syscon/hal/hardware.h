#ifndef HARDWARE_H
#define HARDWARE_H

#define NRF_BAUD(x) (int)(x * 268.435456) // 2^28/1MHz
#define UART_BAUDRATE NRF_BAUD(350000)
#define CYCLES_MS(ms) (int)(32768 * 256.0f * ms / 1000.0f)

//#define NATHAN_WANTS_DEMO
//#define BACKPACK_DEMO
//#define DEBUG_MESSAGES

enum e_nrf_gpio {
  // Encoders
  PIN_ENCODER_LEFT    = 14, // ENC1
  PIN_ENCODER_RIGHT   = 4,  // ENC2
  PIN_ENCODER_HEADA   = 27, // ENC3
  PIN_ENCODER_HEADB   = 24,
  PIN_ENCODER_LIFTA   = 30, // ENC4
  PIN_ENCODER_LIFTB   = 29,

  // Motors + charge OK signal
  PIN_LEFT_P          = 19,
  PIN_LEFT_N1         = 18,
  PIN_LEFT_N2         = 16,
  PIN_RIGHT_P         = 8,
  PIN_RIGHT_N1        = 1,
  PIN_RIGHT_N2        = 0,
  PIN_HEAD_P          = 9, 
  PIN_HEAD_N1         = 7,
  PIN_HEAD_N2         = 13,
  PIN_LIFT_P          = 28,
  PIN_LIFT_N1         = 25,
  PIN_LIFT_N2         = 23,

  // Backpack leds
  PIN_LED1            = 17,
  PIN_LED2            = 12,
  PIN_LED3            = 11,
  PIN_LED4            = 10,

  // Charging
  PIN_CHARGE_EN       = 20,  // TODO
  PIN_nCHGOK          = 8,   // TODO

  // Power
  PIN_PWR_EN          = 2,
  PIN_nVDDs_EN        = 3,
  PIN_V_BAT_SENSE     = 6,
  PIN_V_EXT_SENSE     = 5,

  // IR drop sensor
  PIN_IR_SENSE        = 26,   // TODO
  PIN_IR_DROP         = 21,   // TODO
  PIN_IR_FORWARD      = 22,   // TODO

  // Spine
  PIN_TX_HEAD         = 15,
  PIN_TX_VEXT         = 5,    // This is the debug port on the battery charger
};

enum AnalogInput {
  ANALOG_V_EXT_SENSE = ADC_CONFIG_PSEL_AnalogInput6,
  ANALOG_V_BAT_SENSE = ADC_CONFIG_PSEL_AnalogInput7
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
