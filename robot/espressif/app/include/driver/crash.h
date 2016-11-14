/** @file Custom crash handler for Espressif
 * Handles firmware crashes, logs them and provides for reporing them later
 * @author Nathan Monson <nathan@anki.com>
 */
 
#ifndef __ESPRESSIF_DRIVER_CRASH_H_
#define __ESPRESSIF_DRIVER_CRASH_H_

#include "anki/cozmo/robot/flash_map.h"
#include "anki/cozmo/robot/crashLogs.h"

#define MAX_CRASH_LOGS (SECTOR_SIZE / CRASH_RECORD_SIZE)

/** Initlize the custom crash handler
 * Erases any already reported records.
 */
void crashHandlerInit(void);

/** Fetches the stored report of given indexinto the buffer
 * @param[in]  index which crash record index to fetch from. Must be in range [0, MAX_CRASH_LOGS)
 * @param[out] record A pointer to a CrashRecord structure to copy the record from flash into
 * @return 0 on success or other on failure
 */
int crashHandlerGetReport(const int index, CrashRecord* record);

/** Writes a crash record manually into flash
 * This is for storing crash records from the other processor for later reporting
 * @param[in] record A pointer to the record to write
 * @return the index the report was written true on success or a negative number on error
 */
int crashHandlerPutReport(CrashRecord* record);

/** Marks the crash report at index as reported so it may be erased later.
 * @param index The report index to mark reported
 * @return 0 on success or other on error
 */
int crashHandlerMarkReported(const int index);

void crashHandlerShowStatus(void);

/** Stores the location and value of an error occurring during boot
 * @param func_addr The function that returned an error code
 * @param error_code The error code 
 */
void recordBootError(void* func_addr, int32_t error_code);
int crashHandlerBootErrorCount(uint32_t* dump_data);

/** Sets the App run id to be included in crash logs
 * @param app_run_id[in] 128 bit app run ID as array of 4 uint32_t
 */
void setAppRunID(uint32_t* app_run_id);

#endif
