/**
 * Espressif crash handler - will eventually help us log crashes for later transmission to client
 * Derived from MIT-licensed code in nodemcu (no copyright restrictions)
 */
 
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"

void c_puts(char* s)
{
  while (*s != 0)
    uart_tx_one_char(UART0, *s++);
}

typedef struct {
	int epc1;
	int ps;
	int sar;
	int xx1;
	int a0;
	int a2;
	int a3;
	int a4;
	int a5;
	int a6;
	int a7;
	int a8;
	int a9;
	int a10;
	int a11;
	int a12;
	int a13;
	int a14;
	int a15;
	int exccause;
} ex_regs;

/* use some link symbols to get a kind of firmware fingerprint */
extern char _text_start;
extern char _text_end;
extern char _rodata_start;
extern char _rodata_end;
extern char _data_start;
extern char _data_end;
extern char _bss_start;
extern char _bss_end;

extern void crash_handler(int exccause, ex_regs regs);

#define L(x) (((unsigned int)(x)) & 0xFFFF)

static char TINYBUF[120];

#define cal_tprintfOFF(fmt,...) do {					\
	ets_vsprintf(TINYBUF, fmt, __VA_ARGS__);	\
	c_puts(TINYBUF);					\
} while(0)
#define P4X(s,n) c_puts(s); print4x(n);
#define P8X(s,n) c_puts(s); print8x((int)(n));

// quick and dirty print hex bytes to avoid printf.
void print8x(unsigned int n) {
	int i = 0;
	for (i = 0; i < 8; i++) {
		int nib = n & 0xf;
		TINYBUF[7-i] = (nib < 10) ? '0' + nib : 'a' - 10 + nib;
		n = n>>4;
	}
	TINYBUF[8] = 0;
	c_puts(TINYBUF);
}
void print4x(unsigned int n) {
	int i = 0;
	for (i = 0; i < 4; i++) {
		int nib = n & 0xf;
		TINYBUF[3-i] = (nib < 10) ? '0' + nib : 'a' - 10 + nib;
		n = n>>4;
	}
	TINYBUF[4] = 0;
	c_puts(TINYBUF);
}
void print2d(unsigned int n) {
	int i = 0;
	for (i = 0; i < 2; i++) {
		int nib = n % 10;
		TINYBUF[1-i] = '0' + nib;
		n = n / 10;
	}
	TINYBUF[2] = 0;
	c_puts(TINYBUF);
}

#define NUM 8

// TODO: could use TINYBUF per line
//__attribute__((noinline))
void crash_dump_hex(int* p, int cnt) {
	int i;
	for (i = 0; i < cnt; i++) {
		if (i % NUM == 0) {
			print8x((int )(p + i));
			c_puts(" ");
		}
		c_puts(" ");
		print8x(p[i]);
		if ((i + 1) % NUM == 0) {
			c_puts("\n");
		}
	}
}

int stackok(int* sp) {
	int i = (unsigned int) sp;
	return (i > 0x3fffc000) && (i < 0x40000000) && ((i & 3) == 0);
}

//__attribute__((noinline))
void crash_dump_stack(int* sp) {
	int* p = sp - 8;
	if (stackok(p)) {
		int n = (0x40000000 - (unsigned int)p);
		if (n > 512) {
			n = 512;
		}
		c_puts("Stack (");
		print8x((unsigned int)sp);
		c_puts(")\n");
		crash_dump_hex(p, n / 4);
	} else {
		c_puts("Stack pointer may be corrupt: ");
		print8x((unsigned int)sp);
		c_puts("\n");
	}
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

//__attribute__((noinline))
extern void crash_dump(int* sp) {
	ex_regs *regs = (ex_regs*) sp;
	// stack pointer at exception place
	int* ex_sp = (sp + 256 / 4);
	int* p = ex_sp - 8;
	int usestack = stackok(p);

	c_puts("Fatal Exception: ");
	print4x(regs->exccause);
	c_puts(" (");
	print2d(regs->exccause);
	c_puts("), sp ");
	print8x((int)sp);
	c_puts("\n");

  if (usestack) {
		int excvaddr = get_excvaddr();
		int depc = get_depc();
		c_puts("Fingerprint: 1/");
		P8X("xh=", crash_handler);
		P4X(",t=", L(&_text_start)); P4X("-", L(&_text_end));
		P4X(",d=", L(&_data_start)); P4X("-", L(&_data_end));
		P4X(",b=", L(&_bss_start)); P4X("-", L(&_bss_end));
		P4X(",ro=", L(&_rodata_start)); P4X("-", L(&_rodata_end));
		c_puts("\n");
		P8X(" epc1: ", regs->epc1); P8X("  exccause: ", regs->exccause); P8X("  excvaddr: ", excvaddr);
		P8X("  depc: ", depc);
		c_puts("\n");
		P8X(" ps  : ", regs->ps);   P8X("  sar     : ", regs->sar);	     P8X("  unk1    : ", regs->xx1);
		c_puts("\n");
		// a1 is stack at exception
		P8X(" a0 :  ", regs->a0);
		P8X("  a1 :  ", (int )ex_sp);
		P8X("  a2 :  ", regs->a2);
		P8X("  a3 :  ", regs->a3);
		c_puts("\n");
		P8X(" a4 :  ", regs->a4);
		P8X("  a5 :  ", regs->a5);
		P8X("  a6 :  ", regs->a6);
		P8X("  a7 :  ", regs->a7);
		c_puts("\n");
		P8X(" a8 :  ", regs->a8);
		P8X("  a9 :  ", regs->a9);
		P8X("  a10:  ", regs->a10);
		P8X("  a11:  ", regs->a11);
		c_puts("\n");
		P8X(" a12:  ", regs->a12);
		P8X("  a13:  ", regs->a13);
		P8X("  a14:  ", regs->a14);
		P8X("  a15:  ", regs->a15);
		c_puts("\n");
		crash_dump_stack(p);
	} else {
		c_puts("Stack pointer may be corrupted: ");
		print8x((int)sp);
		c_puts("\n");
	}
  
  while (1);    // Wait for watchdog to get us
}

/*
 * a2 contains exception cause, current stack contains register dump.
 * a3 and a4 may be used because they are not used by standard handler.
 */
//__attribute__((noinline))
void crash_handler(int exccause, ex_regs regs) {
	asm("addi a1, a1, -16");    // Adjust stack to point to crash registers
  asm("s32i.n	a0, a1, 12");   // Store a0 in crash registers
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
