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
#include "core/anki_dev_unit.h"

#undef LCD_FRAME
#if 0   //list of frames that can be selected
#define LCD_FRAME 77890    //ORIGIN
#define LCD_FRAME 3022     //NV3022
#define LCD_FRAME 7789     //ST7789
#define LCD_FRAME 7735     //ST7735
#define LCD_FRAME MIDAS    //hybrid kinda
#endif
#define LCD_FRAME MIDAS
#undef INIT_PROG
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
#define GPIO_LCD_WRX   13
//<RevI>
#undef GPIO_LCD_WRX
#define GPIO_LCD_WRX   110
//<!RevI>
#define GPIO_LCD_RESET1 96
#define GPIO_LCD_RESET2 55

static GPIO RESET_PIN1;
static GPIO RESET_PIN2;
static GPIO DnC_PIN;


#define RSHIFT 0x1C
#define XSHIFT 0x00
#define YSHIFT 0x17

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
  uint8_t cmd;
  uint8_t data_bytes;
  uint8_t data[128];
  uint32_t delay_ms;
} INIT_SCRIPT;

static const INIT_SCRIPT st77890_init_scr[] =
{
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
static const INIT_SCRIPT st7789_init_scr[] =
{
  { 0x01, 1, { 0x00 }, 150}, // RESET
  { 0x11, 1, { 0x00 }, 10}, // Sleep out
  { 0x3A, 1, { 0x55 }, 10 }, // Interface pixel format (16 bit/pixel 65k RGB data)
  { 0x36, 1, { 0x08 }, 0 }, // Interface pixel format (16 bit/pixel 65k RGB data)
  { 0x2A, 4, { 0x00,0,0,240 }, 0 }, // Interface pixel format (16 bit/pixel 65k RGB data)
  { 0x2B, 4, { 0x00,0,0,135 }, 0 }, // Interface pixel format (16 bit/pixel 65k RGB data)

  { 0x21, 1, { 0x00 }, 10 }, // Display inversion on
  { 0x13, 1, { 0x00 }, 10 }, // Display normal on
  { 0x29, 1, { 0x00 }, 10 }, // Display on
  { 0 }
};
static const INIT_SCRIPT st7735_init_scr[] =
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
        { 0xE1, 16, { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10}, 0}, 
        { 0xE0, 16, { 0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d, 0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10}, 0}, 

        { 0x13, 0, { 0x00}, 100},
        { 0x21, 1, { 0x00 }, 0 }, // Display inversion on
        { 0x29, 0, { 0x00}, 10},

#if 0
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
#else
  { 0x2A, 4, { 0x00, 0x00, 0x00, 0x4F } }, //CASET
  { 0x2B, 4, { 0x00, 0x00, 0x00, 0x9F } }, //RASET
        { 0x36, 1, { 0xC8 }, 0}, //MADCTL
        { 0x36, 1, { 0xC0 }, 0}, //MADCTL
        { 0x36, 1, { 0xE0 }, 0}, //MADCTL
#endif
{ 0 } 
};

