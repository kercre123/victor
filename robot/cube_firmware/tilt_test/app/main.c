#include <stdint.h>

#include "datasheet.h"
#include "protocol.h"
#include "board.h"
#include "lights.h"
#include "accel.h"

int main(void) {
  hal_led_init();
  hal_acc_init();
}

void deinit(void) {
  hal_acc_stop();
  hal_led_stop();
}

void tick(void) {
  hal_acc_tick();
}

void recv(uint8_t length, const void* data) {
  // Do nothing, this is a hands off test
}
