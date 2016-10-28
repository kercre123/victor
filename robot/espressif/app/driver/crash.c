/**
 * Espressif crash handler - will eventually help us log crashes for later transmission to client
 * Derived from MIT-licensed code in nodemcu (no copyright restrictions)
 */
 
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/crash.h"
#include "driver/uart.h"
#include "spi_flash.h"
#include "anki/cozmo/transport/reliableTransport.h"

#define CRASH_DUMP_SOFISTICATED 1

#define BOOT_ERROR_SOURCE 5 //from robot/clad/src/clad/types/robotErrors.clad.

extern ReliableConnection g_conn;   // So we can check canaries when we crash

extern int COZMO_VERSION_ID; // Initalized in factoryData.c

static int nextCrashRecordSlot;
static int bootErrorRecordSlot;
static int bootErrorRecordIndex;

void os_put_str(char* str)
{
  while (*str != 0)
  {
    os_put_char(*str);
    str++;
  }
}

#define STACKOK(i) (((unsigned int)i > STACK_END) && ((unsigned int)i < STACK_START) && (((unsigned int)i & 3) == 0))

#if CRASH_DUMP_SOFISTICATED
static int get_excvaddr() {
  int v;
  asm volatile (
      "rsr.excvaddr %0\n\t"
      : "=r" (v)
  );
  return v;
}

static int get_depc() {
  int v;
  asm volatile (
      "rsr.depc %0\n\t"
      : "=r" (v)
  );
  return v;
}
#endif

extern void crash_dump(int* sp) {
  ex_regs_esp *regs = (ex_regs_esp*) sp;
#if CRASH_DUMP_SOFISTICATED
  // stack pointer at exception place
  int* ex_sp = (sp + 256 / 4);
  int* p = ex_sp - 8;
  int usestack = STACKOK(p);
  int i;
  CrashRecord record;
  CrashLog_ESP* cle = (CrashLog_ESP*)record.dump;
#endif
  ets_intr_lock(); // Disable all interrupts

  os_put_str("Fatal Exception: ");
  os_put_hex(regs->epc1, 8);
  os_put_str(" (");
  os_put_hex(regs->exccause, 2);
  os_put_str("), sp ");
  os_put_hex((int)sp, 8);
  os_put_char('\r');
  os_put_char('\n');
  
#if CRASH_DUMP_SOFISTICATED  
  record.nWritten = 0;
  record.nReported = 0xFFFFffff;
  record.reporter  = 0; // Espressif is 0
  record.errorCode = 0; // Regular crash
  // Copy in exception registers
  for (i=0; i<sizeof(ex_regs_esp)/sizeof(int); ++i)
  {
    record.dump[i] = sp[i];
  }
  cle->sp = (int)sp;
  if (usestack)
  {
    cle->excvaddr = get_excvaddr();
    cle->depc     = get_depc();
    cle->version  = COZMO_VERSION_ID;
    cle->stackDumpSize = (0x40000000 - (unsigned int)p);
    if (cle->stackDumpSize > ESP_STACK_DUMP_SIZE) cle->stackDumpSize = ESP_STACK_DUMP_SIZE;
    for (i=0; i<cle->stackDumpSize; i++) cle->stack[i] = p[i];
  }
  else
  {
    cle->stackDumpSize =  0;
  }

  os_put_hex(crashHandlerPutReport(&record), 2);
#endif

  while (1);    // Wait for watchdog to get us
}

/*
 * a2 contains exception cause, current stack contains register dump.
 * a3 and a4 may be used because they are not used by standard handler.
 */
//__attribute__((noinline))
void crash_handler(int exccause, ex_regs_esp regs) {
  asm("addi a1, a1, -16");    // Adjust stack to point to crash registers
  asm("s32i.n  a0, a1, 12");   // Store a0 in crash registers
  asm("addi a2, a1, 16");     // Pass stack as first argument to crash_dump
  asm("j crash_dump");        // Proceed to crash_dump - never returning
  // This code never returns
}
extern void* _xtos_set_exception_handler(int exno, void (*exhandler)());

static struct initStatus {
  uint32_t addr;
  uint32_t code;
  uint8_t  n_rec;
  uint8_t  n_rep;
} initResult;


/* Finds first entry that has not been written 
 *Assumes memory starts at all 0xFFFFs
 */
int crashHandlerBootErrorCount(uint32_t* dump_data)
{
   CrashLog_BootError* error_entry = (CrashLog_BootError*)dump_data;
   int i;
   for (i = 0; i< BOOT_ERROR_MAX_ENTRIES; i++)
   {
      if (error_entry[i].addr == (void*)0xFFFFffff)
      {
         break;
      }
   }
   return i;   
}

