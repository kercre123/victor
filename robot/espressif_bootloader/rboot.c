/*******************************************************************************
 * @file rBoot open source boot loader for ESP8266.
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

#include "rboot-private.h"

/** Loads data and instruction RAMs from the firmware.
 * We put this function in a different RAM section to make sure it winds up safely at the end of instruction RAM
 * see the LD script for more details. Note that the firmware must not use all of the instruction RAM since that would
 * cause this function to overwrite itself.
 * @param readpos The starting read position to find firmware section headers
 * @param count The number of sections to copy
 * @param entry The firmware entry point, passed through this function for tail return from the caller
 */
__attribute__((section(".iram2.text"))) usercode* NOINLINE load_rom(uint32 readpos, const uint8 count, usercode* entry)
{	
	uint8 buffer[BUFFER_SIZE];
	uint8 *writepos;
	uint32 remaining;
	section_header *section = (section_header*)buffer;
  uint8 sectcount;
  
	// copy all the sections
	for (sectcount = count; sectcount > 0; sectcount--) {
		// read section header
		SPIRead(readpos, section, sizeof(section_header));
		readpos += sizeof(section_header);

		// get section address and length
		writepos = section->address;
		remaining = section->length;

		while (remaining > 0) {
			// work out how much to read, up to 16 bytes at a time
			uint32 readlen = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
			// read the block
			SPIRead(readpos, buffer, readlen);
			readpos += readlen;
			// copy the block
			ets_memcpy(writepos, buffer, readlen);
			// increment next write position
			writepos += readlen;
			// decrement remaining count
			remaining -= readlen;
		}
	}
  
  return entry;
}

static uint32 check_image(uint32 readpos) {
	
	uint8 buffer[BUFFER_SIZE];
	uint8 sectcount;
	uint8 sectcurrent;
	uint8 *writepos;
	uint8 chksum = CHKSUM_INIT;
	uint32 loop;
	uint32 remaining;
	uint32 romaddr;
	
	rom_header_new *header = (rom_header_new*)buffer;
	section_header *section = (section_header*)buffer;
	
	if (readpos == 0 || readpos == 0xffffffff) {
		return 0;
	}
	
	// read rom header
	if (SPIRead(readpos, header, sizeof(rom_header_new)) != 0) {
		return 0;
	}
	
	// check header type
	if (header->magic == ROM_MAGIC) {
		// old type, no extra header or irom section to skip over
		romaddr = readpos;
		readpos += sizeof(rom_header);
		sectcount = header->count;
	} else if (header->magic == ROM_MAGIC_NEW1 && header->count == ROM_MAGIC_NEW2) {
		// new type, has extra header and irom section first
		romaddr = readpos + header->len + sizeof(rom_header_new);
		// skip the extra header and irom section
		readpos = romaddr;
		// read the normal header that follows
		if (SPIRead(readpos, header, sizeof(rom_header)) != 0) {
			return 0;
		}
		sectcount = header->count;
		readpos += sizeof(rom_header);
	} else {
		return 0;
	}
	
	// test each section
	for (sectcurrent = 0; sectcurrent < sectcount; sectcurrent++) {
		
		// read section header
		if (SPIRead(readpos, section, sizeof(section_header)) != 0) {
			return 0;
		}
		readpos += sizeof(section_header);

		// get section address and length
		writepos = section->address;
		remaining = section->length;
		
		while (remaining > 0) {
			// work out how much to read, up to BUFFER_SIZE
			uint32 readlen = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
			// read the block
			if (SPIRead(readpos, buffer, readlen) != 0) {
				return 0;
			}
			// increment next read and write positions
			readpos += readlen;
			writepos += readlen;
			// decrement remaining count
			remaining -= readlen;
			// add to chksum
			for (loop = 0; loop < readlen; loop++) {
				chksum ^= buffer[loop];
			}
		}
	}
	
	// round up to next 16 and get checksum
	readpos = readpos | 0x0f;
	if (SPIRead(readpos, buffer, 1) != 0) {
		return 0;
	}
	
	// compare calculated and stored checksums
	if (buffer[0] != chksum) {
		return 0;
	}
	
	return romaddr;
}


/** calculate checksum for block of data
 * from start up to (but excluding) end
 */
static uint8 calc_chksum(uint8 *start, uint8 *end) {
	uint8 chksum = CHKSUM_INIT;
	while(start < end) {
		chksum ^= *start;
		start++;
	}
	return chksum;
}

/** Simplified find_image function just wraps check_image's RAM initalization point finding function with error messages
 * prevent this function being placed inline with main
 * to keep main's stack size as small as possible
 * don't mark as static or it'll be optimised out when
 * using the assembler stub
 */
