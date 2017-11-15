#include <stdint.h>

#include "datasheet.h"
#include "board.h"
#include "lights.h"

extern void (*ble_send)(uint8_t length, const void* data);

int main(void) {
  static const uint8_t BOOT_MESSAGE[] = "hello!";
  ble_send(sizeof(BOOT_MESSAGE), BOOT_MESSAGE);

  board_init(); //cfg pins
  hal_led_init();
}

void deinit(void) {
  hal_led_stop();
}

void tick(void) {
}

void recv(uint8_t length, const void* data) {
  ble_send(length, data);
}
