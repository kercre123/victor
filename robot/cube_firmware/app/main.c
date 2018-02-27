#include <stdint.h>

#include "datasheet.h"
#include "protocol.h"
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
  // Must have received at least the header
  if (length < 2) return ;

  // Zero payload
  Payload payload;
  memset(&payload, 0, sizeof(Payload));
  memcpy(&payload, data, length);

  switch (payload.command) {
    case COMMAND_LIGHT_KEYFRAMES:
      animation_frames(payload.flags, payload.frames);
      break ;
    case COMMAND_LIGHT_INDEX:
      animation_index(&payload.framemap);
      break ;
  }
}
