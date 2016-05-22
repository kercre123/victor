/** @file Custom crash handler for Espressif
 * Handles firmware crashes, logs them and provides for reporing them later
 * @author Nathan Monson <nathan@anki.com>
 */
 
#ifndef __ESPRESSIF_DRIVER_CRASH_H_
#define __ESPRESSIF_DRIVER_CRASH_H_

#define MAX_CRASH_REPORT_SIZE (512)

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

/// Initlize the custom crash handler
void crashHandlerInit(void);

/// Returns true if there is a logged crash to report
bool crashHandlerHasReport(void);

/// Clears the stored crash report
bool crashHandlerClearReport(void);

/// Fetches the stored report into the buffer
int crashHandlerGetReport(uint32_t* dest, const int available);

#endif
