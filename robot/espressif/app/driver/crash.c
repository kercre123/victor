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

extern ReliableConnection g_conn;   // So we can check canaries when we crash

extern int COZMO_VERSION_ID; // Initalized in factoryData.c

static int nextCrashRecordSlot;

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

// Register crash handler with XTOS - must be called before 
void ICACHE_FLASH_ATTR crashHandlerInit(void)
{
  int reportedRecords = 0;
  int recordNumber;
  nextCrashRecordSlot = -1; // Invalid
  initResult.addr = 0;
  initResult.code = 0;
  
  // Find how many records we have written and how many we have reported
  for (recordNumber=0; recordNumber<MAX_CRASH_LOGS; ++recordNumber)
  {
    CrashRecord rec;
    const uint32 recordAddress = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * recordNumber);
    const SpiFlashOpResult rslt = spi_flash_read(recordAddress, (uint32*)&rec, CRASH_RECORD_SIZE);
    if (rslt != SPI_FLASH_RESULT_OK)
    {
      initResult.addr = recordAddress;
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
    }
  }
  
  if ((recordNumber > 0) && (reportedRecords == recordNumber)) // Have records but all reported
  {
    initResult.addr = CRASH_DUMP_SECTOR;
    SpiFlashOpResult rslt = spi_flash_erase_sector(CRASH_DUMP_SECTOR);
    if (rslt != SPI_FLASH_RESULT_OK)
    {
      initResult.code = rslt;
    }
    else
    {
      nextCrashRecordSlot = 0;
    }
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
  if (initResult.addr == CRASH_DUMP_SECTOR) {
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
    os_printf("CH: Couldn't read existing records at 0x%x, %d\r\n",
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
  const uint32 recordAddr = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * index);
  const SpiFlashOpResult rslt = spi_flash_read(recordAddr, (uint32*)record, CRASH_RECORD_SIZE);
  if (rslt == SPI_FLASH_RESULT_OK) return 0;
  else
  {
    os_printf("crashHandlerGetReport: Error reading record from flash at 0x%x, %d\r\n", recordAddr, rslt);
    return rslt;
  }
}

/// Must not be ICACHE_FLASH_ATTR if we want to use in actual crash handler
int crashHandlerPutReport(CrashRecord* record)
{
  STACK_LEFT(true);
  if (nextCrashRecordSlot < 0 || nextCrashRecordSlot >= MAX_CRASH_LOGS) return -1;
  if (record == NULL) return -2;
  const uint32 recordWriteAddress = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * nextCrashRecordSlot);
  record->nWritten  = 0;
  record->nReported = 0xFFFFffff;
  const SpiFlashOpResult rslt = spi_flash_write(recordWriteAddress, (uint32*)record, CRASH_RECORD_SIZE);
  if (rslt == SPI_FLASH_RESULT_OK)
  {
    const int ret = nextCrashRecordSlot;
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
  const uint32 recordAddr = (CRASH_DUMP_SECTOR * SECTOR_SIZE) + (CRASH_RECORD_SIZE * index);
  uint32 nReported = 0;
  const SpiFlashOpResult rslt = spi_flash_write(recordAddr + 4, &nReported, 4);
  return rslt;
}
