#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>

#include "core/common.h"
#include "core/clock.h"
#include "core/gpio.h"

#include "core/lcd.h"

#define FALSE 0
#define TRUE (!FALSE)


static const int MAX_TRANSFER = 0x1000;

/*
Bad jetway wiring has the following assignment
GPIO 12 WRX
GPIO 13 CS
GPIO 14 MOSI
GPIO 15 CLK
*/
//#define GPIO_LCD_WRX   110
#define GPIO_LCD_WRX   13
#define GPIO_LCD_RESET1 96
#define GPIO_LCD_RESET2 55

static GPIO RESET_PIN1;
static GPIO RESET_PIN2;
static GPIO DnC_PIN;


#define RSHIFT 0x1C
#define XSHIFT 0x01
#define YSHIFT 0x1A

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
  uint8_t cmd;
  uint8_t data_bytes;
  uint8_t data[128];
  uint32_t delay_ms;
} INIT_SCRIPT;

static const INIT_SCRIPT old_init_scr[] = {
  { 0x10, 1, { 0x00 }, 120}, // Sleep in
  { 0x2A, 4, { 0x00, RSHIFT, (LCD_FRAME_WIDTH + RSHIFT - 1) >> 8, (LCD_FRAME_WIDTH + RSHIFT - 1) & 0xFF } }, // Column address set
  { 0x2B, 4, { 0x00, 0x00, (LCD_FRAME_HEIGHT -1) >> 8, (LCD_FRAME_HEIGHT -1) & 0xFF } }, // Row address set
  { 0x36, 1, { 0x00 }, 0 }, // Memory data access control
  { 0x3A, 1, { 0x55 }, 0 }, // Interface pixel format (16 bit/pixel 65k RGB data)
  { 0xB0, 2, { 0x00, 0x08 } }, // RAM control (LSB first)
  { 0xB2, 5, { 0x0C, 0x0C, 0x00, 0x33, 0x33 }, 0 }, // Porch setting
  { 0xB7, 1, { 0x72 }, 0 }, // Gate control (VGH 14.97v, VGL -8.23v)
  { 0xBB, 1, { 0x3B }, 0 }, // VCOMS setting (1.575v)
  { 0xC0, 1, { 0x2C }, 0 }, // LCM control
  { 0xC2, 1, { 0x01 }, 0 }, // VDV and VRH command enable
  { 0xC3, 1, { 0x14 }, 0 }, // VRH set
  { 0xC4, 1, { 0x20 }, 0 }, // VDV set
  { 0xC6, 1, { 0x0F }, 0 }, // Frame rate control in normal mode (60hz)
  { 0xD0, 2, { 0xA4, 0xA1 }, 0 }, // Power control 1
  { 0xE0, 14, { 0xD0, 0x10, 0x16, 0x0A, 0x0A, 0x26, 0x3C, 0x53, 0x53, 0x18, 0x15, 0x12, 0x36, 0x3C }, 0 }, // Positive voltage gamma control
  { 0xE1, 14, { 0xD0, 0x11, 0x19, 0x0A, 0x09, 0x25, 0x3D, 0x35, 0x54, 0x17, 0x15, 0x12, 0x36, 0x3C }, 0 }, // Negative voltage gamma control
  { 0xE9, 3, { 0x05, 0x05, 0x01 }, 0 }, // Equalize time control
  { 0x21, 1, { 0x00 }, 0 }, // Display inversion on
  { 0 }
};
static const INIT_SCRIPT init_scr[] = 
{
        { 0x01, 0, { 0x00}, 150},
        { 0x11, 0, { 0x00}, 500},
        { 0xB1, 3, { 0x01, 0x2c, 0x2d}, 0},
        { 0xB2, 3, { 0x01, 0x2c, 0x2d}, 0},
        { 0xB3, 6, { 0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d}, 0},
        { 0xB4, 1, { 0x07}, 0},
        { 0xC0, 3, { 0xa2, 0x02, 0x84}, 0},
        { 0xC1, 1, { 0xc5}, 0},
        { 0xC2, 2, { 0x0a, 0x00}, 0},
        { 0xC3, 2, { 0x8a, 0x2a}, 0},
        { 0xC4, 2, { 0x8a, 0xee}, 0},
        { 0xC5, 1, { 0x0e}, 0},
        { 0x20, 0, { 0x00}, 0},
        { 0x36, 1, { 0xE0}, 0}, //exchange rotate and reverse order in each of 2 dim
        { 0x3A, 1, { 0x05}, 0}, // Interface pixel format (16 bit/pixel 65k RGB data)
        { 0xE0, 16, { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10}, 0}, 
        { 0xE1, 16, { 0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d, 0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10}, 0}, 

        { 0x13, 0, { 0x00}, 100},
        { 0x21, 1, { 0x00 }, 0 }, // Display inversion on
        { 0x29, 0, { 0x00}, 10},

	{ 0x2A, 4, { XSHIFT >> 8, XSHIFT & 0xFF, (LCD_FRAME_WIDTH + XSHIFT -1) >> 8, (LCD_FRAME_WIDTH + XSHIFT -1) & 0xFF } }, // Column address set
	{ 0x2B, 4, { YSHIFT >> 8, YSHIFT & 0xFF, (LCD_FRAME_HEIGHT + YSHIFT -1) >> 8, (LCD_FRAME_HEIGHT + YSHIFT -1) & 0xFF } }, // Row address set
	{ 0x2D ,128, { 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F 
		}, 0},

{ 0 } 
};

