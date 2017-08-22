// Compile with: ../../generated/android/tools/arm/bin/arm-linux-androideabi-gcc -pie lcd_test.c
// Generate frames from animated gif with: python3 gif_to_raw.py <gif_filename>
// adb push a.out and <gif_filename>.raw to robot temp dir.
//
// Usage:  ./a.out frames.raw.
//
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

typedef struct {
	uint8_t cmd;
	uint8_t data_bytes;
	uint8_t data[5];
} INIT_SCRIPT;

static const int DAT_CLOCK = 10000000;
static const int MAX_TRANSFER = 0x1000;

#define FRAME_WIDTH     184 
#define FRAME_HEIGHT    96 

#define RSHIFT 0x20

static const INIT_SCRIPT init_scr[] = {
	{ 0x11, 0 },
	{ 0x36, 1, { 0x00 } },
//	{ 0xB2, 5, { 0x0C, 0x0C, 0x00, 0x33, 0x33 } },
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
	{ 0x2A, 4, { 0x00, RSHIFT, (FRAME_WIDTH + RSHIFT - 1) >> 8, (FRAME_WIDTH + RSHIFT - 1) & 0xFF } },
	{ 0x2B, 4, { 0x00, 0x00, (FRAME_HEIGHT -1) >> 8, (FRAME_HEIGHT -1) & 0xFF } },
	{ 0x29, 0 },

	{ 0 }
};


#define LOW 0
#define HIGH 1
#define FALSE 0
#define TRUE (!FALSE)
#define OUTPUT "out"

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
//   if (gp.fd > 0)  {
      write(gp.fd, trigger[value!=0], 1);
//   }
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

void wiringPiSetupGpio(void)
{
  DnC_PIN = gpio_create(GPIO_LCD_WRX, 1, 1);
//  CS_PIN = gpio_create(14, 1, 1);
  RESET_PIN = gpio_create(GPIO_LCD_RESET, 1, 1);
   
}
#define digitalWrite gpio_set_value

void pinMode(GPIO pin, const char* dir){
   gpio_set_direction(pin, dir[0]=='o');
}

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
#define delay(x) MicroWait((x))



static int spi_fd;

void spi(int cmd, int bytes, const void* data) {
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)data,
		.rx_buf = 0,
		.len = bytes,
		.delay_usecs = 0,
		.speed_hz = DAT_CLOCK,
		.bits_per_word = 8,
	};

	while (bytes > 0) {
//     digitalWrite(CS_PIN, LOW);
     tr.len = (bytes > MAX_TRANSFER) ? MAX_TRANSFER : bytes;

		digitalWrite(DnC_PIN, cmd ? LOW : HIGH);
		ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

		bytes -= tr.len;
		tr.tx_buf += tr.len;
//  digitalWrite(CS_PIN, HIGH);
	}
}

static void init() {
	int idx;
  /* uint8_t SLEEP_IN=0x10; */

  /* printf("OFF!?\n"); */
  /* /\* NEW: Try sleep *\/ */
  /* digitalWrite(CS_PIN, LOW); */
  /* spi(TRUE, 1, &SLEEP_IN); */
  /* digitalWrite(CS_PIN, HIGH); */
  /* delay(1200); */

  /* printf("ON!?\n"); */
  
	for (idx = 0; init_scr[idx].cmd; idx++) {
//     digitalWrite(CS_PIN, LOW);
		spi(TRUE, 1, &init_scr[idx].cmd);
		spi(FALSE, init_scr[idx].data_bytes, init_scr[idx].data);
//    digitalWrite(CS_PIN, HIGH);
	}
}

static void animate(const char* fn) {
	uint16_t frame[FRAME_WIDTH * FRAME_HEIGHT];
	FILE *fo = fopen(fn, "rb");

	while (!feof(fo)) {
		static const uint8_t WRITE_RAM = 0x2C;

		fread(frame, 1, sizeof(frame), fo);
//  digitalWrite(CS_PIN, LOW);
		spi(TRUE, 1, &WRITE_RAM);
		spi(FALSE, sizeof(frame), &frame);
//  digitalWrite(CS_PIN, HIGH);
	}
  fclose(fo);
}

int main(int argc, char** argv) {
	static const uint8_t    MODE = 0;
	static const uint8_t    BITS = 8;

  /* struct regulator* reg_k1; */
  /* struct regulator* reg_k2; */

  /* reg_k1 = regulator_get(dev, id); */
  /* reg_k2 = regulator_get(dev, id); */

  /* regulator_enable(reg_k1); */
  /* regulator_enable(reg_k2); */
  /* int current_limit = regulator_get_current_limit(reg_k1); */
  /* regulator_set_current_limit(reg_k1, 10*1000, 20*1000 ); */
  /* regulator_set_current_limit(reg_k2, 10*1000, 20*1000 ); */

	wiringPiSetupGpio();
//	pinMode(CS_PIN, OUTPUT);
	pinMode(DnC_PIN, OUTPUT);
	pinMode(RESET_PIN, OUTPUT);
//	digitalWrite(CS_PIN,1);

	spi_fd = open("/dev/spidev1.0", O_RDWR);
	ioctl(spi_fd, SPI_IOC_RD_MODE, &MODE);
  if (!spi_fd) { printf("ERROR!\n"); }

	// Send reset signal
//	digitalWrite(CS_PIN,1);
	delay(50);
	digitalWrite(RESET_PIN, 0);
	delay(50);
	digitalWrite(RESET_PIN, 1);
	delay(50);

#define ENDLESS 1
	// Start drawing stuff to the screen
	init();
  if (argc>1) {
     do {
        animate(argv[1]);
     }  while (ENDLESS);
  }
  else
  {
     printf("Usage: %s <filename>\n", argv[0]);
  }

  /* regulator_disable(reg_k1); */
  /* regulator_disable(reg_k2); */
  /* regulator_put(reg_k1); */
  /* regulator_put(reg_k2); */

	close(spi_fd);

	return 0;
}
