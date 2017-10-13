#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "core/common.h"
#include "core/clock.h"
#include "core/gpio.h"

#include "core/lcd.h"

#define FALSE 0
#define TRUE (!FALSE)


static const int DAT_CLOCK = 10000000;
static const int MAX_TRANSFER = 0x1000;


#define GPIO_LCD_WRX   110
#define GPIO_LCD_RESET 96
static GPIO RESET_PIN;
static GPIO DnC_PIN;


#define RSHIFT 0x1C

typedef struct {
	uint8_t cmd;
	uint8_t data_bytes;
	uint8_t data[5];
} INIT_SCRIPT;


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
	{ 0x2A, 4, { 0x00, RSHIFT, (LCD_FRAME_WIDTH + RSHIFT - 1) >> 8, (LCD_FRAME_WIDTH + RSHIFT - 1) & 0xFF } },
	{ 0x2B, 4, { 0x00, 0x00, (LCD_FRAME_HEIGHT -1) >> 8, (LCD_FRAME_HEIGHT -1) & 0xFF } },
	{ 0x29, 0 },

	{ 0 }
};



/************* LCD SPI Interface ***************/


static int spi_fd;

static int lcd_spi_init()
{
  // SPI setup
  static const uint8_t    MODE = 0;
	int spi_fd = open("/dev/spidev1.0", O_RDWR);
  if (!spi_fd) {
    error_exit(app_DEVICE_OPEN_ERROR, "Can't open LCD SPI interface\n");
  }
	ioctl(spi_fd, SPI_IOC_RD_MODE, &MODE);
  /* if (err<0) { */
  /*   error_exit(app_IO_ERROR, "Can't configure LCD SPI. (%d)\n", errno); */
  /* } */
  return spi_fd;
}

static void lcd_spi_transfer(int cmd, int bytes, const void* data) {
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

		gpio_set_value(DnC_PIN, cmd ? gpio_LOW : gpio_HIGH);
		ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

		bytes -= tr.len;
		tr.tx_buf += tr.len;
	}
}

/************ LCD Device Interface *************/

static void lcd_device_init() {
	int idx;

	for (idx = 0; init_scr[idx].cmd; idx++) {
		lcd_spi_transfer(TRUE, 1, &init_scr[idx].cmd);
		lcd_spi_transfer(FALSE, init_scr[idx].data_bytes, init_scr[idx].data);
	}
}

void lcd_draw_frame(LcdFrame* frame) {
   static const uint8_t WRITE_RAM = 0x2C;
   lcd_spi_transfer(TRUE, 1, &WRITE_RAM);
   lcd_spi_transfer(FALSE, sizeof(frame->data), frame->data);
}

int lcd_init(void) {

//TODO : BACKLIGHT and REGULATOR INTERFACE
  // Echo to device to activate backlight
  system("echo 10 > /sys/class/leds/face-backlight/brightness");
  system("echo 1 > /sys/kernel/debug/regulator/8916_l17/enable");


  // IO Setup
  DnC_PIN = gpio_create(GPIO_LCD_WRX, gpio_DIR_OUTPUT, gpio_HIGH);
  RESET_PIN = gpio_create(GPIO_LCD_RESET, gpio_DIR_OUTPUT, gpio_HIGH);

  gpio_set_direction(DnC_PIN, gpio_DIR_OUTPUT);  //TODO: test if needed
  gpio_set_direction(RESET_PIN, gpio_DIR_OUTPUT);

  // SPI setup

	spi_fd = lcd_spi_init();

	// Send reset signal
	microwait(50);
	gpio_set_value(RESET_PIN, 0);
	microwait(50);
	gpio_set_value(RESET_PIN, 1);
	microwait(50);

  lcd_device_init();

  return 0;
}


void lcd_shutdown(void) {
  //todo: turn off screen?
  if (spi_fd) {
    static const uint8_t SLEEP = 0x10;
    lcd_spi_transfer(TRUE, 1, &SLEEP);
    close(spi_fd);
  }
  if (DnC_PIN) {
    gpio_close(DnC_PIN);
  }
  if (RESET_PIN) {
    gpio_close(RESET_PIN);
  }

}
