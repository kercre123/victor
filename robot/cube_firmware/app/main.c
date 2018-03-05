#include <stdint.h>

#include "datasheet.h"
#include "protocol.h"
#include "board.h"
#include "lights.h"
#include "accel.h"
#include "uart.h"
#include "animation.h"

int main(void) {
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
  hal_acc_tick();
}

void recv(uint8_t length, const void* data) {
  // Must have received at least the header
  if (length < 2) return ;

  // Zero payload
  __align(2) BaseCommand payload;
  memset(&payload, 0, sizeof(payload));
  memcpy(&payload, data, length);

  switch (payload.command) {
    case COMMAND_LIGHT_KEYFRAMES:
      animation_frames((const FrameCommand*) &payload);
      break ;
    case COMMAND_LIGHT_INDEX:
      animation_index((const MapCommand*) &payload);
      break ;
  }
}
