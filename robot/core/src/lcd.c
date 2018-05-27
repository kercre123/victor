#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "core/common.h"
#include "core/clock.h"
#include "core/gpio.h"

#include "core/lcd.h"

static int fd;

static int lcd_hw_init()
{
  int _fd = open("/dev/fb0", O_RDWR | O_NONBLOCK);
  if (!_fd) {
    error_exit(app_DEVICE_OPEN_ERROR, "Can't open LCD HW interface\n");
  }
  return _fd;
}

void lcd_clear_screen(void) {
  const LcdFrame frame={{0}};
  lcd_draw_frame(&frame);
}

void lcd_draw_frame(const LcdFrame* frame) {
   lseek(fd, 0, SEEK_SET);
   write(fd, frame->data, sizeof(frame->data));
}

void lcd_draw_frame2(const uint16_t* frame, size_t size) {
   lseek(fd, 0, SEEK_SET);
   write(fd, frame, size);
}

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

static const char* BACKLIGHT_DEVICES[] = {
  "/sys/class/leds/face-backlight-left/brightness",
  "/sys/class/leds/face-backlight-right/brightness"
};

static void _led_set_brightness(const int brightness, const char* led)
{
  int _fd = open(led,O_WRONLY);
  if (_fd) {
    char buf[3];
    snprintf(buf,3,"%02d\n",brightness);
    (void)write(_fd, buf, 3);
    close(_fd);
  }
}

void lcd_set_brightness(int brightness)
{
  int l;
  brightness = MIN(brightness, 20);
  brightness = MAX(brightness, 0);
  for (l=0; l<2; ++l) {
    _led_set_brightness(brightness, BACKLIGHT_DEVICES[l]);
  }
}

int lcd_init(void) {

  lcd_set_brightness(10);

  fd = lcd_hw_init();

  return 0;
}


void lcd_shutdown(void) {
  //todo: turn off screen?

  if (fd) {
    close(fd);
  }
}
