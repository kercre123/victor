#ifndef _BRINGUP_SABRE_IIC_DEF_H_
#define _BRINGUP_SABRE_IIC_DEF_H_

#define IIC_CON                          (0x00) 
#define IIC_TAR                          (0x04) 
#define IIC_SAR                          (0x08) 
#define IIC_HS_MADDR                     (0x0c) 
#define IIC_DATA_CMD                     (0x10) 
#define IIC_SS_HCNT                      (0x14) 
#define IIC_SS_LCNT                      (0x18) 
#define IIC_FS_HCNT                      (0x1c) 
#define IIC_FS_LCNT                      (0x20) 
#define IIC_HS_HCNT                      (0x24) 
#define IIC_HS_LCNT                      (0x28) 
#define IIC_INTR_STAT                    (0x2c) 
#define IIC_INTR_MASK                    (0x30) 
#define IIC_RAW_INTR_STAT                (0x34)  
#define IIC_RX_TL                        (0x38)  
#define IIC_TX_TL                        (0x3c)  
#define IIC_CLR_INTR                     (0x40) 
#define IIC_CLR_RX_UNDER                 (0x44) 
#define IIC_CLR_RX_OVER                  (0x48) 
#define IIC_CLR_TX_OVER                  (0x4c) 
#define IIC_CLR_RD_REQ                   (0x50) 
#define IIC_CLR_TX_ABRT                  (0x54) 
#define IIC_CLR_RX_DONE                  (0x58) 
#define IIC_CLR_ACTIVITY                 (0x5c) 
#define IIC_CLR_STOP_DET                 (0x60) 
#define IIC_CLR_START_DET                (0x64)  
#define IIC_CLR_GEN_CALL                 (0x68) 
#define IIC_ENABLE                       (0x6c) 
#define IIC_STATUS                       (0x70) 
#define IIC_TXFLR                        (0x74)  
#define IIC_RXFLR                        (0x78)  
#define IIC_SRESET                       (0x7c) 
#define IIC_TX_ABRT_SOURCE               (0x80)  
#define IIC_VERSION_ID                   (0xf8) 
#define IIC_DMA_CR                       (0x88) 
#define IIC_DMA_TDLR                     (0x8c) 
#define IIC_DMA_RDLR                     (0x90) 
#define IIC_COMP_PARAM_1                 (0xf4) 
#define IIC_COMP_VERSION                 (0xf8) 
#define IIC_COMP_TYPE                    (0xfc)

/* IIC_RAW_INTR_STAT */
#define IIC_IRQ_GEN_CALL                 0x0800
#define IIC_IRQ_START_DET                0x0400
#define IIC_IRQ_STOP_DET                 0x0200
#define IIC_IRQ_ACTIVITY                 0x0100
#define IIC_IRQ_RX_DONE                  0x0080
#define IIC_IRQ_TX_ABRT                  0x0040
#define IIC_IRQ_RD_REQ                   0x0020
#define IIC_IRQ_TX_EMPTY                 0x0010
#define IIC_IRQ_TX_OVER                  0x0008
#define IIC_IRQ_RX_FULL                  0x0004
#define IIC_IRQ_RX_OVER                  0x0002
#define IIC_IRQ_RX_UNDER                 0x0001

/* IIC_STATUS */
#define IIC_STATUS_ACTIVITY_SLAVE         (0x40)
#define IIC_STATUS_ACTIVITY_MASTER        (0x20)
#define IIC_STATUS_RX_FIFO_FULL           (0x10)
#define IIC_STATUS_RX_FIFO_NOT_EMPTY      (0x08)
#define IIC_STATUS_TX_FIFO_EMPTY          (0x04)
#define IIC_STATUS_TX_FIFO_NOT_FULL       (0x02)
#define IIC_STATUS_ACTIVITY               (0x01)


/******************************************************************************
 3:  Enums and structs shared by both I2C slave component and drvMaster
******************************************************************************/

