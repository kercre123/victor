#include <stdint.h>

#include "datasheet.h"
#include "board.h"
#include "lights.h"
#include "animation.h"

extern void (*ble_send)(uint8_t length, const void* data);

int main(void) {
  static const uint8_t BOOT_MESSAGE[] = "hello!";
  ble_send(sizeof(BOOT_MESSAGE), BOOT_MESSAGE);

  board_init();
  hal_led_init();
  animation_init();
}

void deinit(void) {
  hal_led_stop();
}

void tick(void) {
  animation_tick();
}

void recv(uint8_t length, const void* data) {
  if (length < 1) return ;
  
  animation_write(length, data);
}