static bool crashDumpReadWriteOkay(const u32 address, const u32 length)
{
  if ((address + length) <= address) return false; // Check for integer overflow or 0 length
  // Crash dump sector region is default okay region
  else if ((address >= (CRASH_DUMP_SECTOR * SECTOR_SIZE)) && ((address + length) <= (APPLICATION_A_SECTOR * SECTOR_SIZE))) return true; // First allowable segment of NVStorage
  return false;
}

static SpiFlashOpResult crashRecordRead(uint32_t recordNumber, CrashRecord* dest)
{
  const uint32_t recordAddress = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * recordNumber);
  const uint32_t length = CRASH_RECORD_SIZE; 
   
   if (!crashDumpReadWriteOkay(recordAddress, length))
   {
      return SPI_FLASH_RESULT_ERR;
   }
   return spi_flash_read(recordAddress, (uint32_t*)dest, length);
}

static SpiFlashOpResult crashRecordWrite(uint32_t recordNumber, const CrashRecord* data)
{
  const uint32_t recordAddress = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * recordNumber);
  const uint32_t length = CRASH_RECORD_SIZE; 
  if (!crashDumpReadWriteOkay(recordAddress, length))
  {
     return SPI_FLASH_RESULT_ERR;
  }
  return spi_flash_write(recordAddress, (uint32_t*)data, length);
}

static SpiFlashOpResult crashFlashWrite(uint32_t address, const uint32_t* data, uint32_t length)
{
   if (!crashDumpReadWriteOkay(address, length))
   {
      return SPI_FLASH_RESULT_ERR;
   }
   return spi_flash_write(address, (uint32_t*)data, length);
}

static SpiFlashOpResult crashFlashErase()
{
   return spi_flash_erase_sector(CRASH_DUMP_SECTOR);
}




// Register crash handler with XTOS - must be called before 
void ICACHE_FLASH_ATTR crashHandlerInit(void)
{
  int reportedRecords = 0;
  int recordNumber;
  nextCrashRecordSlot = -1; // Invalid
  bootErrorRecordSlot = -1;
  initResult.addr = 0;
  initResult.code = 0;
  
  // Find how many records we have written and how many we have reported
  for (recordNumber=0; recordNumber<MAX_CRASH_LOGS; ++recordNumber)
  {
    CrashRecord rec;
    const SpiFlashOpResult rslt = crashRecordRead(recordNumber, &rec);
    if (rslt != SPI_FLASH_RESULT_OK)
    {
      initResult.addr = recordNumber;
      initResult.code = rslt;
      break;
    }
    else
    {
      if (rec.nWritten == 0xFFFFffff) // This slot is blank, quit here
      {
        nextCrashRecordSlot = recordNumber;
        break;
      }
      else if (rec.nReported != 0xFFFFffff) // This record has been reported
      {
        reportedRecords = recordNumber + 1;
      }
      else if (rec.reporter == BOOT_ERROR_SOURCE) { //this record is not reported and contains boot errors
         bootErrorRecordSlot = recordNumber;
         bootErrorRecordIndex = crashHandlerBootErrorCount(rec.dump);
      }
    }
  }
  
  if ((recordNumber > 0) && (reportedRecords == recordNumber)) // Have records but all reported
  {
    initResult.addr = CRASH_DUMP_SECTOR*SECTOR_SIZE;
    SpiFlashOpResult rslt = crashFlashErase();
    if (rslt != SPI_FLASH_RESULT_OK)
    {
      initResult.code = rslt;
    }
    else
    {
      nextCrashRecordSlot = 0;
    }
  }

  if (bootErrorRecordSlot < 0 || bootErrorRecordIndex >= BOOT_ERROR_MAX_ENTRIES)
  { //boot error block not found, use first available slot.
     bootErrorRecordSlot = nextCrashRecordSlot;
     bootErrorRecordIndex = 0;
  }

  initResult.n_rec = recordNumber;
  initResult.n_rep = reportedRecords;
  
  // See lx106 datasheet for details - NOTE: ALL XTENSA CORES USE DIFFERENT EXCEPTION NUMBERS
  _xtos_set_exception_handler(0, crash_handler);    // Bad instruction  
  _xtos_set_exception_handler(2, crash_handler);    // Bad instruction address
  _xtos_set_exception_handler(3, crash_handler);    // Bad load/store address
  _xtos_set_exception_handler(9, crash_handler);    // Bad load/store alignment  
  _xtos_set_exception_handler(28, crash_handler);   // Bad load address
  _xtos_set_exception_handler(29, crash_handler);   // Bad store address
}

