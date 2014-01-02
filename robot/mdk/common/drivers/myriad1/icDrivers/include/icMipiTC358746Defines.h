///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Simple Eeprom driver
/// 
#ifndef IC_TC358746_DEF_H
#define IC_TC358746_DEF_H 

#include "mv_types.h"
#include <DrvI2cMaster.h>

// 1: Defines
// ----------------------------------------------------------------------------

#define PIN_NOT_CONNECTED_TO_GPIO (255)
#define TC358746XBG_SLAVE_ADDRESS (0x07)
#define TC358746AXBG_SLAVE_ADDRESS (0x0e)

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef enum 
{ 
     CSI2RXParOut = 0,    // when connected to a camera 
     CSI2TXParIn  = 1     // when connected to Panda
} TBridgeType;

typedef struct 
{
      I2CM_Device * i2cDev;
      TBridgeType   BridgeType;      // needed for init
      u8            BridgeI2CSlaveAddr;
    
      // gpio section
      // other gpios don't need to be changed during operation
      // If a gpio is not connected to myriad, then use the special value PIN_NOT_CONNECTED_TO_GPIO
      u8            ChipSelectGPIO;  
      u8            ResetGPIO;
      u8            ModeSelectGPIO;  
    
} TMipiBridge;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// 4: Register definitions:
// ----------------------------------------------------------------------------
// generates a division by zero warning in case of a mistake
#define MIPIB_ASSERT(truth) ((truth) ? 0 : ((truth) / 0))

// Construct a field specification for writing, and checking values
// These are designed to be binary-OR-ed together when they belong
// to the same register
#define MIPIB_FIELD(address, nonreserved_bits, field_mask, field_value) \
    (((((u64)(address))          << 48) | \
      (((u64)(nonreserved_bits)) << 32) | \
      (((u64)(field_mask))       << 16) | \
      (((u64)(field_value))      <<  0)) | \
     MIPIB_ASSERT((address) >= 0 && (address) <= 0x7fff) | \
     MIPIB_ASSERT((nonreserved_bits) >= 0 && (nonreserved_bits) <= 0xffff) | \
     MIPIB_ASSERT((field_mask) >= 0 && (field_mask) <= 0xffff) | \
     MIPIB_ASSERT((field_value) >= 0 && (field_value) <= 0xffff) | \
     MIPIB_ASSERT(((~(field_mask)) & (field_value)) == 0) | \
     MIPIB_ASSERT(((~(nonreserved_bits)) & (field_mask)) == 0))

#define MIPIB_FIELD32R(address, nonreserved_bits, field_mask, field_value) \
        (MIPIB_FIELD((address), (nonreserved_bits), (field_mask), (field_value)) | \
        (((u64)0x8000) << 48))

#define MIPIB_GET_ADDRESS(fieldspec)           ((u16)(((fieldspec) >> 48) & 0x7fff))
#define MIPIB_GET_NONRESERVED_BITS(fieldspec)  ((u16)(((fieldspec) >> 32) & 0xffff))
#define MIPIB_GET_FIELD_MASK(fieldspec)        ((u16)(((fieldspec) >> 16) & 0xffff))
#define MIPIB_GET_FIELD_VALUE(fieldspec)       ((u16)(((fieldspec) >>  0) & 0xffff))
// Is this a 32-bit register, with the top 16 bits reserved?
// ( in this case a 0 can be written automatically to the top 16 bits.
//   This is required for the register to be properly updated.
//   Register which have useful information in their top 16 bits
//   should have this flag set to zero. )
#define MIPIB_GET_IS_RESERVED_32BIT(fieldspec) ((u16)(((fieldspec) >> 63) & 1))

// Construct a field specification for reading numeric values.
// These are NOT designed to be or-ed together.
#define MIPIB_READ_FIELD(address, field_mask, shift_amount) \
    (((((u64)(address))          << 48) | \
      (((u64)(field_mask))       << 16) | \
      (((u64)(shift_amount))     <<  0)) | \
     MIPIB_ASSERT((address) >= 0 && (address) <= 0x7fff) | \
     MIPIB_ASSERT((field_mask) >= 0 && (field_mask) <= 0xffff) | \
     MIPIB_ASSERT((shift_amount) >= 0 && (shift_amount) <= 15) | \
     MIPIB_ASSERT((((field_mask) << (shift_amount)) & (0xffff0000)) == 0))

#define MIPIB_GET_SHIFT_AMOUNT(fieldspec)     ((u16)(((fieldspec) >>  0) & 0xffff))

// note: the following two definitions are only used to check if the ChipID value is correct
#define MIPIB_ChipID__ChipID(value)    MIPIB_FIELD(0x0000, 0xffff, 0xffff, value  << 0) | \
    MIPIB_ASSERT(value == 0x4400 || value == 0x4401)
#define MIPIB_ChipID__ChipID_0x440X    MIPIB_FIELD(0x0000, 0xffff, 0xfffe, 0x4400 << 0)
#define MIPIB_ChipID__ChipID_Read MIPIB_READ_FIELD(0x0000,         0xffff,           0)

#define MIPIB_SysCtl__SLEEP__Normal_operation  MIPIB_FIELD(0x0002, 0x0003, 0x0002,  0x0000)
#define MIPIB_SysCtl__SLEEP__Sleep_mode        MIPIB_FIELD(0x0002, 0x0003, 0x0002,  0x0002)

#define MIPIB_SysCtl__Sreset__Normal_operation MIPIB_FIELD(0x0002, 0x0003, 0x0001,  0x0000)
#define MIPIB_SysCtl__Sreset__Reset_operation  MIPIB_FIELD(0x0002, 0x0003, 0x0001,  0x0001)

#define MIPIB_ConfCtl__TriEn__Enable           MIPIB_FIELD(0x0004, 0xb37f, 1 << 15, 1 << 15)
#define MIPIB_ConfCtl__TriEn__Disable          MIPIB_FIELD(0x0004, 0xb37f, 1 << 15, 0 << 15)

#define MIPIB_ConfCtl__INTE2n__Normal          MIPIB_FIELD(0x0004, 0xb37f, 1 << 13, 0 << 13)
#define MIPIB_ConfCtl__INTE2n__Enable          MIPIB_FIELD(0x0004, 0xb37f, 1 << 13, 1 << 13)

#define MIPIB_ConfCtl__Bt656En__Disable        MIPIB_FIELD(0x0004, 0xb37f, 1 << 12, 0 << 12)
#define MIPIB_ConfCtl__Bt656En__Enable         MIPIB_FIELD(0x0004, 0xb37f, 1 << 12, 1 << 12)

#define MIPIB_ConfCtl__PdataF__Mode_0          MIPIB_FIELD(0x0004, 0xb37f, 3 << 8, 0 << 8)
#define MIPIB_ConfCtl__PdataF__Mode_1          MIPIB_FIELD(0x0004, 0xb37f, 3 << 8, 1 << 8)
#define MIPIB_ConfCtl__PdataF__Mode_2          MIPIB_FIELD(0x0004, 0xb37f, 3 << 8, 2 << 8)

#define MIPIB_ConfCtl__PPEn__Disable           MIPIB_FIELD(0x0004, 0xb37f, 1 << 6, 0 << 6)
#define MIPIB_ConfCtl__PPEn__Enable            MIPIB_FIELD(0x0004, 0xb37f, 1 << 6, 1 << 6)

#define MIPIB_ConfCtl__VsyncP__active_high     MIPIB_FIELD(0x0004, 0xb37f, 1 << 5, 0 << 5)
#define MIPIB_ConfCtl__VsyncP__active_low      MIPIB_FIELD(0x0004, 0xb37f, 1 << 5, 1 << 5)

#define MIPIB_ConfCtl__HsyncP__active_high     MIPIB_FIELD(0x0004, 0xb37f, 1 << 4, 0 << 4)
#define MIPIB_ConfCtl__HsyncP__active_low      MIPIB_FIELD(0x0004, 0xb37f, 1 << 4, 1 << 4)

#define MIPIB_ConfCtl__PCLKP__Normal           MIPIB_FIELD(0x0004, 0xb37f, 1 << 3, 0 << 3)
#define MIPIB_ConfCtl__PCLKP__Inverted         MIPIB_FIELD(0x0004, 0xb37f, 1 << 3, 1 << 3)

#define MIPIB_ConfCtl__Auto__NOINCREMENT       MIPIB_FIELD(0x0004, 0xb37f, 1 << 2, 0 << 2)
#define MIPIB_ConfCtl__Auto__INCREMENT         MIPIB_FIELD(0x0004, 0xb37f, 1 << 2, 1 << 2)

// Deprecated:
#define MIPIB_ConfCtl__DataLanes__NR_1         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 0 << 0)
#define MIPIB_ConfCtl__DataLanes__NR_2         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 1 << 0)
#define MIPIB_ConfCtl__DataLanes__NR_3         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 2 << 0)
#define MIPIB_ConfCtl__DataLanes__NR_4         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 3 << 0)

#define MIPIB_ConfCtl__DataLanesRXOnly__NR_1         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 0 << 0)
#define MIPIB_ConfCtl__DataLanesRXOnly__NR_2         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 1 << 0)
#define MIPIB_ConfCtl__DataLanesRXOnly__NR_3         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 2 << 0)
#define MIPIB_ConfCtl__DataLanesRXOnly__NR_4         MIPIB_FIELD(0x0004, 0xb37f, 3 << 0, 3 << 0)

#define MIPIB_FiFoCtl__FifoLevel(value)    MIPIB_FIELD(0x0006, 0x01ff, 0x01ff, (value) << 0)
#define MIPIB_FiFoCtl__FifoLevel_Read MIPIB_READ_FIELD(0x0006,         0x01ff,            0)


#define MIPIB_DataFmt__PDFormat__RAW8                 MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 0 << 4)
#define MIPIB_DataFmt__PDFormat__RAW10                MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 1 << 4)
#define MIPIB_DataFmt__PDFormat__RAW12                MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 2 << 4)
#define MIPIB_DataFmt__PDFormat__RGB888               MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 3 << 4)
#define MIPIB_DataFmt__PDFormat__RGB666               MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 4 << 4)
#define MIPIB_DataFmt__PDFormat__RGB565               MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 5 << 4)
#define MIPIB_DataFmt__PDFormat__YUV422_8bit          MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 6 << 4)
#define MIPIB_DataFmt__PDFormat__RAW14                MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 8 << 4)
#define MIPIB_DataFmt__PDFormat__YUV422_10bit         MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 9 << 4)
#define MIPIB_DataFmt__PDFormat__YUV444               MIPIB_FIELD(0x0008, 0x00f1, 0x00f0, 10 << 4)

