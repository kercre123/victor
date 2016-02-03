/**
 * Espressif crash handler - will eventually help us log crashes for later transmission to client
 * Derived from MIT-licensed code in nodemcu (no copyright restrictions)
 */
 
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/crash.h"
#include "driver/uart.h"
#include "transport/reliableTransport.h"
#include "anki/cozmo/robot/version.h" // Must only be included in one place

#define DUMP_TO_UART 1
#define DUMP_TO_RTC_MEM 1

#define RTC_MEM_START (64)

static const unsigned int CRASH_DUMP_STORAGE_HEADER = 0xCDFAF320;

extern ReliableConnection g_conn;   // So we can check canaries when we crash

void os_put_str(char* str)
{
  while (*str != 0)
  {
    os_put_char(*str);
    str++;
  }
}

#define P4X(s,n) os_put_str(s); os_put_hex(n, 4);
#define P8X(s,n) os_put_str(s); os_put_hex((int)(n), 8);

#define STACK_DUMP_NUM_COLS 8
#define STACK_DUMP_MAX_SIZE 256

void crash_dump_hex(int* p, int cnt) {
  int i;
  for (i = 0; i < cnt; i++) {
    if (i % STACK_DUMP_NUM_COLS == 0) {
      os_put_hex((int )(p + i), 8);
      os_put_char('|');
    }
    os_put_hex(p[i], 8);
    os_put_char((i + 1) % STACK_DUMP_NUM_COLS ? ' ' : '\n');
  }
}

#define STACKOK(i) (((unsigned int)i > 0x3fffc000) && ((unsigned int)i < 0x40000000) && (((unsigned int)i & 3) == 0))

extern void crash_handler(int exccause, ex_regs regs);

void crash_dump_stack_uart(int* sp) {
  int* p = sp - 8; // @nathan-anki Should we really be subtracting 8 again here?
  int n = (0x40000000 - (unsigned int)p);
  if (n > STACK_DUMP_MAX_SIZE) {
    n = STACK_DUMP_MAX_SIZE;
  }
  os_put_str("Stack (");
  os_put_hex((unsigned int)sp, 8);
  os_put_str(")\n");
  crash_dump_hex(p, n / 4);
}

unsigned int crash_dump_stack_rtc(int* sp, unsigned int waddr) {
  int* p = sp - 8; // @nathan-anki Should we really be subtracting 8 again here?
  int n = (0x40000000 - (unsigned int)p);
  if (n > STACK_DUMP_MAX_SIZE) {
    n = STACK_DUMP_MAX_SIZE;
  }
  system_rtc_mem_write(waddr++, &n, 4);
  system_rtc_mem_write(waddr, p, n);
  waddr += n/4;
  return waddr;
}

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

extern void crash_dump(int* sp) {
  ex_regs *regs = (ex_regs*) sp;
  // stack pointer at exception place
  int* ex_sp = (sp + 256 / 4);
  int* p = ex_sp - 8;
  int usestack = STACKOK(p);

  os_put_str("Fatal Exception: ");
  os_put_hex(regs->exccause, 4);
  os_put_str(" (");
  os_put_hex(regs->exccause, 2);
  os_put_str("), sp ");
  os_put_hex((int)sp, 8);
  os_put_char('\n');

#if DUMP_TO_RTC_MEM
  unsigned int rtcWadder = RTC_MEM_START;
  system_rtc_mem_write(rtcWadder++, &CRASH_DUMP_STORAGE_HEADER, 4);
  system_rtc_mem_write(rtcWadder++, &COZMO_VERSION_COMMIT, 4);
  system_rtc_mem_write(rtcWadder++, &COZMO_BUILD_DATE, 4);
  system_rtc_mem_write(rtcWadder,   regs, sizeof(ex_regs));
  rtcWadder += sizeof(ex_regs)/4;
  unsigned int canaries = g_conn.canary1 | (g_conn.canary2 << 8) | (g_conn.canary3 << 16);
  system_rtc_mem_write(rtcWadder++, &canaries, 4);
  if (usestack)
  {
    int exdepc[2];
    exdepc[0] = get_excvaddr();
    exdepc[1] = get_depc();
    system_rtc_mem_write(rtcWadder, exdepc, 8);
    rtcWadder += 2;
    crash_dump_stack_rtc(p, rtcWadder);
  }
  else
  {
    int zero = 0;
    system_rtc_mem_write(rtcWadder, &zero, 4);
  }

#endif // DUMP_TO_RTC_MEM

#if DUMP_TO_UART
  os_put_str("RT canaries: ");
  os_put_hex(g_conn.canary1, 2);
  os_put_hex(g_conn.canary2, 2);
  os_put_hex(g_conn.canary3, 2);
  os_put_char('\n');
  
  if (usestack) {
    int excvaddr = get_excvaddr();
    int depc = get_depc();
    os_put_str("Fingerprint: 1/");
    P8X("xh=", crash_handler);
    P8X(",v=0x", COZMO_VERSION_COMMIT);
    P8X(",b=0x", COZMO_BUILD_DATE);
    os_put_char('\n');
    P8X(" epc1: ", regs->epc1); P8X("  exccause: ", regs->exccause); P8X("  excvaddr: ", excvaddr);
    P8X("  depc: ", depc);
    os_put_char('\n');
    P8X(" ps  : ", regs->ps);   P8X("  sar     : ", regs->sar);       P8X("  unk1    : ", regs->xx1);
    os_put_char('\n');
    // a1 is stack at exception
    P8X(" a0 :  ", regs->a0);
    P8X("  a1 :  ", (int )ex_sp);
    P8X("  a2 :  ", regs->a2);
    P8X("  a3 :  ", regs->a3);
    os_put_char('\n');
    P8X(" a4 :  ", regs->a4);
    P8X("  a5 :  ", regs->a5);
    P8X("  a6 :  ", regs->a6);
    P8X("  a7 :  ", regs->a7);
    os_put_char('\n');
    P8X(" a8 :  ", regs->a8);
    P8X("  a9 :  ", regs->a9);
    P8X("  a10:  ", regs->a10);
    P8X("  a11:  ", regs->a11);
    os_put_char('\n');
    P8X(" a12:  ", regs->a12);
    P8X("  a13:  ", regs->a13);
    P8X("  a14:  ", regs->a14);
    P8X("  a15:  ", regs->a15);
    os_put_char('\n');
    crash_dump_stack_uart(p);
  } else {
    os_put_str("Stack pointer may be corrupted: ");
    os_put_hex((int)sp, 4);
    os_put_char('\n');
  }
#endif // DUMP_TO_UART

  while (1);    // Wait for watchdog to get us
}