void ICACHE_FLASH_ATTR crashHandlerShowStatus(){
  os_printf("Found %d crash logs, %d were reported\r\n",
            initResult.n_rec, initResult.n_rep);
  if (initResult.addr == CRASH_DUMP_SECTOR*SECTOR_SIZE) {
    if (initResult.code != SPI_FLASH_RESULT_OK)
    {
      os_printf("CH: Couldn't erase reported records in sector 0x%x, %d\r\n", CRASH_DUMP_SECTOR, initResult.code);
    }
    else
    {
      os_printf("CH: Erased Crash Flash");
    }
  }
  else if (initResult.code != 0) {
    os_printf("CH: Couldn't read existing records at idx %x, %d\r\n",
              initResult.addr, initResult.code);
  }
  else if (initResult.n_rec == MAX_CRASH_LOGS) {
     os_printf("CH: No slots available for new records\r\n");
  }
}


int ICACHE_FLASH_ATTR crashHandlerGetReport(const int index, CrashRecord* record)
{
  if (index < 0 || index >= MAX_CRASH_LOGS) return -1;
  if (record == NULL)
  {
    os_printf("crashHandlerGetReport: NULL record ptr\r\n");
    return -2;
  }
  const SpiFlashOpResult rslt = crashRecordRead(index, record);
  if (rslt == SPI_FLASH_RESULT_OK) return 0;
  else
  {
    os_printf("crashHandlerGetReport: Error reading record from flash at idx %x, %d\r\n", index, rslt);
    return rslt;
  }
}

/// Must not be ICACHE_FLASH_ATTR if we want to use in actual crash handler
int crashHandlerPutReport(CrashRecord* record)
{
  STACK_LEFT(true);
  if (nextCrashRecordSlot < 0 || nextCrashRecordSlot >= MAX_CRASH_LOGS) return -1;
  if (record == NULL) return -2;
  record->nWritten  = 0;
  record->nReported = 0xFFFFffff;
  const SpiFlashOpResult rslt = crashRecordWrite(nextCrashRecordSlot, record);
  if (rslt == SPI_FLASH_RESULT_OK)
  {
    const int ret = nextCrashRecordSlot;
    if (bootErrorRecordSlot == nextCrashRecordSlot) {
       bootErrorRecordSlot++;
    }
    nextCrashRecordSlot++;
    return ret;
  }
  else
  {
    return -3;
  }
}

int ICACHE_FLASH_ATTR crashHandlerMarkReported(const int index)
{
  if (index < 0 || index >= MAX_CRASH_LOGS) return -1;
  if (index == bootErrorRecordSlot)  { // if we are about to report our boot error log,
     bootErrorRecordIndex = 0;                   // move  future logging to beginning 
     bootErrorRecordSlot = nextCrashRecordSlot;  // of the next free slot
  }
  const uint32 recordAddr = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * index);
  uint32 nReported = 0;
  const SpiFlashOpResult rslt = crashFlashWrite(recordAddr + 4, &nReported, sizeof(nReported));
  return rslt;
}


void recordBootError(void* errorFunc, int32_t errorCode)
{
   os_printf("Recording Boot error %d @ %p\r\n", errorCode, errorFunc);
   SpiFlashOpResult rslt = SPI_FLASH_RESULT_OK;
   const uint32 recordAddr = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * bootErrorRecordSlot);
   
   if (bootErrorRecordSlot < 0 || bootErrorRecordSlot >= MAX_CRASH_LOGS ||
       bootErrorRecordIndex >= BOOT_ERROR_MAX_ENTRIES ) {
      return;
   }
   if  (bootErrorRecordSlot == nextCrashRecordSlot) {
      nextCrashRecordSlot++; //we are taking this slot, next crash goes one after.
   }
   if (bootErrorRecordIndex == 0) { //first entry
      uint32_t CrashHeader[4] = {0,0xFFFFffff, BOOT_ERROR_SOURCE, 0};
      rslt = crashFlashWrite(recordAddr, CrashHeader, sizeof(CrashHeader));
   }
   if (rslt == SPI_FLASH_RESULT_OK) {
      const int recordOffset = 16 + bootErrorRecordIndex * sizeof(CrashLog_BootError);
      CrashLog_BootError entry = { errorFunc, errorCode};
      rslt = crashFlashWrite(recordAddr + recordOffset, (uint32_t*)&entry, sizeof(entry));
   }
   if (rslt == SPI_FLASH_RESULT_OK) {
      bootErrorRecordIndex++;
   }
}


