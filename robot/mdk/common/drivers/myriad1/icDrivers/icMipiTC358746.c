///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief      Driver for Toshiba mipi bridge chip TC358746
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <isaac_registers.h>
#include <mv_types.h>
#include <DrvGpio.h>
#include <icMipiTC358746.h>
#include <DrvI2cMaster.h>
#include <DrvTimer.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

#define DRV_TC358746_DEBUG1  // Level 1: Debug Messages (Highest priority, e.g. Errors)
//#define DRV_TC358746_DEBUG2  // Level 2:
//#define DRV_TC358746_DEBUG3  // Level 3: per-transaction messages
#define CHECK_REGISTERS_WERE_WRITTEN_CORRECTLY

#ifdef DRV_TC358746_DEBUG1
#include <stdio.h>
#define DPRINTF1(...)  printf(__VA_ARGS__)
#else
#define DPRINTF1(...)  {}
#endif

#ifdef DRV_TC358746_DEBUG2
#include <stdio.h>
#define DPRINTF2(...)  printf(__VA_ARGS__)
#else
#define DPRINTF2(...)  {}
#endif

#ifdef DRV_TC358746_DEBUG3
#include <stdio.h>
#define DPRINTF3(...)  printf(__VA_ARGS__)
#else
#define DPRINTF3(...)  {}
#endif

#define L_ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

const u16 icTC358746noReadbackWrittenValueRegisters[] = {
        0x500, 0x502,
        0x518,
        0x204,
        0x0064
};

const int icTC358746noReadbackWrittenValueRegistersCount = L_ARRAY_SIZE(icTC358746noReadbackWrittenValueRegisters);