static const INIT_SCRIPT nv3022_init_scr[] =
{
        { 0x01, 0, { 0x00}, 150}, // Software reset
        { 0x11, 0, { 0x00}, 500}, // Sleep out
        { 0x20, 0, { 0x00}, 0}, // Display inversion off
        { 0x36, 1, { 0xA8}, 0}, //exchange rotate and reverse order in each of 2 dim 
        { 0x3A, 1, { 0x05}, 0}, // Interface pixel format (16 bit/pixel 65k RGB data)

        { 0xE0, 16, { 0x07, 0x0e, 0x08, 0x07, 0x10, 0x07, 0x02, 0x07, 0x09, 0x0f, 0x25, 0x36, 0x00, 0x08, 0x04, 0x10 }, 0},  // 
        { 0xE1, 16, { 0x0a, 0x0d, 0x08, 0x07, 0x0f, 0x07, 0x02, 0x07, 0x09, 0x0f, 0x25, 0x35, 0x00, 0x09, 0x04, 0x10 }, 0}, 

        { 0xFC, 1, { 128+64}, 0},
  
        { 0x13, 0, { 0x00}, 100}, // Normal Display Mode On
  { 0x21, 0, { 0x00 }, 10 }, // Display inversion on
        { 0x20, 0, { 0x00 }, 10 }, // Display inversion off
  { 0x26, 1, {0x02} , 10}, // Set Gamma
        { 0x29, 0, { 0x00}, 10}, // Display On

       { 0x2A, 4, { XSHIFT >> 8, XSHIFT & 0xFF, (LCD_FRAME_WIDTH + XSHIFT -1) >> 8, (LCD_FRAME_WIDTH + XSHIFT -1) & 0xFF } }, // Column address set
       { 0x2B, 4, { YSHIFT >> 8, YSHIFT & 0xFF, (LCD_FRAME_HEIGHT + YSHIFT -1) >> 8, (LCD_FRAME_HEIGHT + YSHIFT -1) & 0xFF } }, // Row address set

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
  lcd_run_script(INIT_PROG);

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

void scale_row(int kscale, int divk, int difk, uint16_t **ofr_pp, uint16_t **ifr_pp)
{
  int k = 0, ihg = 0, idifk = 0;
  if ( !ofr_pp || !ifr_pp  )
    return;
  idifk = difk;
        for (k = 0; k < OLD_FRAME_WIDTH; k++)
        {
                if (kscale)
                {
                        if ( (idifk>0) &&  divk &&!(k % divk) )
                        {
        for ( ihg = 0 ; ihg < ((difk/OLD_FRAME_WIDTH)?:1); ihg++)
        {
                                  **ofr_pp = **ifr_pp;
                                  (*ofr_pp)++; 
                                  idifk--;
          // distribute(ihg--) if evnd erlier to smoo
        }
                        }
                        **ofr_pp = **ifr_pp;
                        (*ofr_pp)++; (*ifr_pp)++;
                }
                else
                {
                        if ( (idifk>0) &&  divk &&!(k % divk) )
                        {
                                (*ifr_pp)++;
                                idifk--;
                        }
                        else
                        {
                          **ofr_pp = **ifr_pp;
                          (*ofr_pp)++; (*ifr_pp)++;
                        }
                }
        }
}


/*

TEST THE SIX OFFICIAL EYE COLORS IN ROTATION

#define RGB_PACK(r,g,b)    ( ((r >> 3) << 11) + ( (g >> 2) << 5) + (b >> 3))
#define RGB_PACK_F(r,g,b) ( ( ( (uint8_t)(r * 255.0) >> 3) << 11 ) + ( ( (uint8_t)(g*255.0) >> 2) << 5) + ((uint8_t)(b * 255.0) >> 3) )

void *memset16(void *m, uint16_t val, size_t count)
{
    uint16_t *buf = m;

    while(count--) *buf++ = val;
    return m;
}


void lcd_draw_frame2(const uint16_t* frame, size_t size) {
  static const uint8_t WRITE_RAM = 0x2C;
  static const uint8_t PAD = 0x0;
  
  uint16_t colors[] = {
    RGB_PACK_F(0.21,0.76,0.44),
    RGB_PACK_F(0.93,0.44,0.1),
    RGB_PACK_F(0.94, 0.84, 0.0),
    RGB_PACK_F(0.66, 0.9, 0.04),
    RGB_PACK_F(0.1,0.62,0.92),
    RGB_PACK_F(0.74,0.29,1.0),
  };

  for (int x=0;x<6;x++) {
    uint16_t row[OLD_FRAME_WIDTH];
    memset16(row, colors[x] ,OLD_FRAME_WIDTH);
  
    lcd_spi_transfer(TRUE, 1, &WRITE_RAM);

    lcd_spi_transfer(FALSE, 1, &PAD);
  
    for(int i=0;i<LCD_FRAME_HEIGHT;i++)
    {
if 0
      const uint16_t* row = (uint16_t*)frame + ((i+12) * OLD_FRAME_WIDTH ) + 8;
#else
      const uint16_t* row = (uint16_t*)anki_dev_unit + ((i+12) * OLD_FRAME_WIDTH ) + 8;
#endif
      lcd_spi_transfer(FALSE, (LCD_FRAME_WIDTH )* 2, row);
    }

    sleep(5);
  };
  return;
}
*/

void lcd_draw_frame2(const uint16_t* frame, size_t size) {
  static const uint8_t WRITE_RAM = 0x2C;
  static const uint8_t PAD = 0x0;

  lcd_spi_transfer(TRUE, 1, &WRITE_RAM);

  // For some reason we need this to align properly. We should be able
  // To fix this on the initialization but haven't figured that out yet
  // If not we don't align on bytes properly and effectively have a swap
  // of MSB/LSB causing all sorts of problems with the color pallette.
  
  lcd_spi_transfer(FALSE, 1, &PAD);
  
  for(int i=0;i<LCD_FRAME_HEIGHT-1;i++)
  {
#if 1 // WRITE REAL DATA
    const uint16_t* row = (uint16_t*)frame + ((i+12) * OLD_FRAME_WIDTH ) + 8;
#else // USE CLOUD PATTERN FOR TESTING PURPOSES
    const uint16_t* row = (uint16_t*)anki_dev_unit + ((i+12) * OLD_FRAME_WIDTH ) + 8;
#endif
    lcd_spi_transfer(FALSE, (LCD_FRAME_WIDTH )* 2, row);
  }

  return;
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

  // Why do we mess with IMU reset pin here? Is this needed?
  RESET_PIN1 = gpio_create_open_drain_output(GPIO_LCD_RESET1, gpio_HIGH);
  RESET_PIN2 = gpio_create(GPIO_LCD_RESET2, gpio_DIR_OUTPUT, gpio_HIGH);
  usleep(200);

  // SPI setup

  spi_fd = lcd_spi_init();

  // Send reset signal
  microwait(50);
  gpio_set_value(RESET_PIN1, 0);
  gpio_set_value(RESET_PIN2, 0);
  usleep(50);
  gpio_set_value(RESET_PIN1, 1);
  gpio_set_value(RESET_PIN2, 1);
  usleep(250);

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
