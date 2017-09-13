/**
 * File: faceDisplay_android.cpp
 *
 * Author: Kevin Yoon
 * Created: 07/20/2017
 *
 * Description:
 *               Defines interface to simulated face display
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "util/logging/logging.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>

typedef struct {
  uint8_t cmd;
  uint8_t data_bytes;
  uint8_t data[5];
} INIT_SCRIPT;

static const int DAT_CLOCK = 10000000;
static const int MAX_TRANSFER = 0x1000;


#define RSHIFT 0x1C

static const INIT_SCRIPT init_scr[] = {
  { 0x11, 0 },
  { 0x36, 1, { 0x00 } },
  { 0xB7, 1, { 0x56 } },
  { 0xBB, 1, { 0x18 } },
  { 0xC0, 1, { 0x2C } },
  { 0xC2, 1, { 0x01 } },
  { 0xC3, 1, { 0x1F } },
  { 0xC4, 1, { 0x20 } },
  { 0xC6, 1, { 0x0F } },
  { 0xD0, 2, { 0xA4, 0xA1 } },
  { 0x3A, 1, { 0x55 } },
  { 0x55, 1, { 0xA0 } },
  { 0x21, 0 },
  { 0x2A, 4, { 0x00, RSHIFT, (Anki::Cozmo::FaceDisplay::FACE_DISPLAY_WIDTH + RSHIFT - 1) >> 8, (Anki::Cozmo::FaceDisplay::FACE_DISPLAY_WIDTH + RSHIFT - 1) & 0xFF } },
  { 0x2B, 4, { 0x00, 0x00, (Anki::Cozmo::FaceDisplay::FACE_DISPLAY_HEIGHT -1) >> 8, (Anki::Cozmo::FaceDisplay::FACE_DISPLAY_HEIGHT -1) & 0xFF } },
  { 0x29, 0 },
  
  { 0 }
};


#define LOW 0
#define HIGH 1
#define FALSE 0
#define TRUE (!FALSE)
#define DIR_OUTPUT 1
#define DIR_INPUT 0

#define GPIO_LCD_WRX   110
#define GPIO_LCD_RESET 96


typedef struct GPIO_t {
  int pin;
  int fd;
} GPIO;


GPIO RESET_PIN;
GPIO DnC_PIN;
GPIO CS_PIN;

#define CS_PIN_EARLY 0

/************* GPIO Interface ***************/

void gpio_set_direction(GPIO gp, int isOutput)
{
  char ioname[40];
  snprintf(ioname, 40, "/sys/class/gpio/gpio%d/direction", gp.pin+911);
  int fd =  open(ioname, O_WRONLY );
  if (isOutput) {
    write(fd, "out", 3);
  }
  else {
    write(fd, "in", 2);
  }
  close(fd);
}

void gpio_set_value(GPIO gp, int value) {
  static const char* trigger[] = {"0","1"};
  write(gp.fd, trigger[value!=0], 1);
}


GPIO gpio_create(int gpio_number, int isOutput, int initial_value) {
  char ioname[32];
  GPIO gp = {gpio_number, 0};
  
  //create io
  int fd = open("/sys/class/gpio/export", O_WRONLY);
  snprintf(ioname, 32, "%d", gpio_number+911);
  write(fd, ioname, strlen(ioname));
  close(fd);
  
  //set direction
  gpio_set_direction(gp, isOutput);
  
  //open value fd
  snprintf(ioname, 32, "/sys/class/gpio/gpio%d/value", gpio_number+911);
  gp.fd = open(ioname, O_WRONLY );
  
  if (gp.fd > 0) {
    gpio_set_value(gp, initial_value);
  }
  return gp;
}


void gpio_close(GPIO gp) {
  if (gp.fd > 0) {
    close(gp.fd);
  }
}


/************* CLOCK Interface ***************/

uint64_t steady_clock_now(void) {
  struct timespec time;
  clock_gettime(CLOCK_REALTIME,&time);
  return time.tv_nsec;
}
static void MicroWait(long microsec)
{
  uint64_t start = steady_clock_now();
  while ((steady_clock_now() - start) < (microsec * 1000000)) {
    ;
  }
}

