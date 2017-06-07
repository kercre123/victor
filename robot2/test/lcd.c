#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <wiringPi.h>

typedef struct {
	uint8_t cmd;
	uint8_t data_bytes;
	uint8_t data[5];
} INIT_SCRIPT;

static const int RESET_PIN = 21;
static const int DnC_PIN = 20;
static const int DAT_CLOCK = 90000000;
static const int MAX_TRANSFER = 0x1000;

#define FRAME_WIDTH     240
#define FRAME_HEIGHT    180

static const INIT_SCRIPT init_scr[] = {
	{ 0x0E, 0 },
	{ 0x11, 0 },
	{ 0x36, 1, { 0x00 } },
	{ 0xB2, 5, { 0x0C, 0x0C, 0x00, 0x33, 0x33 } },
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
	{ 0x2A, 4, { 0x00, 0x00, (FRAME_WIDTH - 1) >> 8, (FRAME_WIDTH - 1) & 0xFF } },
	{ 0x2B, 4, { 0x00, 0x28, (FRAME_HEIGHT + 0x27) >> 8, (FRAME_HEIGHT + 0x27) & 0xFF } },
	{ 0x29, 0 },

	{ 0 }
};

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
		tr.len = (bytes > MAX_TRANSFER) ? MAX_TRANSFER : bytes;

		digitalWrite(DnC_PIN, cmd ? LOW : HIGH);
		ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

		bytes -= tr.len;
		tr.tx_buf += tr.len;
	}
}

static void init() {
	int idx;
	for (idx = 0; init_scr[idx].cmd; idx++) {
		spi(TRUE, 1, &init_scr[idx].cmd);
		spi(FALSE, init_scr[idx].data_bytes, init_scr[idx].data);
	}
}

static void animate(const char* fn) {
	uint16_t frame[FRAME_WIDTH * FRAME_HEIGHT];
	FILE *fo = fopen(fn, "rb");

	while (!feof(fo)) {
		static const uint8_t WRITE_RAM = 0x2C;

		fread(frame, 1, sizeof(frame), fo);
		spi(TRUE, 1, &WRITE_RAM);
		spi(FALSE, sizeof(frame), &frame);
	}
}

int main(int argc, char** argv) {
	static const uint8_t    MODE = 0;
	static const uint8_t    BITS = 8;

	wiringPiSetupGpio();
	pinMode(RESET_PIN, OUTPUT);
	pinMode(DnC_PIN, OUTPUT);

	spi_fd = open("/dev/spidev0.0", O_RDWR);
	ioctl(spi_fd, SPI_IOC_RD_MODE, &MODE);

	// Send reset signal
	digitalWrite(RESET_PIN, 0);
	delay(50);
	digitalWrite(RESET_PIN, 1);
	delay(50);

	// Start drawing stuff to the screen
	init();
	animate("./color.raw");

	close(spi_fd);

	return 0;
}