#define MIPIB_DataFmt__UDT_en__CSIRX_from_CSI_Bus      MIPIB_FIELD(0x0008, 0x00f1, 0x0001, 0)
#define MIPIB_DataFmt__UDT_en__CSITX_from_PDFormat_reg MIPIB_FIELD(0x0008, 0x00f1, 0x0001, 0)
#define MIPIB_DataFmt__UDT_en__CSIRX_from_PDFormat_reg MIPIB_FIELD(0x0008, 0x00f1, 0x0001, 1)
#define MIPIB_DataFmt__UDT_en__CSITX_from_CSITX_DT_reg MIPIB_FIELD(0x0008, 0x00f1, 0x0001, 1)

#define MIPIB_MclkCtl__mclk_high(value)      MIPIB_FIELD(0x000c, 0xffff, 0xff00, (value) << 8)
#define MIPIB_MclkCtl__mclk_high_Read   MIPIB_READ_FIELD(0x000c,         0xff00,            8)
#define MIPIB_MclkCtl__mclk_low(value)       MIPIB_FIELD(0x000c, 0xffff, 0x00ff, (value) << 0)
#define MIPIB_MclkCtl__mclk_low_Read    MIPIB_READ_FIELD(0x000c,         0x00ff,            0)

// The bit positions are relevant here, so the shift amount is going to be zero
#define MIPIB_GPIOEn__GPIOEn(value)           MIPIB_FIELD(0x000e, 0xfff0, 0xfff0, (value) << 0)
#define MIPIB_GPIOEn__GPIOEn_Read        MIPIB_READ_FIELD(0x000e,         0xfff0,            0)
#define MIPIB_GPIOEn__GPIOEn__Parallel_Port_23_12__Enable  MIPIB_FIELD(0x000e, 0xfff0, 0xfff0, 0xfff0)
#define MIPIB_GPIOEn__GPIOEn__Parallel_Port_23_12__Disable MIPIB_FIELD(0x000e, 0xfff0, 0xfff0, 0x0000)
#define MIPIB_GPIOEn__GPIOEn__Set_Bit_to_Enable(bit) MIPIB_FIELD(0x000e, 0xfff0, 1 << (bit), 1 << (bit))
#define MIPIB_GPIOEn__GPIOEn__Set_Bit_to_Disable(bit) MIPIB_FIELD(0x000e, 0xfff0, 1 << (bit), 0 << (bit))

#define MIPIB_GPIODir__GPIODir(value)      MIPIB_FIELD(0x0010, 0xffff, 0xffff, (value) << 0)
#define MIPIB_GPIODir__GPIODir_Read   MIPIB_READ_FIELD(0x0010,         0xffff,            0)
#define MIPIB_GPIODir__GPIODir__Set_Bit_to_Input(bit)  MIPIB_FIELD(0x0010, 0xffff, 1 << (bit), 1 << (bit))
#define MIPIB_GPIODir__GPIODir__Set_Bit_to_Output(bit) MIPIB_FIELD(0x0010, 0xffff, 1 << (bit), 0 << (bit))

// GPIOPin is a readonly register, so the following definitions will only be used
// to check gpio input values
#define MIPIB_GPIOPin__GPIOIn_read                      MIPIB_READ_FIELD(0x0012,             0xffff,          0)
#define MIPIB_GPIOPin__GPIOIn__Check__state_of_bit_is_1(bit) MIPIB_FIELD(0x0012, 0xffff, 1 << (bit), 1 << (bit))
#define MIPIB_GPIOPin__GPIOIn__Check__state_of_bit_is_0(bit) MIPIB_FIELD(0x0010, 0xffff, 1 << (bit), 0 << (bit))

#define MIPIB_GPIOOut__GPIOOut(value)          MIPIB_FIELD(0x0014, 0xffff, 0xffff, (value) << 0)
#define MIPIB_GPIOOut__GPIOOut_read       MIPIB_READ_FIELD(0x0014,         0xffff,            0)
#define MIPIB_GPIOOut__GPIOOut__Set_Bit(bit)   MIPIB_FIELD(0x0014, 0xffff, 1 << (bit), 1 << (bit))
#define MIPIB_GPIOOut__GPIOOut__Clear_Bit(bit) MIPIB_FIELD(0x0014, 0xffff, 1 << (bit), 0 << (bit))

#define MIPIB_PLLCtl0__PLL_PRD(value)        MIPIB_FIELD(0x0016, 0xf1ff, 0xf000, (value) << 12)
#define MIPIB_PLLCtl0__PLL_PRD_Read     MIPIB_READ_FIELD(0x0016,         0xf000,            12)
#define MIPIB_PLLCtl0__PLL_FBD(value)        MIPIB_FIELD(0x0016, 0xf1ff, 0x01ff, (value) << 0)
#define MIPIB_PLLCtl0__PLL_FBD_Read     MIPIB_READ_FIELD(0x0016,         0x01ff,            0)

// Deprecated:
#define MIPIB_PLLCtl1__PLL_FRS__500M_1G_HSCK_freq   MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 0 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__250M_500M_HSCK_freq MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 1 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__125M_250M_HSCK_freq MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 2 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__62M_125M_HSCK_freq  MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 3 << 10)

#define MIPIB_PLLCtl1__PLL_FRS__DIV1_500M_1G_HSCK_freq   MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 0 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__DIV2_250M_500M_HSCK_freq MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 1 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__DIV4_125M_250M_HSCK_freq MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 2 << 10)
#define MIPIB_PLLCtl1__PLL_FRS__DIV8_62M_125M_HSCK_freq  MIPIB_FIELD(0x0018, 0x0f73, 3 << 10, 3 << 10)

#define MIPIB_PLLCtl1__PLL_LBWS__PERCENT_25  MIPIB_FIELD(0x0018, 0x0f73, 3 << 8, 0 << 8)
#define MIPIB_PLLCtl1__PLL_LBWS__PERCENT_33  MIPIB_FIELD(0x0018, 0x0f73, 3 << 8, 1 << 8)
#define MIPIB_PLLCtl1__PLL_LBWS__PERCENT_50  MIPIB_FIELD(0x0018, 0x0f73, 3 << 8, 2 << 8)
#define MIPIB_PLLCtl1__PLL_LBWS__PERCENT_MAX MIPIB_FIELD(0x0018, 0x0f73, 3 << 8, 3 << 8)

#define MIPIB_PLLCtl1__PLL_LFBREN__no_oscillation   MIPIB_FIELD(0x0018, 0x0f73, 1 << 6, 0 << 6)
#define MIPIB_PLLCtl1__PLL_LFBREN__free_running_PLL MIPIB_FIELD(0x0018, 0x0f73, 1 << 6, 1 << 6)

#define MIPIB_PLLCtl1__PLL_BYPCKEN__Normal_operation MIPIB_FIELD(0x0018, 0x0f73, 1 << 5, 0 << 5)
#define MIPIB_PLLCtl1__PLL_BYPCKEN__Bypass_mode      MIPIB_FIELD(0x0018, 0x0f73, 1 << 5, 1 << 5)

#define MIPIB_PLLCtl1__PLL_CKEN__clocks_off MIPIB_FIELD(0x0018, 0x0f73, 1 << 4, 0 << 4)
#define MIPIB_PLLCtl1__PLL_CKEN__clocks_on  MIPIB_FIELD(0x0018, 0x0f73, 1 << 4, 1 << 4)

#define MIPIB_PLLCtl1__PLL_RESETB__Reset            MIPIB_FIELD(0x0018, 0x0f73, 1 << 1, 0 << 1)
#define MIPIB_PLLCtl1__PLL_RESETB__Normal_operation MIPIB_FIELD(0x0018, 0x0f73, 1 << 1, 1 << 1)

#define MIPIB_PLLCtl1__PLL_EN__off  MIPIB_FIELD(0x0018, 0x0f73, 1 << 0, 0 << 0)
#define MIPIB_PLLCtl1__PLL_EN__on   MIPIB_FIELD(0x0018, 0x0f73, 1 << 0, 1 << 0)


#define MIPIB_ClkCtl__PPIclkDiv__DIV8 MIPIB_FIELD(0x0020, 0x003f, 3 << 4, 0 << 4)
#define MIPIB_ClkCtl__PPIclkDiv__DIV4 MIPIB_FIELD(0x0020, 0x003f, 3 << 4, 1 << 4)
#define MIPIB_ClkCtl__PPIclkDiv__DIV2 MIPIB_FIELD(0x0020, 0x003f, 3 << 4, 2 << 4)

#define MIPIB_ClkCtl__MclkRefDiv__DIV8 MIPIB_FIELD(0x0020, 0x003f, 3 << 2, 0 << 2)
#define MIPIB_ClkCtl__MclkRefDiv__DIV4 MIPIB_FIELD(0x0020, 0x003f, 3 << 2, 1 << 2)
#define MIPIB_ClkCtl__MclkRefDiv__DIV2 MIPIB_FIELD(0x0020, 0x003f, 3 << 2, 2 << 2)

#define MIPIB_ClkCtl__SclkDiv__DIV8 MIPIB_FIELD(0x0020, 0x003f, 3 << 0, 0 << 0)
#define MIPIB_ClkCtl__SclkDiv__DIV4 MIPIB_FIELD(0x0020, 0x003f, 3 << 0, 1 << 0)
#define MIPIB_ClkCtl__SclkDiv__DIV2 MIPIB_FIELD(0x0020, 0x003f, 3 << 0, 2 << 0)

#define MIPIB_WordCnt__WordCnt(value)      MIPIB_FIELD(0x0022, 0xffff, 0xffff, (value) << 0)
#define MIPIB_WordCnt__WordCnt_Read   MIPIB_READ_FIELD(0x0022,         0xffff,            0)


// The following two are AXBG-specific register bits
#define MIPIB_PP_MISC__RstPtr__reset_buffer_pointers MIPIB_FIELD(0x0032, 0xc000, 0x4000, 0x4000)
#define MIPIB_PP_MISC__RstPtr__dont_reset            MIPIB_FIELD(0x0032, 0xc000, 0x4000, 0x0000)
#define MIPIB_PP_MISC__FrmStop__stop_output_at_next_vsync MIPIB_FIELD(0x0032, 0xc000, 0x8000, 0x8000)
#define MIPIB_PP_MISC__FrmStop__normal_operation          MIPIB_FIELD(0x0032, 0xc000, 0x8000, 0x0000)
// EOF AXBG-specific registers

#define MIPIB_USER_DT__user_dt(value)      MIPIB_FIELD(0x0050, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_USER_DT__user_dt_Read   MIPIB_READ_FIELD(0x0050,         0x00ff,            0)

