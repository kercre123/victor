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
 
#include "cboot-private.h"
#include "cboot-gpio.h"
#include "cboot-rtc.h"

#define DEBUG 0

#define RECOVERY_MODE_PIN (3)
#define HANDSHAKE_PIN (12)
#define FACTORY_FW_START (FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE)

NOINLINE uint32 check_image(uint32 readpos, uint8* buffer) {

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
  if (SPIRead(readpos, header, sizeof(rom_header_new)) != SPI_FLASH_RESULT_OK) {
    ets_printf("Failed to read ROM header at %x\r\n", readpos);
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
    if (SPIRead(readpos, header, sizeof(rom_header)) != SPI_FLASH_RESULT_OK) {
      ets_printf("Failed to read stage two ROM header at %x\r\n", readpos);
      return 0;
    }
    sectcount = header->count;
    readpos += sizeof(rom_header);
  } else {
    ets_printf("No ROM header found at %x, got %x\r\n", readpos, buffer[0]);
    return 0;
  }

  // test each section
  for (sectcurrent = 0; sectcurrent < sectcount; sectcurrent++) {

    // read section header
    if (SPIRead(readpos, section, sizeof(section_header)) != SPI_FLASH_RESULT_OK) {
      ets_printf("Failed to read section header at %x\r\n", readpos);
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
      if (SPIRead(readpos, buffer, readlen) != SPI_FLASH_RESULT_OK) {
        ets_printf("Failed to read block at %x\r\n", readpos);
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
  if (SPIRead(readpos, buffer, 1) != SPI_FLASH_RESULT_OK) {
    ets_printf("Failed to read end of block at %x\r\n", readpos);
    return 0;
  }

  // compare calculated and stored checksums
  if (buffer[0] != chksum) {
    ets_printf("Checksum doesn't match, %x != %x\r\n", buffer[0], chksum);
    return 0;
  }

  //ets_printf("Found valid rom at %x\r\n", romaddr);

  return romaddr;
}

/** Check that the hash of the image matches it's header
 * @param header The image header data, already populated.
 * @param imageStart The start of the image, address of header in flash
 * @param buffer A pointer to at least BUFFER_SIZE bytes of memory for a read buffer
 * @return true if the hash matches the header, false if it doesn't.
 */
NOINLINE uint32 verify_firmware_hash(const AppImageHeader* header, const uint32 imageStart, uint8* const buffer)
{
  SHA1_CTX ctx; // SHA1 calculation state
  uint8 digest[SHA1_DIGEST_LENGTH]; // Where to put the ultimate sha1 digest we calculate
  int i;
  uint32 index = sizeof(AppImageHeader) + APP_IMAGE_HEADER_OFFSET; // Current read index
  
  //ets_printf("AIH @ %x: size=%x, image#=%x, evil=%x\r\n", imageStart, header->size, header->imageNumber, header->evil);
  
  if (header->imageNumber < 1) return FALSE;
  if (header->evil != 0) return FALSE;
  
  SHA1Init(&ctx);
  
  int retries = 3;
  
  while (index < header->size)
  {
    const uint32 remaining  = APP_IMAGE_HEADER_OFFSET + header->size - index;
    const uint32 readPos    = imageStart + index;
    const uint32 readLength = SHA_CHECK_READ_LENGTH < remaining ? SHA_CHECK_READ_LENGTH : remaining;
    
    if (SPIRead(readPos, buffer, readLength) != SPI_FLASH_RESULT_OK)
    {
      if (retries-- <= 0)
      {
        ets_printf("Failed to read back firmware at %x for hash validation\r\n", readPos);
        return FALSE;
      }
      else continue;
    }
    
    retries = 3;
  
    //ets_printf("SHA1Update: %x, %x\r\n", readPos, readLength);
    SHA1Update(&ctx, buffer, readLength);
    index += SHA_CHECK_READ_LENGTH;
  }
    
  SHA1Final(digest, &ctx);
  
  for (i=0; i<SHA1_DIGEST_LENGTH; ++i)
  {
    if (digest[i] != header->sha1[i])
    {
      int j;
      ets_printf("SHA1 Signature for image at %x doesn't match\r\n\t", imageStart);
      for (j=0; j<SHA1_DIGEST_LENGTH; ++j) ets_printf("%02x ", header->sha1[j]);
      ets_printf("\r\n\t");
      for (j=0; j<SHA1_DIGEST_LENGTH; ++j) ets_printf("%02x ", digest[j]);
      ets_printf("\r\n\t");
      for (j=0; j<i; ++j) ets_printf("---");
      ets_printf("^^\r\n");
      return FALSE;
    }
  }
  
  return check_image(imageStart, buffer);
}

/** Returns the start of the firmware image to load and boot
 * @param buffer A pointer to at least BUFFER_SIZE bytes of memory for a read buffer
 * @return The flash address of the start of the image to load and boot.
 */
NOINLINE uint32 select_image(uint8* const buffer)
{
  uint32 retImage       = FACTORY_FW_START;
  uint32 preferredImage = FACTORY_FW_START;
  uint32 secondImage    = FACTORY_FW_START;
  AppImageHeader* preferredHeader = NULL;
  AppImageHeader* secondHeader    = NULL;
  FWImageSelection preferredEnum  = FW_IMAGE_FACTORY;
  FWImageSelection secondEnum     = FW_IMAGE_FACTORY;
  uint32 retSelection = FW_IMAGE_FACTORY;
  uint32 readPos = 0;
  if (get_gpio(RECOVERY_MODE_PIN) == FALSE)
  {
    ets_printf("Selecting recovery image\r\n");
    retImage = check_image(retImage, buffer);
  }
  else
  {
    AppImageHeader aihA, aihB;
    
    if (SPIRead(APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET, &aihA, sizeof(AppImageHeader)) != SPI_FLASH_RESULT_OK)
    {
      ets_printf("Failed to read application image A header\r\nFalling back to factory image\r\n");
    }
    else if (SPIRead(APPLICATION_B_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET, &aihB, sizeof(AppImageHeader)) != SPI_FLASH_RESULT_OK)
    {
      ets_printf("Failed to read application image B header\r\n");
    }
    else
    {
      if (aihA.imageNumber > aihB.imageNumber)
      {
        preferredImage  = APPLICATION_A_SECTOR * SECTOR_SIZE;
        secondImage     = APPLICATION_B_SECTOR * SECTOR_SIZE;
        preferredHeader = &aihA;
        secondHeader    = &aihB;
        preferredEnum   = FW_IMAGE_A;
        secondEnum      = FW_IMAGE_B;
      }
      else
      {
        preferredImage  = APPLICATION_B_SECTOR * SECTOR_SIZE;
        secondImage     = APPLICATION_A_SECTOR * SECTOR_SIZE;
        preferredHeader = &aihB;
        secondHeader    = &aihA;
        preferredEnum   = FW_IMAGE_B;
        secondEnum      = FW_IMAGE_A;
      }
      
      readPos = verify_firmware_hash(preferredHeader, preferredImage, buffer);
      if (readPos != 0)
      {  
        ets_printf("Selecting newest image\r\n");
        retImage = readPos;
        retSelection = preferredEnum;
      }
      else
      {
        readPos = verify_firmware_hash(secondHeader, secondImage, buffer);
        if (readPos != 0)
        {
          ets_printf("Selecting older image\r\n");
          retImage = readPos;
          retSelection = secondEnum;
        }
        else
        {
          ets_printf("Falling back to factory image\r\n");
          retImage = check_image(retImage, buffer);
        }
      }
    }
  }
  
  system_rtc_mem(RTC_IMAGE_SELECTION, &retSelection, 1, TRUE);
  return retImage;
}


/** Simplified find_image function
 */
__attribute__((section(".iram2.text"))) usercode* NOINLINE find_image(void)
{
  uint8 buffer[BUFFER_SIZE];
  uint8 sectcount;
  uint8 *writepos;
  uint32 remaining;
  usercode* usercode;
  uint32 readpos;

  readpos = select_image(buffer);
  
  rom_header *header = (rom_header*)buffer;
  section_header *section = (section_header*)buffer;
  
  // read rom header
  SPIRead(readpos, header, sizeof(rom_header));
  readpos += sizeof(rom_header);

  // create function pointer for entry point
  usercode = header->entry;
  
  //ets_printf("Header %d sections, entry at %x\r\n", header->count, header->entry);
  
  // copy all the sections
  for (sectcount = header->count; sectcount > 0; sectcount--) {

    //ets_printf("\tCopying section %d: rp = %x\twp = %x\tlen = %x\r\n", sectcount, readpos, section->address, section->length);

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

  //ets_printf("Jumping to user code at %x\r\n", usercode);

  return usercode;
}

/** Sets up the UART so we can get debugging output from the bootloader
 */
void NOINLINE setupSerial(void)
{
  // Update the clock rate here since it's the first function we call
  uart_div_modify(0, (50*1000000)/230400);
  // Debugging delay
  ets_delay_us(100);
  set_gpio(HANDSHAKE_PIN, 0);
  
  ets_printf("Welcome to cboot\r\nVersion 2.2\r\n");
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
}

#define TEST_BASE ((uint32_t*)0x40108000)
#define TEST_CEIL ((uint32_t*)0x4010Fee0)
#define ZERO(d)  ((d<<16)|((~d)&0xffff))
#define ONEZ(d)  (~ZERO(d))

int show_test_err(int n, uint32_t addr, uint32_t val)
{
   ets_printf("Memory Error (%d) at %x: %x -> %x\r\n", n,
              addr, val, *((uint32_t*)addr));
   return 0;
   
}
void NOINLINE memtest(void)
{
   //const uint32_t tick = xthal_get_ccount();
   uint32_t offset;
   uint32_t val;
   volatile uint32_t* base = TEST_BASE;
   int passed  = 1;  //assume good until proven otherwise
   do {
      //U:w0
      for (offset=0; TEST_BASE+offset < TEST_CEIL; offset++) {
         val = ZERO(offset);
         *(base+offset) = val;
      }
      //U:r0w1r1
      for (offset=0; TEST_BASE+offset < TEST_CEIL; offset++) {
         val = ZERO(offset);
         if ( *(base+offset) != val)
         {
            passed = show_test_err(1, (uint32_t)(base+offset), val);
         }
         val = ONEZ(offset);
         *(base+offset) = val;
         if ( *(base+offset) != val)
         {
            passed = show_test_err(2, (uint32_t)(base+offset), val);
         }
      }
      //D:r1w0r0
      for (offset=TEST_CEIL-TEST_BASE-1;offset!=(uint32_t)-1;--offset) {
         val = ONEZ(offset);
         if (*(base+offset) != val) {
            passed = show_test_err(3, (uint32_t)(base+offset), val);
         }
         val = ZERO(offset);
         *(base+offset) = val;
         if ( *(base+offset) != val)
         {
            passed = show_test_err(4, (uint32_t)(base+offset), val);
         }
      }
      //D:r0
      for (offset=TEST_CEIL-TEST_BASE-1;offset!=(uint32_t)-1;--offset) {
         val = ZERO(offset);
         if ( *(base+offset) != val)
         {
            passed = show_test_err(5, (uint32_t)(base+offset), val);
         }
      }
      ets_printf("Memtest %s\r\n", passed?"PASS":"FAIL");
   } while (!passed);
   //const uint32_t tock = xthal_get_ccount();
   //ets_printf("Memtest time %d cycles\r\n", tock-tick);
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
    "call0 setupSerial\n"            // Set serial baudrate
    "call0 memtest\n"
    "call0 writeProtect\n"           // apply flash write protection
    "mov a0, a15\n"                  // restore return addr
    "movi a3, stage2a\n"             // Load pointer to stage 2a 
    "jx a3\n"                        // Jump to the high memory code
  );
}
