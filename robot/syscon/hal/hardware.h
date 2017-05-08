#ifndef HARDWARE_H
#define HARDWARE_H

#define NRF_BAUD(x) (int)(x * 4194304.0f / 15625.0f) // 2^28/1MHz

static uint32_t* const FIXTURE_HOOK = (uint32_t*)0x20003FFC;
static const int MAX_MODELS = 4;

#define BODY_VER       (*((s32*) 0x1F010))  // Between -1 (unprogrammed) and BODY_VERS
#define BODY_ESN       (*((u32*) 0x1F014))
#define BODY_COLOR(i)  (((s32*) 0x1F018)[i])


// NOTE: Make sure changes to this enum are reflected on the Unity side (SettingsVersionsPanel.cs)
//       The actual value itself is written to robot by factory fixture and the values are
//       "artisinally" hard-coded into the sys_boot assembly file.
//       We should try to keep this enum in sync with whatever the fixtures write.
enum BODY_VERS {
  BODY_VER_EP    = 0,
  BODY_VER_PILOT = 1,   // 8T head encoder
  BODY_VER_PROD  = 2,    // Final hardware version, 4T head encoder
  BODY_VER_SHIP  = 3,    // First 1000 v1.0 shipping units
  BODY_VER_1v0   = 4,    // Full production v1.0
  BODY_VER_1v5   = 5,    // v1.5 EP2
  BODY_VER_1v5c  = 6,    // v1.5 with glass-filled nylon lift gear and reduced ratio (149.7:1)
};

enum IRQ_Priority {
  ENCODER_PRIORITY = 0,
  RADIO_TIMER_PRIORITY = 1,
  LIGHT_PRIORITY = 1,
  UART_PRIORITY = 1,
  RADIO_SERVICE_PRIORITY = 2,
  TIMER_PRIORITY = 3
};

enum e_nrf_gpio {
  // Encoders
  PIN_ENCODER_LEFT    = 13, // ENC1
  PIN_ENCODER_RIGHT   = 4,  // ENC2
  PIN_ENCODER_HEADA   = 14, // ENC3 - wire harness reversed since EP3
  PIN_ENCODER_HEADB   = 17,
  PIN_ENCODER_LIFTA   = 0,  // ENC4 - wire harness reversed since EP3
  PIN_ENCODER_LIFTB   = 1,

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

  // Backpack button
  PIN_BUTTON_SENSE    = 8,
  PIN_BUTTON_DRIVE    = 10,

  // Charging
  PIN_CHARGE_EN       = 20,
  PIN_nCHGOK          = 2,

  // Power
  PIN_PWR_EN          = 15,
  PIN_VDDs_EN         = 7,
  PIN_V_BAT_SENSE     = 5,
  PIN_V_EXT_SENSE     = 6,

  // IR drop sensor
  PIN_CLIFF_SENSE     = 26,
  PIN_IR_DROP         = 21,
  PIN_IR_FORWARD      = 22,

  // Spine
  PIN_TX_HEAD         = 12,
  PIN_TX_VEXT         = 6,    // This is the debug port on the battery charger
};

#ifndef FIXTURE
enum AnalogInput {
  ANALOG_CLIFF_SENSE = ADC_CONFIG_PSEL_AnalogInput0,  // P26
  ANALOG_V_EXT_SENSE = ADC_CONFIG_PSEL_AnalogInput7,  // P6
  ANALOG_V_BAT_SENSE = ADC_CONFIG_PSEL_AnalogInput6   // P5
};
#endif

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
