#ifndef HDMITXITE_PRIVATE_H
#define HDMITXITE_PRIVATE_H

int HDMITX_WriteI2C_Byte(int RegAddr, u8 value);
u8 HDMITX_ReadI2C_Byte(int RegAddr);
void AbortDDC(void);
SYS_STATUS HDMITX_WriteI2C_ByteN(SHORT RegAddr, BYTE *pData, int N);
SYS_STATUS HDMITX_ReadI2C_ByteN(BYTE RegAddr, BYTE *pData, int N);
void ClearDDCFIFO(void);
void SetTxAVMute (void);
void ClrTxAVMute(void);

#endif