const u64 icTC358746CommonDefaultRegisterFieldValues[] = { // TODO
        MIPIB_ChipID__ChipID_0x440X,

        ( MIPIB_SysCtl__SLEEP__Sleep_mode |
          MIPIB_SysCtl__Sreset__Normal_operation ),

        (
          // MIPIB_ConfCtl__TriEn__Enable | // note: Enable is the real default,
          // but icTC358746Reset sets it to a Disable default. So better not to test this bit.
          MIPIB_ConfCtl__INTE2n__Normal |
          MIPIB_ConfCtl__Bt656En__Disable |
          MIPIB_ConfCtl__PdataF__Mode_0 |
          MIPIB_ConfCtl__PPEn__Disable |
          MIPIB_ConfCtl__VsyncP__active_high |
          MIPIB_ConfCtl__HsyncP__active_high |
          MIPIB_ConfCtl__PCLKP__Normal |
          MIPIB_ConfCtl__Auto__INCREMENT |
          MIPIB_ConfCtl__DataLanes__NR_1 ),

        // The following is not as in the datasheet:
        // MIPIB_FiFoCtl__FifoLevel(0x0001),

        ( MIPIB_DataFmt__PDFormat__RAW8 |
          MIPIB_DataFmt__UDT_en__CSITX_from_PDFormat_reg ),

        ( MIPIB_ClkCtl__PPIclkDiv__DIV2 |
          MIPIB_ClkCtl__MclkRefDiv__DIV8 |
          MIPIB_ClkCtl__SclkDiv__DIV8 ),

          MIPIB_GPIOEn__GPIOEn(0),

          MIPIB_GPIODir__GPIODir(0xffff),

          MIPIB_GPIOOut__GPIOOut(0),

        ( MIPIB_PLLCtl0__PLL_PRD(0x4) |
          MIPIB_PLLCtl0__PLL_FBD(0x063) ),

        ( MIPIB_PLLCtl1__PLL_FRS__250M_500M_HSCK_freq |
          MIPIB_PLLCtl1__PLL_LBWS__PERCENT_50 |
          MIPIB_PLLCtl1__PLL_LFBREN__no_oscillation |
          MIPIB_PLLCtl1__PLL_BYPCKEN__Normal_operation |
          MIPIB_PLLCtl1__PLL_CKEN__clocks_off |
          MIPIB_PLLCtl1__PLL_RESETB__Reset |
          MIPIB_PLLCtl1__PLL_EN__off ),

        ( MIPIB_ClkCtl__PPIclkDiv__DIV2 |
          MIPIB_ClkCtl__MclkRefDiv__DIV8 |
          MIPIB_ClkCtl__SclkDiv__DIV8 ),

        MIPIB_WordCnt__WordCnt(0x0100),

        MIPIB_USER_DT__user_dt(0x30),

        ( MIPIB_PHYClkCtl__ClkDly(0) |
          MIPIB_PHYClkCtl__HsRxRs__2_00k |
          MIPIB_PHYClkCtl__Cap__no_additional_capacitance ),

        ( MIPIB_PHYData0Ctl__DataDly(0) |
          MIPIB_PHYData0Ctl__HsRxRs__2_00k |
          MIPIB_PHYData0Ctl__Cap__no_additional_capacitance ),

        ( MIPIB_PHYData1Ctl__DataDly(0) |
          MIPIB_PHYData1Ctl__HsRxRs__2_00k |
          MIPIB_PHYData1Ctl__Cap__no_additional_capacitance ),

        ( MIPIB_PHYData2Ctl__DataDly(0) |
          MIPIB_PHYData2Ctl__HsRxRs__2_00k |
          MIPIB_PHYData2Ctl__Cap__no_additional_capacitance ),

        ( MIPIB_PHYData3Ctl__DataDly(0) |
          MIPIB_PHYData3Ctl__HsRxRs__2_00k |
          MIPIB_PHYData3Ctl__Cap__no_additional_capacitance ),

        ( MIPIB_PHYTimDly__DSettle(5) |
          MIPIB_PHYTimDly__Td_term_sel__HS_termination_after_2_to_3_PPIRxClk |
          MIPIB_PHYTimDly__Tc_term_sel__bad_default_is_0 ),

        ( MIPIB_CSIErrEn__MDLEn__do_not_assert_CSIErr_if_MDLErr_occurs |
          MIPIB_CSIErrEn__FrmEn__do_not_assert_CSIErr_if_FrmErr_occurs |
          MIPIB_CSIErrEn__CRCEn__do_not_assert_CSIErr_if_CRCErr_occurs |
          MIPIB_CSIErrEn__CorEn__do_not_assert_CSIErr_if_CorErr_occurs |
          MIPIB_CSIErrEn__HdrEn__do_not_assert_CSIErr_if_HdrErr_occurs |
          MIPIB_CSIErrEn__EIDEn__do_not_assert_CSIErr_if_EIDErr_occurs |
          MIPIB_CSIErrEn__CtlEn__do_not_assert_CSIErr_if_CtlErr_occurs |
          MIPIB_CSIErrEn__SoTEn__do_not_assert_CSIErr_if_SoTErr_occurs |
          MIPIB_CSIErrEn__SynEn__do_not_assert_CSIErr_if_SynErr_occurs ),

        ( MIPIB_CSIRX_DPCtl__rxch0_cntrl(0) |
          MIPIB_CSIRX_DPCtl__rxch1_cntrl(0) |
          MIPIB_CSIRX_DPCtl__rxch2_cntrl(0) |
          MIPIB_CSIRX_DPCtl__rxch3_cntrl(0) |
          MIPIB_CSIRX_DPCtl__rxck_cntrl(0) ),

        // The MIPIB_**W_DPHYCONTTX are not as in the datasheet

        // The following 5 are not as in the datasheet:
        // MIPIB_CLW_CNTRL__CLW_LaneDisable__Force_Disable,
        // MIPIB_D0W_CNTRL__D0W_LaneDisable__Force_Disable,
        // MIPIB_D1W_CNTRL__D1W_LaneDisable__Force_Disable,
        // MIPIB_D2W_CNTRL__D2W_LaneDisable__Force_Disable,
        // MIPIB_D3W_CNTRL__D3W_LaneDisable__Force_Disable,

        MIPIB_STARTCNTRL__Check__START__Stop_function,

        // The following are not as in the datasheet:
        // MIPIB_LINEINITCNT__LINEINITCNT(0x208e),
        // MIPIB_LPTXTIMECNT__LPTXTIMECNT(1),
        // MIPIB_TCLK_HEADERCNT__TCLK_ZEROCNT(1),

        // The following are not as in the datasheet:
        // MIPIB_TCLK_HEADERCNT__TCLK_PREPARECNT(1),
        // MIPIB_TCLK_TRAILCNT__TCLKTRAILCNT(1),
        // MIPIB_THS_HEADERCNT__THS_ZEROCNT(1),

        // The following are not as in the datasheet:
        // MIPIB_THS_HEADERCNT__THS_PREPARECNT(1),
        // MIPIB_TWAKEUP__TWAKEUPCNT(0x4e20),
        // MIPIB_TCLK_POSTCNT__TCLK_POSTCNT(0x200),
        // MIPIB_THS_TRAILCNT__THS_TRAILCNT(2),
        // MIPIB_HSTXVREGCNT__HSTXVREGCNT(0x0020),

        MIPIB_HSTXVREGEN__D3M_HSTXVREGEN__Disable,
        MIPIB_HSTXVREGEN__D2M_HSTXVREGEN__Disable,
        MIPIB_HSTXVREGEN__D1M_HSTXVREGEN__Disable,
        MIPIB_HSTXVREGEN__D0M_HSTXVREGEN__Disable,
        MIPIB_HSTXVREGEN__CLM_HSTXVREGEN__Disable,

        MIPIB_TXOPTIONCNTRL__CONTCLKMODE__Non_continuous,
};