usercode* NOINLINE find_image(void)
{
  uint32 readpos = check_image(SECTOR_SIZE * FIRMWARE_START_SECTOR);
  if (readpos == 0)
  {
    ets_printf("DIE: Couldn't find firmware run addr!\r\n");
    return 0;
  }
  else
  {
    rom_header header;
    
    // read rom header
    SPIRead(readpos, &header, sizeof(rom_header));
    readpos += sizeof(rom_header);
    return load_rom(readpos, header.count, header.entry); // XXX Counting on tail return to make this work
  }
}

/** Checks the boot config sector to see if there's a new image to write and if so, erases old and copies in new
 * @note Must be NOINLINE and not static to call from ASM without generating a stack which will be left in memory
 */
void NOINLINE copyNewImage(void)
{
  BootloaderConfig config;
  SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &config, sizeof(BootloaderConfig));
  if (config.header != BOOT_CONFIG_HEADER)
  {
    ets_printf("WARNING: Incorrect boot config header, %08x != %08x, skipping\r\n", config.header, BOOT_CONFIG_HEADER);
    return;
  }
  else if (calc_chksum((uint8*)&config, (uint8*)&config.chksum) != config.chksum)
  {
    ets_printf("ERROR: boot config has bad checksum, %02x, expecting %02x\r\n",
               config.chksum, calc_chksum((uint8*)&config, (uint8*)&config.chksum));
    return;
  }
  else if (config.newImageStart == 0 || config.newImageSize == 0)
  {
    // No new image
    return;
  }
  else
  {
    uint32 buffer[SECTOR_SIZE/4]; // Buffer to copy from one sector to another
    uint32 sector;
    SpiFlashOpResult rslt;
    
    ets_printf("Found new image\r\n");
    
    ets_printf("\tErasing old firmware\r\n");
    for (sector = FIRMWARE_START_SECTOR; sector < FIRMWARE_START_SECTOR + config.newImageSize; sector++)
    {
      rslt = SPIEraseSector(sector);
      if (rslt != SPI_FLASH_RESULT_OK)
      {
        ets_printf("\tError erasing sector %x: %d\r\n", sector, rslt);
        sector--; // Try this sector again
      }
    }
    
    ets_printf("\tCopying in new firmware\r\n");
    for (sector = 0; sector < config.newImageSize; sector++)
    {
      rslt = SPIRead((sector + config.newImageStart)*SECTOR_SIZE, buffer, SECTOR_SIZE);
      if (rslt != SPI_FLASH_RESULT_OK)
      {
        ets_printf("\tError reading sector %x: %d\r\n", sector + config.newImageStart, rslt);
        sector--; // Retry the same sector
      }
      else
      {
        rslt = SPIWrite((sector + FIRMWARE_START_SECTOR)*SECTOR_SIZE, buffer, SECTOR_SIZE);
        if (rslt != SPI_FLASH_RESULT_OK)
        {
          ets_printf("\tError writing sector %x: %d\r\n", sector + FIRMWARE_START_SECTOR, rslt);
          sector--; // Retry the same sector
        }
      }
    }
    ets_printf("Done copying new image, %d sectors\r\n", sector);
  }
}

/** Command SPI flash to make certain sectors read only until next power cycle
 * @note Must be NOINLINE and not static to call from ASM without generating a stack which will be left in memory
 */
void NOINLINE writeProtect(void)
{
  uint32 factoryDoneFlag = 0xFFFFffff;
  uint8  protectSectors = 1; // Always protecting outselves
  SPIRead(FACTORY_SECTOR * SECTOR_SIZE + SECTOR_SIZE - 4, &factoryDoneFlag, 4); // Read the last word from the factory sector
  if (factoryDoneFlag != 0xFFFFffff)
  {
    ets_printf("Factory done flag %08x, write protecting factory sector\r\n", factoryDoneFlag);
    protectSectors++; // If there's data there, write protect the factory sector
  }
  
  // XXX Exectute the flash protect command!
  ets_printf("WARNING: Flash write protection not yet implemented\r\n");
}

__attribute__((section(".iram2.text"))) void NOINLINE stage2a(void)
{
  __asm volatile (
  "call0 find_image\n"             // find the start of the image and load into RAM
  "mov a0, a15\n"                  // restore return addr
  "bnez a2, 1f\n"                  // ?success
  "ret\n"                          // no, return
  "1:\n"                           // yes...
  "jx a2\n"                        // Jump to firmware entry point
  );
}

// assembler stub uses no stack space
// works with gcc
void call_user_start() {
	__asm volatile (
		"mov a15, a0\n"                  // store return addr, hope nobody wanted a15!
		"call0 writeProtect\n"           // apply flash write protection
    "call0 copyNewImage\n"           // check for and copy in new image
    "mov a0, a15\n"                  // restore return addr
    "movi a3, stage2a\n"             // Load pointer to stage 2a 
    "jx a3\n"                        // Jump to the high memory code
	);
}
