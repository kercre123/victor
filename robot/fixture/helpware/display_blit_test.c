/* #include <stdint.h> */
/* #include <stdio.h> */
/* #include <stdbool.h> */
#include <stdlib.h>
/* #include <assert.h> */
/* #include <string.h> */
#include <ctype.h>
/* #include <fcntl.h> */
#include <unistd.h>
/* #include <limits.h> */

#include "core/common.h"
#include "core/lcd.h"

#include "helpware/display.h"

#ifdef EXTENDED_DISPLAY_DEBUGGING
#define ddprintf printf
#else
#define ddprintf(f,...)
#endif



void on_exit(void) {
  lcd_shutdown();
}

#define FRAME_WORDS (LCD_FRAME_WIDTH*LCD_FRAME_HEIGHT)

int main(int argc, const char* argv[])
{
  size_t i;

  ddprintf("Initializing\n");

  lcd_init();

  display_init();

  LcdFrame frame;

  if (argc > 1)
  {
    int brightness = atoi(argv[1]);
    lcd_set_brightness(brightness);
  }


  while (true) {
    for (i=0; i<FRAME_WORDS; ++i) frame.data[i] = lcd_GREEN;
    lcd_draw_frame(&frame);
    for (i=0; i<FRAME_WORDS; ++i) frame.data[i] = lcd_RED;
    lcd_draw_frame(&frame);
    for (i=0; i<FRAME_WORDS; ++i) frame.data[i] = lcd_BLUE;
    lcd_draw_frame(&frame);
  }

  return 0;
}