#define MIPIB_PHYClkCtl__ClkDly(value)      MIPIB_FIELD(0x0056, 0x00ff, 0x000f, (value) << 0)
#define MIPIB_PHYClkCtl__ClkDly_Read   MIPIB_READ_FIELD(0x0056,         0x000f,            0)
#define MIPIB_PHYClkCtl__HsRxRs__1_5k       MIPIB_FIELD(0x0056, 0x00ff, 0x0030, 0 << 4)
#define MIPIB_PHYClkCtl__HsRxRs__1_75k      MIPIB_FIELD(0x0056, 0x00ff, 0x0030, 1 << 4)
#define MIPIB_PHYClkCtl__HsRxRs__2_00k      MIPIB_FIELD(0x0056, 0x00ff, 0x0030, 2 << 4)
#define MIPIB_PHYClkCtl__HsRxRs__2_25k      MIPIB_FIELD(0x0056, 0x00ff, 0x0030, 3 << 4)
#define MIPIB_PHYClkCtl__Cap__no_additional_capacitance     MIPIB_FIELD(0x0056, 0x00ff, 0x00c0, 0 << 6)
#define MIPIB_PHYClkCtl__Cap__2_4pF__additional_capacitance MIPIB_FIELD(0x0056, 0x00ff, 0x00c0, 1 << 6)
#define MIPIB_PHYClkCtl__Cap__2_6pF__additional_capacitance MIPIB_FIELD(0x0056, 0x00ff, 0x00c0, 2 << 6)
#define MIPIB_PHYClkCtl__Cap__2_8pF__additional_capacitance MIPIB_FIELD(0x0056, 0x00ff, 0x00c0, 3 << 6)

#define MIPIB_PHYData0Ctl__DataDly(value)      MIPIB_FIELD(0x0058, 0x00ff, 0x000f, (value) << 0)
#define MIPIB_PHYData0Ctl__DataDly_Read   MIPIB_READ_FIELD(0x0058,         0x000f,            0)
#define MIPIB_PHYData0Ctl__HsRxRs__1_5k        MIPIB_FIELD(0x0058, 0x00ff, 0x0030, 0 << 4)
#define MIPIB_PHYData0Ctl__HsRxRs__1_75k       MIPIB_FIELD(0x0058, 0x00ff, 0x0030, 1 << 4)
#define MIPIB_PHYData0Ctl__HsRxRs__2_00k       MIPIB_FIELD(0x0058, 0x00ff, 0x0030, 2 << 4)
#define MIPIB_PHYData0Ctl__HsRxRs__2_25k       MIPIB_FIELD(0x0058, 0x00ff, 0x0030, 3 << 4)
#define MIPIB_PHYData0Ctl__Cap__no_additional_capacitance     MIPIB_FIELD(0x0058, 0x00ff, 0x00c0, 0 << 6)
#define MIPIB_PHYData0Ctl__Cap__2_4pF__additional_capacitance MIPIB_FIELD(0x0058, 0x00ff, 0x00c0, 1 << 6)
#define MIPIB_PHYData0Ctl__Cap__2_6pF__additional_capacitance MIPIB_FIELD(0x0058, 0x00ff, 0x00c0, 2 << 6)
#define MIPIB_PHYData0Ctl__Cap__2_8pF__additional_capacitance MIPIB_FIELD(0x0058, 0x00ff, 0x00c0, 3 << 6)

#define MIPIB_PHYData1Ctl__DataDly(value)      MIPIB_FIELD(0x005a, 0x00ff, 0x000f, (value) << 0)
#define MIPIB_PHYData1Ctl__DataDly_Read   MIPIB_READ_FIELD(0x005a,         0x000f,            0)
#define MIPIB_PHYData1Ctl__HsRxRs__1_5k        MIPIB_FIELD(0x005a, 0x00ff, 0x0030, 0 << 4)
#define MIPIB_PHYData1Ctl__HsRxRs__1_75k       MIPIB_FIELD(0x005a, 0x00ff, 0x0030, 1 << 4)
#define MIPIB_PHYData1Ctl__HsRxRs__2_00k       MIPIB_FIELD(0x005a, 0x00ff, 0x0030, 2 << 4)
#define MIPIB_PHYData1Ctl__HsRxRs__2_25k       MIPIB_FIELD(0x005a, 0x00ff, 0x0030, 3 << 4)
#define MIPIB_PHYData1Ctl__Cap__no_additional_capacitance     MIPIB_FIELD(0x005a, 0x00ff, 0x00c0, 0 << 6)
#define MIPIB_PHYData1Ctl__Cap__2_4pF__additional_capacitance MIPIB_FIELD(0x005a, 0x00ff, 0x00c0, 1 << 6)
#define MIPIB_PHYData1Ctl__Cap__2_6pF__additional_capacitance MIPIB_FIELD(0x005a, 0x00ff, 0x00c0, 2 << 6)
#define MIPIB_PHYData1Ctl__Cap__2_8pF__additional_capacitance MIPIB_FIELD(0x005a, 0x00ff, 0x00c0, 3 << 6)

#define MIPIB_PHYData2Ctl__DataDly(value)      MIPIB_FIELD(0x005e, 0x00ff, 0x000f, (value) << 0)
#define MIPIB_PHYData2Ctl__DataDly_Read   MIPIB_READ_FIELD(0x005e,         0x000f,            0)
#define MIPIB_PHYData2Ctl__HsRxRs__1_5k        MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 0 << 4)
#define MIPIB_PHYData2Ctl__HsRxRs__1_75k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 1 << 4)
#define MIPIB_PHYData2Ctl__HsRxRs__2_00k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 2 << 4)
#define MIPIB_PHYData2Ctl__HsRxRs__2_25k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 3 << 4)
#define MIPIB_PHYData2Ctl__Cap__no_additional_capacitance     MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 0 << 6)
#define MIPIB_PHYData2Ctl__Cap__2_4pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 1 << 6)
#define MIPIB_PHYData2Ctl__Cap__2_6pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 2 << 6)
#define MIPIB_PHYData2Ctl__Cap__2_8pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 3 << 6)

#define MIPIB_PHYData3Ctl__DataDly(value)      MIPIB_FIELD(0x005e, 0x00ff, 0x000f, (value) << 0)
#define MIPIB_PHYData3Ctl__DataDly_Read   MIPIB_READ_FIELD(0x005e,         0x000f,            0)
#define MIPIB_PHYData3Ctl__HsRxRs__1_5k        MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 0 << 4)
#define MIPIB_PHYData3Ctl__HsRxRs__1_75k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 1 << 4)
#define MIPIB_PHYData3Ctl__HsRxRs__2_00k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 2 << 4)
#define MIPIB_PHYData3Ctl__HsRxRs__2_25k       MIPIB_FIELD(0x005e, 0x00ff, 0x0030, 3 << 4)
#define MIPIB_PHYData3Ctl__Cap__no_additional_capacitance     MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 0 << 6)
#define MIPIB_PHYData3Ctl__Cap__2_4pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 1 << 6)
#define MIPIB_PHYData3Ctl__Cap__2_6pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 2 << 6)
#define MIPIB_PHYData3Ctl__Cap__2_8pF__additional_capacitance MIPIB_FIELD(0x005e, 0x00ff, 0x00c0, 3 << 6)

#define MIPIB_PHYTimDly__DSettle(value)      MIPIB_FIELD(0x0060, 0x80ff, 0x007f, (value) << 0)
#define MIPIB_PHYTimDly__DSettle_Read   MIPIB_READ_FIELD(0x0060,         0x007f,            0)
#define MIPIB_PHYTimDly__Td_term_sel__HS_termination_after_2_to_3_PPIRxClk MIPIB_FIELD(0x0060, 0x80ff, 0x0080, 0 << 7)
#define MIPIB_PHYTimDly__Td_term_sel__HS_termination_immediately           MIPIB_FIELD(0x0060, 0x80ff, 0x0080, 1 << 7)
#define MIPIB_PHYTimDly__Tc_term_sel__please_set_to_1                      MIPIB_FIELD(0x0060, 0x80ff, 0x8000, 1 << 15)
#define MIPIB_PHYTimDly__Tc_term_sel__bad_default_is_0                     MIPIB_FIELD(0x0060, 0x80ff, 0x8000, 0 << 15)

#define MIPIB_PHYSta__SoTErr3__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 7, 1 << 7)
#define MIPIB_PHYSta__SoTErr3__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 7, 0 << 7)
#define MIPIB_PHYSta__SynErr3__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 6, 1 << 6)
#define MIPIB_PHYSta__SynErr3__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 6, 0 << 6)
#define MIPIB_PHYSta__SoTErr2__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 5, 1 << 5)
#define MIPIB_PHYSta__SoTErr2__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 5, 0 << 5)
#define MIPIB_PHYSta__SynErr2__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 4, 1 << 4)
#define MIPIB_PHYSta__SynErr2__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 4, 0 << 4)
#define MIPIB_PHYSta__SoTErr1__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 3, 1 << 3)
#define MIPIB_PHYSta__SoTErr1__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 3, 0 << 3)
#define MIPIB_PHYSta__SynErr1__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 2, 1 << 2)
#define MIPIB_PHYSta__SynErr1__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 2, 0 << 2)
#define MIPIB_PHYSta__SoTErr0__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 1, 1 << 1)
#define MIPIB_PHYSta__SoTErr0__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 1, 0 << 1)
#define MIPIB_PHYSta__SynErr0__occurred          MIPIB_FIELD(0x0062, 0x00ff, 1 << 0, 1 << 0)
#define MIPIB_PHYSta__SynErr0__no_error_reported MIPIB_FIELD(0x0062, 0x00ff, 1 << 0, 0 << 0)

// Warning: evil register naming scheme. Do not confuse with CSI_STATUS!
// This register should only be relevant for CSI RX operation.
#define MIPIB_CSIStatus__MDLErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 8, 1 << 8)
#define MIPIB_CSIStatus__MDLErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 8, 0 << 8)
#define MIPIB_CSIStatus__FrmErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 7, 1 << 7)
#define MIPIB_CSIStatus__FrmErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 7, 0 << 7)
#define MIPIB_CSIStatus__CRCErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 6, 1 << 6)
#define MIPIB_CSIStatus__CRCErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 6, 0 << 6)
#define MIPIB_CSIStatus__CorErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 5, 1 << 5)
#define MIPIB_CSIStatus__CorErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 5, 0 << 5)
#define MIPIB_CSIStatus__HdrErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 4, 1 << 4)
#define MIPIB_CSIStatus__HdrErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 4, 0 << 4)
#define MIPIB_CSIStatus__EIDErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 3, 1 << 3)
#define MIPIB_CSIStatus__EIDErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 3, 0 << 3)
#define MIPIB_CSIStatus__CtlErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 2, 1 << 2)
#define MIPIB_CSIStatus__CtlErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 2, 0 << 2)
#define MIPIB_CSIStatus__SoTErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 1, 1 << 1)
#define MIPIB_CSIStatus__SoTErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 1, 0 << 1)
#define MIPIB_CSIStatus__SynErr__occurred        MIPIB_FIELD(0x0064, 0x01ff, 1 << 0, 1 << 0)
#define MIPIB_CSIStatus__SynErr__did_not_occur   MIPIB_FIELD(0x0064, 0x01ff, 1 << 0, 0 << 0)

