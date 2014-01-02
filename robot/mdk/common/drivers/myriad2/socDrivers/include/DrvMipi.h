#ifndef DRV_MIPI_H
#define DRV_MIPI_H

#include "DrvMipiDefines.h"

void DrvMipiTestClear(u8 MipiCtrlNo);
void DrvMipiTestClearAll();
u8 DrvMipiTestModeSendCmd(u8 MipiCtrlNo, u8 Command, u8 *DataIn, u8 *DataOut, u8 Length);
u32 DrvMipiPllProg(u32 mipiCtrlNo, u32 refClkKHz, u32 desiredClkKHz);
int DrvMipiWaitPllLock(u32 mipiCtrlNo, u32 timeout);
u32 DrvMipiInitDphy(u32 mipiDphyNo);
u32 DrvMipiTxLPData(u32 mipiLaneNo, u32 data);

#endif