/*
 * a2 contains exception cause, current stack contains register dump.
 * a3 and a4 may be used because they are not used by standard handler.
 */
//__attribute__((noinline))
void crash_handler(int exccause, ex_regs regs) {
  asm("addi a1, a1, -16");    // Adjust stack to point to crash registers
  asm("s32i.n  a0, a1, 12");   // Store a0 in crash registers
  asm("addi a2, a1, 16");     // Pass stack as first argument to crash_dump
  asm("j crash_dump");        // Proceed to crash_dump - never returning
  // This code never returns
}
extern void* _xtos_set_exception_handler(int exno, void (*exhandler)());

// Register crash handler with XTOS - must be called before 
void ICACHE_FLASH_ATTR crashHandlerInit(void)
{
  // See lx106 datasheet for details - NOTE: ALL XTENSA CORES USE DIFFERENT EXCEPTION NUMBERS
  _xtos_set_exception_handler(0, crash_handler);    // Bad instruction  
  _xtos_set_exception_handler(2, crash_handler);    // Bad instruction address
  _xtos_set_exception_handler(3, crash_handler);    // Bad load/store address 
  _xtos_set_exception_handler(9, crash_handler);    // Bad load/store alignment  
  _xtos_set_exception_handler(28, crash_handler);   // Bad load address
  _xtos_set_exception_handler(29, crash_handler);   // Bad store address
}

extern void FacePrintf(const char *format, ...); // Forward declaration

bool ICACHE_FLASH_ATTR crashHandlerHasReport(void)
{
  unsigned int header;
  system_rtc_mem_read(RTC_MEM_START, &header, 4);
  if(header == CRASH_DUMP_STORAGE_HEADER)
  {
    static const char crashMessageFormat[] ICACHE_RODATA_ATTR = "CRASH REPORT AVAILABLE\npc = 0x%x";
    const uint32_t fmtSz = ((sizeof(crashMessageFormat)+3)/4)*4;
    char fmtBuf[fmtSz];
    unsigned int pc;
    system_rtc_mem_read(RTC_MEM_START + 3, &pc, 4);
    os_memcpy(fmtBuf, crashMessageFormat, fmtSz);
    FacePrintf(fmtBuf, pc);
    return true;
  }
  else
  {
    return false;
  }
}

bool ICACHE_FLASH_ATTR crashHandlerClearReport(void)
{
  int zero = 0;
  return system_rtc_mem_write(RTC_MEM_START, &zero, 4);
}

int ICACHE_FLASH_ATTR crashHandlerGetReport(uint32_t* dest, const int available)
{
  const int readLen = available < MAX_CRASH_REPORT_SIZE ? available : MAX_CRASH_REPORT_SIZE;
  if (system_rtc_mem_read(RTC_MEM_START, dest, readLen))
  {
    return readLen;
  }
  else
  {
    return -1;
  }
}