#define MIPIB_CSIErrEn__MDLEn__assert_CSIErr_if_MDLErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 8, 1 << 8)
#define MIPIB_CSIErrEn__MDLEn__do_not_assert_CSIErr_if_MDLErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 8, 0 << 8)
#define MIPIB_CSIErrEn__FrmEn__assert_CSIErr_if_FrmErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 7, 1 << 7)
#define MIPIB_CSIErrEn__FrmEn__do_not_assert_CSIErr_if_FrmErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 7, 0 << 7)
#define MIPIB_CSIErrEn__CRCEn__assert_CSIErr_if_CRCErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 6, 1 << 6)
#define MIPIB_CSIErrEn__CRCEn__do_not_assert_CSIErr_if_CRCErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 6, 0 << 6)
#define MIPIB_CSIErrEn__CorEn__assert_CSIErr_if_CorErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 5, 1 << 5)
#define MIPIB_CSIErrEn__CorEn__do_not_assert_CSIErr_if_CorErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 5, 0 << 5)
#define MIPIB_CSIErrEn__HdrEn__assert_CSIErr_if_HdrErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 4, 1 << 4)
#define MIPIB_CSIErrEn__HdrEn__do_not_assert_CSIErr_if_HdrErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 4, 0 << 4)
#define MIPIB_CSIErrEn__EIDEn__assert_CSIErr_if_EIDErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 3, 1 << 3)
#define MIPIB_CSIErrEn__EIDEn__do_not_assert_CSIErr_if_EIDErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 3, 0 << 3)
#define MIPIB_CSIErrEn__CtlEn__assert_CSIErr_if_CtlErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 2, 1 << 2)
#define MIPIB_CSIErrEn__CtlEn__do_not_assert_CSIErr_if_CtlErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 2, 0 << 2)
#define MIPIB_CSIErrEn__SoTEn__assert_CSIErr_if_SoTErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 1, 1 << 1)
#define MIPIB_CSIErrEn__SoTEn__do_not_assert_CSIErr_if_SoTErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 1, 0 << 1)
#define MIPIB_CSIErrEn__SynEn__assert_CSIErr_if_SynErr_occurs         MIPIB_FIELD(0x0066, 0x01ff, 1 << 0, 1 << 0)
#define MIPIB_CSIErrEn__SynEn__do_not_assert_CSIErr_if_SynErr_occurs  MIPIB_FIELD(0x0066, 0x01ff, 1 << 0, 0 << 0)

#define MIPIB_MDLSynErr__Sync0_detected     MIPIB_FIELD(0x0068, 0x000f, 1 << 0, 1 << 0)
#define MIPIB_MDLSynErr__Sync0_not_detected MIPIB_FIELD(0x0068, 0x000f, 1 << 0, 0 << 0)
#define MIPIB_MDLSynErr__Sync1_detected     MIPIB_FIELD(0x0068, 0x000f, 1 << 1, 1 << 1)
#define MIPIB_MDLSynErr__Sync1_not_detected MIPIB_FIELD(0x0068, 0x000f, 1 << 1, 0 << 1)
#define MIPIB_MDLSynErr__Sync2_detected     MIPIB_FIELD(0x0068, 0x000f, 1 << 2, 1 << 2)
#define MIPIB_MDLSynErr__Sync2_not_detected MIPIB_FIELD(0x0068, 0x000f, 1 << 2, 0 << 2)
#define MIPIB_MDLSynErr__Sync3_detected     MIPIB_FIELD(0x0068, 0x000f, 1 << 3, 1 << 3)
#define MIPIB_MDLSynErr__Sync3_not_detected MIPIB_FIELD(0x0068, 0x000f, 1 << 3, 0 << 3)

#define MIPIB_CSIDID__Check__DataType(value)      MIPIB_FIELD(0x006a, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_CSIDID__Check__DataType_Read   MIPIB_READ_FIELD(0x006a,         0x00ff,            0)

#define MIPIB_CSIDIDErr__Check__DataType(value)      MIPIB_FIELD(0x006c, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_CSIDIDErr__Check__DataType_Read   MIPIB_READ_FIELD(0x006c,         0x00ff,            0)

#define MIPIB_CSIPktLen__Check__PktLen(value)      MIPIB_FIELD(0x006e, 0xffff, 0xffff, (value) << 0)
#define MIPIB_CSIPktLen__Check__PktLen_Read   MIPIB_READ_FIELD(0x006e,         0xffff,            0)

#define MIPIB_CSIRX_DPCtl__rxch0_cntrl(value)      MIPIB_FIELD(0x0070, 0x03ff, 3 << 0, (value) << 0)
#define MIPIB_CSIRX_DPCtl__rxch0_cntrl_Read   MIPIB_READ_FIELD(0x0070,         3 << 0,            0)
#define MIPIB_CSIRX_DPCtl__rxch1_cntrl(value)      MIPIB_FIELD(0x0070, 0x03ff, 3 << 2, (value) << 2)
#define MIPIB_CSIRX_DPCtl__rxch1_cntrl_Read   MIPIB_READ_FIELD(0x0070,         3 << 2,            2)
#define MIPIB_CSIRX_DPCtl__rxch2_cntrl(value)      MIPIB_FIELD(0x0070, 0x03ff, 3 << 4, (value) << 4)
#define MIPIB_CSIRX_DPCtl__rxch2_cntrl_Read   MIPIB_READ_FIELD(0x0070,         3 << 4,            4)
#define MIPIB_CSIRX_DPCtl__rxch3_cntrl(value)      MIPIB_FIELD(0x0070, 0x03ff, 3 << 6, (value) << 6)
#define MIPIB_CSIRX_DPCtl__rxch3_cntrl_Read   MIPIB_READ_FIELD(0x0070,         3 << 6,            6)
#define MIPIB_CSIRX_DPCtl__rxck_cntrl(value)       MIPIB_FIELD(0x0070, 0x03ff, 3 << 8, (value) << 8)
#define MIPIB_CSIRX_DPCtl__rxck_cntrl_Read    MIPIB_READ_FIELD(0x0070,         3 << 8,            8)

#define MIPIB_FrmErrCnt__FrmErrCnt(value)      MIPIB_FIELD(0x0080, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_FrmErrCnt__FrmErrCnt_Read   MIPIB_READ_FIELD(0x0080,         0x00ff,            0)

#define MIPIB_CRCErrCnt__CRCErrCnt(value)      MIPIB_FIELD(0x0082, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_CRCErrCnt__CRCErrCnt_Read   MIPIB_READ_FIELD(0x0082,         0x00ff,            0)

#define MIPIB_CorErrCnt__CorErrCnt(value)      MIPIB_FIELD(0x0084, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_CorErrCnt__CorErrCnt_Read   MIPIB_READ_FIELD(0x0084,         0x00ff,            0)

#define MIPIB_HdrErrCnt__HdrErrCnt(value)      MIPIB_FIELD(0x0086, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_HdrErrCnt__HdrErrCnt_Read   MIPIB_READ_FIELD(0x0086,         0x00ff,            0)

#define MIPIB_EIDErrCnt__EIDErrCnt(value)      MIPIB_FIELD(0x0088, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_EIDErrCnt__EIDErrCnt_Read   MIPIB_READ_FIELD(0x0088,         0x00ff,            0)

#define MIPIB_CtlErrCnt__CtlErrCnt(value)      MIPIB_FIELD(0x008a, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_CtlErrCnt__CtlErrCnt_Read   MIPIB_READ_FIELD(0x008a,         0x00ff,            0)

#define MIPIB_SoTErrCnt__SoTErrCnt(value)      MIPIB_FIELD(0x008c, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_SoTErrCnt__SoTErrCnt_Read   MIPIB_READ_FIELD(0x008c,         0x00ff,            0)

#define MIPIB_SynErrCnt__SynErrCnt(value)      MIPIB_FIELD(0x008e, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_SynErrCnt__SynErrCnt_Read   MIPIB_READ_FIELD(0x008e,         0x00ff,            0)

#define MIPIB_MDLErrCnt__MDLErrCnt(value)      MIPIB_FIELD(0x0090, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_MDLErrCnt__MDLErrCnt_Read   MIPIB_READ_FIELD(0x0090,         0x00ff,            0)

#define MIPIB_FIFOSTATUS__Vb_uflow__Under_flow MIPIB_FIELD32R(0x00f8, 0x0003, 1 << 1, 1 << 1)
#define MIPIB_FIFOSTATUS__Vb_uflow__Normal     MIPIB_FIELD32R(0x00f8, 0x0003, 1 << 1, 0 << 1)

#define MIPIB_FIFOSTATUS__Vb_oflow__Over_flow  MIPIB_FIELD32R(0x00f8, 0x0003, 1 << 0, 1 << 0)
#define MIPIB_FIFOSTATUS__Vb_oflow__Normal     MIPIB_FIELD32R(0x00f8, 0x0003, 1 << 0, 0 << 0)


#define MIPIB_CLW_DPHYCONTTX__CLW_CAP__0pF     MIPIB_FIELD32R(0x0100, 0x03f3, 3 << 8, 0 << 8)
#define MIPIB_CLW_DPHYCONTTX__CLW_CAP__2_8pF   MIPIB_FIELD32R(0x0100, 0x03f3, 3 << 8, 1 << 8)
#define MIPIB_CLW_DPHYCONTTX__CLW_CAP__3_2pF   MIPIB_FIELD32R(0x0100, 0x03f3, 3 << 8, 2 << 8)
#define MIPIB_CLW_DPHYCONTTX__CLW_CAP__3_6pF   MIPIB_FIELD32R(0x0100, 0x03f3, 3 << 8, 3 << 8)

#define MIPIB_CLW_DPHYCONTTX__DLYCNTRL(value)   MIPIB_FIELD32R(0x0100, 0x03f3, 0xf << 4, (value) << 4)
#define MIPIB_CLW_DPHYCONTTX__DLYCNTRL_Read   MIPIB_READ_FIELD(0x0100,         0xf << 4,            4)

#define MIPIB_CLW_DPHYCONTTX__LPTXCURR_no_additional_output_current           MIPIB_FIELD32R(0x0100, 0x03f3, 3, 0)
#define MIPIB_CLW_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current   MIPIB_FIELD32R(0x0100, 0x03f3, 3, 1)
#define MIPIB_CLW_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current_2 MIPIB_FIELD32R(0x0100, 0x03f3, 3, 2)
#define MIPIB_CLW_DPHYCONTTX__LPTXCURR_50_percent_additional_output_current   MIPIB_FIELD32R(0x0100, 0x03f3, 3, 3)