typedef enum
{
    I2CM_STAT_OK                        =  0,
    I2CM_STAT_ERR                       = -1,
    I2CM_STAT_TIMEOUT                   = -2,
    I2CM_STAT_INVALID_MODULE            = -3,
    I2CM_STAT_INVALID_PIN               = -4,
    I2CM_STAT_INVALID_SPEED             = -5,
    I2CM_STAT_SDA_STUCK_LOW             = -6,
    I2CM_STAT_SCL_STUCK_LOW             = -7,
    I2CM_STAT_TX_ABORT                  = -8,
    I2CM_STAT_FIFO_ERROR                = -9,
    I2CM_STAT_ABRT_7B_ADDR_NOACK        = -10,
    I2CM_STAT_ABRT_10ADDR1_NOACK        = -11,
    I2CM_STAT_ABRT_10ADDR2_NOACK        = -12,
    I2CM_STAT_ABRT_TXDATA_NOACK         = -13,
    I2CM_STAT_ABRT_GCALL_NOACK          = -14,
    I2CM_STAT_ABRT_GCALL_READ           = -15,
    I2CM_STAT_ABRT_HS_ACKDET            = -16,
    I2CM_STAT_ABRT_SBYTE_ACKDET         = -17,
    I2CM_STAT_ABRT_HS_NORSTRT           = -18,
    I2CM_STAT_ABRT_SBYTE_NORSTRT        = -19,
    I2CM_STAT_ABRT_10B_RD_NORSTRT       = -20,
    I2CM_STAT_ABRT_MASTER_DIS           = -21,
    I2CM_STAT_ABRT_LOST                 = -22,
    I2CM_STAT_ABRT_SLV_FLUSH_TXFIFO     = -23,
    I2CM_STAT_ABRT_SLV_ARBLOST          = -24,
    I2CM_STAT_ABRT_SLVDR_INTX           = -25,
    I2CM_STAT_ABRT_ERROR                = -26,  // TXABORT, but source didn't tell us why
    I2CM_STAT_NOT_INIT                  = -27,
    I2CM_STAT_VERIFY_FAIL               = -28,
    I2CM_STAT_WRITE_UNDERFLOW           = -29,
    I2CM_STAT_READ_UNDERFLOW            = -30
} I2CM_StatusType;


typedef u32 (*I2CErrorHandler) (I2CM_StatusType error, u32 param1, u32 param2);

typedef enum
{
    ADDR_7BIT,
    ADDR_10BIT
} I2CMAddrType;


typedef enum          // Note: These values are defined by hardware
{
    I2CM_SPEED_SLOW=1, // < 100Khz
    I2CM_SPEED_FAST=2, // < 400Khz
    I2CM_SPEED_HIGH=3 // < 3400Khz
} I2CMSpeedMode;

typedef enum
{
    STOPPED  = 0,
    STARTED  = 1
} I2CMProtoStartedType;


typedef struct
{
    u32                     i2cDeviceAddr;
    u32                     sclPin;
    u32                     sdaPin;
    u32                     sclModeRegAddr;          // Used as a cache for performance reasons
    u32                     sdaModeRegAddr;          // Used as a cache for performance reasons
    u32                     sclRawRegAddr;          // Used as a cache for performance reasons
    u32                     sdaRawRegAddr;          // Used as a cache for performance reasons
    u32                     sclRawOffset;           // Used as a cache for performance reasons
    u32                     sdaRawOffset;           // Used as a cache for performance reasons
    I2CMSpeedMode           speedMode;
    I2CMAddrType            addrSize;
    u32                     timeoutUs;
    u32                     maxAckPoll;
    I2CErrorHandler         errorFunc;
    u32                     clksPerI2cCycle;
    u32                     clksPerHalfCycle;        // Used as a cache for performance reasons
    u32                     clksPerQuarterCycle;     // Used as a cache for performance reasons
    u32                     initialisedMagic;
    I2CMProtoStartedType    stateProtoStart;
    u32                     stateError;
    u32                     speedKhz;
} I2CM_Device;


#endif