const int icTC358746CommonDefaultRegisterFieldValuesCount = L_ARRAY_SIZE(icTC358746CommonDefaultRegisterFieldValues);

// 4: Static Local Data
// ----------------------------------------------------------------------------
// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

static u8 protoTC358746Write[] =
    {
        S_ADDR_WR,
        R_ADDR_H,
        R_ADDR_L,
        DATAW,
        LOOP_MINUS_1
    };

static u8 protoTC358746Read[] =
    {
        S_ADDR_WR,
        R_ADDR_H,
        R_ADDR_L,
        S_ADDR_RD,
        DATAR,
        LOOP_MINUS_1
    };

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void icTC358746Reset(TMipiBridge *bridge)
{
      int i;
      
      icTC358746SetCs(bridge, 0); // csi2 tx type bridges have to be reset with CS=0

      DPRINTF2("        INFO %s line:%d reset bridge %d\n",__FUNCTION__, __LINE__, i);

      if (bridge->ModeSelectGPIO != PIN_NOT_CONNECTED_TO_GPIO) {
          // set MSEL signal to the proper state
          if (bridge->BridgeType == CSI2RXParOut)
              DrvGpioSetPinLo(bridge->ModeSelectGPIO);   // camera type bridge
          else
              DrvGpioSetPinHi(bridge->ModeSelectGPIO);   // csi2 tx bridge, like mipi C in katana
      }

      if (bridge->ResetGPIO != PIN_NOT_CONNECTED_TO_GPIO) {
          DrvGpioSetPinLo(bridge->ResetGPIO);
          SleepMs(2);
          DrvGpioSetPinHi(bridge->ResetGPIO);
      }

      icTC358746SetCs(bridge, 1);

      icTC358746UpdateSingleRegFields(bridge, MIPIB_ConfCtl__TriEn__Disable);
}

void icTC358746SetCs(TMipiBridge *bridge, int value)
{
    u8 cs_pin = bridge->ChipSelectGPIO;
    if (cs_pin != PIN_NOT_CONNECTED_TO_GPIO) {
        DrvGpioSetPin(cs_pin, value);
    }   
} 

int icTC358746AssignI2c(TMipiBridge *bridge, I2CM_Device * i2cDev)
{
    // error if bridges pointer is null 
    if ( bridge == NULL )
    {
        DPRINTF1("    ER %s line:%d bridge param NULL\n",__FUNCTION__, __LINE__);
        return 1;
    }
    // error if i2c device ptr is null
    if ( i2cDev == NULL )
    {
        DPRINTF1("    ER %s line:%d i2cDev param NULL\n",__FUNCTION__, __LINE__);
        return 2;
    }

    bridge->i2cDev = i2cDev;
    return 0;    
}

// assumes that the Mipi bridge is already selected
static inline int setReg(TMipiBridge * bridge, u16 address, u16 value) {
    u8 RegValueTBW[2];
    I2CM_StatusType retVal;

    // prepare data for the write function
    RegValueTBW[0] = (value>>8)    & 0xFF;
    RegValueTBW[1] = (value   )    & 0xFF;

    DPRINTF3("writing [%04x] := %04x\n", address, value);

    // write the register
    retVal = DrvI2cMTransaction(bridge->i2cDev,
            bridge->BridgeI2CSlaveAddr,
            address,
            protoTC358746Write,
            RegValueTBW,
            2);
    if (I2CM_STAT_OK != retVal)
    {
        DPRINTF1("    ER %s line:%d, transmitting data to bridge\n      (sda=%d,scl=%d,cs=%d,i2cslave=0x%x), ret %d\n",__FUNCTION__, __LINE__, bridge->i2cDev->sdaPin, bridge->i2cDev->sclPin, bridge->ChipSelectGPIO, bridge->BridgeI2CSlaveAddr, retVal);
        return retVal;
    }
    return 0;
}

int icTC358746SetRegs(TMipiBridge *bridge, const u32 data[][2], u32 len)
{
     u32 i;
     int retVal;

     if (bridge->i2cDev == NULL)
     {
         DPRINTF1("    ER %s line:%d i2cDev param NULL\n",__FUNCTION__, __LINE__);
         return 1;
     }
     
     icTC358746SetCs(bridge, 0);
     
     for (i=0; i<len ; ++i)
     {
         retVal = setReg(bridge, data[i][0], data[i][1]);
         if (retVal != 0) {
             icTC358746SetCs(bridge, 1);
             return retVal;
         }
     }
     
     icTC358746SetCs(bridge, 1);

     // no errors, cool
     return 0;
}