#define MIPIB_D0W_DPHYCONTTX__D0W_CAP__0pF     MIPIB_FIELD32R(0x0104, 0x03f3, 3 << 8, 0 << 8)
#define MIPIB_D0W_DPHYCONTTX__D0W_CAP__2_8pF   MIPIB_FIELD32R(0x0104, 0x03f3, 3 << 8, 1 << 8)
#define MIPIB_D0W_DPHYCONTTX__D0W_CAP__3_2pF   MIPIB_FIELD32R(0x0104, 0x03f3, 3 << 8, 2 << 8)
#define MIPIB_D0W_DPHYCONTTX__D0W_CAP__3_6pF   MIPIB_FIELD32R(0x0104, 0x03f3, 3 << 8, 3 << 8)

#define MIPIB_D0W_DPHYCONTTX__DLYCNTRL(value)   MIPIB_FIELD32R(0x0104, 0x03f3, 0xf << 4, (value) << 4)
#define MIPIB_D0W_DPHYCONTTX__DLYCNTRL_Read   MIPIB_READ_FIELD(0x0104,         0xf << 4,            4)

#define MIPIB_D0W_DPHYCONTTX__LPTXCURR_no_additional_output_current           MIPIB_FIELD32R(0x0104, 0x03f3, 3, 0)
#define MIPIB_D0W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current   MIPIB_FIELD32R(0x0104, 0x03f3, 3, 1)
#define MIPIB_D0W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current_2 MIPIB_FIELD32R(0x0104, 0x03f3, 3, 2)
#define MIPIB_D0W_DPHYCONTTX__LPTXCURR_50_percent_additional_output_current   MIPIB_FIELD32R(0x0104, 0x03f3, 3, 3)


#define MIPIB_D1W_DPHYCONTTX__D1W_CAP__0pF     MIPIB_FIELD32R(0x0108, 0x03f3, 3 << 8, 0 << 8)
#define MIPIB_D1W_DPHYCONTTX__D1W_CAP__2_8pF   MIPIB_FIELD32R(0x0108, 0x03f3, 3 << 8, 1 << 8)
#define MIPIB_D1W_DPHYCONTTX__D1W_CAP__3_2pF   MIPIB_FIELD32R(0x0108, 0x03f3, 3 << 8, 2 << 8)
#define MIPIB_D1W_DPHYCONTTX__D1W_CAP__3_6pF   MIPIB_FIELD32R(0x0108, 0x03f3, 3 << 8, 3 << 8)

#define MIPIB_D1W_DPHYCONTTX__DLYCNTRL(value)   MIPIB_FIELD32R(0x0108, 0x03f3, 0xf << 4, (value) << 4)
#define MIPIB_D1W_DPHYCONTTX__DLYCNTRL_Read   MIPIB_READ_FIELD(0x0108,         0xf << 4,            4)

#define MIPIB_D1W_DPHYCONTTX__LPTXCURR_no_additional_output_current           MIPIB_FIELD32R(0x0108, 0x03f3, 3, 0)
#define MIPIB_D1W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current   MIPIB_FIELD32R(0x0108, 0x03f3, 3, 1)
#define MIPIB_D1W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current_2 MIPIB_FIELD32R(0x0108, 0x03f3, 3, 2)
#define MIPIB_D1W_DPHYCONTTX__LPTXCURR_50_percent_additional_output_current   MIPIB_FIELD32R(0x0108, 0x03f3, 3, 3)



#define MIPIB_D2W_DPHYCONTTX__D2W_CAP__0pF     MIPIB_FIELD32R(0x010c, 0x03f3, 3 << 8, 0 << 8)
#define MIPIB_D2W_DPHYCONTTX__D2W_CAP__2_8pF   MIPIB_FIELD32R(0x010c, 0x03f3, 3 << 8, 1 << 8)
#define MIPIB_D2W_DPHYCONTTX__D2W_CAP__3_2pF   MIPIB_FIELD32R(0x010c, 0x03f3, 3 << 8, 2 << 8)
#define MIPIB_D2W_DPHYCONTTX__D2W_CAP__3_6pF   MIPIB_FIELD32R(0x010c, 0x03f3, 3 << 8, 3 << 8)

#define MIPIB_D2W_DPHYCONTTX__DLYCNTRL(value)   MIPIB_FIELD32R(0x010c, 0x03f3, 0xf << 4, (value) << 4)
#define MIPIB_D2W_DPHYCONTTX__DLYCNTRL_Read   MIPIB_READ_FIELD(0x010c,         0xf << 4,            4)

#define MIPIB_D2W_DPHYCONTTX__LPTXCURR_no_additional_output_current           MIPIB_FIELD32R(0x010c, 0x03f3, 3, 0)
#define MIPIB_D2W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current   MIPIB_FIELD32R(0x010c, 0x03f3, 3, 1)
#define MIPIB_D2W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current_2 MIPIB_FIELD32R(0x010c, 0x03f3, 3, 2)
#define MIPIB_D2W_DPHYCONTTX__LPTXCURR_50_percent_additional_output_current   MIPIB_FIELD32R(0x010c, 0x03f3, 3, 3)


#define MIPIB_D3W_DPHYCONTTX__D3W_CAP__0pF     MIPIB_FIELD32R(0x0110, 0x03f3, 3 << 8, 0 << 8)
#define MIPIB_D3W_DPHYCONTTX__D3W_CAP__2_8pF   MIPIB_FIELD32R(0x0110, 0x03f3, 3 << 8, 1 << 8)
#define MIPIB_D3W_DPHYCONTTX__D3W_CAP__3_2pF   MIPIB_FIELD32R(0x0110, 0x03f3, 3 << 8, 2 << 8)
#define MIPIB_D3W_DPHYCONTTX__D3W_CAP__3_6pF   MIPIB_FIELD32R(0x0110, 0x03f3, 3 << 8, 3 << 8)

#define MIPIB_D3W_DPHYCONTTX__DLYCNTRL(value)   MIPIB_FIELD32R(0x0110, 0x03f3, 0xf << 4, (value) << 4)
#define MIPIB_D3W_DPHYCONTTX__DLYCNTRL_Read   MIPIB_READ_FIELD(0x0110,         0xf << 4,            4)

#define MIPIB_D3W_DPHYCONTTX__LPTXCURR_no_additional_output_current           MIPIB_FIELD32R(0x0110, 0x03f3, 3, 0)
#define MIPIB_D3W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current   MIPIB_FIELD32R(0x0110, 0x03f3, 3, 1)
#define MIPIB_D3W_DPHYCONTTX__LPTXCURR_25_percent_additional_output_current_2 MIPIB_FIELD32R(0x0110, 0x03f3, 3, 2)
#define MIPIB_D3W_DPHYCONTTX__LPTXCURR_50_percent_additional_output_current   MIPIB_FIELD32R(0x0110, 0x03f3, 3, 3)


#define MIPIB_CLW_CNTRL__CLW_LaneDisable__Force_Disable                MIPIB_FIELD32R(0x0140, 0x0001, 0x0001, 0x0001)
#define MIPIB_CLW_CNTRL__CLW_LaneDisable__Bypass_from_PPI_Layer_Enable MIPIB_FIELD32R(0x0140, 0x0001, 0x0001, 0x0000)

#define MIPIB_D0W_CNTRL__D0W_LaneDisable__Force_Disable                MIPIB_FIELD32R(0x0144, 0x0001, 0x0001, 0x0001)
#define MIPIB_D0W_CNTRL__D0W_LaneDisable__Bypass_from_PPI_Layer_Enable MIPIB_FIELD32R(0x0144, 0x0001, 0x0001, 0x0000)

#define MIPIB_D1W_CNTRL__D1W_LaneDisable__Force_Disable                MIPIB_FIELD32R(0x0148, 0x0001, 0x0001, 0x0001)
#define MIPIB_D1W_CNTRL__D1W_LaneDisable__Bypass_from_PPI_Layer_Enable MIPIB_FIELD32R(0x0148, 0x0001, 0x0001, 0x0000)

#define MIPIB_D2W_CNTRL__D2W_LaneDisable__Force_Disable                MIPIB_FIELD32R(0x014c, 0x0001, 0x0001, 0x0001)
#define MIPIB_D2W_CNTRL__D2W_LaneDisable__Bypass_from_PPI_Layer_Enable MIPIB_FIELD32R(0x014c, 0x0001, 0x0001, 0x0000)

#define MIPIB_D3W_CNTRL__D3W_LaneDisable__Force_Disable                MIPIB_FIELD32R(0x0150, 0x0001, 0x0001, 0x0001)
#define MIPIB_D3W_CNTRL__D3W_LaneDisable__Bypass_from_PPI_Layer_Enable MIPIB_FIELD32R(0x0150, 0x0001, 0x0001, 0x0000)

#define MIPIB_STARTCNTRL__Check__START__Stop_function  MIPIB_FIELD32R(0x0204, 1, 1, 0)
// ^ use this only for checking. Writing 0 to this bit is invalid.
#define MIPIB_STARTCNTRL__START__Start_function        MIPIB_FIELD32R(0x0204, 1, 1, 1)

#define MIPIB_PPISTATUS__BUSY__Not_Busy    MIPIB_FIELD32R(0x0208, 1, 1, 0)
#define MIPIB_PPISTATUS__BUSY__Busy        MIPIB_FIELD32R(0x0208, 1, 1, 1)

#define MIPIB_LINEINITCNT__LINEINITCNT(value)     MIPIB_FIELD32R(0x0210, 0xffff, 0xffff, (value) << 0)
#define MIPIB_LINEINITCNT__LINEINITCNT_Read     MIPIB_READ_FIELD(0x0210,         0xffff,            0)

#define MIPIB_LPTXTIMECNT__LPTXTIMECNT(value)      MIPIB_FIELD32R(0x0214, 0x07ff, 0x07ff, (value) << 0)
#define MIPIB_LPTXTIMECNT__LPTXTIMECNT_Read   MIPIB_READ_FIELD(0x0214,         0x07ff,            0)

#define MIPIB_TCLK_HEADERCNT__TCLK_ZEROCNT(value)    MIPIB_FIELD32R(0x0218, 0xff7f, 0xff << 8, (value) << 8)
#define MIPIB_TCLK_HEADERCNT__TCLK_ZEROCNT_Read    MIPIB_READ_FIELD(0x0218,         0xff << 8,            8)
#define MIPIB_TCLK_HEADERCNT__TCLK_PREPARECNT(value) MIPIB_FIELD32R(0x0218, 0xff7f, 0x7f << 0, (value) << 0)
#define MIPIB_TCLK_HEADERCNT__TCLK_PREPARECNT_Read MIPIB_READ_FIELD(0x0218,         0x7f << 0,            0)

