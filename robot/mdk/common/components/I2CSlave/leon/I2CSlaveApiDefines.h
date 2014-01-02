///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the I2C Slave
///

#ifndef _I2C_SLAVE_DEFINES_H
#define _I2C_SLAVE_DEFINES_H

#include "DrvI2cDefines.h"

// 1: Defines
// ----------------------------------------------------------------------------

#ifndef I2CSLAVE_DATA_SECTION
#define I2CSLAVE_DATA_SECTION(x) x
#endif

#ifndef I2CSLAVE_CODE_SECTION
#define I2CSLAVE_CODE_SECTION(x) x
#endif

#ifndef I2CSLAVE_INTERRUPT_LEVEL
#define I2CSLAVE_INTERRUPT_LEVEL 10
#endif

//#ifdef I2C2
//#define GET_I2C_REG(reg)         GET_REG_WORD_VAL(IIC2_BASE_ADR + reg)
//#define SET_I2C_REG(reg, val)    SET_REG_WORD(IIC2_BASE_ADR + reg, val)
//#else
//#define GET_I2C_REG(reg)         GET_REG_WORD_VAL(IIC1_BASE_ADR + reg)
//#define SET_I2C_REG(reg, val)    SET_REG_WORD(IIC1_BASE_ADR + reg, val)
//#endif

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef struct
{
    u32 device;
    u8  sclPin;
    u8  sdaPin;
    u32 speedKhz;
    u32 addressSize;
}I2cSLaveConfig;



typedef enum
{
    READ_I2C,
    WRITE_I2C,
    END_I2C
}I2cSlaveAction;

typedef void (i2cReadAction)(u32 reg, u32* val);
typedef void (i2cWriteAction)(u32 reg, u32 val, u32 valNo);
typedef u32 (i2cBytesToSend)(u32 reg);

typedef struct
{
    I2CM_Device *i2cDevHandle;
    I2cSLaveConfig i2cConfig;
    i2cReadAction* cbReadAction;
    i2cWriteAction* cbWriteAction;
    i2cBytesToSend* cbBytesToSend; //returns number of bytes to read in bulk mode
    u32 irqSource;
}I2CSlaveHandle;


typedef struct
{
    u32 slvAddr;
    u32 regMemAddr;
    u32 regMemSize;
    u32 dataByteNo;
}I2CSlaveAddrCfg;

typedef struct
{
    I2CSlaveHandle *i2cHndl;
    u32* regSpaceMemory;
    u32 regMemSize;
    u32 addrReg;
    u32 addrSize;
    u32 dataSize;
    u32 i2cState;
    int i2CCmd;
    u32 volatile status;
    u32 receivedBytes;
}I2CStat;

enum {ADDR_BYTE1, ADDR_BYTE2, DATA_BYTE1, DATA_BYTE2};
enum {NO_CMD = -1, READ_CMD, WRITE_CMD};

enum {I2C_BLOCK_1, I2C_BLOCK_2};

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------



#endif
