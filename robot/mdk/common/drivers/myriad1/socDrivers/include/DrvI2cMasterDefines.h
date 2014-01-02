///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     I2C Master Driver API type definitions 
/// 
/// Types etc needed to use the I2C Master Driver API 
///

#ifndef _DRV_I2C_MASTER_API_H_
#define _DRV_I2C_MASTER_API_H_

#include "mv_types.h"
#include "DrvI2cDefines.h"
#include "isaac_registers.h"


/******************************************************************************
 1:  Basic typdefs
******************************************************************************/


typedef enum
{
    IIC1_DEVICE   = IIC1_BASE_ADR,      // TODO: Rename to I2C for consistency
    IIC2_DEVICE   = IIC2_BASE_ADR,
    GPIO_BITBASH  = GPIO_BASE_ADR
} I2CMModuleOption;

typedef enum
{
 S_ADDR_WR            = 0,   ///< Device Slave Address Byte for I2C write operation 
 S_ADDR_RD            = 1,   ///< Device Slave Address Byte for I2C read operation  
 R_ADDR_HH            = 2,   ///< Register Address bits 31..24 
 R_ADDR_HL            = 3,   ///< Register Address bits 23..16 
 R_ADDR_H             = 4,   ///< Register Address bits 15..8 
 R_ADDR_L             = 5,   ///< Register Address bits  7..0 
 DATAW                = 6,   ///< Write byte from address buffer and increment to next byte in buffer
 DATAR                = 7,   ///< Read byte from address buffer and increment to next byte in buffer
 DATAV                = 8,   ///< Same as read only data read is compared against buffer and error reported if different
 LOOP_MINUS_1         = 9    ///< Cause a loop back to the previous entity in the protocol
} I2CMProtocolType;


typedef struct 
{
    u32 device;
    u8  sclPin;
    u8  sdaPin;
    u32 speedKhz;
    u32 addressSize;
    I2CErrorHandler errorHandler;
} tyI2cConfig;


/// @brief This value is used to help determine if the passed device handle has already been initlialised
/******************************************************************************
 2:  Module Specific #defines
******************************************************************************/

#define INITIALISED_MAGIC_VALUE (0xCAFEBABE)

#define ABRT_SLVDR_INTX                     ( 1 << 15 )
#define ABRT_SLV_ARBLOST                    ( 1 << 14 )
#define ABRT_SLV_FLUSH_TXFIFO               ( 1 << 13 )
#define ABRT_LOST                           ( 1 << 12 )
#define ABRT_MASTER_DIS                     ( 1 << 11 )
#define ABRT_10B_RD_NORSTRT                 ( 1 << 10 )
#define ABRT_SBYTE_NORSTRT                  ( 1 <<  9 )
#define ABRT_HS_NORSTRT                     ( 1 <<  8 )
#define ABRT_SBYTE_ACKDET                   ( 1 <<  7 )
#define ABRT_HS_ACKDET                      ( 1 <<  6 )
#define ABRT_GCALL_READ                     ( 1 <<  5 )
#define ABRT_GCALL_NOACK                    ( 1 <<  4 )
#define ABRT_TXDATA_NOACK                   ( 1 <<  3 )
#define ABRT_10ADDR2_NOACK                  ( 1 <<  2 )
#define ABRT_10ADDR1_NOACK                  ( 1 <<  1 )
#define ABRT_7B_ADDR_NOACK                  ( 1 <<  0 )

// Some Default Protocols
#define I2C_PROTO_WRITE_8BA  {S_ADDR_WR, R_ADDR_L, DATAW, LOOP_MINUS_1}
#define I2C_PROTO_READ_8BA   {S_ADDR_WR, R_ADDR_L, S_ADDR_RD, DATAR, LOOP_MINUS_1}

#define I2C_PROTO_WRITE_16BA {S_ADDR_WR, R_ADDR_H, R_ADDR_L, DATAW, LOOP_MINUS_1}
#define I2C_PROTO_READ_16BA  {S_ADDR_WR, R_ADDR_H, R_ADDR_L, S_ADDR_RD, DATAR, LOOP_MINUS_1}
                                       


/******************************************************************************
 3:  Local enums/structs
******************************************************************************/

/******************************************************************************
 4:  Local const declarations     NB: ONLY const declarations go here
******************************************************************************/

#endif //_DRV_I2C_MASTER_API_H_