static const INIT_SCRIPT display_on_scr[] = {
  { 0x11, 1, { 0x00 }, 120 }, // Sleep out
  { 0x29, 1, { 0x00 }, 120 }, // Display on
  {0}
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
  const uint8_t* tx_buf = data;

  gpio_set_value(DnC_PIN, cmd ? gpio_LOW : gpio_HIGH);

  while (bytes > 0) {
    const size_t count = bytes > MAX_TRANSFER ? MAX_TRANSFER : bytes;

    (void)write(spi_fd, tx_buf, count);

    bytes -= count;
    tx_buf += count;
  }
}

static void lcd_run_script(const INIT_SCRIPT* script)
{
  int idx;
  for (idx = 0; script[idx].cmd; idx++) {
    lcd_spi_transfer(TRUE, 1, &script[idx].cmd);
    lcd_spi_transfer(FALSE, script[idx].data_bytes, script[idx].data);
    milliwait(script[idx].delay_ms);
  }
}

/************ LCD Device Interface *************/

static void lcd_device_init()
{
  // Init registers and put the display in sleep mode
  lcd_run_script(init_scr);

  // Clear lcd memory before turning display on
  // as the contents of memory are set randomly on
  // power on
  lcd_clear_screen();

  // Turn display on
  lcd_run_script(display_on_scr);
}

void lcd_clear_screen(void) {
  const LcdFrame frame={{0}};
  lcd_draw_frame(&frame);
}
void lcd_test(int test_var)
{
  	static const uint8_t down_com = 0x28;
  	static const uint8_t up_com = 0x29;
	LcdFrame fld;
	int iter;
	for (iter=0; iter < PXL_CNT; iter++)
	{
		if (iter%(PXL_CNT/2) < PXL_CNT/4)
		{
			if ( iter%80 < 40 )
				fld.data[iter] = (test_var? lcd_BLUE: lcd_YELLOW);
			else 
				fld.data[iter] = (test_var? lcd_YELLOW: lcd_BLUE);
		}
		else
		{
			if ( iter%80 < 40 )
				fld.data[iter] = (test_var? lcd_GREEN: lcd_RED);
			else 
				fld.data[iter] = (test_var? lcd_RED: lcd_GREEN);
		}
	}
  	lcd_spi_transfer(TRUE, 1, &down_com);
	lcd_draw_frame(&fld);
  	lcd_spi_transfer(TRUE, 1, &up_com);
/*
  gpio_set_value(DnC_PIN, test_var ? gpio_LOW : gpio_HIGH);
  gpio_set_value(RESET_PIN2, test_var ? gpio_LOW : gpio_HIGH);
*/

}

void lcd_draw_frame(const LcdFrame* frame) {
   static const uint8_t WRITE_RAM = 0x2C;
   lcd_spi_transfer(TRUE, 1, &WRITE_RAM);
   lcd_spi_transfer(FALSE, sizeof(frame->data), frame->data);
}

void lcd_draw_frame2(const uint16_t* frame, size_t size) {
   	static const uint8_t WRITE_RAM = 0x2C;
	int totfr = 0;
	uint8_t *frame2, *ifr_p, *ofr_p;
	int i, j, k, size2 = 0;

	if (!size)
		return;
	ifr_p = (uint8_t *) frame;
	totfr = size / (OLP_CNT*2);
	if (size % (OLP_CNT*2) )
		totfr++;
	size2 = totfr*PXL_CNT*2;
	frame2 = (uint8_t *) malloc(size2);
	ofr_p = frame2;
	for (i = 0; i < totfr; i++)
	{
		ifr_p += 2*3*OLD_FRAME_WIDTH;
		for ( j = 0; j < OLD_FRAME_HEIGHT-6; j++)
		{
			if ( j % 9 )
			{
				ifr_p += 2*2;
				for (k = 0; k < (OLD_FRAME_WIDTH-4)*2; k++)
				{
					if ( k % 18 )
					{
						*ofr_p = *ifr_p;
						ofr_p++;
						ifr_p++;
					}
					else
					{
						ifr_p+=2;
						k++;
					}
				}
				ifr_p += 2*2;
			}
			else 
			{
				ifr_p += 2*OLD_FRAME_WIDTH;
			}
		}
		ifr_p += 2*3*OLD_FRAME_WIDTH;
	}

   	lcd_spi_transfer(TRUE, 1, &WRITE_RAM);
   	lcd_spi_transfer(FALSE, size2, frame2);
	free(frame2);
}


static const char* BACKLIGHT_DEVICES[] = {
  "/sys/class/leds/face-backlight-left/brightness",
  "/sys/class/leds/face-backlight-right/brightness"
};

static void _led_set_brightness(const int brightness, const char* led)
{
  int fd = open(led,O_WRONLY);
  if (fd) {
    char buf[3];
    snprintf(buf,3,"%02d\n",brightness);
    (void)write(fd, buf, 3);
    close(fd);
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

  // IO Setup
  DnC_PIN = gpio_create(GPIO_LCD_WRX, gpio_DIR_OUTPUT, gpio_HIGH);

  RESET_PIN1 = gpio_create_open_drain_output(GPIO_LCD_RESET1, gpio_HIGH);
  RESET_PIN2 = gpio_create_open_drain_output(GPIO_LCD_RESET2, gpio_HIGH);
  usleep(200);

  // SPI setup

  spi_fd = lcd_spi_init();

  // Send reset signal
  microwait(50);
  gpio_set_value(RESET_PIN1, 0);
  gpio_set_value(RESET_PIN2, 0);
  microwait(50);
  gpio_set_value(RESET_PIN1, 1);
  gpio_set_value(RESET_PIN2, 1);
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
  if (RESET_PIN1) {
    gpio_close(RESET_PIN1);
  }
  if (RESET_PIN2) {
    gpio_close(RESET_PIN2);
  }

}
