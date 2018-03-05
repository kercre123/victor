#ifndef BOARD_H_
#define BOARD_H_

//#include <core_cm0.h>
#include <stdint.h>
#include <stdio.h>
//#include "common.h"
//#include "hal_pwm.h"
//#include "hal_uart.h"
//#include "vbat.h"

//-----------------------------------------------------------
//        Board Config
//-----------------------------------------------------------
static const uint32_t SYSTEM_CLOCK = 16*1000000;

//-----------------------------------------------------------
//        pin defines
//-----------------------------------------------------------
#include "gpio.h"

#define GPIO_DEFINE(PORT,PIN,NAME) \
  static const GPIO_PORT  NAME##_PORT = (GPIO_PORT)(PORT); \
  static const GPIO_PIN   NAME##_PIN = (GPIO_PIN)(PIN);

GPIO_DEFINE(GPIO_PORT_0, 0, ACC_nCS);
//GPIO_DEFINE(GPIO_PORT_0, 1, xxx);
GPIO_DEFINE(GPIO_PORT_0, 2, ACC_PWR);
GPIO_DEFINE(GPIO_PORT_0, 3, ACC_SCK);
GPIO_DEFINE(GPIO_PORT_0, 4, UTX);
GPIO_DEFINE(GPIO_PORT_0, 5, URX);
GPIO_DEFINE(GPIO_PORT_0, 6, ACC_SDA);
GPIO_DEFINE(GPIO_PORT_0, 7, D1);

GPIO_DEFINE(GPIO_PORT_1, 0, D7);
GPIO_DEFINE(GPIO_PORT_1, 1, D10);
//GPIO_DEFINE(GPIO_PORT_1, 2, xxx); // DO NOT USE CORRUPTS 16M
//GPIO_DEFINE(GPIO_PORT_1, 3, xxx); // DO NOT USE CORRUPTS 16M

GPIO_DEFINE(GPIO_PORT_2, 0, BOOST_EN);
GPIO_DEFINE(GPIO_PORT_2, 1, D0);
GPIO_DEFINE(GPIO_PORT_2, 2, D2);
GPIO_DEFINE(GPIO_PORT_2, 3, D11);
GPIO_DEFINE(GPIO_PORT_2, 4, D9);
GPIO_DEFINE(GPIO_PORT_2, 5, D6);
GPIO_DEFINE(GPIO_PORT_2, 6, D8);
GPIO_DEFINE(GPIO_PORT_2, 7, D3);
GPIO_DEFINE(GPIO_PORT_2, 8, D4);
GPIO_DEFINE(GPIO_PORT_2, 9, D5);

//some shortcuts
#define GPIOPP(NAME) NAME##_PORT, NAME##_PIN /*expand pin name to 'port, pin' for GPIO_x() functions*/
#define GPIO_INIT_PIN(NAME, MODE_, FUNC_, PIN_STATE_, POWER_RAIL_ ) { \
  GPIO_ConfigurePin( GPIOPP(NAME), (MODE_), (FUNC_), (PIN_STATE_) ); \
  GPIO_ConfigurePinPower( GPIOPP(NAME), (POWER_RAIL_) ); \
}
#define GPIO_PIN_FUNC(NAME, MODE_, FUNC_) { \
  GPIO_SetPinFunction( GPIOPP(NAME), (MODE_), (FUNC_) ); \
}

#define GPIO_READ(NAME) GPIO_GetPinStatus( GPIOPP(NAME) )
#define GPIO_SET(NAME)  GPIO_SetActive( GPIOPP(NAME) )
#define GPIO_CLR(NAME)  GPIO_SetInactive( GPIOPP(NAME) )
#define GPIO_WRITE(NAME, PIN_STATE_) { \
  if( (PIN_STATE_) ) {  \
    GPIO_SET(NAME);     \
  } else {              \
    GPIO_CLR(NAME);     \
  } }

//-----------------------------------------------------------
//        [LEGACY] -- DEPRECIATED
//-----------------------------------------------------------
//empty


#endif //BOARD_H_
