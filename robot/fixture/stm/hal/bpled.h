#ifndef BPLED_H
#define BPLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//backpack LEDs
namespace BPLED
{  
  const uint8_t num = 12; //number of leds
  
  void enable(); //init pins etc.
  void disable(); //release pins etc
  
  //----------------------------------
  //  LED control
  //----------------------------------
  
  void on(uint8_t n); //turn on led<n>, n={0..num-1}. Note, only 1 LED can be on at a time
  void off(); //clear all leds
  
  //display a single frame. lights each enabled led for a short time
  //@param frame<n> = led<n> {0=off,1=on} ...see BF_Dx_XXX defines
  //@param display_time_us - time to light each enabled led (total frame time ~= display_time_us * num)
  void putFrame(uint32_t frame, uint16_t display_time_us = 150);
  
  //----------------------------------
  //  Debug
  //----------------------------------
  
  enum color_e {
    COLOR_INVALID = 0,
    COLOR_RED,
    COLOR_GRN,
    COLOR_BLU,
  };
  
  int getDx(uint8_t n); //return D{1..4} to indicate which physical component led<n> is part of
  color_e getColor(uint8_t n);
  char *description(uint8_t n); //return descriptive string, e.g. "D2.RED"
  
  //----------------------------------
  //  Frame bit defines
  //----------------------------------
  
  #define BF_D1_RED     (1 << 0)
  #define BF_D1_GRN     (1 << 1)
  #define BF_D1_BLU     (1 << 2)
  #define BF_D1_MAGENTA (BF_D1_BLU | BF_D1_RED)
  #define BF_D1_CYAN    (BF_D1_BLU | BF_D1_GRN)
  #define BF_D1_YELLOW  (BF_D1_RED | BF_D1_GRN)
  #define BF_D1_WHITE   (BF_D1_BLU | BF_D1_GRN | BF_D1_RED)
  #define BF_D2_RED     (1 << 3)
  #define BF_D2_GRN     (1 << 4)
  #define BF_D2_BLU     (1 << 5)
  #define BF_D2_MAGENTA (BF_D2_BLU | BF_D2_RED)
  #define BF_D2_CYAN    (BF_D2_BLU | BF_D2_GRN)
  #define BF_D2_YELLOW  (BF_D2_RED | BF_D2_GRN)
  #define BF_D2_WHITE   (BF_D2_BLU | BF_D2_GRN | BF_D2_RED)
  #define BF_D3_RED     (1 << 6)
  #define BF_D3_GRN     (1 << 7)
  #define BF_D3_BLU     (1 << 8)
  #define BF_D3_MAGENTA (BF_D3_BLU | BF_D3_RED)
  #define BF_D3_CYAN    (BF_D3_BLU | BF_D3_GRN)
  #define BF_D3_YELLOW  (BF_D3_RED | BF_D3_GRN)
  #define BF_D3_WHITE   (BF_D3_BLU | BF_D3_GRN | BF_D3_RED)
  #define BF_D4_RED     (1 << 9)
  #define BF_D4_GRN     (1 << 10)
  #define BF_D4_BLU     (1 << 11)
  #define BF_D4_MAGENTA (BF_D4_BLU | BF_D4_RED)
  #define BF_D4_CYAN    (BF_D4_BLU | BF_D4_GRN)
  #define BF_D4_YELLOW  (BF_D4_RED | BF_D4_GRN)
  #define BF_D4_WHITE   (BF_D4_BLU | BF_D4_GRN | BF_D4_RED)

}

#ifdef __cplusplus
}
#endif

#endif //BPLED_H
