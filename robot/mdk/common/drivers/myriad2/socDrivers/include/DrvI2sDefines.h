#ifndef _DRV_I2S_DEF_H_
#define _DRV_I2S_DEF_H_

/* **************************************************************************************** 
                               Register def
**************************************************************************************** */

// #define I2S_IER                          (0x000) 
// #define I2S_IRER                         (0x004) 
// #define I2S_IRER                         (0x004) 
// #define I2S_ITER                         (0x008) 
// #define I2S_CER                          (0x00c) 
// #define I2S_CCR                          (0x010) 
// #define I2S_RXFFR                        (0x014) 
// #define I2S_TXFFR                        (0x018) 
// #define I2S_LRBR0                        (0x020)  
// #define I2S_LTHR0                        (0x020)  
// #define I2S_RRBR0                        (0x024) 
// #define I2S_RTHR0                        (0x024)  
// #define I2S_RER0                         (0x028)  
// #define I2S_TER0                         (0x02c)  
// #define I2S_RCR0                         (0x030)  
// #define I2S_TCR0                         (0x034)  
// #define I2S_ISR0                         (0x038)  
// #define I2S_IMR0                         (0x03c) 
// #define I2S_ROR0                         (0x040) 
// #define I2S_TOR0                         (0x044)  
// #define I2S_RFCR0                        (0x048)  
// #define I2S_TFCR0                        (0x04c)  
// #define I2S_RFF0                         (0x050)  
// #define I2S_TFF0                         (0x054) 
// #define I2S_LRBR1                        (0x060)  
// #define I2S_LTHR1                        (0x060)  
// #define I2S_RRBR1                        (0x064) 
// #define I2S_RTHR1                        (0x064) 
// #define I2S_RER1                         (0x068)  
// #define I2S_TER1                         (0x06c) 
// #define I2S_RCR1                         (0x070)  
// #define I2S_TCR1                         (0x074)  
// #define I2S_ISR1                         (0x078) 
// #define I2S_IMR1                         (0x07c) 
// #define I2S_ROR1                         (0x080) 
// #define I2S_TOR1                         (0x084)  
// #define I2S_RFCR1                        (0x088) 
// #define I2S_TFCR1                        (0x08c)  
// #define I2S_RFF1                         (0x090)  
// #define I2S_TFF1                         (0x094) 
// #define I2S_LTHR2                        (0x0a0) 
// #define I2S_RTHR2                        (0x0a4) 
// #define I2S_TER2                         (0x0ac) 
// #define I2S_TCR2                         (0x0b4)  
// #define I2S_ISR2                         (0x0b8) 
// #define I2S_IMR2                         (0x0bc) 
// #define I2S_TOR2                         (0x0C4) 
// #define I2S_TFCR2                        (0x0cc)  
// #define I2S_TFF2                         (0x0d4) 
// #define I2S_RXDMA                        (0x1c0) 
// #define I2S_RRXDMA                       (0x1c4) 
// #define I2S_TXDMA                        (0x1c8) 
// #define I2S_RTXDMA                       (0x1cc) 
// #define I2S_COMP_PARAM_2                 (0x1f0) 
// #define I2S_COMP_PARAM_1                 (0x1f4)  
// #define I2S_COMP_VERSION                 (0x1f8) 
// #define I2S_COMP_TYPE                    (0x1fc) 

/* **************************************************************************************** 
                               register fields defines
**************************************************************************************** */
/* **************************************************************************************** 
                                CCR
**************************************************************************************** */
//bit 0:2
#define D_I2S_SCLKG_NO      (0x000)
#define D_I2S_SCLKG_12CC    (0x001)
#define D_I2S_SCLKG_16CC    (0x002)
#define D_I2S_SCLKG_20CC    (0x003)
#define D_I2S_SCLKG_24CC    (0x004)
// bit 3:4
#define D_I2S_WSS_16CC      (0x0000) 
#define D_I2S_WSS_24CC      (0x0008) 
#define D_I2S_WSS_32CC      (0x0010) 

/* **************************************************************************************** 
                                TCR / RCR
**************************************************************************************** */
// bit 0:2
#define D_I2S_TCRRCR_WLEN_IGNORE  (0x00)
#define D_I2S_TCRRCR_WLEN_12BIT   (0x01)
#define D_I2S_TCRRCR_WLEN_16BIT   (0x02)
#define D_I2S_TCRRCR_WLEN_20BIT   (0x03)
#define D_I2S_TCRRCR_WLEN_24BIT   (0x04)
#define D_I2S_TCRRCR_WLEN_32BIT   (0x05)

/* **************************************************************************************** 
                           IMR
**************************************************************************************** */
//bit 0
#define D_I2S_IMR_RXDAM     (0x0001)
//bit 1
#define D_I2S_IMR_RXFOM     (0x0002)
// bit 4
#define D_I2S_IMR_TXFEM     (0x0010)
//bit 5
#define D_I2S_IMR_TXFOM     (0x0020)


/* **************************************************************************************** 
                    General
**************************************************************************************** */
#define D_I2S_TX     (0x01)
#define D_I2S_RX     (0x00)

#define D_I2S_MASTER (0x01)
#define D_I2S_SLAVE  (0x00)



#define D_I2S_CH0   (1<<0)
#define D_I2S_CH1   (1<<1)
#define D_I2S_CH2   (1<<2)



#endif //_DRV_I2S_DEF_H_

