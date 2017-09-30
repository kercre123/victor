// Compile with: ../../generated/android/tools/arm/bin/arm-linux-androideabi-gcc -pie lcd_test.c
// Generate frames from animated gif with: python3 gif_to_raw.py <gif_filename>
// adb push a.out and <gif_filename>.raw to robot temp dir.
//
// Usage:  ./a.out frames.raw.
//
#include <stdint.h>
/* #include <unistd.h> */
#include <stdio.h>
/* #include <stdlib.h> */
/* #include <fcntl.h> */
/* #include <sys/ioctl.h> */
/* #include <linux/types.h> */
/* #include <linux/spi/spidev.h> */

#include "core/common.h"
#include "core/lcd.h"



/*************** TEST CODE *********************/

static void animate(const char* fn) {
	uint16_t frame[LCD_FRAME_WIDTH * LCD_FRAME_HEIGHT];
	FILE *fo = fopen(fn, "rb");

	while (!feof(fo)) {
		fread(frame, 1, sizeof(frame), fo);
    lcd_draw_frame((uint8_t*)frame, sizeof(frame));
	}
  fclose(fo);
}


#define ENDLESS 1




void on_exit(void) {
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

  on_exit();
	return 0;
}
