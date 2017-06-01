#ifndef __CBOOT_PRIVATE_H__
#define __CBOOT_PRIVATE_H__

/*******************************************************************************
 * @copyright Copyright 2015 Richard A Burton
 * @author richardaburton@gmail.com
 *
 * @license The MIT License (MIT)
 * 
 * Copyright (c) 2015 Richard A Burton (richardaburton@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * =============================================================================
 *
 * Modified extensively for a different ROM selection process and flash operation.
 * @author Daniel Casner
 */

typedef int int32;
typedef int int32_t;
typedef unsigned int uint32;
typedef unsigned int uint32_t;
typedef unsigned char uint8;
typedef unsigned char uint8_t;

#include "sha1.h"
#include "anki/cozmo/robot/flash_map.h"
#include "anki/cozmo/robot/espAppImageHeader.h"

#define NOINLINE __attribute__ ((noinline))

#define CRYSTAL_FREQ 26000000
#define CPU_CLK_FREQ (80*(CRYSTAL_FREQ)/16000000)
#define UART_CLK_FREQ (80*1000000)

#define ROM_MAGIC	   0xe9
#define ROM_MAGIC_NEW1 0xea
#define ROM_MAGIC_NEW2 0x04

#define CHKSUM_INIT 0xef

#define TRUE 1
#define FALSE 0
#define NULL 0

// buffer size, must be at least sizeof(rom_header_new)
#define BUFFER_SIZE 0x100

// How many bytes to add to the SHA1 at once
#define SHA_CHECK_READ_LENGTH 64

// esp8266 built in rom functions
extern uint32 SPIRead(uint32 addr, void *outptr, uint32 len);
extern uint32 SPIEraseSector(int);
extern uint32 SPIWrite(uint32 addr, void *inptr, uint32 len);
extern void ets_printf(char*, ...);
extern void ets_delay_us(int);
extern void ets_memset(void*, uint8, uint32);
extern void ets_memcpy(void*, const void*, uint32);
extern void uart_div_modify(int no, int freq);
extern uint32 xthal_get_ccount(void);

// functions we'll call by address
typedef void usercode(void);

// standard rom header
typedef struct {
	// general rom header
	uint8 magic;
	uint8 count;
	uint8 flags1;
	uint8 flags2;
	usercode* entry;
} rom_header;

typedef struct {
	uint8* address;
	uint32 length;
} section_header;

// new rom header (irom section first) there is
// another 8 byte header straight afterward the
// standard header
typedef struct {
	// general rom header
	uint8 magic;
	uint8 count; // second magic for new header
	uint8 flags1;
	uint8 flags2;
	usercode* entry;
	// new type rom, lib header
	uint32 add; // zero
	uint32 len; // length of irom section
} rom_header_new;

typedef enum {
  SPI_FLASH_RESULT_OK,
  SPI_FLASH_RESULT_ERR,
  SPI_FLASH_RESULT_TIMEOUT,
} SpiFlashOpResult;

#endif