#define MIPIB_TCLK_TRAILCNT__TCLKTRAILCNT(value)  MIPIB_FIELD32R(0x021C, 0x00ff, 0x00ff, (value) << 0)
#define MIPIB_TCLK_TRAILCNT__TCLKTRAILCNT_Read  MIPIB_READ_FIELD(0x021C, 0x00ff, 0x00ff, (value) << 0)

#define MIPIB_THS_HEADERCNT__THS_ZEROCNT(value)    MIPIB_FIELD32R(0x0220, 0x7f7f, 0x7f << 8, (value) << 8)
#define MIPIB_THS_HEADERCNT__THS_ZEROCNT_Read    MIPIB_READ_FIELD(0x0220,         0x7f << 8,            8)
#define MIPIB_THS_HEADERCNT__THS_PREPARECNT(value) MIPIB_FIELD32R(0x0220, 0x7f7f, 0x7f << 0, (value) << 0)
#define MIPIB_THS_HEADERCNT__THS_PREPARECNT_Read MIPIB_READ_FIELD(0x0220,         0x7f << 0,            0)

#define MIPIB_TWAKEUP__TWAKEUPCNT(value)  MIPIB_FIELD32R(0x0224, 0xffff, 0xffff, (value) << 0)
#define MIPIB_TWAKEUP__TWAKEUPCNT_Read  MIPIB_READ_FIELD(0x0224,         0xffff,            0)

#define MIPIB_TCLK_POSTCNT__TCLK_POSTCNT(value)   MIPIB_FIELD32R(0x0228, 0x07ff, 0x07ff, (value) << 0)
#define MIPIB_TCLK_POSTCNT__TCLK_POSTCNT_Read   MIPIB_READ_FIELD(0x0228,         0x07ff,            0)

#define MIPIB_THS_TRAILCNT__THS_TRAILCNT(value)   MIPIB_FIELD32R(0x022C, 0x000f, 0x000f, (value) << 0)
#define MIPIB_THS_TRAILCNT__THS_TRAILCNT_Read   MIPIB_READ_FIELD(0x022C,         0x000f,            0)

#define MIPIB_HSTXVREGCNT__HSTXVREGCNT(value) MIPIB_FIELD32R(0x0230, 0xffff, 0xffff, (value) << 0)
#define MIPIB_HSTXVREGCNT__HSTXVREGCNT_Read MIPIB_READ_FIELD(0x0230,         0xffff,            0)

#define MIPIB_HSTXVREGEN__D3M_HSTXVREGEN__Disable  MIPIB_FIELD32R(0x0234, 0x001f, 1 << 4, 0 << 4)
#define MIPIB_HSTXVREGEN__D3M_HSTXVREGEN__Enable   MIPIB_FIELD32R(0x0234, 0x001f, 1 << 4, 1 << 4)
#define MIPIB_HSTXVREGEN__D2M_HSTXVREGEN__Disable  MIPIB_FIELD32R(0x0234, 0x001f, 1 << 3, 0 << 3)
#define MIPIB_HSTXVREGEN__D2M_HSTXVREGEN__Enable   MIPIB_FIELD32R(0x0234, 0x001f, 1 << 3, 1 << 3)
#define MIPIB_HSTXVREGEN__D1M_HSTXVREGEN__Disable  MIPIB_FIELD32R(0x0234, 0x001f, 1 << 2, 0 << 2)
#define MIPIB_HSTXVREGEN__D1M_HSTXVREGEN__Enable   MIPIB_FIELD32R(0x0234, 0x001f, 1 << 2, 1 << 2)
#define MIPIB_HSTXVREGEN__D0M_HSTXVREGEN__Disable  MIPIB_FIELD32R(0x0234, 0x001f, 1 << 1, 0 << 1)
#define MIPIB_HSTXVREGEN__D0M_HSTXVREGEN__Enable   MIPIB_FIELD32R(0x0234, 0x001f, 1 << 1, 1 << 1)
#define MIPIB_HSTXVREGEN__CLM_HSTXVREGEN__Disable  MIPIB_FIELD32R(0x0234, 0x001f, 1 << 0, 0 << 0)
#define MIPIB_HSTXVREGEN__CLM_HSTXVREGEN__Enable   MIPIB_FIELD32R(0x0234, 0x001f, 1 << 0, 1 << 0)

#define MIPIB_TXOPTIONCNTRL__CONTCLKMODE__Non_continuous  MIPIB_FIELD32R(0x0238, 1, 1, 0)
#define MIPIB_TXOPTIONCNTRL__CONTCLKMODE__Continuous      MIPIB_FIELD32R(0x0238, 1, 1, 1)

#define MIPIB_CSI_CONTROL__Check__Csi_mode__CSI_MOde       MIPIB_FIELD(0x040C, 0x84e7, 0x8000, 0x8000)

#define MIPIB_CSI_CONTROL__Check__HtxToEn__Enable_HTX_TO  MIPIB_FIELD(0x040c, 0x84e7, 0x0400, 0x0000)
#define MIPIB_CSI_CONTROL__Check__HtxToEn__Disable_HTX_TO MIPIB_FIELD(0x040c, 0x84e7, 0x0400, 0x0400)

#define MIPIB_CSI_CONTROL__Check__TxMd__High_Speed_TX     MIPIB_FIELD(0x040c, 0x84e7, 1 << 7, 1 << 7)
#define MIPIB_CSI_CONTROL__Check__TxMd__Low_power_TX      MIPIB_FIELD(0x040c, 0x84e7, 1 << 7, 0 << 7)

#define MIPIB_CSI_CONTROL__Check__HsCkMd__continuous      MIPIB_FIELD(0x040c, 0x84e7, 1 << 5, 1 << 5)
#define MIPIB_CSI_CONTROL__Check__HsCkMd__discontinuous   MIPIB_FIELD(0x040c, 0x84e7, 1 << 5, 0 << 5)

#define MIPIB_CSI_CONTROL__Check__NOL__data_lane_just_0   MIPIB_FIELD(0x040c, 0x84e7, 3 << 1, 0 << 1)
#define MIPIB_CSI_CONTROL__Check__NOL__data_lanes_0_to_1  MIPIB_FIELD(0x040c, 0x84e7, 3 << 1, 1 << 1)
#define MIPIB_CSI_CONTROL__Check__NOL__data_lanes_0_to_2  MIPIB_FIELD(0x040c, 0x84e7, 3 << 1, 2 << 1)
#define MIPIB_CSI_CONTROL__Check__NOL__data_lanes_0_to_3  MIPIB_FIELD(0x040c, 0x84e7, 3 << 1, 3 << 1)

#define MIPIB_CSI_CONTROL__Check__EoTDis__EOT_not_auto_granted_and_not_transmitted MIPIB_FIELD(0x040c, 0x84e7, 1, 1)
#define MIPIB_CSI_CONTROL__Check__EoTDis__EOT_auto_granted_and_transmitted         MIPIB_FIELD(0x040c, 0x84e7, 1, 0)

// Warning: Evil register naming scheme. Do not confuse with CSIStatus!
#define MIPIB_CSI_STATUS__WSync__Waiting_For_Sync           MIPIB_FIELD(0x0410, 0x0701, 1 << 10, 1 << 10)
#define MIPIB_CSI_STATUS__WSync__Not_Waiting_For_Sync       MIPIB_FIELD(0x0410, 0x0701, 1 << 10, 1 << 10)
#define MIPIB_CSI_STATUS__TxAct__CSITX_in_Transmit_Mode     MIPIB_FIELD(0x0410, 0x0701, 1 << 9, 1 << 9)
#define MIPIB_CSI_STATUS__TxAct__CSITX_not_in_Transmit_Mode MIPIB_FIELD(0x0410, 0x0701, 1 << 9, 0 << 9)
#define MIPIB_CSI_STATUS__RxAct__CSITX_in_Receive_Mode      MIPIB_FIELD(0x0410, 0x0701, 1 << 8, 1 << 8)
#define MIPIB_CSI_STATUS__RxAct__CSITX_not_in_Receive_Mode  MIPIB_FIELD(0x0410, 0x0701, 1 << 8, 0 << 8)
#define MIPIB_CSI_STATUS__Hlt__CSITX_is_stopped             MIPIB_FIELD(0x0410, 0x0701, 1 << 0, 1 << 0)
#define MIPIB_CSI_STATUS__Hlt__CSITX_is_not_stopped         MIPIB_FIELD(0x0410, 0x0701, 1 << 0, 0 << 0)

#define MIPIB_CSI_INT_L__IntHlt__received     MIPIB_FIELD(0x0414, 0x000c, 1 << 3, 1 << 3)
#define MIPIB_CSI_INT_L__IntHlt__not_received MIPIB_FIELD(0x0414, 0x000c, 1 << 3, 0 << 3)

#define MIPIB_CSI_INT_L__IntEr__received      MIPIB_FIELD(0x0414, 0x000c, 1 << 2, 1 << 2)
#define MIPIB_CSI_INT_L__IntEr__not_received  MIPIB_FIELD(0x0414, 0x000c, 1 << 2, 0 << 2)

#define MIPIB_CSI_INT_H__IntAck__received     MIPIB_FIELD(0x0414 + 2, 0x0004 << (16 - 16), 1 << (18 - 16), 1 << (18 - 16))
#define MIPIB_CSI_INT_H__IntAck__not_received MIPIB_FIELD(0x0414 + 2, 0x0004 << (16 - 16), 1 << (18 - 16), 0 << (18 - 16))

#define MIPIB_CSI_INT_ENA_H__Check__IEnAk__enabled  MIPIB_FIELD(0x0418 + 2, 0x0004 << (16 - 16), 1 << (18 - 16), 1 << (18 - 16))
#define MIPIB_CSI_INT_ENA_H__Check__IEnAk__disabled MIPIB_FIELD(0x0418 + 2, 0x0004 << (16 - 16), 1 << (18 - 16), 0 << (18 - 16))

#define MIPIB_CSI_INT_ENA_L__Check__IEnHlt__enabled  MIPIB_FIELD(0x0418, 0x000c, 1 << 3, 1 << 3)
#define MIPIB_CSI_INT_ENA_L__Check__IEnHlt__disabled MIPIB_FIELD(0x0418, 0x000c, 1 << 3, 0 << 3)

#define MIPIB_CSI_INT_ENA_L__Check__IEnEr__enabled   MIPIB_FIELD(0x0418, 0x000c, 1 << 2, 1 << 2)
#define MIPIB_CSI_INT_ENA_L__Check__IEnEr__disabled  MIPIB_FIELD(0x0418, 0x000c, 1 << 2, 0 << 2)

