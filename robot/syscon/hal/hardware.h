#ifndef HARDWARE_H
#define HARDWARE_H

#define NRF_BAUD(x) (int)(x * 268.435456) // 2^28/1MHz
#define UART_BAUDRATE NRF_BAUD(350000)

//#define NATHAN_WANTS_DEMO
//#define BACKPACK_DEMO
//#define DEBUG_MESSAGES

enum e_nrf_gpio {
  PIN_LIFT_P        = 0,
  PIN_TX_DEBUG      = 1,
  PIN_TX_HEAD       = 2,
  PIN_VDD_EN        = 3, // 3.0
  PIN_ENCODER_LIFTA = 4, // M4/Lift on schematic
  PIN_VUSBs_EN      = 5, // equivalent to old CHARGE_EN 
  PIN_I_SENSE       = 6,
  PIN_HEAD_P        = 7, 
  PIN_VDDs_EN       = 8,
  PIN_LED3          = 9,
  PIN_ENCODER_HEADB = 10,
  PIN_ENCODER_HEADA = 11, // M3/Head on schematic
  PIN_VBATs_EN      = 12,
  PIN_HEAD_N1       = 13, // M3/Head on schematic
  PIN_HEAD_N2       = 14,
  PIN_LEFT_N2       = 15,
  PIN_LEFT_N1       = 16, // M1/Left on schematic
  PIN_LEFT_P        = 17,
  PIN_LED1          = 18,
  PIN_LED2          = 19,
  PIN_ENCODER_LEFT  = 20, // M1/Left on schematic
  PIN_RIGHT_N2      = 21,
  PIN_RIGHT_N1      = 22, // M2/Right on schematic
  PIN_RIGHT_P       = 23,
  PIN_ENCODER_RIGHT = 24, // M2/Right on schematic
  PIN_LIFT_N2       = 25,
  PIN_V_BAT_SENSE   = 26,
  PIN_V_USB_SENSE   = 27,
  PIN_ENCODER_LIFTB = 28,
  PIN_LED4          = 29,
  PIN_LIFT_N1       = 30  // M4/Lift on schematic
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
