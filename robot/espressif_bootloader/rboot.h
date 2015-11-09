#ifndef __RBOOT_H__
#define __RBOOT_H__

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
 *
 * The original rBoot selected one of a number of complete images to map and 
 * execute based on a configuration section in flash.
 * This version instead looks at configuration to determine if a newer image has
 * been written by over the air update and overwrites the existing image with
 * the new one if nesissary. This allows the program to be linked once against
 * a specific address point and also allows the remainig flash to be used for
 * assets etc. when not being used for an OTA update.
 */

#define CHKSUM_INIT 0xef

/// Flash sector size always 4kb
#define SECTOR_SIZE 0x1000
/// Flash block size (for faster block erasure), depends on chip, might be 0x10000
#define BLOCK_SIZE 0x8000
/// Map of the sectors of flash where various things are stored
typedef enum {
  BOOTLOADER_SECTOR,    ///< Where the boot loader (this code) lives.
  FACTORY_SECTOR,       ///< Where factory build information will be stored
  BOOT_CONFIG_SECTOR,   ///< Where the boot configuration is stored
  FIRMWARE_START_SECTOR ///< Where the user firmware starts
} FlashSector;
/** Defines a header to look for in flash to determine that the bootloader control structure is valid
 * This value MUST change if a new bootloader is written with an incompatible structure and it will be flashed onto
 * devices with existing bootlaoders. */
#define BOOT_CONFIG_HEADER 0xe1df0c05

/** Boot configuration status structure
 */
typedef struct {
	uint32 header;        ///< Header to indicate flash has a valid config structure
  uint32 newImageStart; ///< New image starting sector or 0 if no new image ready
  uint32 newImageSize;  ///< Size of the new image in sectors or 0 if no new image ready
  uint32 version;       ///< Version number of the firmware, not used by boot loader but by firmware to check itself
	uint8 chksum;		      ///< config chksum
} BootloaderConfig;


#endif