#define MIPIB_CSI_ERR__InEr__occurred                 MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 9, 1 << 9)
#define MIPIB_CSI_ERR__InEr__did_not_occur            MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 9, 0 << 9)
#define MIPIB_CSI_ERR__WCEr__occurred                 MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 8, 1 << 8)
#define MIPIB_CSI_ERR__WCEr__did_not_occur            MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 8, 0 << 8)
#define MIPIB_CSI_ERR__SynTo__occurred                MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 7, 1 << 7)
#define MIPIB_CSI_ERR__SynTo__did_not_occur           MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 7, 0 << 7)
#define MIPIB_CSI_ERR__RxFRdEr__occurred              MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 6, 1 << 6)
#define MIPIB_CSI_ERR__RxFRdEr__did_not_occur         MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 6, 0 << 6)
#define MIPIB_CSI_ERR__TeEr__occurred                 MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 5, 1 << 5)
#define MIPIB_CSI_ERR__TeEr__did_not_occur            MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 5, 0 << 5)
#define MIPIB_CSI_ERR__QUnk__occurred                 MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 4, 1 << 4)
#define MIPIB_CSI_ERR__QUnk__did_not_occur            MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 4, 0 << 4)
#define MIPIB_CSI_ERR__QWrEr__occurred                MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 3, 1 << 3)
#define MIPIB_CSI_ERR__QWrEr__did_not_occur           MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 3, 0 << 3)
#define MIPIB_CSI_ERR__HTxTo__occurred                MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 2, 1 << 2)
#define MIPIB_CSI_ERR__HTxTo__did_not_occur           MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 2, 0 << 2)
#define MIPIB_CSI_ERR__HTxBrk__occurred               MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 1, 1 << 1)
#define MIPIB_CSI_ERR__HTxBrk__did_not_occur          MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 1, 0 << 1)
#define MIPIB_CSI_ERR__Cntn__contention_occurred      MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 0, 1 << 0)
#define MIPIB_CSI_ERR__Cntn__contention_did_not_occur MIPIB_FIELD32R(0x044c, 0x03ff, 1 << 0, 0 << 0)

#define MIPIB_CSI_ERR_INTENA__Check__InEr__enabled                 MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 9, 1 << 9)
#define MIPIB_CSI_ERR_INTENA__Check__InEr__disabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 9, 0 << 9)
#define MIPIB_CSI_ERR_INTENA__Check__WCEr__enabled                 MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 8, 1 << 8)
#define MIPIB_CSI_ERR_INTENA__Check__WCEr__disabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 8, 0 << 8)
#define MIPIB_CSI_ERR_INTENA__Check__SynTo__enabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 7, 1 << 7)
#define MIPIB_CSI_ERR_INTENA__Check__SynTo__disabled               MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 7, 0 << 7)
#define MIPIB_CSI_ERR_INTENA__Check__RxFRdEr__enabled              MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 6, 1 << 6)
#define MIPIB_CSI_ERR_INTENA__Check__RxFRdEr__disabled             MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 6, 0 << 6)
#define MIPIB_CSI_ERR_INTENA__Check__TeEr__enabled                 MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 5, 1 << 5)
#define MIPIB_CSI_ERR_INTENA__Check__TeEr__disabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 5, 0 << 5)
#define MIPIB_CSI_ERR_INTENA__Check__QUnk__enabled                 MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 4, 1 << 4)
#define MIPIB_CSI_ERR_INTENA__Check__QUnk__disabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 4, 0 << 4)
#define MIPIB_CSI_ERR_INTENA__Check__QWrEr__enabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 3, 1 << 3)
#define MIPIB_CSI_ERR_INTENA__Check__QWrEr__disabled               MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 3, 0 << 3)
#define MIPIB_CSI_ERR_INTENA__Check__HTxTo__enabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 2, 1 << 2)
#define MIPIB_CSI_ERR_INTENA__Check__HTxTo__disabled               MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 2, 0 << 2)
#define MIPIB_CSI_ERR_INTENA__Check__HTxBrk__enabled               MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 1, 1 << 1)
#define MIPIB_CSI_ERR_INTENA__Check__HTxBrk__disabled              MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 1, 0 << 1)
#define MIPIB_CSI_ERR_INTENA__Check__Cntn__enabled                 MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 0, 1 << 0)
#define MIPIB_CSI_ERR_INTENA__Check__Cntn__disabled                MIPIB_FIELD32R(0x0450, 0x03ff, 1 << 0, 0 << 0)

#define MIPIB_CSI_ERR_HALT__Check__InEr__stop_on_error                 MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 9, 1 << 9)
#define MIPIB_CSI_ERR_HALT__Check__InEr__dont_stop_on_error            MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 9, 0 << 9)
#define MIPIB_CSI_ERR_HALT__Check__WCEr__stop_on_error                 MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 8, 1 << 8)
#define MIPIB_CSI_ERR_HALT__Check__WCEr__dont_stop_on_error            MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 8, 0 << 8)
#define MIPIB_CSI_ERR_HALT__Check__SynTo__stop_on_error                MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 7, 1 << 7)
#define MIPIB_CSI_ERR_HALT__Check__SynTo__dont_stop_on_error           MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 7, 0 << 7)
#define MIPIB_CSI_ERR_HALT__Check__RxFRdEr__stop_on_error              MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 6, 1 << 6)
#define MIPIB_CSI_ERR_HALT__Check__RxFRdEr__dont_stop_on_error         MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 6, 0 << 6)
#define MIPIB_CSI_ERR_HALT__Check__TeEr__stop_on_error                 MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 5, 1 << 5)
#define MIPIB_CSI_ERR_HALT__Check__TeEr__dont_stop_on_error            MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 5, 0 << 5)
#define MIPIB_CSI_ERR_HALT__Check__QUnk__stop_on_error                 MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 4, 1 << 4)
#define MIPIB_CSI_ERR_HALT__Check__QUnk__dont_stop_on_error            MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 4, 0 << 4)
#define MIPIB_CSI_ERR_HALT__Check__QWrEr__stop_on_error                MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 3, 1 << 3)
#define MIPIB_CSI_ERR_HALT__Check__QWrEr__dont_stop_on_error           MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 3, 0 << 3)
#define MIPIB_CSI_ERR_HALT__Check__HTxTo__stop_on_error                MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 2, 1 << 2)
#define MIPIB_CSI_ERR_HALT__Check__HTxTo__dont_stop_on_error           MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 2, 0 << 2)
#define MIPIB_CSI_ERR_HALT__Check__HTxBrk__stop_on_error               MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 1, 1 << 1)
#define MIPIB_CSI_ERR_HALT__Check__HTxBrk__dont_stop_on_error          MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 1, 0 << 1)
#define MIPIB_CSI_ERR_HALT__Check__Cntn__stop_on_error                 MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 0, 1 << 0)
#define MIPIB_CSI_ERR_HALT__Check__Cntn__dont_stop_on_error            MIPIB_FIELD32R(0x0454, 0x03ff, 1 << 0, 0 << 0)

// for CONFW data it makes no sense to read back values so we set the nonreserved bits to equal the field bits
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__Csi_mode__CSI_MOde      MIPIB_FIELD(0x0500, 0x8000, 0x8000, 0x8000)

#define MIPIB_CSI_CONFW_L__CSI_CONTROL__HtxToEn__Enable_HTX_TO  MIPIB_FIELD(0x0500, 0x0400, 0x0400, 0x0000)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__HtxToEn__Disable_HTX_TO MIPIB_FIELD(0x0500, 0x0400, 0x0400, 0x0400)

#define MIPIB_CSI_CONFW_L__CSI_CONTROL__TxMd__High_Speed_TX     MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 1 << 7)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__TxMd__Low_power_TX      MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 0 << 7)

#define MIPIB_CSI_CONFW_L__CSI_CONTROL__HsCkMd__continuous      MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 1 << 5)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__HsCkMd__discontinuous   MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 0 << 5)

#define MIPIB_CSI_CONFW_L__CSI_CONTROL__NOL__data_lane_just_0   MIPIB_FIELD(0x0500, 3 << 1, 3 << 1, 0 << 1)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__NOL__data_lanes_0_to_1  MIPIB_FIELD(0x0500, 3 << 1, 3 << 1, 1 << 1)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__NOL__data_lanes_0_to_2  MIPIB_FIELD(0x0500, 3 << 1, 3 << 1, 2 << 1)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__NOL__data_lanes_0_to_3  MIPIB_FIELD(0x0500, 3 << 1, 3 << 1, 3 << 1)

#define MIPIB_CSI_CONFW_L__CSI_CONTROL__EoTDis__EOT_not_auto_granted_and_not_transmitted MIPIB_FIELD(0x0500, 1, 1, 1)
#define MIPIB_CSI_CONFW_L__CSI_CONTROL__EoTDis__EOT_auto_granted_and_transmitted         MIPIB_FIELD(0x0500, 1, 1, 0)

#define MIPIB_CSI_CONFW_H__CSI_CONTROL__Set_Bits                MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xa300)
#define MIPIB_CSI_CONFW_H__CSI_CONTROL__Clear_Bits              MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xc300)
// the following macro is to be used in an array of field specs:
#define MIPIB_CSI_CONFW_H__CSI_CONTROL__Set_and_Clear_Bits(spec) \
    (spec), \
    MIPIB_CSI_CONFW_H__CSI_CONTROL__Set_Bits, \
    (spec) ^ MIPIB_GET_FIELD_MASK(spec), \
    MIPIB_CSI_CONFW_H__CSI_CONTROL__Clear_Bits

// it's not clear if it's possible to set/clear the higher 16 bits of CSI_INT_ENA

#define MIPIB_CSI_CONFW_L__CSI_INT_ENA_L__IEnHlt__enabled  MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 1 << 3)
#define MIPIB_CSI_CONFW_L__CSI_INT_ENA_L__IEnHlt__disabled MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 0 << 3)

#define MIPIB_CSI_CONFW_L__CSI_INT_ENA_L__IEnEr__enabled   MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 1 << 2)
#define MIPIB_CSI_CONFW_L__CSI_INT_ENA_L__IEnEr__disabled  MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 0 << 2)

#define MIPIB_CSI_CONFW_H__CSI_INT_ENA_L__Set_Bits         MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xa600)
#define MIPIB_CSI_CONFW_H__CSI_INT_ENA_L__Clear_Bits       MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xc600)
// the following macro is to be used in an array of field specs:
#define MIPIB_CSI_CONFW_H__CSI_INT_ENA_L__Set_and_Clear_Bits(spec) \
    (spec), \
    MIPIB_CSI_CONFW_H__CSI_INT_ENA_L__Set_Bits, \
    (spec) ^ MIPIB_GET_FIELD_MASK(spec), \
    MIPIB_CSI_CONFW_H__CSI_INT_ENA_L__Clear_Bits

