#include <stdint.h>

#include "datasheet.h"
#include "board.h"
#include "lights.h"
#include "accel.h"
#include "uart.h"
#include "animation.h"

extern void (*ble_send)(uint8_t length, const void* data);

int main(void) {
  static const uint8_t BOOT_MESSAGE[] = "hello!";
  ble_send(sizeof(BOOT_MESSAGE), BOOT_MESSAGE);

  hal_uart_init();
  hal_led_init();
  hal_acc_init();

  animation_init(); // Setup animation controller
}

void deinit(void) {
  hal_acc_stop();
  hal_led_stop();
  hal_uart_stop();
}

void tick(void) {
  animation_tick();
}

void recv(uint8_t length, const void* data) {
  if (length < 1) return ;

  animation_write(length, data);
}