// assumes that the Mipi bridge is already selected
// returns negativ on error, or value otherwise
static inline int getReg(TMipiBridge *bridge, u16 address) {
    u8 RegValueTBR[2];
    I2CM_StatusType retVal;
    // Read array of registers
    retVal = DrvI2cMTransaction(bridge->i2cDev,
            bridge->BridgeI2CSlaveAddr,
            address,
            protoTC358746Read,
            RegValueTBR,
            2);
    if (retVal < 0) return retVal;

    DPRINTF3("reading [%04x]  = %04x\n", address, ((RegValueTBR[0]<<8) & 0xFF00) | (RegValueTBR[1] & 0xFF));
    return ((RegValueTBR[0]<<8) & 0xFF00) | (RegValueTBR[1] & 0xFF);
}

int icTC358746GetRegs(TMipiBridge *bridge, u32 data[][2], u32 len)
{
     u32 i;
     I2CM_StatusType retVal;
     
     icTC358746SetCs(bridge, 0);
     
     for (i=0; i<len ; ++i)
     {
         retVal = getReg(bridge, data[i][0]);
          if (retVal >= 0)
              data[i][1] = retVal;
          else 
          {   
              DPRINTF1("    ER %s line:%d, on receiving data from bridge %d, ret %d\n",__FUNCTION__, __LINE__, retVal);
              icTC358746SetCs(bridge, 1);
              return retVal;
          }
     }
     
     icTC358746SetCs(bridge, 1);
     // no errors, cool
     return 0;
}

int icTC358746CheckRegs(TMipiBridge *bridge, const u32 data[][2], u32 len)
{
     u32 i;
     I2CM_StatusType retVal;

     if (bridge->i2cDev == NULL)
     {
         DPRINTF1("    ER %s line:%d i2cDev param NULL\n",__FUNCTION__, __LINE__);
         return -1;
     }

     icTC358746SetCs(bridge, 0);

     for (i=0; i<len ; ++i)
     {
         retVal = getReg(bridge, data[i][0]);

         if (retVal >= 0)
         {
             SleepMs(1);
             if ((int)data[i][1] != retVal)
             {
                 DPRINTF1("@%x: expected: %x got: %x\n", data[i][0], data[i][1], retVal);
                 return 0x00010000 | data[i][0];
             }
         }
         else
         {
             DPRINTF2("    ER %s line:%d, on receiving from bridge %d, ret %d\n",__FUNCTION__, __LINE__, whichBridge, retVal);
             icTC358746SetCs(bridge, 1);
             return 0x00100000 | data[i][0];
         }
     }

     icTC358746SetCs(bridge, 1);
     // no errors, cool
     return 0;
}

int icTC358746CheckSingleRegFields(TMipiBridge *bridge, u64 fieldSpec)
{
    u32 registerAddress = MIPIB_GET_ADDRESS(fieldSpec);
    u32 nonReservedBits = MIPIB_GET_NONRESERVED_BITS(fieldSpec);
    u32 fieldsMask = MIPIB_GET_FIELD_MASK(fieldSpec);
    u32 fieldsValue = MIPIB_GET_FIELD_VALUE(fieldSpec);

    DPRINTF2("Mipi Bridge Checking: %04x %04x %04x %04x\n", registerAddress, nonReservedBits, fieldsMask, fieldsValue);

    u8 RegValueTBR[2];
    I2CM_StatusType retVal;

    icTC358746SetCs(bridge, 0);
    retVal = getReg(bridge, registerAddress);
    icTC358746SetCs(bridge, 1);

    if (retVal < 0) return retVal;

    return (retVal ^ fieldsValue) & fieldsMask;
}

#define MIPIB_CONTINUE_ON_ERROR