#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__InEr__enabled                 MIPIB_FIELD(0x0500, 1 << 9, 1 << 9, 1 << 9)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__InEr__disabled                MIPIB_FIELD(0x0500, 1 << 9, 1 << 9, 0 << 9)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__WCEr__enabled                 MIPIB_FIELD(0x0500, 1 << 8, 1 << 8, 1 << 8)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__WCEr__disabled                MIPIB_FIELD(0x0500, 1 << 8, 1 << 8, 0 << 8)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__SynTo__enabled                MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 1 << 7)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__SynTo__disabled               MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 0 << 7)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__RxFRdEr__enabled              MIPIB_FIELD(0x0500, 1 << 6, 1 << 6, 1 << 6)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__RxFRdEr__disabled             MIPIB_FIELD(0x0500, 1 << 6, 1 << 6, 0 << 6)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__TeEr__enabled                 MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 1 << 5)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__TeEr__disabled                MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 0 << 5)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__QUnk__enabled                 MIPIB_FIELD(0x0500, 1 << 4, 1 << 4, 1 << 4)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__QUnk__disabled                MIPIB_FIELD(0x0500, 1 << 4, 1 << 4, 0 << 4)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__QWrEr__enabled                MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 1 << 3)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__QWrEr__disabled               MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 0 << 3)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__HTxTo__enabled                MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 1 << 2)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__HTxTo__disabled               MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 0 << 2)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__HTxBrk__enabled               MIPIB_FIELD(0x0500, 1 << 1, 1 << 1, 1 << 1)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__HTxBrk__disabled              MIPIB_FIELD(0x0500, 1 << 1, 1 << 1, 0 << 1)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__Cntn__enabled                 MIPIB_FIELD(0x0500, 1 << 0, 1 << 0, 1 << 0)
#define MIPIB_CSI_CONFW_L__CSI_ERR_INTENA__Cntn__disabled                MIPIB_FIELD(0x0500, 1 << 0, 1 << 0, 0 << 0)

#define MIPIB_CSI_CONFW_H__CSI_ERR_INTENA__Set_Bits         MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xb400)
#define MIPIB_CSI_CONFW_H__CSI_ERR_INTENA__Clear_Bits       MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xd400)
// the following macro is to be used in an array of field specs:
#define MIPIB_CSI_CONFW_H__CSI_ERR_INTENA__Set_and_Clear_Bits(spec) \
    (spec), \
    MIPIB_CSI_CONFW_H__CSI_ERR_INTENA__Set_Bits, \
    (spec) ^ MIPIB_GET_FIELD_MASK(spec), \
    MIPIB_CSI_CONFW_H__CSI_ERR_INTENA__Clear_Bits

#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__InEr__stop_on_error                 MIPIB_FIELD(0x0500, 1 << 9, 1 << 9, 1 << 9)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__InEr__dont_stop_on_error            MIPIB_FIELD(0x0500, 1 << 9, 1 << 9, 0 << 9)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__WCEr__stop_on_error                 MIPIB_FIELD(0x0500, 1 << 8, 1 << 8, 1 << 8)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__WCEr__dont_stop_on_error            MIPIB_FIELD(0x0500, 1 << 8, 1 << 8, 0 << 8)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__SynTo__stop_on_error                MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 1 << 7)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__SynTo__dont_stop_on_error           MIPIB_FIELD(0x0500, 1 << 7, 1 << 7, 0 << 7)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__RxFRdEr__stop_on_error              MIPIB_FIELD(0x0500, 1 << 6, 1 << 6, 1 << 6)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__RxFRdEr__dont_stop_on_error         MIPIB_FIELD(0x0500, 1 << 6, 1 << 6, 0 << 6)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__TeEr__stop_on_error                 MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 1 << 5)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__TeEr__dont_stop_on_error            MIPIB_FIELD(0x0500, 1 << 5, 1 << 5, 0 << 5)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__QUnk__stop_on_error                 MIPIB_FIELD(0x0500, 1 << 4, 1 << 4, 1 << 4)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__QUnk__dont_stop_on_error            MIPIB_FIELD(0x0500, 1 << 4, 1 << 4, 0 << 4)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__QWrEr__stop_on_error                MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 1 << 3)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__QWrEr__dont_stop_on_error           MIPIB_FIELD(0x0500, 1 << 3, 1 << 3, 0 << 3)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__HTxTo__stop_on_error                MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 1 << 2)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__HTxTo__dont_stop_on_error           MIPIB_FIELD(0x0500, 1 << 2, 1 << 2, 0 << 2)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__HTxBrk__stop_on_error               MIPIB_FIELD(0x0500, 1 << 1, 1 << 1, 1 << 1)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__HTxBrk__dont_stop_on_error          MIPIB_FIELD(0x0500, 1 << 1, 1 << 1, 0 << 1)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__Cntn__stop_on_error                 MIPIB_FIELD(0x0500, 1 << 0, 1 << 0, 1 << 0)
#define MIPIB_CSI_CONFW_L__CSI_ERR_HALT__Cntn__dont_stop_on_error            MIPIB_FIELD(0x0500, 1 << 0, 1 << 0, 0 << 0)

#define MIPIB_CSI_CONFW_H__CSI_ERR_HALT__Set_Bits         MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xb500)
#define MIPIB_CSI_CONFW_H__CSI_ERR_HALT__Clear_Bits       MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0xd500)
// the following macro is to be used in an array of field specs:
#define MIPIB_CSI_CONFW_H__CSI_ERR_HALT__Set_and_Clear_Bits(spec) \
    (spec), \
    MIPIB_CSI_CONFW_H__CSI_ERR_HALT__Set_Bits, \
    (spec) ^ MIPIB_GET_FIELD_MASK(spec), \
    MIPIB_CSI_CONFW_H__CSI_ERR_HALT__Clear_Bits

#define MIPIB_CSI_LPCMD_L__LANE_ENA_Clock_Lanes__selected     MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 1 << 3)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Clock_Lanes__not_selected MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 0 << 3)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_0__selected          MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 1 << 4)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_0__not_selected      MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 0 << 4)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_1__selected          MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 1 << 5)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_1__not_selected      MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 0 << 5)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_2__selected          MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 1 << 6)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_2__not_selected      MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 0 << 6)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_3__selected          MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 1 << 7)
#define MIPIB_CSI_LPCMD_L__LANE_ENA_Lane_3__not_selected      MIPIB_FIELD(0x0500, 0x00ff, 0x00f8, 0 << 7)
#define MIPIB_CSI_LPCMD_L__LP_CODE__Lane_transitions_to_ULPS                     MIPIB_FIELD(0x0500, 0x00ff, 0x0007, 0)
#define MIPIB_CSI_LPCMD_L__LP_CODE__Lane_transitions_to_LP_stop                  MIPIB_FIELD(0x0500, 0x00ff, 0x0007, 1)
#define MIPIB_CSI_LPCMD_L__LP_CODE__Remote_application_reset_to_Lane_0_then_stop MIPIB_FIELD(0x0500, 0x00ff, 0x0007, 2)
#define MIPIB_CSI_LPCMD_L__LP_CODE__Bus_direction_change_on_Lane_0               MIPIB_FIELD(0x0500, 0x00ff, 0x0007, 3)

#define MIPIB_CSI_LPCMD_H__LPCommand_Mode    MIPIB_FIELD(0x0500 + 2, 0xff00, 0xff00, 0x3000)

#define MIPIB_CSI_RESET__RstCnf__setting_register_is_reset        MIPIB_FIELD32R(0x0504, 0x0003, 1 << 1, 1 << 1)
#define MIPIB_CSI_RESET__RstCnf__operation_not_affected           MIPIB_FIELD32R(0x0504, 0x0003, 1 << 1, 0 << 1)
#define MIPIB_CSI_RESET__RstModule__CSI_modules_reset_but_not_PHY MIPIB_FIELD32R(0x0504, 0x0003, 1 << 0, 1 << 0)
#define MIPIB_CSI_RESET__RstModule__operation_not_affected        MIPIB_FIELD32R(0x0504, 0x0003, 1 << 0, 0 << 0)

#define MIPIB_CSI_INT_CLR_L__IntHlt__received     MIPIB_FIELD(0x050c, 0x000c, 1 << 3, 1 << 3)
#define MIPIB_CSI_INT_CLR_L__IntHlt__not_received MIPIB_FIELD(0x050c, 0x000c, 1 << 3, 0 << 3)

#define MIPIB_CSI_INT_CLR_L__IntEr__received      MIPIB_FIELD(0x050c, 0x000c, 1 << 2, 1 << 2)
#define MIPIB_CSI_INT_CLR_L__IntEr__not_received  MIPIB_FIELD(0x050c, 0x000c, 1 << 2, 0 << 2)

#define MIPIB_CSI_INT_CLR_H__IntAck__received     MIPIB_FIELD(0x050c + 2, 1 << (18 - 16), 1 << (18 - 16), 1 << (18 - 16))
#define MIPIB_CSI_INT_CLR_H__IntAck__not_received MIPIB_FIELD(0x050c + 2, 1 << (18 - 16), 1 << (18 - 16), 0 << (18 - 16))

#define MIPIB_CSI_START__Strt__Start              MIPIB_FIELD32R(0x0518, 1, 1, 1)
// after setting to 1, writing of 0 is not allowed.

#define MIPIB_DBG_LCNT__db_wsram__enable_I2C_write_to_VB_sram MIPIB_FIELD(0x00e0, 0xc3ff, 1 << 15, 1 << 15)
#define MIPIB_DBG_LCNT__db_wsram__normal                      MIPIB_FIELD(0x00e0, 0xc3ff, 1 << 15, 0 << 15)
#define MIPIB_DBG_LCNT__db_cen__debug_mode_color_bar_logic    MIPIB_FIELD(0x00e0, 0xc3ff, 1 << 14, 1 << 14)
#define MIPIB_DBG_LCNT__db_cen__normal                        MIPIB_FIELD(0x00e0, 0xc3ff, 1 << 14, 0 << 14)
#define MIPIB_DBG_LCNT__db_active_line_count(value)           MIPIB_FIELD(0x00e0, 0xc3ff, 0x03ff, (value) << 0)
#define MIPIB_DBG_LCNT__db_active_line_count_Read        MIPIB_READ_FIELD(0x00e0,         0x03ff,            0)

#define MIPIB_DBG_Width__Db_width(value)                      MIPIB_FIELD(0x00e2, 0x0fff, 0x0fff, (value) << 0)
#define MIPIB_DBG_Width__Db_width_Read                   MIPIB_READ_FIELD(0x00e2,         0x0fff,            0)

#define MIPIB_DBG_VBlank__Db_vb(value)                        MIPIB_FIELD(0x00e4, 0x007f, 0x007f, (value) << 0)
#define MIPIB_DBG_VBlank__Db_vb_Read                     MIPIB_READ_FIELD(0x00e4,         0x007f,            0)

#define MIPIB_DBG_Data__Db_data(value)                        MIPIB_FIELD(0x00e8, 0xffff, 0xffff, (value) << 0)

#endif // IC_TC358746_DEF_H

