#ifndef NV_RESET_H
#define NV_RESET_H

#include "hal/portable.h"

//Max data size (bytes) nvReset can handle
#define NV_RESET_MAX_LEN  0x400

//Soft Resets the MCU. User data is stored in no-init ram for recovery at boot. dat==NULL clears stored data.
void nvReset(u8 *dat, u16 len);

//@return valid data size (bytes) from last nvReset. -1 if no data exists (e.g. POR).
int nvResetGetLen(void);

//Try get data stored by the last nvReset() call. Data is written to user buffer (if non-NULL), up to the user's max len (safe).
//@return 'len' data bytes available from last reset
//@return -1 if no data exists.
int nvResetGet(u8 *out_dat, u16 max_out_len);


//inspect/print user data. NULL prints internal stored data.
//prefix - string ptr to console prefix, before data dump
void nvResetDbgInspect(char* prefix, u8 *dat, u16 len);

//inspect the nv memory region (advanced debug. view internal mechanisms)
void nvResetDbgInspectMemRegion(void);


#endif