int icTC358746CheckMultipleRegFields(TMipiBridge *bridge, const u64 data[], u32 len)
{
    if (bridge->i2cDev == NULL)
    {
        DPRINTF1("    ER %s line:%d i2cDev param NULL\n",__FUNCTION__, __LINE__);
        return -1;
    }
    u32 i;
    int finalRetVal = 0;

    icTC358746SetCs(bridge, 0);

    for (i=0; i<len ; ++i)
    {
        u64 spec = data[i];
        int retVal = icTC358746CheckSingleRegFields(bridge, spec);
        if (retVal > 0)
        {
            DPRINTF1("MIPIB: @%04x: expected: %04x, mask: %04x, different: %04x\n",
                    MIPIB_GET_ADDRESS(spec),
                    MIPIB_GET_FIELD_VALUE(spec),
                    MIPIB_GET_FIELD_MASK(spec),
                    retVal);
            if (finalRetVal == 0)
                finalRetVal = 0x00010000 | MIPIB_GET_ADDRESS(spec);
#ifndef MIPIB_CONTINUE_ON_ERROR
            icTC358746SetCs(bridge, 1);
            return finalRetVal;
#endif
        }
        if (retVal < 0) // I2C communication error
        {
            DPRINTF2("    ER %s line:%d, on receiving from bridge %d, ret %d\n",__FUNCTION__, __LINE__, whichBridge, retVal);
            icTC358746SetCs(bridge, 1);
            return retVal;
        }
    }
    icTC358746SetCs(bridge, 1);
    return finalRetVal;
}

int icTC358746UpdateSingleRegFields(TMipiBridge *bridge, u64 fieldSpec)
{
    u32 registerAddress = MIPIB_GET_ADDRESS(fieldSpec);
    u32 nonReservedBits = MIPIB_GET_NONRESERVED_BITS(fieldSpec);
    u32 fieldsMask = MIPIB_GET_FIELD_MASK(fieldSpec);
    u32 fieldsValue = MIPIB_GET_FIELD_VALUE(fieldSpec);

    DPRINTF2("Mipi Bridge Updating: %04x %04x %04x %04x\n", registerAddress, nonReservedBits, fieldsMask, fieldsValue);

    icTC358746SetCs(bridge, 0);

    u32 previousValue = 0;

    if (nonReservedBits != fieldsMask) {
        // this is an incomplete write. First we read back the current value
        I2CM_StatusType retVal;

        retVal = getReg(bridge, registerAddress);

        if (retVal < 0) {
            icTC358746SetCs(bridge, 1);
            return retVal;
        }
        previousValue = retVal;
        previousValue &= ~fieldsMask;
    }
    if (fieldsValue & ~fieldsMask) {
        icTC358746SetCs(bridge, 1);
        return 1; // trying to set bits which are outside of the mask?
    }
    if (nonReservedBits == 0) {
        icTC358746SetCs(bridge, 1);
        return 2; // this is a read spec
    }
    if ((nonReservedBits & fieldsMask)!=fieldsMask) {
        icTC358746SetCs(bridge, 1);
        return 3; // trying to set reserved bits?
    }
    u32 newValue = previousValue | (fieldsValue & fieldsMask);
    int retVal = setReg(bridge, registerAddress, newValue);

    if (MIPIB_GET_IS_RESERVED_32BIT(fieldSpec)) {
        setReg(bridge, registerAddress + 2, 0x0000);
    }

#ifdef CHECK_REGISTERS_WERE_WRITTEN_CORRECTLY
    {
        unsigned int i;
        int check_this_register = 1;
        for (i=0;i<L_ARRAY_SIZE(icTC358746noReadbackWrittenValueRegisters); i++) {
            if (icTC358746noReadbackWrittenValueRegisters[i]==registerAddress) {
                check_this_register = 0;
            }
        }
        if (check_this_register)
            if (0!=icTC358746CheckSingleRegFields(bridge, fieldSpec))
                printf("WARNING: MIPI CSI bridge: update of register 0x%04x failed!\n", registerAddress);
    }
#endif
    icTC358746SetCs(bridge, 1);
    return retVal;
}

int icTC358746UpdateMultipleRegFields(TMipiBridge *bridge, const u64 data[], u32 len)
{
    int retVal;
    u32 i;
    for (i=0;i<len;i++) {
        retVal = icTC358746UpdateSingleRegFields(bridge, data[i]);
        if (retVal) return retVal;
    }
    return 0;
}

int icTC358746ReadNumericField(TMipiBridge *bridge, u64 readFieldSpec) {
    u32 registerAddress = MIPIB_GET_ADDRESS(readFieldSpec);
    u32 nonReservedBits = MIPIB_GET_NONRESERVED_BITS(readFieldSpec);
    u32 fieldsMask = MIPIB_GET_FIELD_MASK(readFieldSpec);
    u32 shiftAmount = MIPIB_GET_SHIFT_AMOUNT(readFieldSpec);
    if (nonReservedBits != 0) return 1; // this is not a read spec
    int regValue = getReg(bridge, registerAddress);
    if (regValue < 0) return regValue;
    u32 fieldValue = (regValue >> shiftAmount) & fieldsMask;
    return fieldValue;
}