/************* SPI Interface ***************/


static int spi_fd;

void spi(int cmd, uint32_t bytes, const void* data) {
  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)data,
    .rx_buf = 0,
    .len = bytes,
    .delay_usecs = 0,
    .speed_hz = DAT_CLOCK,
    .bits_per_word = 8,
  };
  
  while (bytes > 0) {
    tr.len = (bytes > MAX_TRANSFER) ? MAX_TRANSFER : bytes;
    
    gpio_set_value(DnC_PIN, cmd ? LOW : HIGH);
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    
    bytes -= tr.len;
    tr.tx_buf += tr.len;
  }
}


/************ LCD Device Interface *************/

static void lcd_device_init() {
  int idx;
  
  for (idx = 0; init_scr[idx].cmd; idx++) {
    spi(TRUE, 1, &init_scr[idx].cmd);
    spi(FALSE, init_scr[idx].data_bytes, init_scr[idx].data);
  }
}

static void lcd_draw_frame(uint8_t* frame, int sz) {
  static const uint8_t WRITE_RAM = 0x2C;
  spi(TRUE, 1, &WRITE_RAM);
  spi(FALSE, sz, frame);
}

int lcd_init(void) {
  static const uint8_t    MODE = 0;
  
  // Echo to device to activate backlight
  system("echo 10 > /sys/class/leds/face-backlight/brightness");
  system("echo 1 > /sys/kernel/debug/regulator/8916_l17/enable");
  
  
  // IO Setup
  DnC_PIN = gpio_create(GPIO_LCD_WRX, 1, 1);
  RESET_PIN = gpio_create(GPIO_LCD_RESET, 1, 1);
  
  gpio_set_direction(DnC_PIN, DIR_OUTPUT);
  gpio_set_direction(RESET_PIN, DIR_OUTPUT);
  
  // SPI setup
  
  spi_fd = open("/dev/spidev1.0", O_RDWR);
  ioctl(spi_fd, SPI_IOC_RD_MODE, &MODE);
  if (!spi_fd) {
    printf("SPI OPEN ERROR!\n");
    return -1;
  }
  
  // Send reset signal
  MicroWait(50);
  gpio_set_value(RESET_PIN, 0);
  MicroWait(50);
  gpio_set_value(RESET_PIN, 1);
  MicroWait(50);
  
  lcd_device_init();
  
  return 0;
}


void lcd_shutdown(void) {
  //todo: turn off screen?
  static const uint8_t SLEEP = 0x10;
  spi(TRUE, 1, &SLEEP);
  close(spi_fd);
}




namespace Anki {
namespace Cozmo {
  
  // Definition of static field
  FaceDisplay* FaceDisplay::_instance = 0;
  
  /**
   * Returns the single instance of the object.
   */
  FaceDisplay* FaceDisplay::getInstance() {
    // check if the instance has been created yet
    if(0 == _instance) {
      // if not, then create it
      _instance = new FaceDisplay;
    }
    // return the single instance
    return _instance;
  }
  
  /**
   * Removes instance
   */
  void FaceDisplay::removeInstance() {
    // check if the instance has been created yet
    if(0 != _instance) {
      delete _instance;
      _instance = 0;
    }
  };
  
  
  FaceDisplay::FaceDisplay()
  {
    lcd_init();
  }

  FaceDisplay::~FaceDisplay()
  {
    lcd_shutdown();
  }
  
  void FaceDisplay::FaceClear()
  {
    u16 frame[FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT*sizeof(u16)];
    FaceDraw(frame);
  }
  
  void FaceDisplay::FaceDraw(u16* frame)
  {
    lcd_draw_frame((u8*)frame, FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT*sizeof(u16));
  }
  
  void FaceDisplay::FacePrintf(const char* format, ...)
  {
    // Stub
  }

} // namespace Cozmo
} // namespace Anki
