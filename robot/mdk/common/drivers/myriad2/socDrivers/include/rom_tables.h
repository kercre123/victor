#ifndef _SHAVE_UTILS_H_
#define _SHAVE_UTILS_H_

// aac tables
// source: Nelu Gheorghiu
extern const unsigned __SHAVE_AAC_table_1[24*4+3];
extern const unsigned __SHAVE_AAC_table_3[31*4+2];
extern const unsigned __SHAVE_AAC_table_4[66*4];

// mpeg2 tables
// source: Lucian Mirci
extern const unsigned __SHAVE_MPEG2_Table[1070];

// data coefficients CAVLC H.264
// source: Paul Giucoane
extern const unsigned __SHAVE_dataCoefCAVLCh264[1224];

// resize filter coefficients
// source: Lucian Mirci
extern const unsigned __SHAVE_ResizeCoef[96];

#endif
