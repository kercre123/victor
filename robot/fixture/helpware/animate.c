// Displays test animations on face.
// Generate frames from animated gif with: python3 gif_to_raw.py <gif_filename>
// adb push animate and <gif_filename>.raw to robot temp dir.
//
// Usage:  ./animate frames.raw.
//
#include <stdint.h>
#include <stdio.h>

#include "core/common.h"
#include "core/lcd.h"



/*************** TEST CODE *********************/

static void animate(const char* fn) {
  LcdFrame frame;
  FILE *fo = fopen(fn, "rb");

  while (!feof(fo)) {
    fread(frame.data, 1, sizeof(frame.data), fo);
    lcd_draw_frame(&frame);
  }
  fclose(fo);
}


#define ENDLESS 1


void on_vic_exit(void) {
   lcd_shutdown();
}


int main(int argc, char** argv) {

  int rc = lcd_init();
  if (rc!=0)
  {
    return rc;
  }

  // Start drawing stuff to the screen
  if (argc>1) {
     do {
        animate(argv[1]);
     }  while (ENDLESS);
  }
  else
  {
     error_exit(app_USAGE, "Usage: %s <filename>\n", argv[0]);
  }

  on_vic_exit();
  return 0;
}
