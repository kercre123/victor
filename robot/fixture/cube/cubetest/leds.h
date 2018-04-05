#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

//led bits
#define LED_D1_RED    0x0001
#define LED_D1_GRN    0x0002
#define LED_D1_BLU    0x0004
#define LED_D2_RED    0x0008
#define LED_D2_GRN    0x0010
#define LED_D2_BLU    0x0020
#define LED_D3_RED    0x0040
#define LED_D3_GRN    0x0080
#define LED_D3_BLU    0x0100
#define LED_D4_RED    0x0200
#define LED_D4_GRN    0x0400
#define LED_D4_BLU    0x0800

#define LED_D1_WHITE  (LED_D1_RED | LED_D1_GRN | LED_D1_BLU)
#define LED_D2_WHITE  (LED_D2_RED | LED_D2_GRN | LED_D2_BLU)
#define LED_D3_WHITE  (LED_D3_RED | LED_D3_GRN | LED_D3_BLU)
#define LED_D4_WHITE  (LED_D4_RED | LED_D4_GRN | LED_D4_BLU)
#define LED_BF_ALL    (LED_D1_WHITE | LED_D2_WHITE | LED_D3_WHITE | LED_D4_WHITE)

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void leds_init(void);
void leds_set(uint16_t led_bf); //sets all led states led_bf<n> 1=on 0=off
void leds_on(uint16_t led_bf);  //turn on led<n>=1
void leds_off(uint16_t led_bf); //turn off led<n>=1
void leds_manage(void);

#endif //LEDS_H_

