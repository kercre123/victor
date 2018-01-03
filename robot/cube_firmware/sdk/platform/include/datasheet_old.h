
#ifndef _IODA14580_INCLUDED
#define _IODA14580_INCLUDED

/* 
 *----------------------------------------------------------------------------------------------------------------------------------------
 * (C) Copyright Dialog Semiconductor BV, unpublished work.
 * This computer program includes Confidential, Proprietary Information and
 * is a Trade Secret of Dialog Semiconductor BV.
 * All use, disclosure, and/or reproduction is prohibited unless authorized
 * in writing. All Rights Reserved.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 *
 * File:      "DA14580.h" (Datasheet)
 * Purpose:   C header file of the device registers and bit fields
 * Created:   Jun 19, 2013
 * By:        DoxBox					    
 * Country:   The Netherlands
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 * Header    : $Header: /services/syncdata/hazelaar/8000/server_vault/bb/tester/development/software_include/sc14580a0m2_nl1/datasheet.h.rca 1.21 Wed Jun 19 10:12:15 2013 nmoschop Experimental $	   
 * Date      : $Date: Wed Jun 19 10:12:15 2013 $
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 *  Id       : $Id: datasheet.h.rca 1.21 Wed Jun 19 10:12:15 2013 nmoschop Experimental $
 *
 *  Log      : $Log: datasheet.h.rca $
 *  Log      : 
 *  Log      :  Revision: 1.21 Wed Jun 19 10:12:15 2013 nmoschop
 *  Log      :  various updates
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
*/ 



/*===========================================================================*/ 
/*========================== Start of header part ===========================*/ 
/*===========================================================================*/ 
/*
 * =============================================================================================
 * ---------- Interrupt ReQuest number definitions (see also CMSDK_CM0.h) ----------------------
 * =============================================================================================
 */

/*
 * --------------------------------------------------------------------------------------------
 * The Cortex-M0 Processor Exceptions Numbers and the SC14580 Specific Interrupt Numbers.     -
 * --------------------------------------------------------------------------------------------
 */
typedef enum IRQn
{
/****** Cortex-M0 Processor Exceptions Numbers ***********************************************/
NonMaskableInt_IRQn            = -14,    /* 2 Non Maskable Interrupt                         */
HardFault_IRQn                 = -13,    /* 3 Cortex-M0 Hard Fault Interrupt                 */
SVCall_IRQn                    = -5,     /* 11 Cortex-M0 SV Call Interrupt                   */
PendSV_IRQn                    = -2,     /* 14 Cortex-M0 Pend SV Interrupt                   */
SysTick_IRQn                   = -1,     /* 15 Cortex-M0 System Tick Interrupt               */

/****** SC14580 Specific Interrupt Numbers ***************************************************/
BLE_WAKEUP_LP_IRQn             = 0,      /* Wakeup from Low Power interrupt from BLE.                                                                                       */
BLE_FINETGTIM_IRQn             = 1,      /* Fine Target Timer interrupt generated when Fine Target timer expired. Timer resolution is 625us base time reference.            */
BLE_GROSSTGTIM_IRQn            = 2,      /* Gross Target Timer interrupt generated when Gross Target timer expired. Timer resolution is 16 times 625us base time reference. */
BLE_CSCNT_IRQn                 = 3,      /* 625us base time reference interrupt, available in active modes.                                                                 */
BLE_SLP_IRQn                   = 4,      /* End of Sleep mode interrupt.                                                                                                    */
BLE_ERROR_IRQn                 = 5,      /* Error interrupt, generated when undesired behavior or bad programming occurs in the RW-BLE Core.                                */
BLE_RX_IRQn                    = 6,      /* Receipt interrupt at the end of each received packets.                                                                          */
BLE_EVENT_IRQn                 = 7,      /* End of Advertising / Scanning / Connection events interrupt.                                                                    */
SWTIM_IRQn                     = 8,      /* Software Timer (Timer0) interrupt.                                                                                              */
WKUP_QUADEC_IRQn               = 9,      /* Combines the Wake up Capture Timer interrupt, the GPIO interrupt and the QuadDecoder interrupt          */
BLE_RF_DIAG_IRQn              = 10,     /* Baseband or Radio Diagnostics Interrupt. Signals Radio or Baseband internal events                                */
BLE_CRYPT_IRQn                 = 11,     /* Encryption / Decryption interrupt, generated either when AES and/or CCM processing is finished.                                 */
UART_IRQn                      = 12,     /* UART interrupt.                                                                                                                 */
UART2_IRQn                     = 13,     /* UART2 interrupt.                                                                                                                */
I2C_IRQn                       = 14,     /* I2C interrupt.                                                                                                                  */
SPI_IRQn                       = 15,     /* SPI interrupt.                                                                                                                  */
ADC_IRQn                       = 16,     /* Analog-Digital Converter interrupt.                                                                                             */
KEYBRD_IRQn                    = 17,     /* Keyboard interrupt.                                                                                                             */
RFCAL_IRQn                     = 18,     /* Radio Calibration interrupt.                                                                                                    */
GPIO0_IRQn                     = 19,     /* GPIO interrupt through debounce.                                                                                                */
GPIO1_IRQn                     = 20,     /* GPIO interrupt through debounce.                                                                                                */
GPIO2_IRQn                     = 21,     /* GPIO interrupt through debounce.                                                                                                */
GPIO3_IRQn                     = 22,     /* GPIO interrupt through debounce.                                                                                                */
GPIO4_IRQn                     = 23,     /* GPIO interrupt through debounce.                                                                                                */
} IRQn_Type;


/*--------------------------------------------------------------------------------------------*/
/* Configuration of the Cortex-M0 Processor and Core Peripherals (see also CMSDK_CM0.h).      */
/*--------------------------------------------------------------------------------------------*/
/* Note: The next 2 defines are used in "core_cm0.h" for the CMSIS Core SysTick function.     */
//#define __NVIC_PRIO_BITS          (2)         /* Number of Bits used for Priority Levels        */
//#define __Vendor_SysTickConfig    (0)         /* Set to 1 if different SysTick Config is used   */



/*===========================================================================*/ 
/*=========================== End of header part ============================*/ 
/*===========================================================================*/ 



/*===========================================================================*/ 
/*==================== Start of automatic generated code ====================*/ 
/*===========================================================================*/ 



/*===========================================================================*/
/* memory map anonymous */
/*===========================================================================*/
#define RF_IO_CTRL1_REG                      (0x50000030) /* (in CRG) */
#define RF_LNA_CTRL1_REG                     (0x50000032) /* (in CRG) */
#define RF_LNA_CTRL2_REG                     (0x50000034) /* (in CRG) */
#define RF_LNA_CTRL3_REG                     (0x50000036) /* (in CRG) */
#define RF_RSSI_COMP_CTRL_REG                (0x50000038) /* (in CRG) */
#define RF_VCO_CTRL_REG                      (0x5000003A) /*  */
#define SPOTP_TEST_REG                       (0x5000003E) /* (in CRG) */
#define DCDC_CTRL_REG                        (0x50000080) /* DCDC control register */
#define DCDC_CTRL2_REG                       (0x50000082) /* DCDC second control register */
#define DCDC_CTRL3_REG                       (0x50000084) /* DCDC thirth control register */
#define DCDC_CAL1_REG                        (0x50000086) /* DCDC first calibration register */
#define DCDC_CAL2_REG                        (0x50000088) /* DCDC second calibration register */
#define DCDC_CAL3_REG                        (0x5000008A) /* DCDC thirth calibration register */
#define CLK_REF_SEL_REG                      (0x50001600) /* Select clock for oscillator calibration */
#define CLK_REF_CNT_REG                      (0x50001602) /* Count value for oscillator calibration */
#define CLK_REF_VAL_L_REG                    (0x50001604) /* XTAL16M reference cycles, lower 16 bits */
#define CLK_REF_VAL_H_REG                    (0x50001606) /* XTAL16M reference cycles, upper 16 bits */
#define RF_BMCW_REG                          (0x50002000) /*  */
#define RF_OVERRULE_REG                      (0x50002020) /*  */
#define RF_CALSTATE_REG                      (0x50002040) /*  */
#define RF_CALCAP1_REG                       (0x50002060) /*  */
#define RF_CALCAP2_REG                       (0x50002062) /*  */
#define RF_SCAN_FEEDBACK_REG                 (0x50002080) /*  */
#define RF_CAL_CTRL_REG                      (0x50002200) /*  */
#define RF_REF_OSC_REG                       (0x50002202) /*  */
#define RF_IRQ_CTRL_REG                      (0x50002204) /*  */
#define RF_IFF_RESULT_REG                    (0x50002300) /* Must be Retained */
#define RF_ADCI_DC_OFFSET_REG                (0x50002310) /* Must be Retained */
#define RF_ADCQ_DC_OFFSET_REG                (0x50002312) /* Must be Retained */
#define RF_DC_OFFSET_RESULT_REG              (0x50002314) /* Must be Retained */
#define RF_SYNTH_RESULT_REG                  (0x50002316) /* Must be Retained */
#define RF_SYNTH_RESULT2_REG                 (0x50002318) /* Must be Retained */
#define RF_SYNTH_RESULT3_REG                 (0x5000231A) /*  */
#define RF_ENABLE_CONFIG1_REG                (0x50002400) /*  */
#define RF_ENABLE_CONFIG2_REG                (0x50002402) /*  */
#define RF_ENABLE_CONFIG3_REG                (0x50002404) /*  */
#define RF_ENABLE_CONFIG4_REG                (0x50002406) /*  */
#define RF_ENABLE_CONFIG5_REG                (0x50002408) /*  */
#define RF_ENABLE_CONFIG6_REG                (0x5000240A) /*  */
#define RF_ENABLE_CONFIG7_REG                (0x5000240C) /*  */
#define RF_ENABLE_CONFIG8_REG                (0x5000240E) /*  */
#define RF_ENABLE_CONFIG9_REG                (0x50002410) /*  */
#define RF_ENABLE_CONFIG10_REG               (0x50002412) /*  */
#define RF_ENABLE_CONFIG11_REG               (0x50002414) /*  */
#define RF_ENABLE_CONFIG12_REG               (0x50002416) /*  */
#define RF_ENABLE_CONFIG13_REG               (0x50002418) /*  */
#define RF_ENABLE_CONFIG14_REG               (0x5000241A) /*  */
#define RF_ENABLE_CONFIG15_REG               (0x5000241C) /*  */
#define RF_ENABLE_CONFIG16_REG               (0x5000241E) /*  */
#define RF_ENABLE_CONFIG17_REG               (0x50002420) /*  */
#define RF_ENABLE_CONFIG18_REG               (0x50002422) /*  */
#define RF_ENABLE_CONFIG19_REG               (0x50002424) /*  */
#define RF_ENABLE_CONFIG20_REG               (0x50002426) /*  */
#define RF_ENABLE_CONFIG21_REG               (0x50002428) /*  */
#define RF_ENABLE_CONFIG22_REG               (0x5000242A) /*  */
#define RF_ENABLE_CONFIG23_REG               (0x5000242C) /*  */
#define RF_CNTRL_TIMER_1_REG                 (0x50002500) /*  */
#define RF_CNTRL_TIMER_2_REG                 (0x50002502) /*  */
#define RF_CNTRL_TIMER_3_REG                 (0x50002504) /*  */
#define RF_CNTRL_TIMER_4_REG                 (0x50002506) /*  */
#define RF_CNTRL_TIMER_5_REG                 (0x50002508) /*  */
#define RF_CNTRL_TIMER_6_REG                 (0x5000250A) /*  */
#define RF_CNTRL_TIMER_7_REG                 (0x5000250C) /*  */
#define RF_CNTRL_TIMER_8_REG                 (0x5000250E) /*  */
#define RF_CNTRL_TIMER_9_REG                 (0x50002510) /*  */
#define RF_CNTRL_TIMER_10_REG                (0x50002512) /*  */
#define RF_CNTRL_TIMER_11_REG                (0x50002514) /*  */
#define RF_CNTRL_TIMER_12_REG                (0x50002516) /*  */
#define RF_CNTRL_TIMER_13_REG                (0x50002518) /*  */
#define RF_CNTRL_TIMER_14_REG                (0x5000251A) /*  */
#define BIAS_CTRL1_REG                       (0x50002600) /*  */
#define RF_SPARE1_REG                        (0x50002602) /*  */
#define RF_MIXER_CTRL1_REG                   (0x50002810) /*  */
#define RF_MIXER_CTRL2_REG                   (0x50002812) /*  */
#define RF_IFF_CTRL1_REG                     (0x50002820) /*  */
#define RF_ADC_CTRL1_REG                     (0x50002830) /*  */
#define RF_ADC_CTRL2_REG                     (0x50002832) /*  */
#define RF_ADC_CTRL3_REG                     (0x50002834) /*  */
#define RF_DEM_CTRL_REG                      (0x50002840) /*  */
#define RF_AGC_LUT_01_REG                    (0x50002850) /*  */
#define RF_AGC_LUT_23_REG                    (0x50002852) /*  */
#define RF_AGC_LUT_45_REG                    (0x50002854) /*  */
#define RF_AGC_LUT_67_REG                    (0x50002856) /*  */
#define RF_AGC_LUT_89_REG                    (0x50002858) /*  */
#define RF_AGC_CTRL1_REG                     (0x50002860) /*  */
#define RF_AGC_CTRL2_REG                     (0x50002862) /*  */
#define RF_AFC_CTRL_REG                      (0x50002864) /*  */
#define RF_DC_OFFSET_CTRL1_REG               (0x50002866) /*  */
#define RF_DC_OFFSET_CTRL2_REG               (0x50002868) /*  */
#define RF_DC_OFFSET_CTRL3_REG               (0x5000286A) /*  */
#define RF_DC_OFFSET_CTRL4_REG               (0x5000286C) /*  */
#define RF_RADIG_SPARE_REG                   (0x5000286E) /*  */
#define RF_AGC_RESULT_REG                    (0x50002900) /*  */
#define RF_RSSI_RESULT_REG                   (0x50002902) /*  */
#define RF_PA_CTRL_REG                       (0x50002A00) /*  */
#define RF_SYNTH_CTRL1_REG                   (0x50002C00) /*  */
#define RF_SYNTH_CTRL2_REG                   (0x50002C02) /*  */
#define RF_SYNTH_CTRL3_REG                   (0x50002C04) /*  */
#define RF_VCOCAL_CTRL_REG                   (0x50002C06) /*  */
#define RF_MGAIN_CTRL_REG                    (0x50002C08) /*  */
#define RF_MGC_CTRL_REG                      (0x50002C10) /*  */
#define RF_VCOVAR_CTRL_REG                   (0x50002C20) /*  */
#define RF_PFD_CTRL_REG                      (0x50002C40) /*  */
#define RF_CP_CTRL_REG                       (0x50002C50) /*  */
#define RF_LF_RES_CTRL_REG                   (0x50002C52) /* LF resistor setting */
#define RF_LF_CTRL_REG                       (0x50002C60) /*  */
#define RF_TDC_CTRL_REG                      (0x50002C70) /* TDC settings */
#define TEST_CTRL_REG                        (0x500030F0) /*  */
#define TEST_CTRL2_REG                       (0x500030F2) /*  */
#define TEST_CTRL3_REG                       (0x500030F4) /*  */
#define TEST_CTRL4_REG                       (0x500030F6) /*  */
#define TEST_CTRL5_REG                       (0x500030F8) /*  */
#define BIST_CTRL_REG                        (0x500030FA) /*  */
#define ROMBIST_RESULTL_REG                  (0x500030FC) /*  */
#define ROMBIST_RESULTH_REG                  (0x500030FE) /*  */
#define RFPT_CTRL_REG                        (0x50003500) /* Control register */
#define RFPT_ADDR_REG                        (0x50003502) /* AHB master start address */
#define RFPT_LEN_REG                         (0x50003504) /* Data length register */
#define RFPT_STAT_REG                        (0x50003506) /* Status register */



/*=============================*/
struct __RF_IO_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_RFIO_TRIM1_CAP                       : 8;
};
#define RFIO_TRIM1_CAP                       (0x00FF)



/*=============================*/
struct __RF_LNA_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_LNA_TRIM_CD_LF                       : 6;
    WORD BITFLD_LNA_TRIM_CD_HF                       : 6;
};
#define LNA_TRIM_CD_LF                       (0x003F)
#define LNA_TRIM_CD_HF                       (0x0FC0)



/*=============================*/
struct __RF_LNA_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_LNA_TRIM_GM_HI                       : 6;
    WORD BITFLD_LNA_TRIM_GM_LO                       : 6;
};
#define LNA_TRIM_GM_HI                       (0x003F)
#define LNA_TRIM_GM_LO                       (0x0FC0)



/*=============================*/
struct __RF_LNA_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_LNA_TRIM_CGS                         : 5;
};
#define LNA_TRIM_CGS                         (0x001F)



/*=============================*/
struct __RF_RSSI_COMP_CTRL_REG
/*=============================*/
{
    WORD BITFLD_RSSI_COMP01                          : 4;
    WORD BITFLD_RSSI_COMP10                          : 4;
    WORD BITFLD_RSSI_COMP11                          : 4;
    WORD BITFLD_RSSI_COMP00                          : 4;
};
#define RSSI_COMP01                          (0x000F)
#define RSSI_COMP10                          (0x00F0)
#define RSSI_COMP11                          (0x0F00)
#define RSSI_COMP00                          (0xF000)



/*=============================*/
struct __RF_VCO_CTRL_REG
/*=============================*/
{
    WORD BITFLD_VCO_AMPL_SET                         : 4;
};
#define VCO_AMPL_SET                         (0x000F)



/*=============================*/
struct __SPOTP_TEST_REG
/*=============================*/
{
    WORD BITFLD_SPOTP_ACTIVE                         : 1;
    WORD BITFLD_LDO_OTP_WRITE                        : 1;
};
#define SPOTP_ACTIVE                         (0x0001)
#define LDO_OTP_WRITE                        (0x0002)



/*=============================*/
struct __DCDC_CTRL_REG
/*=============================*/
{
    WORD                                             : 1;
    WORD BITFLD_DCDC_VBAT1V_LEV                      : 3;
    WORD                                             : 1;
    WORD BITFLD_DCDC_MODE                            : 3;
    WORD BITFLD_DCDC_DRIVE_NSW                       : 2;
    WORD BITFLD_DCDC_DRIVE_PSW                       : 2;
    WORD BITFLD_DCDC_DRIVE_OSW                       : 2;
    WORD BITFLD_DCDC_TUNE                            : 2;
};
#define DCDC_VBAT1V_LEV                      (0x000E)
#define DCDC_MODE                            (0x00E0)
#define DCDC_DRIVE_NSW                       (0x0300)
#define DCDC_DRIVE_PSW                       (0x0C00)
#define DCDC_DRIVE_OSW                       (0x3000)
#define DCDC_TUNE                            (0xC000)



/*=============================*/
struct __DCDC_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_DCDC_AUTO_CAL                        : 3;
    WORD BITFLD_DCDC_CUR_LIM                         : 4;
    WORD BITFLD_DCDC_TON                             : 2;
    WORD BITFLD_DCDC_VBAT3V_LEV                      : 3;
    WORD BITFLD_DCDC_VOLT_LEV                        : 4;
};
#define DCDC_AUTO_CAL                        (0x0007)
#define DCDC_CUR_LIM                         (0x0078)
#define DCDC_TON                             (0x0180)
#define DCDC_VBAT3V_LEV                      (0x0E00)
#define DCDC_VOLT_LEV                        (0xF000)



/*=============================*/
struct __DCDC_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_BUCK_ENABLE                          : 1;
    WORD BITFLD_DCDC_IDLE_CLK                        : 2;
    WORD BITFLD_DCDC_TIMEOUT                         : 2;
};
#define BUCK_ENABLE                          (0x0001)
#define DCDC_IDLE_CLK                        (0x0006)
#define DCDC_TIMEOUT                         (0x0018)



/*=============================*/
struct __DCDC_CAL1_REG
/*=============================*/
{
    WORD BITFLD_DCDC_CAL1                            : 6;
};
#define DCDC_CAL1                            (0x003F)



/*=============================*/
struct __DCDC_CAL2_REG
/*=============================*/
{
    WORD BITFLD_DCDC_CAL2                            : 6;
};
#define DCDC_CAL2                            (0x003F)



/*=============================*/
struct __DCDC_CAL3_REG
/*=============================*/
{
    WORD BITFLD_DCDC_CAL3                            : 6;
};
#define DCDC_CAL3                            (0x003F)



/*=============================*/
struct __CLK_REF_SEL_REG
/*=============================*/
{
    WORD BITFLD_REF_CLK_SEL                          : 2;
    WORD BITFLD_REF_CAL_START                        : 1;
};
#define REF_CLK_SEL                          (0x0003)
#define REF_CAL_START                        (0x0004)



/*=============================*/
struct __CLK_REF_CNT_REG
/*=============================*/
{
    WORD BITFLD_REF_CNT_VAL                          : 16;
};
#define REF_CNT_VAL                          (0xFFFF)



/*=============================*/
struct __CLK_REF_VAL_L_REG
/*=============================*/
{
    WORD BITFLD_XTAL_CNT_VAL                         : 16;
};
#define XTAL_CNT_VAL                         (0xFFFF)



/*=============================*/
struct __CLK_REF_VAL_H_REG
/*=============================*/
{
    WORD BITFLD_XTAL_CNT_VAL                         : 16;
};
#define XTAL_CNT_VAL                         (0xFFFF)



/*=============================*/
struct __RF_BMCW_REG
/*=============================*/
{
    WORD BITFLD_CN_WR                                : 8;
    WORD BITFLD_CN_SEL                               : 1;
};
#define CN_WR                                (0x00FF)
#define CN_SEL                               (0x0100)



/*=============================*/
struct __RF_OVERRULE_REG
/*=============================*/
{
    WORD BITFLD_TX_DIS_WR                            : 1;
    WORD BITFLD_TX_EN_WR                             : 1;
    WORD BITFLD_RX_DIS_WR                            : 1;
    WORD BITFLD_RX_EN_WR                             : 1;
};
#define TX_DIS_WR                            (0x0001)
#define TX_EN_WR                             (0x0002)
#define RX_DIS_WR                            (0x0004)
#define RX_EN_WR                             (0x0008)



/*=============================*/
struct __RF_CALSTATE_REG
/*=============================*/
{
    WORD BITFLD_CALSTATE                             : 4;
};
#define CALSTATE                             (0x000F)



/*=============================*/
struct __RF_CALCAP1_REG
/*=============================*/
{
    WORD BITFLD_VCO_CALCAP_LOW                       : 16;
};
#define VCO_CALCAP_LOW                       (0xFFFF)



/*=============================*/
struct __RF_CALCAP2_REG
/*=============================*/
{
    WORD BITFLD_VCO_CALCAP_HIGH                      : 2;
};
#define VCO_CALCAP_HIGH                      (0x0003)



/*=============================*/
struct __RF_SCAN_FEEDBACK_REG
/*=============================*/
{
    WORD BITFLD_LF_RES                               : 4;
    WORD BITFLD_CP_CUR                               : 4;
};
#define LF_RES                               (0x000F)
#define CP_CUR                               (0x00F0)



/*=============================*/
struct __RF_CAL_CTRL_REG
/*=============================*/
{
    WORD BITFLD_SO_CAL                               : 1;
    WORD BITFLD_EO_CAL                               : 1;
    WORD BITFLD_MGAIN_CAL_DIS                        : 1;
    WORD BITFLD_IFF_CAL_DIS                          : 1;
    WORD BITFLD_DC_OFFSET_CAL_DIS                    : 1;
    WORD BITFLD_VCO_CAL_DIS                          : 1;
};
#define SO_CAL                               (0x0001)
#define EO_CAL                               (0x0002)
#define MGAIN_CAL_DIS                        (0x0004)
#define IFF_CAL_DIS                          (0x0008)
#define DC_OFFSET_CAL_DIS                    (0x0010)
#define VCO_CAL_DIS                          (0x0020)



/*=============================*/
struct __RF_REF_OSC_REG
/*=============================*/
{
    WORD BITFLD_CNT_RO                               : 6;
    WORD BITFLD_CNT_CLK                              : 9;
};
#define CNT_RO                               (0x003F)
#define CNT_CLK                              (0x7FC0)



/*=============================*/
struct __RF_IRQ_CTRL_REG
/*=============================*/
{
    WORD BITFLD_EO_CAL_CLEAR                         : 1;
};
#define EO_CAL_CLEAR                         (0x0001)



/*=============================*/
struct __RF_IFF_RESULT_REG
/*=============================*/
{
    WORD BITFLD_IF_CAL_CAP_RD                        : 5;
};
#define IF_CAL_CAP_RD                        (0x001F)



/*=============================*/
struct __RF_ADCI_DC_OFFSET_REG
/*=============================*/
{
    WORD BITFLD_ADC_OFFP_I_RD                        : 8;
    WORD BITFLD_ADC_OFFN_I_RD                        : 8;
};
#define ADC_OFFP_I_RD                        (0x00FF)
#define ADC_OFFN_I_RD                        (0xFF00)



/*=============================*/
struct __RF_ADCQ_DC_OFFSET_REG
/*=============================*/
{
    WORD BITFLD_ADC_OFFP_Q_RD                        : 8;
    WORD BITFLD_ADC_OFFN_Q_RD                        : 8;
};
#define ADC_OFFP_Q_RD                        (0x00FF)
#define ADC_OFFN_Q_RD                        (0xFF00)



/*=============================*/
struct __RF_DC_OFFSET_RESULT_REG
/*=============================*/
{
    WORD BITFLD_DCOFFSET_I_RD                        : 8;
    WORD BITFLD_DCOFFSET_Q_RD                        : 8;
};
#define DCOFFSET_I_RD                        (0x00FF)
#define DCOFFSET_Q_RD                        (0xFF00)



/*=============================*/
struct __RF_SYNTH_RESULT_REG
/*=============================*/
{
    WORD BITFLD_GAUSS_GAIN_CAL_RD                    : 8;
    WORD BITFLD_VCO_FREQTRIM_RD                      : 4;
};
#define GAUSS_GAIN_CAL_RD                    (0x00FF)
#define VCO_FREQTRIM_RD                      (0x0F00)



/*=============================*/
struct __RF_SYNTH_RESULT2_REG
/*=============================*/
{
    WORD BITFLD_GAUSS_GAIN_RD                        : 8;
    WORD BITFLD_CN_CAL_RD                            : 7;
};
#define GAUSS_GAIN_RD                        (0x00FF)
#define CN_CAL_RD                            (0x7F00)



/*=============================*/
struct __RF_SYNTH_RESULT3_REG
/*=============================*/
{
    WORD BITFLD_MDSTATE_RD                           : 16;
};
#define MDSTATE_RD                           (0xFFFF)



/*=============================*/
struct __RF_ENABLE_CONFIG1_REG
/*=============================*/
{
    WORD BITFLD_lna_core_en                          : 8;
    WORD BITFLD_lna_ldo_en                           : 8;
};
#define lna_core_en                          (0x00FF)
#define lna_ldo_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG2_REG
/*=============================*/
{
    WORD BITFLD_mix_ldo_en                           : 8;
    WORD BITFLD_lna_cgm_en                           : 8;
};
#define mix_ldo_en                           (0x00FF)
#define lna_cgm_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG3_REG
/*=============================*/
{
    WORD BITFLD_ifadc_ldo_en                         : 8;
    WORD BITFLD_iff_ldo_en                           : 8;
};
#define ifadc_ldo_en                         (0x00FF)
#define iff_ldo_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG4_REG
/*=============================*/
{
    WORD BITFLD_md_ldo_en                            : 8;
    WORD BITFLD_vco_ldo_en                           : 8;
};
#define md_ldo_en                            (0x00FF)
#define vco_ldo_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG5_REG
/*=============================*/
{
    WORD BITFLD_pa_ldo_en                            : 8;
    WORD BITFLD_pfd_ldo_en                           : 8;
};
#define pa_ldo_en                            (0x00FF)
#define pfd_ldo_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG6_REG
/*=============================*/
{
    WORD BITFLD_vco_bias_en                          : 8;
    WORD BITFLD_cp_switch_en                         : 8;
};
#define vco_bias_en                          (0x00FF)
#define cp_switch_en                         (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG7_REG
/*=============================*/
{
    WORD BITFLD_lna_ldo_zero                         : 8;
    WORD BITFLD_cp_bias_en                           : 8;
};
#define lna_ldo_zero                         (0x00FF)
#define cp_bias_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG8_REG
/*=============================*/
{
    WORD BITFLD_pa_en                                : 8;
    WORD BITFLD_pa_ramp_en                           : 8;
};
#define pa_en                                (0x00FF)
#define pa_ramp_en                           (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG9_REG
/*=============================*/
{
    WORD BITFLD_iff_en                               : 8;
    WORD BITFLD_mix_en                               : 8;
};
#define iff_en                               (0x00FF)
#define mix_en                               (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG10_REG
/*=============================*/
{
    WORD BITFLD_vco_en                               : 8;
    WORD BITFLD_adc_en                               : 8;
};
#define vco_en                               (0x00FF)
#define adc_en                               (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG11_REG
/*=============================*/
{
    WORD BITFLD_cp_en                                : 8;
    WORD BITFLD_md_lobuf_en                          : 8;
};
#define cp_en                                (0x00FF)
#define md_lobuf_en                          (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG12_REG
/*=============================*/
{
    WORD BITFLD_gauss_en                             : 8;
    WORD BITFLD_pfd_en                               : 8;
};
#define gauss_en                             (0x00FF)
#define pfd_en                               (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG13_REG
/*=============================*/
{
    WORD BITFLD_lobuf_pa_en                          : 8;
    WORD BITFLD_rfio_en                              : 8;
};
#define lobuf_pa_en                          (0x00FF)
#define rfio_en                              (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG14_REG
/*=============================*/
{
    WORD BITFLD_div2_en                              : 8;
    WORD BITFLD_lobuf_rxiq_en                        : 8;
};
#define div2_en                              (0x00FF)
#define lobuf_rxiq_en                        (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG15_REG
/*=============================*/
{
    WORD BITFLD_vco_bias_sh_open_en                  : 8;
    WORD BITFLD_cp_bias_sh_open                      : 8;
};
#define vco_bias_sh_open_en                  (0x00FF)
#define cp_bias_sh_open                      (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG16_REG
/*=============================*/
{
    WORD BITFLD_gauss_bias_sh_open_en                : 8;
    WORD BITFLD_iff_bias_sh_open_en                  : 8;
};
#define gauss_bias_sh_open_en                (0x00FF)
#define iff_bias_sh_open_en                  (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG17_REG
/*=============================*/
{
    WORD BITFLD_plldig_en                            : 8;
    WORD BITFLD_mix_bias_sh_open_en                  : 8;
};
#define plldig_en                            (0x00FF)
#define mix_bias_sh_open_en                  (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG18_REG
/*=============================*/
{
    WORD BITFLD_dem_en                               : 8;
    WORD BITFLD_pllclosed_en                         : 8;
};
#define dem_en                               (0x00FF)
#define pllclosed_en                         (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG19_REG
/*=============================*/
{
    WORD BITFLD_cal_en                               : 8;
    WORD BITFLD_ldo_zero_en                          : 8;
};
#define cal_en                               (0x00FF)
#define ldo_zero_en                          (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG20_REG
/*=============================*/
{
    WORD BITFLD_ldo_rfio_en                          : 8;
    WORD BITFLD_tdc_en                               : 8;
};
#define ldo_rfio_en                          (0x00FF)
#define tdc_en                               (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG21_REG
/*=============================*/
{
    WORD BITFLD_rfio_bias_sh_open                    : 8;
    WORD BITFLD_rfio_bias_en                         : 8;
};
#define rfio_bias_sh_open                    (0x00FF)
#define rfio_bias_en                         (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG22_REG
/*=============================*/
{
    WORD BITFLD_adc_clk_en                           : 8;
    WORD BITFLD_ldo_radio_en                         : 8;
};
#define adc_clk_en                           (0x00FF)
#define ldo_radio_en                         (0xFF00)



/*=============================*/
struct __RF_ENABLE_CONFIG23_REG
/*=============================*/
{
    WORD BITFLD_spare_en_3                           : 8;
    WORD BITFLD_tr_pwm_off_en                        : 8;
};
#define spare_en_3                           (0x00FF)
#define tr_pwm_off_en                        (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_1_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_2_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_3_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_4_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_5_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_6_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_7_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_8_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_9_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_10_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_11_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_12_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_13_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __RF_CNTRL_TIMER_14_REG
/*=============================*/
{
    WORD BITFLD_SET_OFFSET                           : 8;
    WORD BITFLD_RESET_OFFSET                         : 8;
};
#define SET_OFFSET                           (0x00FF)
#define RESET_OFFSET                         (0xFF00)



/*=============================*/
struct __BIAS_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_MIX_BIAS_SET                         : 4;
    WORD BITFLD_CP_BIAS_SET                          : 4;
    WORD BITFLD_VCO_BIAS_SET                         : 4;
    WORD BITFLD_IFF_BIAS_SET                         : 4;
};
#define MIX_BIAS_SET                         (0x000F)
#define CP_BIAS_SET                          (0x00F0)
#define VCO_BIAS_SET                         (0x0F00)
#define IFF_BIAS_SET                         (0xF000)



/*=============================*/
struct __RF_SPARE1_REG
/*=============================*/
{
    WORD BITFLD_RF_SPARE1                            : 16;
};
#define RF_SPARE1                            (0xFFFF)



/*=============================*/
struct __RF_MIXER_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_MIX_TRIM_GMBIAS                      : 4;
    WORD BITFLD_MIX_TRIM_IBIAS                       : 4;
    WORD BITFLD_MIX_TRIM_VCM                         : 4;
    WORD BITFLD_MIX_SPARE                            : 4;
};
#define MIX_TRIM_GMBIAS                      (0x000F)
#define MIX_TRIM_IBIAS                       (0x00F0)
#define MIX_TRIM_VCM                         (0x0F00)
#define MIX_SPARE                            (0xF000)



/*=============================*/
struct __RF_MIXER_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_MIX_CAL_CAP_WR                       : 5;
    WORD BITFLD_MIX_CAL_CAP_SEL                      : 1;
};
#define MIX_CAL_CAP_WR                       (0x001F)
#define MIX_CAL_CAP_SEL                      (0x0020)



/*=============================*/
struct __RF_IFF_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_IF_CAL_CAP_WR                        : 5;
    WORD BITFLD_IF_CAL_CAP_SEL                       : 1;
    WORD BITFLD_IF_MUTE                              : 1;
    WORD BITFLD_RO_TO_PINS                           : 1;
    WORD BITFLD_IFF_DCOC_DAC_DIS                     : 1;
};
#define IF_CAL_CAP_WR                        (0x001F)
#define IF_CAL_CAP_SEL                       (0x0020)
#define IF_MUTE                              (0x0040)
#define RO_TO_PINS                           (0x0080)
#define IFF_DCOC_DAC_DIS                     (0x0100)



/*=============================*/
struct __RF_ADC_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_ADC_DC_OFFSET_SEL                    : 1;
    WORD                                             : 12;
    WORD BITFLD_ADC_MUTE                             : 1;
    WORD BITFLD_ADC_SIGN                             : 1;
};
#define ADC_DC_OFFSET_SEL                    (0x0001)
#define ADC_MUTE                             (0x2000)
#define ADC_SIGN                             (0x4000)



/*=============================*/
struct __RF_ADC_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_ADC_OFFP_I_WR                        : 8;
    WORD BITFLD_ADC_OFFN_I_WR                        : 8;
};
#define ADC_OFFP_I_WR                        (0x00FF)
#define ADC_OFFN_I_WR                        (0xFF00)



/*=============================*/
struct __RF_ADC_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_ADC_OFFP_Q_WR                        : 8;
    WORD BITFLD_ADC_OFFN_Q_WR                        : 8;
};
#define ADC_OFFP_Q_WR                        (0x00FF)
#define ADC_OFFN_Q_WR                        (0xFF00)



/*=============================*/
struct __RF_DEM_CTRL_REG
/*=============================*/
{
    WORD BITFLD_RXDATA_INV                           : 1;
    WORD BITFLD_DEM_HSI_POL                          : 1;
    WORD BITFLD_MATCH0101_TH                         : 4;
    WORD BITFLD_EQUAL_EN                             : 1;
};
#define RXDATA_INV                           (0x0001)
#define DEM_HSI_POL                          (0x0002)
#define MATCH0101_TH                         (0x003C)
#define EQUAL_EN                             (0x0040)



/*=============================*/
struct __RF_AGC_LUT_01_REG
/*=============================*/
{
    WORD BITFLD_VGA2_GAIN0                           : 3;
    WORD BITFLD_VGA1_GAIN0                           : 3;
    WORD BITFLD_LNA_GAIN0                            : 2;
    WORD BITFLD_VGA2_GAIN1                           : 3;
    WORD BITFLD_VGA1_GAIN1                           : 3;
    WORD BITFLD_LNA_GAIN1                            : 2;
};
#define VGA2_GAIN0                           (0x0007)
#define VGA1_GAIN0                           (0x0038)
#define LNA_GAIN0                            (0x00C0)
#define VGA2_GAIN1                           (0x0700)
#define VGA1_GAIN1                           (0x3800)
#define LNA_GAIN1                            (0xC000)



/*=============================*/
struct __RF_AGC_LUT_23_REG
/*=============================*/
{
    WORD BITFLD_VGA2_GAIN2                           : 3;
    WORD BITFLD_VGA1_GAIN2                           : 3;
    WORD BITFLD_LNA_GAIN2                            : 2;
    WORD BITFLD_VGA2_GAIN3                           : 3;
    WORD BITFLD_VGA1_GAIN3                           : 3;
    WORD BITFLD_LNA_GAIN3                            : 2;
};
#define VGA2_GAIN2                           (0x0007)
#define VGA1_GAIN2                           (0x0038)
#define LNA_GAIN2                            (0x00C0)
#define VGA2_GAIN3                           (0x0700)
#define VGA1_GAIN3                           (0x3800)
#define LNA_GAIN3                            (0xC000)



/*=============================*/
struct __RF_AGC_LUT_45_REG
/*=============================*/
{
    WORD BITFLD_VGA2_GAIN4                           : 3;
    WORD BITFLD_VGA1_GAIN4                           : 3;
    WORD BITFLD_LNA_GAIN4                            : 2;
    WORD BITFLD_VGA2_GAIN5                           : 3;
    WORD BITFLD_VGA1_GAIN5                           : 3;
    WORD BITFLD_LNA_GAIN5                            : 2;
};
#define VGA2_GAIN4                           (0x0007)
#define VGA1_GAIN4                           (0x0038)
#define LNA_GAIN4                            (0x00C0)
#define VGA2_GAIN5                           (0x0700)
#define VGA1_GAIN5                           (0x3800)
#define LNA_GAIN5                            (0xC000)



/*=============================*/
struct __RF_AGC_LUT_67_REG
/*=============================*/
{
    WORD BITFLD_VGA2_GAIN6                           : 3;
    WORD BITFLD_VGA1_GAIN6                           : 3;
    WORD BITFLD_LNA_GAIN6                            : 2;
    WORD BITFLD_VGA2_GAIN7                           : 3;
    WORD BITFLD_VGA1_GAIN7                           : 3;
    WORD BITFLD_LNA_GAIN7                            : 2;
};
#define VGA2_GAIN6                           (0x0007)
#define VGA1_GAIN6                           (0x0038)
#define LNA_GAIN6                            (0x00C0)
#define VGA2_GAIN7                           (0x0700)
#define VGA1_GAIN7                           (0x3800)
#define LNA_GAIN7                            (0xC000)



/*=============================*/
struct __RF_AGC_LUT_89_REG
/*=============================*/
{
    WORD BITFLD_VGA2_GAIN8                           : 3;
    WORD BITFLD_VGA1_GAIN8                           : 3;
    WORD BITFLD_LNA_GAIN8                            : 2;
    WORD BITFLD_VGA2_GAIN9                           : 3;
    WORD BITFLD_VGA1_GAIN9                           : 3;
    WORD BITFLD_LNA_GAIN9                            : 2;
};
#define VGA2_GAIN8                           (0x0007)
#define VGA1_GAIN8                           (0x0038)
#define LNA_GAIN8                            (0x00C0)
#define VGA2_GAIN9                           (0x0700)
#define VGA1_GAIN9                           (0x3800)
#define LNA_GAIN9                            (0xC000)



/*=============================*/
struct __RF_AGC_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_AGC_TH_LOW                           : 7;
    WORD BITFLD_AGC_TH_HIGH                          : 7;
    WORD BITFLD_AGC_MODE                             : 2;
};
#define AGC_TH_LOW                           (0x007F)
#define AGC_TH_HIGH                          (0x3F80)
#define AGC_MODE                             (0xC000)



/*=============================*/
struct __RF_AGC_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_RSSI_TH                              : 6;
    WORD BITFLD_EN_FRZ_GAIN                          : 1;
    WORD BITFLD_AGCSETTING_SEL                       : 1;
    WORD BITFLD_AGCSETTING_WR                        : 4;
    WORD BITFLD_SLOW_AGC                             : 1;
};
#define RSSI_TH                              (0x003F)
#define EN_FRZ_GAIN                          (0x0040)
#define AGCSETTING_SEL                       (0x0080)
#define AGCSETTING_WR                        (0x0F00)
#define SLOW_AGC                             (0x1000)



/*=============================*/
struct __RF_AFC_CTRL_REG
/*=============================*/
{
    WORD BITFLD_AFC_MODE                             : 4;
    WORD BITFLD_POLE1                                : 2;
    WORD BITFLD_POLE2                                : 2;
};
#define AFC_MODE                             (0x000F)
#define POLE1                                (0x0030)
#define POLE2                                (0x00C0)



/*=============================*/
struct __RF_DC_OFFSET_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_DCOFFSET_I_WR                        : 8;
    WORD BITFLD_DCOFFSET_Q_WR                        : 8;
};
#define DCOFFSET_I_WR                        (0x00FF)
#define DCOFFSET_Q_WR                        (0xFF00)



/*=============================*/
struct __RF_DC_OFFSET_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_DCOFFSET_SEL                         : 1;
    WORD BITFLD_DCPARCAL_EN                          : 1;
    WORD BITFLD_DCPOLE                               : 2;
    WORD BITFLD_DCNSTEP                              : 3;
    WORD BITFLD_DCNGAIN                              : 2;
};
#define DCOFFSET_SEL                         (0x0001)
#define DCPARCAL_EN                          (0x0002)
#define DCPOLE                               (0x000C)
#define DCNSTEP                              (0x0070)
#define DCNGAIN                              (0x0180)



/*=============================*/
struct __RF_DC_OFFSET_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_DCBETA_I                             : 8;
    WORD BITFLD_DCBETA_Q                             : 8;
};
#define DCBETA_I                             (0x00FF)
#define DCBETA_Q                             (0xFF00)



/*=============================*/
struct __RF_DC_OFFSET_CTRL4_REG
/*=============================*/
{
    WORD BITFLD_DCAGCSETTING_FULL0                   : 4;
    WORD BITFLD_DCAGCSETTING_FULL1                   : 4;
    WORD BITFLD_DCAGCSETTING_FULL2                   : 4;
    WORD BITFLD_DCAGCSETTING_FULL3                   : 4;
};
#define DCAGCSETTING_FULL0                   (0x000F)
#define DCAGCSETTING_FULL1                   (0x00F0)
#define DCAGCSETTING_FULL2                   (0x0F00)
#define DCAGCSETTING_FULL3                   (0xF000)



/*=============================*/
struct __RF_RADIG_SPARE_REG
/*=============================*/
{
    WORD BITFLD_RADIG_SPARE                          : 16;
};
#define RADIG_SPARE                          (0xFFFF)



/*=============================*/
struct __RF_AGC_RESULT_REG
/*=============================*/
{
    WORD BITFLD_AFC_RD                               : 8;
    WORD BITFLD_AGCSETTING_RD                        : 4;
};
#define AFC_RD                               (0x00FF)
#define AGCSETTING_RD                        (0x0F00)



/*=============================*/
struct __RF_RSSI_RESULT_REG
/*=============================*/
{
    WORD BITFLD_RSSI_PH_RD                           : 6;
    WORD BITFLD_RSSI_AVG_RD                          : 10;
};
#define RSSI_PH_RD                           (0x003F)
#define RSSI_AVG_RD                          (0xFFC0)



/*=============================*/
struct __RF_PA_CTRL_REG
/*=============================*/
{
    WORD BITFLD_PA_GAIN                              : 2;
    WORD BITFLD_PA_PW                                : 3;
    WORD BITFLD_PA_RAMPSPEED                         : 2;
    WORD                                             : 4;
    WORD BITFLD_LEVEL_LDO_RFPA                       : 4;
};
#define PA_GAIN                              (0x0003)
#define PA_PW                                (0x001C)
#define PA_RAMPSPEED                         (0x0060)
#define LEVEL_LDO_RFPA                       (0x7800)



/*=============================*/
struct __RF_SYNTH_CTRL1_REG
/*=============================*/
{
    WORD BITFLD_CHANNEL_ZERO                         : 12;
    WORD BITFLD_SGN                                  : 1;
    WORD BITFLD_CS                                   : 1;
    WORD BITFLD_PLL_HSI_POL                          : 1;
};
#define CHANNEL_ZERO                         (0x0FFF)
#define SGN                                  (0x1000)
#define CS                                   (0x2000)
#define PLL_HSI_POL                          (0x4000)



/*=============================*/
struct __RF_SYNTH_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_SD_ORDER_RX                          : 2;
    WORD BITFLD_SD_ORDER_TX                          : 2;
    WORD BITFLD_MODINDEX                             : 2;
    WORD BITFLD_DELAY                                : 2;
    WORD BITFLD_GAUSS_INV                            : 1;
    WORD BITFLD_GAUSS_86                             : 1;
    WORD BITFLD_TXDATA_INV                           : 1;
    WORD BITFLD_EO_PACKET_DIS                        : 1;
    WORD BITFLD_BT_SEL                               : 1;
};
#define SD_ORDER_RX                          (0x0003)
#define SD_ORDER_TX                          (0x000C)
#define MODINDEX                             (0x0030)
#define DELAY                                (0x00C0)
#define GAUSS_INV                            (0x0100)
#define GAUSS_86                             (0x0200)
#define TXDATA_INV                           (0x0400)
#define EO_PACKET_DIS                        (0x0800)
#define BT_SEL                               (0x1000)



/*=============================*/
struct __RF_SYNTH_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_MODVAL_WR                            : 14;
    WORD BITFLD_MODVAL_SEL                           : 1;
};
#define MODVAL_WR                            (0x3FFF)
#define MODVAL_SEL                           (0x4000)



/*=============================*/
struct __RF_VCOCAL_CTRL_REG
/*=============================*/
{
    WORD BITFLD_VCO_FREQTRIM_WR                      : 4;
    WORD BITFLD_VCO_FREQTRIM_SEL                     : 1;
    WORD BITFLD_VCOCAL_PERIOD                        : 2;
};
#define VCO_FREQTRIM_WR                      (0x000F)
#define VCO_FREQTRIM_SEL                     (0x0010)
#define VCOCAL_PERIOD                        (0x0060)



/*=============================*/
struct __RF_MGAIN_CTRL_REG
/*=============================*/
{
    WORD BITFLD_GAUSS_GAIN_WR                        : 8;
    WORD BITFLD_GAUSS_GAIN_SEL                       : 1;
    WORD BITFLD_MGAIN_DBL_TRANSMIT                   : 1;
    WORD BITFLD_MGAIN_CMP_INV                        : 1;
    WORD BITFLD_MGAIN_AVER                           : 2;
    WORD BITFLD_KMOD_ALPHA                           : 3;
};
#define GAUSS_GAIN_WR                        (0x00FF)
#define GAUSS_GAIN_SEL                       (0x0100)
#define MGAIN_DBL_TRANSMIT                   (0x0200)
#define MGAIN_CMP_INV                        (0x0400)
#define MGAIN_AVER                           (0x1800)
#define KMOD_ALPHA                           (0xE000)



/*=============================*/
struct __RF_MGC_CTRL_REG
/*=============================*/
{
    WORD BITFLD_MGC_GAIN_SET                         : 1;
    WORD BITFLD_MGC_POLE_SW                          : 1;
    WORD BITFLD_GAUSS_DAC_CTRL                       : 2;
};
#define MGC_GAIN_SET                         (0x0001)
#define MGC_POLE_SW                          (0x0002)
#define GAUSS_DAC_CTRL                       (0x000C)



/*=============================*/
struct __RF_VCOVAR_CTRL_REG
/*=============================*/
{
    WORD BITFLD_VCO_VAR_CTRL                         : 16;
};
#define VCO_VAR_CTRL                         (0xFFFF)



/*=============================*/
struct __RF_PFD_CTRL_REG
/*=============================*/
{
    WORD BITFLD_FIXED_CUR_SET                        : 2;
    WORD BITFLD_FIXED_CUR_EN                         : 1;
    WORD BITFLD_PFD_POLARITY                         : 1;
};
#define FIXED_CUR_SET                        (0x0003)
#define FIXED_CUR_EN                         (0x0004)
#define PFD_POLARITY                         (0x0008)



/*=============================*/
struct __RF_CP_CTRL_REG
/*=============================*/
{
    WORD BITFLD_CP_CUR_SET_RX                        : 4;
    WORD BITFLD_CP_CUR_SET_TX                        : 4;
    WORD BITFLD_CP_CUR_RX                            : 4;
    WORD BITFLD_CP_CUR_TX                            : 4;
};
#define CP_CUR_SET_RX                        (0x000F)
#define CP_CUR_SET_TX                        (0x00F0)
#define CP_CUR_RX                            (0x0F00)
#define CP_CUR_TX                            (0xF000)



/*=============================*/
struct __RF_LF_RES_CTRL_REG
/*=============================*/
{
    WORD BITFLD_LF_RES_SET_RX                        : 4;
    WORD BITFLD_LF_RES_SET_TX                        : 4;
    WORD BITFLD_LF_RES_RX                            : 4;
    WORD BITFLD_LF_RES_TX                            : 4;
};
#define LF_RES_SET_RX                        (0x000F)
#define LF_RES_SET_TX                        (0x00F0)
#define LF_RES_RX                            (0x0F00)
#define LF_RES_TX                            (0xF000)



/*=============================*/
struct __RF_LF_CTRL_REG
/*=============================*/
{
    WORD BITFLD_LF_CAL_CAP_WR                        : 5;
    WORD BITFLD_LF_CAL_CAP_SEL                       : 1;
    WORD BITFLD_LF_SHORT_R4                          : 1;
};
#define LF_CAL_CAP_WR                        (0x001F)
#define LF_CAL_CAP_SEL                       (0x0020)
#define LF_SHORT_R4                          (0x0040)



/*=============================*/
struct __RF_TDC_CTRL_REG
/*=============================*/
{
    WORD BITFLD_CTRL_FAST                            : 4;
    WORD BITFLD_CTRL_SLOW                            : 4;
    WORD BITFLD_CAL_PH_1                             : 1;
    WORD BITFLD_CAL_PH_2                             : 1;
    WORD BITFLD_REF_CTRL                             : 2;
    WORD BITFLD_TDC_CONNECT                          : 1;
};
#define CTRL_FAST                            (0x000F)
#define CTRL_SLOW                            (0x00F0)
#define CAL_PH_1                             (0x0100)
#define CAL_PH_2                             (0x0200)
#define REF_CTRL                             (0x0C00)
#define TDC_CONNECT                          (0x1000)



/*=============================*/
struct __TEST_CTRL_REG
/*=============================*/
{
    WORD BITFLD_SHOW_CLOCKS                          : 1;
    WORD BITFLD_ENABLE_RFPT                          : 1;
    WORD BITFLD_SHOW_DC_STATE                        : 1;
    WORD BITFLD_SHOW_DC_COMP                         : 1;
    WORD BITFLD_XTAL16M_CAP_TEST_EN                  : 1;
    WORD BITFLD_SHOW_IF_RO                           : 1;
    WORD BITFLD_PLL_TST_MODE                         : 1;
};
#define SHOW_CLOCKS                          (0x0001)
#define ENABLE_RFPT                          (0x0002)
#define SHOW_DC_STATE                        (0x0004)
#define SHOW_DC_COMP                         (0x0008)
#define XTAL16M_CAP_TEST_EN                  (0x0010)
#define SHOW_IF_RO                           (0x0020)
#define PLL_TST_MODE                         (0x0040)



/*=============================*/
struct __TEST_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_ANA_TESTMUX_CTRL                     : 4;
    WORD                                             : 4;
    WORD BITFLD_RF_IN_TESTMUX_CTRL                   : 2;
};
#define ANA_TESTMUX_CTRL                     (0x000F)
#define RF_IN_TESTMUX_CTRL                   (0x0300)



/*=============================*/
struct __TEST_CTRL3_REG
/*=============================*/
{
    WORD BITFLD_RF_TEST_OUT_SEL                      : 6;
    WORD                                             : 2;
    WORD BITFLD_RF_TEST_OUT_PARAM                    : 8;
};
#define RF_TEST_OUT_SEL                      (0x003F)
#define RF_TEST_OUT_PARAM                    (0xFF00)



/*=============================*/
struct __TEST_CTRL4_REG
/*=============================*/
{
    WORD BITFLD_RF_TEST_IN_SEL                       : 3;
    WORD                                             : 5;
    WORD BITFLD_RF_TEST_IN_PARAM                     : 8;
};
#define RF_TEST_IN_SEL                       (0x0007)
#define RF_TEST_IN_PARAM                     (0xFF00)



/*=============================*/
struct __TEST_CTRL5_REG
/*=============================*/
{
    WORD BITFLD_TEST_VDD                             : 1;
    WORD BITFLD_TEST_OTP_ADC_VVD                     : 1;
    WORD BITFLD_TEST_OTP_OTA                         : 1;
    WORD BITFLD_TEST_OTP_VSS                         : 1;
    WORD                                             : 4;
    WORD BITFLD_TEST_STRUCT                          : 4;
    WORD BITFLD_DCDC_NSW                             : 1;
    WORD BITFLD_DCDC_PSW                             : 1;
    WORD BITFLD_DCDC_OUTSW                           : 1;
    WORD BITFLD_DCDC_FORCE_IDLE                      : 1;
};
#define TEST_VDD                             (0x0001)
#define TEST_OTP_ADC_VVD                     (0x0002)
#define TEST_OTP_OTA                         (0x0004)
#define TEST_OTP_VSS                         (0x0008)
#define TEST_STRUCT                          (0x0F00)
#define DCDC_NSW                             (0x1000)
#define DCDC_PSW                             (0x2000)
#define DCDC_OUTSW                           (0x4000)
#define DCDC_FORCE_IDLE                      (0x8000)



/*=============================*/
struct __BIST_CTRL_REG
/*=============================*/
{
    WORD BITFLD_RAM_BIST_CONFIG                      : 2;
    WORD BITFLD_ROMBIST_ENABLE                       : 1;
    WORD BITFLD_RAMBIST_ENABLE                       : 1;
    WORD BITFLD_SHOW_BIST                            : 1;
    WORD BITFLD_ROM_BIST_BUSY                        : 1;
    WORD BITFLD_RETRAM_BIST_LINE_FAIL                : 1;
    WORD BITFLD_RETRAM_BIST_FAIL                     : 1;
    WORD BITFLD_RETRAM_BIST_BUSY                     : 1;
    WORD BITFLD_SYSRAM_BIST_LINE_FAIL                : 1;
    WORD BITFLD_SYSRAM_BIST_FAIL                     : 1;
    WORD BITFLD_SYSRAM_BIST_BUSY                     : 1;
    WORD BITFLD_RAM_BIST_PATTERN                     : 2;
};
#define RAM_BIST_CONFIG                      (0x0003)
#define ROMBIST_ENABLE                       (0x0004)
#define RAMBIST_ENABLE                       (0x0008)
#define SHOW_BIST                            (0x0010)
#define ROM_BIST_BUSY                        (0x0020)
#define RETRAM_BIST_LINE_FAIL                (0x0040)
#define RETRAM_BIST_FAIL                     (0x0080)
#define RETRAM_BIST_BUSY                     (0x0100)
#define SYSRAM_BIST_LINE_FAIL                (0x0200)
#define SYSRAM_BIST_FAIL                     (0x0400)
#define SYSRAM_BIST_BUSY                     (0x0800)
#define RAM_BIST_PATTERN                     (0x3000)



/*=============================*/
struct __ROMBIST_RESULTL_REG
/*=============================*/
{
    WORD BITFLD_ROMBIST_RESULTL                      : 16;
};
#define ROMBIST_RESULTL                      (0xFFFF)



/*=============================*/
struct __ROMBIST_RESULTH_REG
/*=============================*/
{
    WORD BITFLD_ROMBIST_RESULTH                      : 16;
};
#define ROMBIST_RESULTH                      (0xFFFF)



/*=============================*/
struct __RFPT_CTRL_REG
/*=============================*/
{
    WORD BITFLD_RFPT_PACK_EN                         : 1;
    WORD                                             : 1;
};
#define RFPT_PACK_EN                         (0x0001)



/*=============================*/
struct __RFPT_ADDR_REG
/*=============================*/
{
    WORD                                             : 2;
    WORD BITFLD_RFPT_ADDR                            : 14;
};
#define RFPT_ADDR                            (0xFFFC)



/*=============================*/
struct __RFPT_LEN_REG
/*=============================*/
{
    WORD BITFLD_RFPT_LEN                             : 14;
};
#define RFPT_LEN                             (0x3FFF)



/*=============================*/
struct __RFPT_STAT_REG
/*=============================*/
{
    WORD BITFLD_RFPT_ACTIVE                          : 1;
    WORD BITFLD_RFPT_OFLOW_STK                       : 1;
};
#define RFPT_ACTIVE                          (0x0001)
#define RFPT_OFLOW_STK                       (0x0002)



/*===========================================================================*/
/* memory map BLE */
/*===========================================================================*/
#define BLE_RWBTLECNTL_REG                   (0x40000000) /* BLE Control register */
#define BLE_VERSION_REG                      (0x40000004) /* Version register */
#define BLE_RWBTLECONF_REG                   (0x40000008) /* Configuration register */
#define BLE_INTCNTL_REG                      (0x4000000C) /* Interrupt controller register */
#define BLE_INTSTAT_REG                      (0x40000010) /* Interrupt status register */
#define BLE_INTRAWSTAT_REG                   (0x40000014) /* Interrupt raw status register */
#define BLE_INTACK_REG                       (0x40000018) /* Interrupt acknowledge register */
#define BLE_BASETIMECNT_REG                  (0x4000001C) /* Base time reference counter */
#define BLE_FINETIMECNT_REG                  (0x40000020) /* Fine time reference counter */
#define BLE_BDADDRL_REG                      (0x40000024) /* BLE device address LSB register */
#define BLE_BDADDRU_REG                      (0x40000028) /* BLE device address MSB register */
#define BLE_CURRENTRXDESCPTR_REG             (0x4000002C) /* Rx Descriptor Pointer for the Receive Buffer Chained List */
#define BLE_DEEPSLCNTL_REG                   (0x40000030) /* Deep-Sleep control register */
#define BLE_DEEPSLWKUP_REG                   (0x40000034) /* Time (measured in Low Power clock cycles) in Deep Sleep Mode before waking-up the device */
#define BLE_DEEPSLSTAT_REG                   (0x40000038) /* Duration of the last deep sleep phase register */
#define BLE_ENBPRESET_REG                    (0x4000003C) /* Time in low power oscillator cycles register */
#define BLE_FINECNTCORR_REG                  (0x40000040) /* Phase correction value register */
#define BLE_BASETIMECNTCORR_REG              (0x40000044) /* Base Time Counter */
#define BLE_DIAGCNTL_REG                     (0x40000050) /* Diagnostics Register */
#define BLE_DIAGSTAT_REG                     (0x40000054) /* Debug use only */
#define BLE_DEBUGADDMAX_REG                  (0x40000058) /* Upper limit for the memory zone */
#define BLE_DEBUGADDMIN_REG                  (0x4000005C) /* Lower limit for the memory zone */
#define BLE_ERRORTYPESTAT_REG                (0x40000060) /* Error Type Status registers */
#define BLE_SWPROFILING_REG                  (0x40000064) /* Software Profiling register */
#define BLE_RADIOCNTL0_REG                   (0x40000070) /* Radio interface control register */
#define BLE_RADIOCNTL1_REG                   (0x40000074) /* Radio interface control register */
#define BLE_RADIOPWRUPDN_REG                 (0x40000080) /* RX/TX power up/down phase register */
#define BLE_ADVCHMAP_REG                     (0x40000090) /* Advertising Channel Map */
#define BLE_ADVTIM_REG                       (0x400000A0) /* Advertising Packet Interval */
#define BLE_ACTSCANSTAT_REG                  (0x400000A4) /* Active scan register */
#define BLE_WLPUBADDPTR_REG                  (0x400000B0) /* Start address of public devices list */
#define BLE_WLPRIVADDPTR_REG                 (0x400000B4) /* Start address of private devices list */
#define BLE_WLNBDEV_REG                      (0x400000B8) /* Devices in white list */
#define BLE_AESCNTL_REG                      (0x400000C0) /* Start AES register */
#define BLE_AESKEY31_0_REG                   (0x400000C4) /* AES encryption key */
#define BLE_AESKEY63_32_REG                  (0x400000C8) /* AES encryption key */
#define BLE_AESKEY95_64_REG                  (0x400000CC) /* AES encryption key */
#define BLE_AESKEY127_96_REG                 (0x400000D0) /* AES encryption key */
#define BLE_AESPTR_REG                       (0x400000D4) /* Pointer to the block to encrypt/decrypt */
#define BLE_TXMICVAL_REG                     (0x400000D8) /* AES / CCM plain MIC value */
#define BLE_RXMICVAL_REG                     (0x400000DC) /* AES / CCM plain MIC value */
#define BLE_RFTESTCNTL_REG                   (0x400000E0) /* RF Testing Register */
#define BLE_TIMGENCNTL_REG                   (0x400000F0) /* Timing Generator Register */
#define BLE_GROSSTIMTGT_REG                  (0x400000F4) /* Gross Timer Target value */
#define BLE_FINETIMTGT_REG                   (0x400000F8) /* Fine Timer Target value */
#define BLE_SAMPLECLK_REG                    (0x400000FC) /* Samples the Base Time Counter */
#define BLE_CNTL2_REG                        (0x40000200) /* BLE Control Register 2 */
#define BLE_RF_DIAGIRQ_REG                   (0x40000204) /* BLE/RF Diagnostic IRQ Control Register */



/*=============================*/
struct __BLE_RWBTLECNTL_REG
/*=============================*/
{
    WORD BITFLD_SYNCERR                              : 3;
    WORD                                             : 1;
    WORD BITFLD_RXWINSZDEF                           : 4;
    WORD BITFLD_RWBLE_EN                             : 1;
    WORD BITFLD_WLSYNC_EN                            : 1;
    WORD BITFLD_ADVERRFILT_EN                        : 1;
    WORD BITFLD_RXDESCPTRSEL                         : 1;
    WORD BITFLD_TXWINOFFSEL                          : 1;
    WORD                                             : 3;
    WORD BITFLD_HOP_REMAP_DSB                        : 1;
    WORD BITFLD_CRC_DSB                              : 1;
    WORD BITFLD_WHIT_DSB                             : 1;
    WORD BITFLD_CRYPT_DSB                            : 1;
    WORD BITFLD_NESN_DSB                             : 1;
    WORD BITFLD_SN_DSB                               : 1;
    WORD BITFLD_MD_DSB                               : 1;
    WORD                                             : 1;
    WORD BITFLD_SCAN_ABORT                           : 1;
    WORD BITFLD_ADVERT_ABORT                         : 1;
    WORD BITFLD_RFTEST_ABORT                         : 1;
    WORD                                             : 2;
    WORD BITFLD_REG_SOFT_RST                         : 1;
    WORD BITFLD_MASTER_TGSOFT_RST                    : 1;
    WORD BITFLD_MASTER_SOFT_RST                      : 1;
};
#define SYNCERR                              (0x0007)
#define RXWINSZDEF                           (0x00F0)
#define RWBLE_EN                             (0x0100)
#define WLSYNC_EN                            (0x0200)
#define ADVERRFILT_EN                        (0x0400)
#define RXDESCPTRSEL                         (0x0800)
#define TXWINOFFSEL                          (0x1000)
#define HOP_REMAP_DSB                        (0x10000)
#define CRC_DSB                              (0x20000)
#define WHIT_DSB                             (0x40000)
#define CRYPT_DSB                            (0x80000)
#define NESN_DSB                             (0x100000)
#define SN_DSB                               (0x200000)
#define MD_DSB                               (0x400000)
#define SCAN_ABORT                           (0x1000000)
#define ADVERT_ABORT                         (0x2000000)
#define RFTEST_ABORT                         (0x4000000)
#define REG_SOFT_RST                         (0x20000000)
#define MASTER_TGSOFT_RST                    (0x40000000)
#define MASTER_SOFT_RST                      (0x80000000)



/*=============================*/
struct __BLE_VERSION_REG
/*=============================*/
{
    WORD BITFLD_BUILD                                : 8;
    WORD BITFLD_REL                                  : 8;
    DWORD BITFLD_UPG                                 : 18;
    WORD BITFLD_TYP                                  : 8;
};
#define BUILD                                (0x00FF)
#define REL                                  (0xFF0000)
#define UPG                                  (0x3FFFF00)
#define TYP                                  (0xFF000000)



/*=============================*/
struct __BLE_RWBTLECONF_REG
/*=============================*/
{
    WORD BITFLD_BUSWIDTH                             : 1;
    WORD BITFLD_USECRYPT                             : 1;
    WORD BITFLD_USEDBG                               : 1;
    WORD BITFLD_WLAN                                 : 1;
    WORD BITFLD_INTMODE                              : 1;
    WORD BITFLD_DMMODE                               : 1;
    WORD                                             : 2;
    WORD BITFLD_CLK_SEL                              : 6;
    WORD                                             : 2;
    WORD BITFLD_RFIF                                 : 7;
    WORD                                             : 1;
    WORD BITFLD_ADD_WIDTH                            : 6;
};
#define BUSWIDTH                             (0x0001)
#define USECRYPT                             (0x0002)
#define USEDBG                               (0x0004)
#define WLAN                                 (0x0008)
#define INTMODE                              (0x0010)
#define DMMODE                               (0x0020)
#define CLK_SEL                              (0x3F00)
#define RFIF                                 (0x7F0000)
#define ADD_WIDTH                            (0x3F000000)



/*=============================*/
struct __BLE_INTCNTL_REG
/*=============================*/
{
    WORD BITFLD_CSCNTINTMSK                          : 1;
    WORD BITFLD_RXINTMSK                             : 1;
    WORD BITFLD_SLPINTMSK                            : 1;
    WORD BITFLD_EVENTINTMSK                          : 1;
    WORD BITFLD_CRYPTINTMSK                          : 1;
    WORD BITFLD_ERRORINTMSK                          : 1;
    WORD BITFLD_GROSSTGTIMINTMSK                     : 1;
    WORD BITFLD_FINETGTIMINTMSK                      : 1;
    WORD BITFLD_RADIOCNTLINTMSK                      : 1;
    WORD                                             : 6;
    WORD BITFLD_CSCNTDEVMSK                          : 1;
    WORD BITFLD_INTCSCNTL                            : 16;
};
#define CSCNTINTMSK                          (0x0001)
#define RXINTMSK                             (0x0002)
#define SLPINTMSK                            (0x0004)
#define EVENTINTMSK                          (0x0008)
#define CRYPTINTMSK                          (0x0010)
#define ERRORINTMSK                          (0x0020)
#define GROSSTGTIMINTMSK                     (0x0040)
#define FINETGTIMINTMSK                      (0x0080)
#define RADIOCNTLINTMSK                      (0x0100)
#define CSCNTDEVMSK                          (0x8000)
#define INTCSCNTL                            (0xFFFF0000)



/*=============================*/
struct __BLE_INTSTAT_REG
/*=============================*/
{
    WORD BITFLD_CSCNTINTSTAT                         : 1;
    WORD BITFLD_RXINTSTAT                            : 1;
    WORD BITFLD_SLPINTSTAT                           : 1;
    WORD BITFLD_EVENTINTSTAT                         : 1;
    WORD BITFLD_CRYPTINTSTAT                         : 1;
    WORD BITFLD_ERRORINTSTAT                         : 1;
    WORD BITFLD_GROSSTGTIMINTSTAT                    : 1;
    WORD BITFLD_FINETGTIMINTSTAT                     : 1;
    WORD BITFLD_RADIOCNTLINTSTAT                     : 1;
};
#define CSCNTINTSTAT                         (0x0001)
#define RXINTSTAT                            (0x0002)
#define SLPINTSTAT                           (0x0004)
#define EVENTINTSTAT                         (0x0008)
#define CRYPTINTSTAT                         (0x0010)
#define ERRORINTSTAT                         (0x0020)
#define GROSSTGTIMINTSTAT                    (0x0040)
#define FINETGTIMINTSTAT                     (0x0080)
#define RADIOCNTLINTSTAT                     (0x0100)



/*=============================*/
struct __BLE_INTRAWSTAT_REG
/*=============================*/
{
    WORD BITFLD_CSCNTINTRAWSTAT                      : 1;
    WORD BITFLD_RXINTRAWSTAT                         : 1;
    WORD BITFLD_SLPINTRAWSTAT                        : 1;
    WORD BITFLD_EVENTINTRAWSTAT                      : 1;
    WORD BITFLD_CRYPTINTRAWSTAT                      : 1;
    WORD BITFLD_ERRORINTRAWSTAT                      : 1;
    WORD BITFLD_GROSSTGTIMINTRAWSTAT                 : 1;
    WORD BITFLD_FINETGTIMINTRAWSTAT                  : 1;
    WORD BITFLD_RADIOCNTLINTRAWSTAT                  : 1;
};
#define CSCNTINTRAWSTAT                      (0x0001)
#define RXINTRAWSTAT                         (0x0002)
#define SLPINTRAWSTAT                        (0x0004)
#define EVENTINTRAWSTAT                      (0x0008)
#define CRYPTINTRAWSTAT                      (0x0010)
#define ERRORINTRAWSTAT                      (0x0020)
#define GROSSTGTIMINTRAWSTAT                 (0x0040)
#define FINETGTIMINTRAWSTAT                  (0x0080)
#define RADIOCNTLINTRAWSTAT                  (0x0100)



/*=============================*/
struct __BLE_INTACK_REG
/*=============================*/
{
    WORD BITFLD_CSCNTINTACK                          : 1;
    WORD BITFLD_RXINTACK                             : 1;
    WORD BITFLD_SLPINTACK                            : 1;
    WORD BITFLD_EVENTINTACK                          : 1;
    WORD BITFLD_CRYPTINTACK                          : 1;
    WORD BITFLD_ERRORINTACK                          : 1;
    WORD BITFLD_GROSSTGTIMINTACK                     : 1;
    WORD BITFLD_FINETGTIMINTACK                      : 1;
    WORD BITFLD_RADIOCNTLINTACK                      : 1;
};
#define CSCNTINTACK                          (0x0001)
#define RXINTACK                             (0x0002)
#define SLPINTACK                            (0x0004)
#define EVENTINTACK                          (0x0008)
#define CRYPTINTACK                          (0x0010)
#define ERRORINTACK                          (0x0020)
#define GROSSTGTIMINTACK                     (0x0040)
#define FINETGTIMINTACK                      (0x0080)
#define RADIOCNTLINTACK                      (0x0100)



/*=============================*/
struct __BLE_BASETIMECNT_REG
/*=============================*/
{
    DWORD BITFLD_BASETIMECNT                         : 27;
};
#define BASETIMECNT                          (0x7FFFFFF)



/*=============================*/
struct __BLE_FINETIMECNT_REG
/*=============================*/
{
    WORD BITFLD_FINECNT                              : 10;
};
#define FINECNT                              (0x03FF)



/*=============================*/
struct __BLE_BDADDRL_REG
/*=============================*/
{
    DWORD BITFLD_BDADDRL                             : 32;
};
#define BDADDRL                              (0xFFFFFFFF)



/*=============================*/
struct __BLE_BDADDRU_REG
/*=============================*/
{
    WORD BITFLD_BDADDRU                              : 16;
    WORD BITFLD_PRIV_NPUB                            : 1;
};
#define BDADDRU                              (0xFFFF)
#define PRIV_NPUB                            (0x10000)



/*=============================*/
struct __BLE_CURRENTRXDESCPTR_REG
/*=============================*/
{
    WORD BITFLD_CURRENTRXDESCPTR                     : 14;
};
#define CURRENTRXDESCPTR                     (0x3FFF)



/*=============================*/
struct __BLE_DEEPSLCNTL_REG
/*=============================*/
{
    WORD BITFLD_DEEP_SLEEP_IRQ_EN                    : 2;
    WORD BITFLD_DEEP_SLEEP_ON                        : 1;
    WORD BITFLD_DEEP_SLEEP_CORR_EN                   : 1;
    WORD BITFLD_SOFT_WAKEUP_REQ                      : 1;
    WORD                                             : 10;
    WORD BITFLD_DEEP_SLEEP_STAT                      : 1;
    WORD                                             : 15;
    WORD BITFLD_EXTWKUPDSB                           : 1;
};
#define DEEP_SLEEP_IRQ_EN                    (0x0003)
#define DEEP_SLEEP_ON                        (0x0004)
#define DEEP_SLEEP_CORR_EN                   (0x0008)
#define SOFT_WAKEUP_REQ                      (0x0010)
#define DEEP_SLEEP_STAT                      (0x8000)
#define EXTWKUPDSB                           (0x80000000)



/*=============================*/
struct __BLE_DEEPSLWKUP_REG
/*=============================*/
{
    DWORD BITFLD_DEEPSLTIME                          : 32;
};
#define DEEPSLTIME                           (0xFFFFFFFF)



/*=============================*/
struct __BLE_DEEPSLSTAT_REG
/*=============================*/
{
    DWORD BITFLD_DEEPSLDUR                           : 32;
};
#define DEEPSLDUR                            (0xFFFFFFFF)



/*=============================*/
struct __BLE_ENBPRESET_REG
/*=============================*/
{
    WORD BITFLD_TWIRQ_RESET                          : 10;
    WORD BITFLD_TWIRQ_SET                            : 11;
    WORD BITFLD_TWEXT                                : 11;
};
#define TWIRQ_RESET                          (0x03FF)
#define TWIRQ_SET                            (0x1FFC00)
#define TWEXT                                (0xFFE00000)



/*=============================*/
struct __BLE_FINECNTCORR_REG
/*=============================*/
{
    WORD BITFLD_FINECNTCORR                          : 10;
};
#define FINECNTCORR                          (0x03FF)



/*=============================*/
struct __BLE_BASETIMECNTCORR_REG
/*=============================*/
{
    DWORD BITFLD_BASETIMECNTCORR                     : 27;
};
#define BASETIMECNTCORR                      (0x7FFFFFF)



/*=============================*/
struct __BLE_DIAGCNTL_REG
/*=============================*/
{
    WORD BITFLD_DIAG0                                : 6;
    WORD                                             : 1;
    WORD BITFLD_DIAG0_EN                             : 1;
    WORD BITFLD_DIAG1                                : 6;
    WORD                                             : 1;
    WORD BITFLD_DIAG1_EN                             : 1;
    WORD BITFLD_DIAG2                                : 6;
    WORD                                             : 1;
    WORD BITFLD_DIAG2_EN                             : 1;
    WORD BITFLD_DIAG3                                : 6;
    WORD                                             : 1;
    WORD BITFLD_DIAG3_EN                             : 1;
};
#define DIAG0                                (0x003F)
#define DIAG0_EN                             (0x0080)
#define DIAG1                                (0x3F00)
#define DIAG1_EN                             (0x8000)
#define DIAG2                                (0x3F0000)
#define DIAG2_EN                             (0x800000)
#define DIAG3                                (0x3F000000)
#define DIAG3_EN                             (0x80000000)



/*=============================*/
struct __BLE_DIAGSTAT_REG
/*=============================*/
{
    WORD BITFLD_DIAG0STAT                            : 8;
    WORD BITFLD_DIAG1STAT                            : 8;
    WORD BITFLD_DIAG2STAT                            : 8;
    WORD BITFLD_DIAG3STAT                            : 8;
};
#define DIAG0STAT                            (0x00FF)
#define DIAG1STAT                            (0xFF00)
#define DIAG2STAT                            (0xFF0000)
#define DIAG3STAT                            (0xFF000000)



/*=============================*/
struct __BLE_DEBUGADDMAX_REG
/*=============================*/
{
    WORD BITFLD_ADDMAX                               : 16;
};
#define ADDMAX                               (0xFFFF)



/*=============================*/
struct __BLE_DEBUGADDMIN_REG
/*=============================*/
{
    WORD BITFLD_ADDMIN                               : 16;
};
#define ADDMIN                               (0xFFFF)



/*=============================*/
struct __BLE_ERRORTYPESTAT_REG
/*=============================*/
{
    WORD BITFLD_TXCRYPT_ERROR                        : 1;
    WORD BITFLD_PKTCNTL_EMACC_ERROR                  : 1;
    WORD BITFLD_TXDESC_ERROR                         : 1;
    WORD BITFLD_APFM_ERROR                           : 1;
    WORD BITFLD_WHITELIST_ERROR                      : 1;
    WORD BITFLD_RXCRYPT_ERROR                        : 1;
    WORD BITFLD_IFS_UNDERRUN                         : 1;
    WORD BITFLD_LLCHMAP_ERROR                        : 1;
    WORD BITFLD_RADIO_EMACC_ERROR                    : 1;
    WORD                                             : 1;
    WORD BITFLD_CSTXPTR_ERROR                        : 1;
    WORD BITFLD_CSFORMAT_ERROR                       : 1;
};
#define TXCRYPT_ERROR                        (0x0001)
#define PKTCNTL_EMACC_ERROR                  (0x0002)
#define TXDESC_ERROR                         (0x0004)
#define APFM_ERROR                           (0x0008)
#define WHITELIST_ERROR                      (0x0010)
#define RXCRYPT_ERROR                        (0x0020)
#define IFS_UNDERRUN                         (0x0040)
#define LLCHMAP_ERROR                        (0x0080)
#define RADIO_EMACC_ERROR                    (0x0100)
#define CSTXPTR_ERROR                        (0x0400)
#define CSFORMAT_ERROR                       (0x0800)



/*=============================*/
struct __BLE_SWPROFILING_REG
/*=============================*/
{
    DWORD BITFLD_SWPROFVAL                           : 32;
};
#define SWPROFVAL                            (0xFFFFFFFF)



/*=============================*/
struct __BLE_RADIOCNTL0_REG
/*=============================*/
{
    DWORD                                            : 22;
    WORD BITFLD_DPCORR_EN                            : 1;
};
#define DPCORR_EN                            (0x400000)



/*=============================*/
struct __BLE_RADIOCNTL1_REG
/*=============================*/
{
    WORD                                             : 16;
    WORD BITFLD_XRFSEL                               : 5;
};
#define XRFSEL                               (0x1F0000)



/*=============================*/
struct __BLE_RADIOPWRUPDN_REG
/*=============================*/
{
    WORD BITFLD_TXPWRUP                              : 8;
    WORD BITFLD_TXPWRDN                              : 4;
    WORD                                             : 4;
    WORD BITFLD_RXPWRUP                              : 8;
    WORD BITFLD_RTRIP_DELAY                          : 7;
};
#define TXPWRUP                              (0x00FF)
#define TXPWRDN                              (0x0F00)
#define RXPWRUP                              (0xFF0000)
#define RTRIP_DELAY                          (0x7F000000)



/*=============================*/
struct __BLE_ADVCHMAP_REG
/*=============================*/
{
    WORD BITFLD_ADVCHMAP                             : 3;
};
#define ADVCHMAP                             (0x0007)



/*=============================*/
struct __BLE_ADVTIM_REG
/*=============================*/
{
    WORD BITFLD_ADVINT                               : 14;
};
#define ADVINT                               (0x3FFF)



/*=============================*/
struct __BLE_ACTSCANSTAT_REG
/*=============================*/
{
    WORD BITFLD_UPPERLIMIT                           : 9;
    WORD                                             : 7;
    WORD BITFLD_BACKOFF                              : 9;
};
#define UPPERLIMIT                           (0x01FF)
#define BACKOFF                              (0x1FF0000)



/*=============================*/
struct __BLE_WLPUBADDPTR_REG
/*=============================*/
{
    WORD BITFLD_WLPUBADDPTR                          : 16;
};
#define WLPUBADDPTR                          (0xFFFF)



/*=============================*/
struct __BLE_WLPRIVADDPTR_REG
/*=============================*/
{
    WORD BITFLD_WLPRIVADDPTR                         : 16;
};
#define WLPRIVADDPTR                         (0xFFFF)



/*=============================*/
struct __BLE_WLNBDEV_REG
/*=============================*/
{
    WORD BITFLD_NBPUBDEV                             : 8;
    WORD BITFLD_NBPRIVDEV                            : 8;
};
#define NBPUBDEV                             (0x00FF)
#define NBPRIVDEV                            (0xFF00)



/*=============================*/
struct __BLE_AESCNTL_REG
/*=============================*/
{
    WORD BITFLD_AES_START                            : 1;
};
#define AES_START                            (0x0001)



/*=============================*/
struct __BLE_AESKEY31_0_REG
/*=============================*/
{
    DWORD BITFLD_AESKEY31_0                          : 32;
};
#define AESKEY31_0                           (0xFFFFFFFF)



/*=============================*/
struct __BLE_AESKEY63_32_REG
/*=============================*/
{
    DWORD BITFLD_AESKEY63_32                         : 32;
};
#define AESKEY63_32                          (0xFFFFFFFF)



/*=============================*/
struct __BLE_AESKEY95_64_REG
/*=============================*/
{
    DWORD BITFLD_AESKEY95_64                         : 32;
};
#define AESKEY95_64                          (0xFFFFFFFF)



/*=============================*/
struct __BLE_AESKEY127_96_REG
/*=============================*/
{
    DWORD BITFLD_AESKEY127_96                        : 32;
};
#define AESKEY127_96                         (0xFFFFFFFF)



/*=============================*/
struct __BLE_AESPTR_REG
/*=============================*/
{
    WORD BITFLD_AESPTR                               : 16;
};
#define AESPTR                               (0xFFFF)



/*=============================*/
struct __BLE_TXMICVAL_REG
/*=============================*/
{
    DWORD BITFLD_TXMICVAL                            : 32;
};
#define TXMICVAL                             (0xFFFFFFFF)



/*=============================*/
struct __BLE_RXMICVAL_REG
/*=============================*/
{
    DWORD BITFLD_RXMICVAL                            : 32;
};
#define RXMICVAL                             (0xFFFFFFFF)



/*=============================*/
struct __BLE_RFTESTCNTL_REG
/*=============================*/
{
    WORD BITFLD_TXLENGTH                             : 9;
    WORD                                             : 3;
    WORD BITFLD_TXPLDSRC                             : 1;
    WORD BITFLD_PRBSTYPE                             : 1;
    WORD BITFLD_TXLENGTHSRC                          : 1;
    WORD BITFLD_INFINITETX                           : 1;
    WORD                                             : 15;
    WORD BITFLD_INFINITERX                           : 1;
};
#define TXLENGTH                             (0x01FF)
#define TXPLDSRC                             (0x1000)
#define PRBSTYPE                             (0x2000)
#define TXLENGTHSRC                          (0x4000)
#define INFINITETX                           (0x8000)
#define INFINITERX                           (0x80000000)



/*=============================*/
struct __BLE_TIMGENCNTL_REG
/*=============================*/
{
    WORD BITFLD_PREFTECH_TIME                        : 8;
};
#define PREFTECH_TIME                        (0x00FF)



/*=============================*/
struct __BLE_GROSSTIMTGT_REG
/*=============================*/
{
    WORD BITFLD_GROSSTARGET                          : 16;
};
#define GROSSTARGET                          (0xFFFF)



/*=============================*/
struct __BLE_FINETIMTGT_REG
/*=============================*/
{
    DWORD BITFLD_FINETARGET                          : 27;
};
#define FINETARGET                           (0x7FFFFFF)



/*=============================*/
struct __BLE_SAMPLECLK_REG
/*=============================*/
{
    WORD BITFLD_SAMP                                 : 1;
};
#define SAMP                                 (0x0001)



/*=============================*/
struct __BLE_CNTL2_REG
/*=============================*/
{
    WORD BITFLD_EMACCERRSTAT                         : 1;
    WORD BITFLD_EMACCERRACK                          : 1;
    WORD BITFLD_EMACCERRMSK                          : 1;
    WORD BITFLD_DIAGPORT_SEL                         : 2;
    WORD BITFLD_DIAGPORT_REVERSE                     : 1;
    WORD BITFLD_BLE_CLK_STAT                         : 1;
    WORD BITFLD_MON_LP_CLK                           : 1;
    WORD BITFLD_RADIO_PWRDN_ALLOW                    : 1;
    WORD BITFLD_BLE_CLK_SEL                          : 6;
    WORD                                             : 2;
    WORD BITFLD_RADIO_ONLY                           : 1;
    WORD BITFLD_BB_ONLY                              : 1;
    WORD BITFLD_SW_RPL_SPI                           : 1;
    WORD BITFLD_WAKEUPLPSTAT                         : 1;
    WORD BITFLD_BLE_RSSI_SEL                         : 1;
};
#define EMACCERRSTAT                         (0x0001)
#define EMACCERRACK                          (0x0002)
#define EMACCERRMSK                          (0x0004)
#define DIAGPORT_SEL                         (0x0018)
#define DIAGPORT_REVERSE                     (0x0020)
#define BLE_CLK_STAT                         (0x0040)
#define MON_LP_CLK                           (0x0080)
#define RADIO_PWRDN_ALLOW                    (0x0100)
#define BLE_CLK_SEL                          (0x7E00)
#define RADIO_ONLY                           (0x20000)
#define BB_ONLY                              (0x40000)
#define SW_RPL_SPI                           (0x80000)
#define WAKEUPLPSTAT                         (0x100000)
#define BLE_RSSI_SEL                         (0x200000)



/*=============================*/
struct __BLE_RF_DIAGIRQ_REG
/*=============================*/
{
    WORD BITFLD_DIAGIRQ_MASK_0                       : 1;
    WORD BITFLD_DIAGIRQ_WSEL_0                       : 2;
    WORD BITFLD_DIAGIRQ_BSEL_0                       : 3;
    WORD BITFLD_DIAGIRQ_EDGE_0                       : 1;
    WORD BITFLD_DIAGIRQ_STAT_0                       : 1;
    WORD BITFLD_DIAGIRQ_MASK_1                       : 1;
    WORD BITFLD_DIAGIRQ_WSEL_1                       : 2;
    WORD BITFLD_DIAGIRQ_BSEL_1                       : 3;
    WORD BITFLD_DIAGIRQ_EDGE_1                       : 1;
    WORD BITFLD_DIAGIRQ_STAT_1                       : 1;
    WORD BITFLD_DIAGIRQ_MASK_2                       : 1;
    WORD BITFLD_DIAGIRQ_WSEL_2                       : 2;
    WORD BITFLD_DIAGIRQ_BSEL_2                       : 3;
    WORD BITFLD_DIAGIRQ_EDGE_2                       : 1;
    WORD BITFLD_DIAGIRQ_STAT_2                       : 1;
    WORD BITFLD_DIAGIRQ_MASK_3                       : 1;
    WORD BITFLD_DIAGIRQ_WSEL_3                       : 2;
    WORD BITFLD_DIAGIRQ_BSEL_3                       : 3;
    WORD BITFLD_DIAGIRQ_EDGE_3                       : 1;
    WORD BITFLD_DIAGIRQ_STAT_3                       : 1;
};
#define DIAGIRQ_MASK_0                       (0x0001)
#define DIAGIRQ_WSEL_0                       (0x0006)
#define DIAGIRQ_BSEL_0                       (0x0038)
#define DIAGIRQ_EDGE_0                       (0x0040)
#define DIAGIRQ_STAT_0                       (0x0080)
#define DIAGIRQ_MASK_1                       (0x0100)
#define DIAGIRQ_WSEL_1                       (0x0600)
#define DIAGIRQ_BSEL_1                       (0x3800)
#define DIAGIRQ_EDGE_1                       (0x4000)
#define DIAGIRQ_STAT_1                       (0x8000)
#define DIAGIRQ_MASK_2                       (0x10000)
#define DIAGIRQ_WSEL_2                       (0x60000)
#define DIAGIRQ_BSEL_2                       (0x380000)
#define DIAGIRQ_EDGE_2                       (0x400000)
#define DIAGIRQ_STAT_2                       (0x800000)
#define DIAGIRQ_MASK_3                       (0x1000000)
#define DIAGIRQ_WSEL_3                       (0x6000000)
#define DIAGIRQ_BSEL_3                       (0x38000000)
#define DIAGIRQ_EDGE_3                       (0x40000000)
#define DIAGIRQ_STAT_3                       (0x80000000)



/*===========================================================================*/
/* memory map CRG */
/*===========================================================================*/
#define CLK_AMBA_REG                         (0x50000000) /* HCLK, PCLK, divider and clock gates */
#define CLK_FREQ_TRIM_REG                    (0x50000002) /* Xtal frequency trimming register. */
#define CLK_PER_REG                          (0x50000004) /* Peripheral divider register */
#define CLK_RADIO_REG                        (0x50000008) /* Radio PLL control register */
#define CLK_CTRL_REG                         (0x5000000A) /* Clock control register */
#define PMU_CTRL_REG                         (0x50000010) /* Power Management Unit control register */
#define SYS_CTRL_REG                         (0x50000012) /* System Control register */
#define SYS_STAT_REG                         (0x50000014) /* System status register */
#define TRIM_CTRL_REG                        (0x50000016) /* Control trimming of the XTAL16M */
#define CLK_32K_REG                          (0x50000020) /* 32 kHz oscillator register */
#define CLK_16M_REG                          (0x50000022) /* 16 MHz RC-oscillator register */
#define CLK_RCX32K_REG                       (0x50000024) /* bla */
#define BANDGAP_REG                          (0x50000028) /* bandgap trimming */
#define ANA_STATUS_REG                       (0x5000002A) /* status bit of analog (power management) circuits */



/*=============================*/
struct __CLK_AMBA_REG
/*=============================*/
{
    WORD BITFLD_HCLK_DIV                             : 2;
    WORD                                             : 2;
    WORD BITFLD_PCLK_DIV                             : 2;
    WORD                                             : 1;
    WORD BITFLD_OTP_ENABLE                           : 1;
};
#define HCLK_DIV                             (0x0003)
#define PCLK_DIV                             (0x0030)
#define OTP_ENABLE                           (0x0080)



/*=============================*/
struct __CLK_FREQ_TRIM_REG
/*=============================*/
{
    WORD BITFLD_FINE_ADJ                             : 8;
    WORD BITFLD_COARSE_ADJ                           : 3;
};
#define FINE_ADJ                             (0x00FF)
#define COARSE_ADJ                           (0x0700)



/*=============================*/
struct __CLK_PER_REG
/*=============================*/
{
    WORD BITFLD_TMR_DIV                              : 2;
    WORD                                             : 1;
    WORD BITFLD_TMR_ENABLE                           : 1;
    WORD BITFLD_WAKEUPCT_ENABLE                      : 1;
    WORD BITFLD_I2C_ENABLE                           : 1;
    WORD BITFLD_UART2_ENABLE                         : 1;
    WORD BITFLD_UART1_ENABLE                         : 1;
    WORD BITFLD_SPI_DIV                              : 2;
    WORD                                             : 1;
    WORD BITFLD_SPI_ENABLE                           : 1;
    WORD                                             : 2;
    WORD BITFLD_QUAD_ENABLE                          : 1;
};
#define TMR_DIV                              (0x0003)
#define TMR_ENABLE                           (0x0008)
#define WAKEUPCT_ENABLE                      (0x0010)
#define I2C_ENABLE                           (0x0020)
#define UART2_ENABLE                         (0x0040)
#define UART1_ENABLE                         (0x0080)
#define SPI_DIV                              (0x0300)
#define SPI_ENABLE                           (0x0800)
#define QUAD_ENABLE                          (0x8000)



/*=============================*/
struct __CLK_RADIO_REG
/*=============================*/
{
    WORD BITFLD_RFCU_DIV                             : 2;
    WORD                                             : 1;
    WORD BITFLD_RFCU_ENABLE                          : 1;
    WORD BITFLD_BLE_DIV                              : 2;
    WORD BITFLD_BLE_LP_RESET                         : 1;
    WORD BITFLD_BLE_ENABLE                           : 1;
};
#define RFCU_DIV                             (0x0003)
#define RFCU_ENABLE                          (0x0008)
#define BLE_DIV                              (0x0030)
#define BLE_LP_RESET                         (0x0040)
#define BLE_ENABLE                           (0x0080)



/*=============================*/
struct __CLK_CTRL_REG
/*=============================*/
{
    WORD BITFLD_SYS_CLK_SEL                          : 2;
    WORD BITFLD_XTAL16M_DISABLE                      : 1;
    WORD BITFLD_DIV2_RFCLK                           : 1;
    WORD BITFLD_DIV2_XTAL16                          : 1;
    WORD BITFLD_RUNNING_AT_32K                       : 1;
    WORD BITFLD_RUNNING_AT_RC16M                     : 1;
    WORD BITFLD_RUNNING_AT_XTAL16M                   : 1;
};
#define SYS_CLK_SEL                          (0x0003)
#define XTAL16M_DISABLE                      (0x0004)
#define DIV2_RFCLK                           (0x0008)
#define DIV2_XTAL16                          (0x0010)
#define RUNNING_AT_32K                       (0x0020)
#define RUNNING_AT_RC16M                     (0x0040)
#define RUNNING_AT_XTAL16M                   (0x0080)



/*=============================*/
struct __PMU_CTRL_REG
/*=============================*/
{
    WORD BITFLD_RESET_ON_WAKEUP                      : 1;
    WORD BITFLD_PERIPH_SLEEP                         : 1;
    WORD BITFLD_RADIO_SLEEP                          : 1;
    WORD BITFLD_OTP_COPY_DIV                         : 2;
    WORD BITFLD_FORCE_BUCK                           : 1;
    WORD BITFLD_FORCE_BOOST                          : 1;
    WORD BITFLD_RETENTION_MODE                       : 4;
};
#define RESET_ON_WAKEUP                      (0x0001)
#define PERIPH_SLEEP                         (0x0002)
#define RADIO_SLEEP                          (0x0004)
#define OTP_COPY_DIV                         (0x0030)
#define FORCE_BUCK                           (0x0040)
#define FORCE_BOOST                          (0x0080)
#define RETENTION_MODE                       (0x0F00)



/*=============================*/
struct __SYS_CTRL_REG
/*=============================*/
{
    WORD BITFLD_REMAP_ADR0                           : 2;
    WORD BITFLD_DEV_PHASE                            : 1;
    WORD BITFLD_CLK32_SOURCE                         : 1;
    WORD BITFLD_OTP_COPY                             : 1;
    WORD BITFLD_PAD_LATCH_EN                         : 1;
    WORD BITFLD_OTPC_RESET_REQ                       : 1;
    WORD BITFLD_DEBUGGER_ENABLE                      : 1;
    WORD                                             : 1;
    WORD BITFLD_TIMEOUT_DISABLE                      : 1;
    WORD BITFLD_SW_RESET                             : 1;
};
#define REMAP_ADR0                           (0x0003)
#define DEV_PHASE                            (0x0004)
#define CLK32_SOURCE                         (0x0008)
#define OTP_COPY                             (0x0010)
#define PAD_LATCH_EN                         (0x0020)
#define OTPC_RESET_REQ                       (0x0040)
#define DEBUGGER_ENABLE                      (0x0080)
#define TIMEOUT_DISABLE                      (0x0200)
#define SW_RESET                             (0x8000)



/*=============================*/
struct __SYS_STAT_REG
/*=============================*/
{
    WORD BITFLD_RAD_IS_DOWN                          : 1;
    WORD BITFLD_RAD_IS_UP                            : 1;
    WORD BITFLD_PER_IS_DOWN                          : 1;
    WORD BITFLD_PER_IS_UP                            : 1;
    WORD BITFLD_DBG_IS_DOWN                          : 1;
    WORD BITFLD_DBG_IS_UP                            : 1;
    WORD BITFLD_XTAL16_TRIM_READY                    : 1;
    WORD BITFLD_XTAL16_SETTLED                       : 1;
};
#define RAD_IS_DOWN                          (0x0001)
#define RAD_IS_UP                            (0x0002)
#define PER_IS_DOWN                          (0x0004)
#define PER_IS_UP                            (0x0008)
#define DBG_IS_DOWN                          (0x0010)
#define DBG_IS_UP                            (0x0020)
#define XTAL16_TRIM_READY                    (0x0040)
#define XTAL16_SETTLED                       (0x0080)



/*=============================*/
struct __TRIM_CTRL_REG
/*=============================*/
{
    WORD BITFLD_SETTLE_TIME                          : 4;
    WORD BITFLD_TRIM_TIME                            : 4;
};
#define SETTLE_TIME                          (0x000F)
#define TRIM_TIME                            (0x00F0)



/*=============================*/
struct __CLK_32K_REG
/*=============================*/
{
    WORD BITFLD_XTAL32K_ENABLE                       : 1;
    WORD BITFLD_XTAL32K_RBIAS                        : 2;
    WORD BITFLD_XTAL32K_CUR                          : 4;
    WORD BITFLD_RC32K_ENABLE                         : 1;
    WORD BITFLD_RC32K_TRIM                           : 4;
    WORD BITFLD_XTAL32K_DISABLE_AMPREG               : 1;
};
#define XTAL32K_ENABLE                       (0x0001)
#define XTAL32K_RBIAS                        (0x0006)
#define XTAL32K_CUR                          (0x0078)
#define RC32K_ENABLE                         (0x0080)
#define RC32K_TRIM                           (0x0F00)
#define XTAL32K_DISABLE_AMPREG               (0x1000)



/*=============================*/
struct __CLK_16M_REG
/*=============================*/
{
    WORD BITFLD_RC16M_ENABLE                         : 1;
    WORD BITFLD_RC16M_TRIM                           : 4;
    WORD BITFLD_XTAL16_CUR_SET                       : 3;
};
#define RC16M_ENABLE                         (0x0001)
#define RC16M_TRIM                           (0x001E)
#define XTAL16_CUR_SET                       (0x00E0)



/*=============================*/
struct __CLK_RCX32K_REG
/*=============================*/
{
    WORD BITFLD_RCX32K_TRIM                          : 4;
    WORD BITFLD_RCX32K_NTC                           : 4;
    WORD BITFLD_RCX32K_BIAS                          : 2;
    WORD BITFLD_RCX32K_LOWF                          : 1;
    WORD BITFLD_RCX32K_ENABLE                        : 1;
    WORD BITFLD_RCX32K_SELECT                        : 1;
};
#define RCX32K_TRIM                          (0x000F)
#define RCX32K_NTC                           (0x00F0)
#define RCX32K_BIAS                          (0x0300)
#define RCX32K_LOWF                          (0x0400)
#define RCX32K_ENABLE                        (0x0800)
#define RCX32K_SELECT                        (0x1000)



/*=============================*/
struct __BANDGAP_REG
/*=============================*/
{
    WORD BITFLD_BGR_TRIM                             : 5;
    WORD BITFLD_BGR_ITRIM                            : 5;
    WORD BITFLD_LDO_RET_TRIM                         : 4;
    WORD BITFLD_BGR_LOWPOWER                         : 1;
};
#define BGR_TRIM                             (0x001F)
#define BGR_ITRIM                            (0x03E0)
#define LDO_RET_TRIM                         (0x3C00)
#define BGR_LOWPOWER                         (0x4000)



/*=============================*/
struct __ANA_STATUS_REG
/*=============================*/
{
    WORD BITFLD_VBAT1V_AVAILABLE                     : 1;
    WORD BITFLD_VBAT1V_OK                            : 1;
    WORD BITFLD_VDCDC_OK                             : 1;
    WORD BITFLD_LDO_OTP_OK                           : 1;
    WORD BITFLD_LDO_VDD_OK                           : 1;
    WORD BITFLD_LDO_ANA_OK                           : 1;
    WORD BITFLD_BOOST_VBAT_OK                        : 1;
    WORD BITFLD_BANDGAP_OK                           : 1;
    WORD                                             : 1;
    WORD BITFLD_BOOST_SELECTED                       : 1;
};
#define VBAT1V_AVAILABLE                     (0x0001)
#define VBAT1V_OK                            (0x0002)
#define VDCDC_OK                             (0x0004)
#define LDO_OTP_OK                           (0x0008)
#define LDO_VDD_OK                           (0x0010)
#define LDO_ANA_OK                           (0x0020)
#define BOOST_VBAT_OK                        (0x0040)
#define BANDGAP_OK                           (0x0080)
#define BOOST_SELECTED                       (0x0200)



/*===========================================================================*/
/* memory map GPADC */
/*===========================================================================*/
#define GP_ADC_CTRL_REG                      (0x50001500) /* General Purpose ADC Control Register */
#define GP_ADC_CTRL2_REG                     (0x50001502) /* General Purpose ADC Second Control Register */
#define GP_ADC_OFFP_REG                      (0x50001504) /* General Purpose ADC Positive Offset Register */
#define GP_ADC_OFFN_REG                      (0x50001506) /* General Purpose ADC Negative Offset Register */
#define GP_ADC_CLEAR_INT_REG                 (0x50001508) /* General Purpose ADC Clear Interrupt Register */
#define GP_ADC_RESULT_REG                    (0x5000150A) /* General Purpose ADC Result Register */
#define GP_ADC_DELAY_REG                     (0x5000150C) /* General Purpose ADC Delay Register */
#define GP_ADC_DELAY2_REG                    (0x5000150E) /* General Purpose ADC Second Delay Register */



/*=============================*/
struct __GP_ADC_CTRL_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_EN                            : 1;
    WORD BITFLD_GP_ADC_START                         : 1;
    WORD BITFLD_GP_ADC_AUTO                          : 1;
    WORD BITFLD_GP_ADC_CLK_SEL                       : 1;
    WORD BITFLD_GP_ADC_INT                           : 1;
    WORD BITFLD_GP_ADC_MINT                          : 1;
    WORD BITFLD_GP_ADC_SEL                           : 4;
    WORD BITFLD_GP_ADC_SIGN                          : 1;
    WORD BITFLD_GP_ADC_SE                            : 1;
    WORD BITFLD_GP_ADC_MUTE                          : 1;
    WORD BITFLD_GP_ADC_CHOP                          : 1;
    WORD BITFLD_GP_ADC_LDO_EN                        : 1;
    WORD BITFLD_GP_ADC_LDO_ZERO                      : 1;
};
#define GP_ADC_EN                            (0x0001)
#define GP_ADC_START                         (0x0002)
#define GP_ADC_AUTO                          (0x0004)
#define GP_ADC_CLK_SEL                       (0x0008)
#define GP_ADC_INT                           (0x0010)
#define GP_ADC_MINT                          (0x0020)
#define GP_ADC_SEL                           (0x03C0)
#define GP_ADC_SIGN                          (0x0400)
#define GP_ADC_SE                            (0x0800)
#define GP_ADC_MUTE                          (0x1000)
#define GP_ADC_CHOP                          (0x2000)
#define GP_ADC_LDO_EN                        (0x4000)
#define GP_ADC_LDO_ZERO                      (0x8000)



/*=============================*/
struct __GP_ADC_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_DELAY_EN                      : 1;
    WORD BITFLD_GP_ADC_ATTN3X                        : 1;
    WORD BITFLD_GP_ADC_IDYN                          : 1;
    WORD BITFLD_GP_ADC_I20U                          : 1;
};
#define GP_ADC_DELAY_EN                      (0x0001)
#define GP_ADC_ATTN3X                        (0x0002)
#define GP_ADC_IDYN                          (0x0004)
#define GP_ADC_I20U                          (0x0008)



/*=============================*/
struct __GP_ADC_OFFP_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_OFFP                          : 10;
};
#define GP_ADC_OFFP                          (0x03FF)



/*=============================*/
struct __GP_ADC_OFFN_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_OFFN                          : 10;
};
#define GP_ADC_OFFN                          (0x03FF)



/*=============================*/
struct __GP_ADC_CLEAR_INT_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_CLR_INT                       : 16;
};
#define GP_ADC_CLR_INT                       (0xFFFF)



/*=============================*/
struct __GP_ADC_RESULT_REG
/*=============================*/
{
    WORD BITFLD_GP_ADC_VAL                           : 10;
};
#define GP_ADC_VAL                           (0x03FF)



/*=============================*/
struct __GP_ADC_DELAY_REG
/*=============================*/
{
    WORD BITFLD_DEL_LDO_EN                           : 8;
    WORD BITFLD_DEL_LDO_ZERO                         : 8;
};
#define DEL_LDO_EN                           (0x00FF)
#define DEL_LDO_ZERO                         (0xFF00)



/*=============================*/
struct __GP_ADC_DELAY2_REG
/*=============================*/
{
    WORD BITFLD_DEL_ADC_EN                           : 8;
    WORD BITFLD_DEL_ADC_START                        : 8;
};
#define DEL_ADC_EN                           (0x00FF)
#define DEL_ADC_START                        (0xFF00)



/*===========================================================================*/
/* memory map GPIO */
/*===========================================================================*/
#define P0_DATA_REG                          (0x50003000) /* P0 Data input / output register */
#define P0_SET_DATA_REG                      (0x50003002) /* P0 Set port pins register */
#define P0_RESET_DATA_REG                    (0x50003004) /* P0 Reset port pins register */
#define P00_MODE_REG                         (0x50003006) /* P00 Mode Register */
#define P01_MODE_REG                         (0x50003008) /* P01 Mode Register */
#define P02_MODE_REG                         (0x5000300A) /* P02 Mode Register */
#define P03_MODE_REG                         (0x5000300C) /* P03 Mode Register */
#define P04_MODE_REG                         (0x5000300E) /* P04 Mode Register */
#define P05_MODE_REG                         (0x50003010) /* P05 Mode Register */
#define P06_MODE_REG                         (0x50003012) /* P06 Mode Register */
#define P07_MODE_REG                         (0x50003014) /* P07 Mode Register */
#define P1_DATA_REG                          (0x50003020) /* P1 Data input / output register */
#define P1_SET_DATA_REG                      (0x50003022) /* P1 Set port pins register */
#define P1_RESET_DATA_REG                    (0x50003024) /* P1 Reset port pins register */
#define P10_MODE_REG                         (0x50003026) /* P10 Mode Register */
#define P11_MODE_REG                         (0x50003028) /* P11 Mode Register */
#define P12_MODE_REG                         (0x5000302A) /* P12 Mode Register */
#define P13_MODE_REG                         (0x5000302C) /* P13 Mode Register */
#define P14_MODE_REG                         (0x5000302E) /* P14 Mode Register */
#define P15_MODE_REG                         (0x50003030) /* P15 Mode Register */
#define P2_DATA_REG                          (0x50003040) /* P2 Data input / output register */
#define P2_SET_DATA_REG                      (0x50003042) /* P2 Set port pins register */
#define P2_RESET_DATA_REG                    (0x50003044) /* P2 Reset port pins register */
#define P20_MODE_REG                         (0x50003046) /* P20 Mode Register */
#define P21_MODE_REG                         (0x50003048) /* P21 Mode Register */
#define P22_MODE_REG                         (0x5000304A) /* P22 Mode Register */
#define P23_MODE_REG                         (0x5000304C) /* P23 Mode Register */
#define P24_MODE_REG                         (0x5000304E) /* P24 Mode Register */
#define P25_MODE_REG                         (0x50003050) /* P25 Mode Register */
#define P26_MODE_REG                         (0x50003052) /* P26 Mode Register */
#define P27_MODE_REG                         (0x50003054) /* P27 Mode Register */
#define P28_MODE_REG                         (0x50003056) /* P28 Mode Register */
#define P29_MODE_REG                         (0x50003058) /* P29 Mode Register */
#define P01_PADPWR_CTRL_REG                  (0x50003070) /* Ports 0 and 1 Output Power Control Register */
#define P2_PADPWR_CTRL_REG                   (0x50003072) /* Port 2 Output Power Control Register */
#define P3_PADPWR_CTRL_REG                   (0x50003074) /* Port 3 Output Power Control Register */
#define P3_DATA_REG                          (0x50003080) /* P3 Data input / output register */
#define P3_SET_DATA_REG                      (0x50003082) /* P3 Set port pins register */
#define P3_RESET_DATA_REG                    (0x50003084) /* P3 Reset port pins register */
#define P30_MODE_REG                         (0x50003086) /* P30 Mode Register */
#define P31_MODE_REG                         (0x50003088) /* P31 Mode Register */
#define P32_MODE_REG                         (0x5000308A) /* P32 Mode Register */
#define P33_MODE_REG                         (0x5000308C) /* P33 Mode Register */
#define P34_MODE_REG                         (0x5000308E) /* P34 Mode Register */
#define P35_MODE_REG                         (0x50003090) /* P35 Mode Register */
#define P36_MODE_REG                         (0x50003092) /* P36 Mode Register */
#define P37_MODE_REG                         (0x50003094) /* P37 Mode Register */



/*=============================*/
struct __P0_DATA_REG
/*=============================*/
{
    WORD BITFLD_P0_DATA                              : 8;
};
#define P0_DATA                              (0x00FF)



/*=============================*/
struct __P0_SET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P0_SET                               : 8;
};
#define P0_SET                               (0x00FF)



/*=============================*/
struct __P0_RESET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P0_RESET                             : 8;
};
#define P0_RESET                             (0x00FF)



/*=============================*/
struct __P00_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P01_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P02_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P03_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P04_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P05_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P06_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P07_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P1_DATA_REG
/*=============================*/
{
    WORD BITFLD_P1_DATA                              : 8;
};
#define P1_DATA                              (0x00FF)



/*=============================*/
struct __P1_SET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P1_SET                               : 8;
};
#define P1_SET                               (0x00FF)



/*=============================*/
struct __P1_RESET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P1_RESET                             : 8;
};
#define P1_RESET                             (0x00FF)



/*=============================*/
struct __P10_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P11_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P12_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P13_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P14_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P15_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P2_DATA_REG
/*=============================*/
{
    WORD BITFLD_P2_DATA                              : 10;
};
#define P2_DATA                              (0x03FF)



/*=============================*/
struct __P2_SET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P2_SET                               : 10;
};
#define P2_SET                               (0x03FF)



/*=============================*/
struct __P2_RESET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P2_RESET                             : 10;
};
#define P2_RESET                             (0x03FF)



/*=============================*/
struct __P20_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P21_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P22_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P23_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P24_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P25_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P26_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P27_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P28_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P29_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P01_PADPWR_CTRL_REG
/*=============================*/
{
    WORD BITFLD_P0_OUT_CTRL                          : 8;
    WORD BITFLD_P1_OUT_CTRL                          : 6;
};
#define P0_OUT_CTRL                          (0x00FF)
#define P1_OUT_CTRL                          (0x3F00)



/*=============================*/
struct __P2_PADPWR_CTRL_REG
/*=============================*/
{
    WORD BITFLD_P2_OUT_CTRL                          : 10;
};
#define P2_OUT_CTRL                          (0x03FF)



/*=============================*/
struct __P3_PADPWR_CTRL_REG
/*=============================*/
{
    WORD BITFLD_P3_OUT_CTRL                          : 8;
};
#define P3_OUT_CTRL                          (0x00FF)



/*=============================*/
struct __P3_DATA_REG
/*=============================*/
{
    WORD BITFLD_P3_DATA                              : 8;
};
#define P3_DATA                              (0x00FF)



/*=============================*/
struct __P3_SET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P3_SET                               : 8;
};
#define P3_SET                               (0x00FF)



/*=============================*/
struct __P3_RESET_DATA_REG
/*=============================*/
{
    WORD BITFLD_P3_RESET                             : 8;
};
#define P3_RESET                             (0x00FF)



/*=============================*/
struct __P30_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P31_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P32_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P33_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P34_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P35_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P36_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*=============================*/
struct __P37_MODE_REG
/*=============================*/
{
    WORD BITFLD_PID                                  : 5;
    WORD                                             : 3;
    WORD BITFLD_PUPD                                 : 2;
};
#define PID                                  (0x001F)
#define PUPD                                 (0x0300)



/*===========================================================================*/
/* memory map GPREG */
/*===========================================================================*/
#define SET_FREEZE_REG                       (0x50003300) /* Controls freezing of various timers/counters. */
#define RESET_FREEZE_REG                     (0x50003302) /* Controls unfreezing of various timers/counters. */
#define DEBUG_REG                            (0x50003304) /* Various debug information register. */
#define GP_STATUS_REG                        (0x50003306) /* General purpose system status register. */
#define GP_CONTROL_REG                       (0x50003308) /* General purpose system control register. */



/*=============================*/
struct __SET_FREEZE_REG
/*=============================*/
{
    WORD BITFLD_FRZ_WKUPTIM                          : 1;
    WORD BITFLD_FRZ_SWTIM                            : 1;
    WORD BITFLD_FRZ_BLETIM                           : 1;
    WORD BITFLD_FRZ_WDOG                             : 1;
};
#define FRZ_WKUPTIM                          (0x0001)
#define FRZ_SWTIM                            (0x0002)
#define FRZ_BLETIM                           (0x0004)
#define FRZ_WDOG                             (0x0008)



/*=============================*/
struct __RESET_FREEZE_REG
/*=============================*/
{
    WORD BITFLD_FRZ_CPTIM                            : 1;
    WORD BITFLD_FRZ_SWTIM                            : 1;
    WORD BITFLD_FRZ_BLETIM                           : 1;
    WORD BITFLD_FRZ_WDOG                             : 1;
};
#define FRZ_CPTIM                            (0x0001)
#define FRZ_SWTIM                            (0x0002)
#define FRZ_BLETIM                           (0x0004)
#define FRZ_WDOG                             (0x0008)



/*=============================*/
struct __DEBUG_REG
/*=============================*/
{
    WORD BITFLD_DEBUGS_FREEZE_EN                     : 1;
};
#define DEBUGS_FREEZE_EN                     (0x0001)



/*=============================*/
struct __GP_STATUS_REG
/*=============================*/
{
    WORD BITFLD_CAL_PHASE                            : 1;
};
#define CAL_PHASE                            (0x0001)



/*=============================*/
struct __GP_CONTROL_REG
/*=============================*/
{
    WORD BITFLD_BLE_WAKEUP_REQ                       : 1;
    WORD BITFLD_EM_MAP                               : 5;
};
#define BLE_WAKEUP_REQ                       (0x0001)
#define EM_MAP                               (0x003E)



/*===========================================================================*/
/* memory map I2C */
/*===========================================================================*/
#define I2C_CON_REG                          (0x50001300) /* I2C Control Register */
#define I2C_TAR_REG                          (0x50001304) /* I2C Target Address Register */
#define I2C_SAR_REG                          (0x50001308) /* I2C Slave Address Register */
#define I2C_DATA_CMD_REG                     (0x50001310) /* I2C Rx/Tx Data Buffer and Command Register */
#define I2C_SS_SCL_HCNT_REG                  (0x50001314) /* Standard Speed I2C Clock SCL High Count Register */
#define I2C_SS_SCL_LCNT_REG                  (0x50001318) /* Standard Speed I2C Clock SCL Low Count Register */
#define I2C_FS_SCL_HCNT_REG                  (0x5000131C) /* Fast Speed I2C Clock SCL High Count Register */
#define I2C_FS_SCL_LCNT_REG                  (0x50001320) /* Fast Speed I2C Clock SCL Low Count Register */
#define I2C_INTR_STAT_REG                    (0x5000132C) /* I2C Interrupt Status Register */
#define I2C_INTR_MASK_REG                    (0x50001330) /* I2C Interrupt Mask Register */
#define I2C_RAW_INTR_STAT_REG                (0x50001334) /* I2C Raw Interrupt Status Register */
#define I2C_RX_TL_REG                        (0x50001338) /* I2C Receive FIFO Threshold Register */
#define I2C_TX_TL_REG                        (0x5000133C) /* I2C Transmit FIFO Threshold Register */
#define I2C_CLR_INTR_REG                     (0x50001340) /* Clear Combined and Individual Interrupt Register */
#define I2C_CLR_RX_UNDER_REG                 (0x50001344) /* Clear RX_UNDER Interrupt Register */
#define I2C_CLR_RX_OVER_REG                  (0x50001348) /* Clear RX_OVER Interrupt Register */
#define I2C_CLR_TX_OVER_REG                  (0x5000134C) /* Clear TX_OVER Interrupt Register */
#define I2C_CLR_RD_REQ_REG                   (0x50001350) /* Clear RD_REQ Interrupt Register */
#define I2C_CLR_TX_ABRT_REG                  (0x50001354) /* Clear TX_ABRT Interrupt Register */
#define I2C_CLR_RX_DONE_REG                  (0x50001358) /* Clear RX_DONE Interrupt Register */
#define I2C_CLR_ACTIVITY_REG                 (0x5000135C) /* Clear ACTIVITY Interrupt Register */
#define I2C_CLR_STOP_DET_REG                 (0x50001360) /* Clear STOP_DET Interrupt Register */
#define I2C_CLR_START_DET_REG                (0x50001364) /* Clear START_DET Interrupt Register */
#define I2C_CLR_GEN_CALL_REG                 (0x50001368) /* Clear GEN_CALL Interrupt Register */
#define I2C_ENABLE_REG                       (0x5000136C) /* I2C Enable Register */
#define I2C_STATUS_REG                       (0x50001370) /* I2C Status Register */
#define I2C_TXFLR_REG                        (0x50001374) /* I2C Transmit FIFO Level Register */
#define I2C_RXFLR_REG                        (0x50001378) /* I2C Receive FIFO Level Register */
#define I2C_SDA_HOLD_REG                     (0x5000137C) /* I2C SDA Hold Time Length Register */
#define I2C_TX_ABRT_SOURCE_REG               (0x50001380) /* I2C Transmit Abort Source Register */
#define I2C_SDA_SETUP_REG                    (0x50001394) /* I2C SDA Setup Register */
#define I2C_ACK_GENERAL_CALL_REG             (0x50001398) /* I2C ACK General Call Register */
#define I2C_ENABLE_STATUS_REG                (0x5000139C) /* I2C Enable Status Register */
#define I2C_IC_FS_SPKLEN_REG                 (0x500013A0) /* I2C SS and FS spike suppression limit Size */



/*=============================*/
struct __I2C_CON_REG
/*=============================*/
{
    WORD BITFLD_I2C_MASTER_MODE                      : 1;
    WORD BITFLD_I2C_SPEED                            : 2;
    WORD BITFLD_I2C_10BITADDR_SLAVE                  : 1;
    WORD BITFLD_I2C_10BITADDR_MASTER                 : 1;
    WORD BITFLD_I2C_RESTART_EN                       : 1;
    WORD BITFLD_I2C_SLAVE_DISABLE                    : 1;
};
#define I2C_MASTER_MODE                      (0x0001)
#define I2C_SPEED                            (0x0006)
#define I2C_10BITADDR_SLAVE                  (0x0008)
#define I2C_10BITADDR_MASTER                 (0x0010)
#define I2C_RESTART_EN                       (0x0020)
#define I2C_SLAVE_DISABLE                    (0x0040)



/*=============================*/
struct __I2C_TAR_REG
/*=============================*/
{
    WORD BITFLD_IC_TAR                               : 10;
    WORD BITFLD_GC_OR_START                          : 1;
    WORD BITFLD_SPECIAL                              : 1;
};
#define IC_TAR                               (0x03FF)
#define GC_OR_START                          (0x0400)
#define SPECIAL                              (0x0800)



/*=============================*/
struct __I2C_SAR_REG
/*=============================*/
{
    WORD BITFLD_IC_SAR                               : 10;
};
#define IC_SAR                               (0x03FF)



/*=============================*/
struct __I2C_DATA_CMD_REG
/*=============================*/
{
    WORD BITFLD_DAT                                  : 8;
    WORD BITFLD_CMD                                  : 1;
};
#define DAT                                  (0x00FF)
#define CMD                                  (0x0100)



/*=============================*/
struct __I2C_SS_SCL_HCNT_REG
/*=============================*/
{
    WORD BITFLD_IC_SS_SCL_HCNT                       : 16;
};
#define IC_SS_SCL_HCNT                       (0xFFFF)



/*=============================*/
struct __I2C_SS_SCL_LCNT_REG
/*=============================*/
{
    WORD BITFLD_IC_SS_SCL_LCNT                       : 16;
};
#define IC_SS_SCL_LCNT                       (0xFFFF)



/*=============================*/
struct __I2C_FS_SCL_HCNT_REG
/*=============================*/
{
    WORD BITFLD_IC_FS_SCL_HCNT                       : 16;
};
#define IC_FS_SCL_HCNT                       (0xFFFF)



/*=============================*/
struct __I2C_FS_SCL_LCNT_REG
/*=============================*/
{
    WORD BITFLD_IC_FS_SCL_LCNT                       : 16;
};
#define IC_FS_SCL_LCNT                       (0xFFFF)



/*=============================*/
struct __I2C_INTR_STAT_REG
/*=============================*/
{
    WORD BITFLD_R_RX_UNDER                           : 1;
    WORD BITFLD_R_RX_OVER                            : 1;
    WORD BITFLD_R_RX_FULL                            : 1;
    WORD BITFLD_R_TX_OVER                            : 1;
    WORD BITFLD_R_TX_EMPTY                           : 1;
    WORD BITFLD_R_RD_REQ                             : 1;
    WORD BITFLD_R_TX_ABRT                            : 1;
    WORD BITFLD_R_RX_DONE                            : 1;
    WORD BITFLD_R_ACTIVITY                           : 1;
    WORD BITFLD_R_STOP_DET                           : 1;
    WORD BITFLD_R_START_DET                          : 1;
    WORD BITFLD_R_GEN_CALL                           : 1;
};
#define R_RX_UNDER                           (0x0001)
#define R_RX_OVER                            (0x0002)
#define R_RX_FULL                            (0x0004)
#define R_TX_OVER                            (0x0008)
#define R_TX_EMPTY                           (0x0010)
#define R_RD_REQ                             (0x0020)
#define R_TX_ABRT                            (0x0040)
#define R_RX_DONE                            (0x0080)
#define R_ACTIVITY                           (0x0100)
#define R_STOP_DET                           (0x0200)
#define R_START_DET                          (0x0400)
#define R_GEN_CALL                           (0x0800)



/*=============================*/
struct __I2C_INTR_MASK_REG
/*=============================*/
{
    WORD BITFLD_M_RX_UNDER                           : 1;
    WORD BITFLD_M_RX_OVER                            : 1;
    WORD BITFLD_M_RX_FULL                            : 1;
    WORD BITFLD_M_TX_OVER                            : 1;
    WORD BITFLD_M_TX_EMPTY                           : 1;
    WORD BITFLD_M_RD_REQ                             : 1;
    WORD BITFLD_M_TX_ABRT                            : 1;
    WORD BITFLD_M_RX_DONE                            : 1;
    WORD BITFLD_M_ACTIVITY                           : 1;
    WORD BITFLD_M_STOP_DET                           : 1;
    WORD BITFLD_M_START_DET                          : 1;
    WORD BITFLD_M_GEN_CALL                           : 1;
};
#define M_RX_UNDER                           (0x0001)
#define M_RX_OVER                            (0x0002)
#define M_RX_FULL                            (0x0004)
#define M_TX_OVER                            (0x0008)
#define M_TX_EMPTY                           (0x0010)
#define M_RD_REQ                             (0x0020)
#define M_TX_ABRT                            (0x0040)
#define M_RX_DONE                            (0x0080)
#define M_ACTIVITY                           (0x0100)
#define M_STOP_DET                           (0x0200)
#define M_START_DET                          (0x0400)
#define M_GEN_CALL                           (0x0800)



/*=============================*/
struct __I2C_RAW_INTR_STAT_REG
/*=============================*/
{
    WORD BITFLD_RX_UNDER                             : 1;
    WORD BITFLD_RX_OVER                              : 1;
    WORD BITFLD_RX_FULL                              : 1;
    WORD BITFLD_TX_OVER                              : 1;
    WORD BITFLD_TX_EMPTY                             : 1;
    WORD BITFLD_RD_REQ                               : 1;
    WORD BITFLD_TX_ABRT                              : 1;
    WORD BITFLD_RX_DONE                              : 1;
    WORD BITFLD_ACTIVITY                             : 1;
    WORD BITFLD_STOP_DET                             : 1;
    WORD BITFLD_START_DET                            : 1;
    WORD BITFLD_GEN_CALL                             : 1;
};
#define RX_UNDER                             (0x0001)
#define RX_OVER                              (0x0002)
#define RX_FULL                              (0x0004)
#define TX_OVER                              (0x0008)
#define TX_EMPTY                             (0x0010)
#define RD_REQ                               (0x0020)
#define TX_ABRT                              (0x0040)
#define RX_DONE                              (0x0080)
#define ACTIVITY                             (0x0100)
#define STOP_DET                             (0x0200)
#define START_DET                            (0x0400)
#define GEN_CALL                             (0x0800)



/*=============================*/
struct __I2C_RX_TL_REG
/*=============================*/
{
    WORD BITFLD_RX_TL                                : 5;
};
#define RX_TL                                (0x001F)



/*=============================*/
struct __I2C_TX_TL_REG
/*=============================*/
{
    WORD BITFLD_RX_TL                                : 5;
};
#define RX_TL                                (0x001F)



/*=============================*/
struct __I2C_CLR_INTR_REG
/*=============================*/
{
    WORD BITFLD_CLR_INTR                             : 1;
};
#define CLR_INTR                             (0x0001)



/*=============================*/
struct __I2C_CLR_RX_UNDER_REG
/*=============================*/
{
    WORD BITFLD_CLR_RX_UNDER                         : 1;
};
#define CLR_RX_UNDER                         (0x0001)



/*=============================*/
struct __I2C_CLR_RX_OVER_REG
/*=============================*/
{
    WORD BITFLD_CLR_RX_OVER                          : 1;
};
#define CLR_RX_OVER                          (0x0001)



/*=============================*/
struct __I2C_CLR_TX_OVER_REG
/*=============================*/
{
    WORD BITFLD_CLR_TX_OVER                          : 1;
};
#define CLR_TX_OVER                          (0x0001)



/*=============================*/
struct __I2C_CLR_RD_REQ_REG
/*=============================*/
{
    WORD BITFLD_CLR_RD_REQ                           : 1;
};
#define CLR_RD_REQ                           (0x0001)



/*=============================*/
struct __I2C_CLR_TX_ABRT_REG
/*=============================*/
{
    WORD BITFLD_CLR_TX_ABRT                          : 1;
};
#define CLR_TX_ABRT                          (0x0001)



/*=============================*/
struct __I2C_CLR_RX_DONE_REG
/*=============================*/
{
    WORD BITFLD_CLR_RX_DONE                          : 1;
};
#define CLR_RX_DONE                          (0x0001)



/*=============================*/
struct __I2C_CLR_ACTIVITY_REG
/*=============================*/
{
    WORD BITFLD_CLR_ACTIVITY                         : 1;
};
#define CLR_ACTIVITY                         (0x0001)



/*=============================*/
struct __I2C_CLR_STOP_DET_REG
/*=============================*/
{
    WORD BITFLD_CLR_ACTIVITY                         : 1;
};
#define CLR_ACTIVITY                         (0x0001)



/*=============================*/
struct __I2C_CLR_START_DET_REG
/*=============================*/
{
    WORD BITFLD_CLR_START_DET                        : 1;
};
#define CLR_START_DET                        (0x0001)



/*=============================*/
struct __I2C_CLR_GEN_CALL_REG
/*=============================*/
{
    WORD BITFLD_CLR_GEN_CALL                         : 1;
};
#define CLR_GEN_CALL                         (0x0001)



/*=============================*/
struct __I2C_ENABLE_REG
/*=============================*/
{
    WORD BITFLD_CTRL_ENABLE                          : 1;
};
#define CTRL_ENABLE                          (0x0001)



/*=============================*/
struct __I2C_STATUS_REG
/*=============================*/
{
    WORD BITFLD_I2C_ACTIVITY                         : 1;
    WORD BITFLD_TFNF                                 : 1;
    WORD BITFLD_TFE                                  : 1;
    WORD BITFLD_RFNE                                 : 1;
    WORD BITFLD_RFF                                  : 1;
    WORD BITFLD_MST_ACTIVITY                         : 1;
    WORD BITFLD_SLV_ACTIVITY                         : 1;
};
#define I2C_ACTIVITY                         (0x0001)
#define TFNF                                 (0x0002)
#define TFE                                  (0x0004)
#define RFNE                                 (0x0008)
#define RFF                                  (0x0010)
#define MST_ACTIVITY                         (0x0020)
#define SLV_ACTIVITY                         (0x0040)



/*=============================*/
struct __I2C_TXFLR_REG
/*=============================*/
{
    WORD BITFLD_TXFLR                                : 6;
};
#define TXFLR                                (0x003F)



/*=============================*/
struct __I2C_RXFLR_REG
/*=============================*/
{
    WORD BITFLD_RXFLR                                : 6;
};
#define RXFLR                                (0x003F)



/*=============================*/
struct __I2C_SDA_HOLD_REG
/*=============================*/
{
    WORD BITFLD_IC_SDA_HOLD                          : 16;
};
#define IC_SDA_HOLD                          (0xFFFF)



/*=============================*/
struct __I2C_TX_ABRT_SOURCE_REG
/*=============================*/
{
    WORD BITFLD_ABRT_7B_ADDR_NOACK                   : 1;
    WORD BITFLD_ABRT_10ADDR1_NOACK                   : 1;
    WORD BITFLD_ABRT_10ADDR2_NOACK                   : 1;
    WORD BITFLD_ABRT_TXDATA_NOACK                    : 1;
    WORD BITFLD_ABRT_GCALL_NOACK                     : 1;
    WORD BITFLD_ABRT_GCALL_READ                      : 1;
    WORD BITFLD_ABRT_HS_ACKDET                       : 1;
    WORD BITFLD_ABRT_SBYTE_ACKDET                    : 1;
    WORD BITFLD_ABRT_HS_NORSTRT                      : 1;
    WORD BITFLD_ABRT_SBYTE_NORSTRT                   : 1;
    WORD BITFLD_ABRT_10B_RD_NORSTRT                  : 1;
    WORD BITFLD_ABRT_MASTER_DIS                      : 1;
    WORD BITFLD_ARB_LOST                             : 1;
    WORD BITFLD_ABRT_SLVFLUSH_TXFIFO                 : 1;
    WORD BITFLD_ABRT_SLV_ARBLOST                     : 1;
    WORD BITFLD_ABRT_SLVRD_INTX                      : 1;
};
#define ABRT_7B_ADDR_NOACK                   (0x0001)
#define ABRT_10ADDR1_NOACK                   (0x0002)
#define ABRT_10ADDR2_NOACK                   (0x0004)
#define ABRT_TXDATA_NOACK                    (0x0008)
#define ABRT_GCALL_NOACK                     (0x0010)
#define ABRT_GCALL_READ                      (0x0020)
#define ABRT_HS_ACKDET                       (0x0040)
#define ABRT_SBYTE_ACKDET                    (0x0080)
#define ABRT_HS_NORSTRT                      (0x0100)
#define ABRT_SBYTE_NORSTRT                   (0x0200)
#define ABRT_10B_RD_NORSTRT                  (0x0400)
#define ABRT_MASTER_DIS                      (0x0800)
#define ARB_LOST                             (0x1000)
#define ABRT_SLVFLUSH_TXFIFO                 (0x2000)
#define ABRT_SLV_ARBLOST                     (0x4000)
#define ABRT_SLVRD_INTX                      (0x8000)



/*=============================*/
struct __I2C_SDA_SETUP_REG
/*=============================*/
{
    WORD BITFLD_SDA_SETUP                            : 8;
};
#define SDA_SETUP                            (0x00FF)



/*=============================*/
struct __I2C_ACK_GENERAL_CALL_REG
/*=============================*/
{
    WORD BITFLD_ACK_GEN_CALL                         : 1;
};
#define ACK_GEN_CALL                         (0x0001)



/*=============================*/
struct __I2C_ENABLE_STATUS_REG
/*=============================*/
{
    WORD BITFLD_IC_EN                                : 1;
    WORD BITFLD_SLV_DISABLED_WHILE_BUSY              : 1;
    WORD BITFLD_SLV_RX_DATA_LOST                     : 1;
};
#define IC_EN                                (0x0001)
#define SLV_DISABLED_WHILE_BUSY              (0x0002)
#define SLV_RX_DATA_LOST                     (0x0004)



/*=============================*/
struct __I2C_IC_FS_SPKLEN_REG
/*=============================*/
{
    WORD BITFLD_IC_FS_SPKLEN                         : 8;
};
#define IC_FS_SPKLEN                         (0x00FF)



/*===========================================================================*/
/* memory map KBRD */
/*===========================================================================*/
#define GPIO_IRQ0_IN_SEL_REG                 (0x50001400) /* GPIO interrupt selection for GPIO_IRQ0 */
#define GPIO_IRQ1_IN_SEL_REG                 (0x50001402) /* GPIO interrupt selection for GPIO_IRQ1 */
#define GPIO_IRQ2_IN_SEL_REG                 (0x50001404) /* GPIO interrupt selection for GPIO_IRQ2 */
#define GPIO_IRQ3_IN_SEL_REG                 (0x50001406) /* GPIO interrupt selection for GPIO_IRQ3 */
#define GPIO_IRQ4_IN_SEL_REG                 (0x50001408) /* GPIO interrupt selection for GPIO_IRQ4 */
#define GPIO_DEBOUNCE_REG                    (0x5000140C) /* debounce counter value for GPIO inputs */
#define GPIO_RESET_IRQ_REG                   (0x5000140E) /* GPIO interrupt reset register */
#define GPIO_INT_LEVEL_CTRL_REG              (0x50001410) /* high or low level select for GPIO interrupts */
#define KBRD_IRQ_IN_SEL0_REG                 (0x50001412) /* GPIO interrupt selection for KBRD_IRQ for P0 */
#define KBRD_IRQ_IN_SEL1_REG                 (0x50001414) /* GPIO interrupt selection for KBRD_IRQ for P1 and P2 */



/*=============================*/
struct __GPIO_IRQ0_IN_SEL_REG
/*=============================*/
{
    WORD BITFLD_KBRD_IRQ0_SEL                        : 5;
};
#define KBRD_IRQ0_SEL                        (0x001F)



/*=============================*/
struct __GPIO_IRQ1_IN_SEL_REG
/*=============================*/
{
    WORD BITFLD_KBRD_IRQ1_SEL                        : 5;
};
#define KBRD_IRQ1_SEL                        (0x001F)



/*=============================*/
struct __GPIO_IRQ2_IN_SEL_REG
/*=============================*/
{
    WORD BITFLD_KBRD_IRQ2_SEL                        : 5;
};
#define KBRD_IRQ2_SEL                        (0x001F)



/*=============================*/
struct __GPIO_IRQ3_IN_SEL_REG
/*=============================*/
{
    WORD BITFLD_KBRD_IRQ3_SEL                        : 5;
};
#define KBRD_IRQ3_SEL                        (0x001F)



/*=============================*/
struct __GPIO_IRQ4_IN_SEL_REG
/*=============================*/
{
    WORD BITFLD_KBRD_IRQ4_SEL                        : 5;
};
#define KBRD_IRQ4_SEL                        (0x001F)



/*=============================*/
struct __GPIO_DEBOUNCE_REG
/*=============================*/
{
    WORD BITFLD_DEB_VALUE                            : 6;
    WORD                                             : 2;
    WORD BITFLD_DEB_ENABLE0                          : 1;
    WORD BITFLD_DEB_ENABLE1                          : 1;
    WORD BITFLD_DEB_ENABLE2                          : 1;
    WORD BITFLD_DEB_ENABLE3                          : 1;
    WORD BITFLD_DEB_ENABLE4                          : 1;
    WORD BITFLD_DEB_ENABLE_KBRD                      : 1;
};
#define DEB_VALUE                            (0x003F)
#define DEB_ENABLE0                          (0x0100)
#define DEB_ENABLE1                          (0x0200)
#define DEB_ENABLE2                          (0x0400)
#define DEB_ENABLE3                          (0x0800)
#define DEB_ENABLE4                          (0x1000)
#define DEB_ENABLE_KBRD                      (0x2000)



/*=============================*/
struct __GPIO_RESET_IRQ_REG
/*=============================*/
{
    WORD BITFLD_RESET_GPIO0_IRQ                      : 1;
    WORD BITFLD_RESET_GPIO1_IRQ                      : 1;
    WORD BITFLD_RESET_GPIO2_IRQ                      : 1;
    WORD BITFLD_RESET_GPIO3_IRQ                      : 1;
    WORD BITFLD_RESET_GPIO4_IRQ                      : 1;
    WORD BITFLD_RESET_KBRD_IRQ                       : 1;
};
#define RESET_GPIO0_IRQ                      (0x0001)
#define RESET_GPIO1_IRQ                      (0x0002)
#define RESET_GPIO2_IRQ                      (0x0004)
#define RESET_GPIO3_IRQ                      (0x0008)
#define RESET_GPIO4_IRQ                      (0x0010)
#define RESET_KBRD_IRQ                       (0x0020)



/*=============================*/
struct __GPIO_INT_LEVEL_CTRL_REG
/*=============================*/
{
    WORD BITFLD_INPUT_LEVEL0                         : 1;
    WORD BITFLD_INPUT_LEVEL1                         : 1;
    WORD BITFLD_INPUT_LEVEL2                         : 1;
    WORD BITFLD_INPUT_LEVEL3                         : 1;
    WORD BITFLD_INPUT_LEVEL4                         : 1;
    WORD                                             : 2;
    WORD BITFLD_EDGE_LEVELn0                         : 1;
    WORD BITFLD_EDGE_LEVELn1                         : 1;
    WORD BITFLD_EDGE_LEVELn2                         : 1;
    WORD BITFLD_EDGE_LEVELn3                         : 1;
    WORD BITFLD_EDGE_LEVELn4                         : 1;
};
#define INPUT_LEVEL0                         (0x0001)
#define INPUT_LEVEL1                         (0x0002)
#define INPUT_LEVEL2                         (0x0004)
#define INPUT_LEVEL3                         (0x0008)
#define INPUT_LEVEL4                         (0x0010)
#define EDGE_LEVELn0                         (0x0100)
#define EDGE_LEVELn1                         (0x0200)
#define EDGE_LEVELn2                         (0x0400)
#define EDGE_LEVELn3                         (0x0800)
#define EDGE_LEVELn4                         (0x1000)



/*=============================*/
struct __KBRD_IRQ_IN_SEL0_REG
/*=============================*/
{
    WORD BITFLD_KBRD_P00_EN                          : 1;
    WORD BITFLD_KBRD_P01_EN                          : 1;
    WORD BITFLD_KBRD_P02_EN                          : 1;
    WORD BITFLD_KBRD_P03_EN                          : 1;
    WORD BITFLD_KBRD_P04_EN                          : 1;
    WORD BITFLD_KBRD_P05_EN                          : 1;
    WORD BITFLD_KBRD_P06_EN                          : 1;
    WORD BITFLD_KBRD_P07_EN                          : 1;
    WORD BITFLD_KEY_REPEAT                           : 6;
    WORD BITFLD_KBRD_LEVEL                           : 1;
    WORD BITFLD_KBRD_REL                             : 1;
};
#define KBRD_P00_EN                          (0x0001)
#define KBRD_P01_EN                          (0x0002)
#define KBRD_P02_EN                          (0x0004)
#define KBRD_P03_EN                          (0x0008)
#define KBRD_P04_EN                          (0x0010)
#define KBRD_P05_EN                          (0x0020)
#define KBRD_P06_EN                          (0x0040)
#define KBRD_P07_EN                          (0x0080)
#define KEY_REPEAT                           (0x3F00)
#define KBRD_LEVEL                           (0x4000)
#define KBRD_REL                             (0x8000)



/*=============================*/
struct __KBRD_IRQ_IN_SEL1_REG
/*=============================*/
{
    WORD BITFLD_KBRD_P20_EN                          : 1;
    WORD BITFLD_KBRD_P21_EN                          : 1;
    WORD BITFLD_KBRD_P22_EN                          : 1;
    WORD BITFLD_KBRD_P23_EN                          : 1;
    WORD BITFLD_KBRD_P24_EN                          : 1;
    WORD BITFLD_KBRD_P25_EN                          : 1;
    WORD BITFLD_KBRD_P26_EN                          : 1;
    WORD BITFLD_KBRD_P27_EN                          : 1;
    WORD BITFLD_KBRD_P28_EN                          : 1;
    WORD BITFLD_KBRD_P29_EN                          : 1;
    WORD BITFLD_KBRD_P10_EN                          : 1;
    WORD BITFLD_KBRD_P11_EN                          : 1;
    WORD BITFLD_KBRD_P12_EN                          : 1;
    WORD BITFLD_KBRD_P13_EN                          : 1;
    WORD BITFLD_KBRD_P14_EN                          : 1;
    WORD BITFLD_KBRD_P15_EN                          : 1;
};
#define KBRD_P20_EN                          (0x0001)
#define KBRD_P21_EN                          (0x0002)
#define KBRD_P22_EN                          (0x0004)
#define KBRD_P23_EN                          (0x0008)
#define KBRD_P24_EN                          (0x0010)
#define KBRD_P25_EN                          (0x0020)
#define KBRD_P26_EN                          (0x0040)
#define KBRD_P27_EN                          (0x0080)
#define KBRD_P28_EN                          (0x0100)
#define KBRD_P29_EN                          (0x0200)
#define KBRD_P10_EN                          (0x0400)
#define KBRD_P11_EN                          (0x0800)
#define KBRD_P12_EN                          (0x1000)
#define KBRD_P13_EN                          (0x2000)
#define KBRD_P14_EN                          (0x4000)
#define KBRD_P15_EN                          (0x8000)



/*===========================================================================*/
/* memory map OTPC */
/*===========================================================================*/
#define OTPC_MODE_REG                        (0x40008000) /* Mode register */
#define OTPC_PCTRL_REG                       (0x40008004) /* Bit-programming control register */
#define OTPC_STAT_REG                        (0x40008008) /* Status register */
#define OTPC_AHBADR_REG                      (0x4000800C) /* AHB master start address */
#define OTPC_CELADR_REG                      (0x40008010) /* Macrocell start address */
#define OTPC_NWORDS_REG                      (0x40008014) /* Number of words */
#define OTPC_FFPRT_REG                       (0x40008018) /* Ports access to fifo logic */
#define OTPC_FFRD_REG                        (0x4000801C) /* The data which have taken with the latest read from the OTPC_FFPRT_REG */



/*=============================*/
struct __OTPC_MODE_REG
/*=============================*/
{
    WORD BITFLD_OTPC_MODE_MODE                       : 3;
    WORD                                             : 1;
    WORD BITFLD_OTPC_MODE_USE_DMA                    : 1;
    WORD BITFLD_OTPC_MODE_FIFO_FLUSH                 : 1;
    WORD BITFLD_OTPC_MODE_TWO_CC_ACC                 : 1;
    WORD BITFLD_OTPC_MODE_PRG_PORT_SEL               : 1;
    WORD BITFLD_OPTC_MODE_PRG_FAST                   : 1;
    DWORD                                            : 19;
    WORD BITFLD_OTPC_MODE_PRG_PORT_MUX               : 2;
};
#define OTPC_MODE_MODE                       (0x0007)
#define OTPC_MODE_USE_DMA                    (0x0010)
#define OTPC_MODE_FIFO_FLUSH                 (0x0020)
#define OTPC_MODE_TWO_CC_ACC                 (0x0040)
#define OTPC_MODE_PRG_PORT_SEL               (0x0080)
#define OPTC_MODE_PRG_FAST                   (0x0100)
#define OTPC_MODE_PRG_PORT_MUX               (0x30000000)



/*=============================*/
struct __OTPC_PCTRL_REG
/*=============================*/
{
    WORD BITFLD_OTPC_PCTRL_WADDR                     : 13;
    WORD                                             : 3;
    WORD BITFLD_OTPC_PCTRL_BADRL                     : 3;
    WORD BITFLD_OTPC_PCTRL_BSELL                     : 1;
    WORD BITFLD_OTPC_PCTRL_BADRU                     : 3;
    WORD BITFLD_OTPC_PCTRL_BSELU                     : 1;
    WORD BITFLD_OTPC_PCTRL_BITL                      : 1;
    WORD BITFLD_OTPC_PCTRL_ENL                       : 1;
    WORD BITFLD_OTPC_PCTRL_BITU                      : 1;
    WORD BITFLD_OTPC_PCTRL_ENU                       : 1;
};
#define OTPC_PCTRL_WADDR                     (0x1FFF)
#define OTPC_PCTRL_BADRL                     (0x70000)
#define OTPC_PCTRL_BSELL                     (0x80000)
#define OTPC_PCTRL_BADRU                     (0x700000)
#define OTPC_PCTRL_BSELU                     (0x800000)
#define OTPC_PCTRL_BITL                      (0x1000000)
#define OTPC_PCTRL_ENL                       (0x2000000)
#define OTPC_PCTRL_BITU                      (0x4000000)
#define OTPC_PCTRL_ENU                       (0x8000000)



/*=============================*/
struct __OTPC_STAT_REG
/*=============================*/
{
    WORD BITFLD_OTPC_STAT_PRDY                       : 1;
    WORD BITFLD_OTPC_STAT_PERROR                     : 1;
    WORD BITFLD_OTPC_STAT_TRDY                       : 1;
    WORD BITFLD_OTPC_STAT_TERROR                     : 1;
    WORD BITFLD_OTPC_STAT_ARDY                       : 1;
    WORD                                             : 3;
    WORD BITFLD_OTPC_STAT_FWORDS                     : 4;
    WORD BITFLD_OTPC_STAT_PERR_L                     : 1;
    WORD BITFLD_OTPC_STAT_PERR_U                     : 1;
    WORD BITFLD_OTPC_STAT_TERR_L                     : 1;
    WORD BITFLD_OTPC_STAT_TERR_U                     : 1;
    WORD BITFLD_OTPC_STAT_NWORDS                     : 13;
};
#define OTPC_STAT_PRDY                       (0x0001)
#define OTPC_STAT_PERROR                     (0x0002)
#define OTPC_STAT_TRDY                       (0x0004)
#define OTPC_STAT_TERROR                     (0x0008)
#define OTPC_STAT_ARDY                       (0x0010)
#define OTPC_STAT_FWORDS                     (0x0F00)
#define OTPC_STAT_PERR_L                     (0x1000)
#define OTPC_STAT_PERR_U                     (0x2000)
#define OTPC_STAT_TERR_L                     (0x4000)
#define OTPC_STAT_TERR_U                     (0x8000)
#define OTPC_STAT_NWORDS                     (0x1FFF0000)



/*=============================*/
struct __OTPC_AHBADR_REG
/*=============================*/
{
    WORD                                             : 2;
    DWORD BITFLD_OTPC_AHBADR                         : 30;
};
#define OTPC_AHBADR                          (0xFFFFFFFC)



/*=============================*/
struct __OTPC_CELADR_REG
/*=============================*/
{
    WORD BITFLD_OTPC_CELADR                          : 13;
};
#define OTPC_CELADR                          (0x1FFF)



/*=============================*/
struct __OTPC_NWORDS_REG
/*=============================*/
{
    WORD BITFLD_OTPC_NWORDS                          : 13;
};
#define OTPC_NWORDS                          (0x1FFF)



/*=============================*/
struct __OTPC_FFPRT_REG
/*=============================*/
{
    DWORD BITFLD_OTPC_FFPRT                          : 32;
};
#define OTPC_FFPRT                           (0xFFFFFFFF)



/*=============================*/
struct __OTPC_FFRD_REG
/*=============================*/
{
    DWORD BITFLD_OTPC_FFRD                           : 32;
};
#define OTPC_FFRD                            (0xFFFFFFFF)



/*===========================================================================*/
/* memory map PATCH */
/*===========================================================================*/
#define PATCH_VALID_REG                      (0x40008400) /* Validity Control Register */
#define PATCH_VALID_SET_REG                  (0x40008404) /* Validity Set Control Register */
#define PATCH_VALID_RESET_REG                (0x40008408) /* Validity Reset Control Register */
#define PATCH_ADDR0_REG                      (0x40008410) /* Patch entry 0: Address field */
#define PATCH_DATA0_REG                      (0x40008414) /* Patch entry 0: Data field */
#define PATCH_ADDR1_REG                      (0x40008418) /* Patch entry 1: Address field */
#define PATCH_DATA1_REG                      (0x4000841C) /* Patch entry 1: Data field */
#define PATCH_ADDR2_REG                      (0x40008420) /* Patch entry 2: Address field */
#define PATCH_DATA2_REG                      (0x40008424) /* Patch entry 2: Data field */
#define PATCH_ADDR3_REG                      (0x40008428) /* Patch entry 3: Address field */
#define PATCH_DATA3_REG                      (0x4000842C) /* Patch entry 3: Data field */
#define PATCH_ADDR4_REG                      (0x40008430) /* Patch entry 4: Address field */
#define PATCH_DATA4_REG                      (0x40008434) /* Patch entry 4: Data field */
#define PATCH_ADDR5_REG                      (0x40008438) /* Patch entry 5: Address field */
#define PATCH_DATA5_REG                      (0x4000843C) /* Patch entry 5: Data field */
#define PATCH_ADDR6_REG                      (0x40008440) /* Patch entry 6: Address field */
#define PATCH_DATA6_REG                      (0x40008444) /* Patch entry 6: Data field */
#define PATCH_ADDR7_REG                      (0x40008448) /* Patch entry 7: Address field */
#define PATCH_DATA7_REG                      (0x4000844C) /* Patch entry 7: Data field */



/*=============================*/
struct __PATCH_VALID_REG
/*=============================*/
{
    WORD BITFLD_PATCH_VALID                          : 8;
};
#define PATCH_VALID                          (0x00FF)



/*=============================*/
struct __PATCH_VALID_SET_REG
/*=============================*/
{
    WORD BITFLD_PATCH_VALID_SET                      : 8;
};
#define PATCH_VALID_SET                      (0x00FF)



/*=============================*/
struct __PATCH_VALID_RESET_REG
/*=============================*/
{
    WORD BITFLD_PATCH_VALID_RESET                    : 8;
};
#define PATCH_VALID_RESET                    (0x00FF)



/*=============================*/
struct __PATCH_ADDR0_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA0_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR1_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA1_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR2_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA2_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR3_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA3_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR4_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA4_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR5_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA5_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR6_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA6_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_ADDR7_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_ADDR                          : 32;
};
#define PATCH_ADDR                           (0xFFFFFFFF)



/*=============================*/
struct __PATCH_DATA7_REG
/*=============================*/
{
    DWORD BITFLD_PATCH_DATA                          : 32;
};
#define PATCH_DATA                           (0xFFFFFFFF)



/*===========================================================================*/
/* memory map Quadrature Decoder */
/*===========================================================================*/
#define QDEC_CTRL_REG                        (0x50000200) /* Quad Decoder control register */
#define QDEC_XCNT_REG                        (0x50000202) /* Counter value of the X Axis */
#define QDEC_YCNT_REG                        (0x50000204) /* Counter value of the Y Axis */
#define QDEC_CLOCKDIV_REG                    (0x50000206) /* Clock divider register */
#define QDEC_CTRL2_REG                       (0x50000208) /* Quad Decoder control register */
#define QDEC_ZCNT_REG                        (0x5000020A) /* Z_counter */



/*=============================*/
struct __QDEC_CTRL_REG
/*=============================*/
{
    WORD BITFLD_QD_IRQ_MASK                          : 1;
    WORD BITFLD_QD_IRQ_CLR                           : 1;
    WORD BITFLD_QD_IRQ_STATUS                        : 1;
    WORD BITFLD_QD_IRQ_THRES                         : 7;
};
#define QD_IRQ_MASK                          (0x0001)
#define QD_IRQ_CLR                           (0x0002)
#define QD_IRQ_STATUS                        (0x0004)
#define QD_IRQ_THRES                         (0x03F8)



/*=============================*/
struct __QDEC_XCNT_REG
/*=============================*/
{
    WORD BITFLD_X_counter                            : 16;
};
#define X_counter                            (0xFFFF)



/*=============================*/
struct __QDEC_YCNT_REG
/*=============================*/
{
    WORD BITFLD_Y_counter                            : 16;
};
#define Y_counter                            (0xFFFF)



/*=============================*/
struct __QDEC_CLOCKDIV_REG
/*=============================*/
{
    WORD BITFLD_clock_divider                        : 10;
};
#define clock_divider                        (0x03FF)



/*=============================*/
struct __QDEC_CTRL2_REG
/*=============================*/
{
    WORD BITFLD_CHX_PORT_SEL                         : 4;
    WORD BITFLD_CHY_PORT_SEL                         : 4;
    WORD BITFLD_CHZ_PORT_SEL                         : 4;
};
#define CHX_PORT_SEL                         (0x000F)
#define CHY_PORT_SEL                         (0x00F0)
#define CHZ_PORT_SEL                         (0x0F00)



/*=============================*/
struct __QDEC_ZCNT_REG
/*=============================*/
{
    WORD BITFLD_Z_counter                            : 16;
};
#define Z_counter                            (0xFFFF)



/*===========================================================================*/
/* memory map SPI */
/*===========================================================================*/
#define SPI_CTRL_REG                         (0x50001200) /* SPI control register 0 */
#define SPI_RX_TX_REG0                       (0x50001202) /* SPI RX/TX register0 */
#define SPI_RX_TX_REG1                       (0x50001204) /* SPI RX/TX register1 */
#define SPI_CLEAR_INT_REG                    (0x50001206) /* SPI clear interrupt register */
#define SPI_CTRL_REG1                        (0x50001208) /* SPI control register 1 */



/*=============================*/
struct __SPI_CTRL_REG
/*=============================*/
{
    WORD BITFLD_SPI_ON                               : 1;
    WORD BITFLD_SPI_PHA                              : 1;
    WORD BITFLD_SPI_POL                              : 1;
    WORD BITFLD_SPI_CLK                              : 2;
    WORD BITFLD_SPI_DO                               : 1;
    WORD BITFLD_SPI_SMN                              : 1;
    WORD BITFLD_SPI_WORD                             : 2;
    WORD BITFLD_SPI_RST                              : 1;
    WORD BITFLD_SPI_FORCE_DO                         : 1;
    WORD BITFLD_SPI_TXH                              : 1;
    WORD BITFLD_SPI_DI                               : 1;
    WORD BITFLD_SPI_INT_BIT                          : 1;
    WORD BITFLD_SPI_MINT                             : 1;
    WORD BITFLD_SPI_EN_CTRL                          : 1;
};
#define SPI_ON                               (0x0001)
#define SPI_PHA                              (0x0002)
#define SPI_POL                              (0x0004)
#define SPI_CLK                              (0x0018)
#define SPI_DO                               (0x0020)
#define SPI_SMN                              (0x0040)
#define SPI_WORD                             (0x0180)
#define SPI_RST                              (0x0200)
#define SPI_FORCE_DO                         (0x0400)
#define SPI_TXH                              (0x0800)
#define SPI_DI                               (0x1000)
#define SPI_INT_BIT                          (0x2000)
#define SPI_MINT                             (0x4000)
#define SPI_EN_CTRL                          (0x8000)



/*=============================*/
struct __SPI_RX_TX_REG0
/*=============================*/
{
    WORD BITFLD_SPI_DATA0                            : 16;
};
#define SPI_DATA0                            (0xFFFF)



/*=============================*/
struct __SPI_RX_TX_REG1
/*=============================*/
{
    WORD BITFLD_SPI_DATA1                            : 16;
};
#define SPI_DATA1                            (0xFFFF)



/*=============================*/
struct __SPI_CLEAR_INT_REG
/*=============================*/
{
    WORD BITFLD_SPI_CLEAR_INT                        : 16;
};
#define SPI_CLEAR_INT                        (0xFFFF)



/*=============================*/
struct __SPI_CTRL_REG1
/*=============================*/
{
    WORD BITFLD_SPI_FIFO_MODE                        : 2;
    WORD BITFLD_SPI_PRIORITY                         : 1;
    WORD BITFLD_SPI_BUSY                             : 1;
    WORD BITFLD_SPI_9BIT_VAL                         : 1;
};
#define SPI_FIFO_MODE                        (0x0003)
#define SPI_PRIORITY                         (0x0004)
#define SPI_BUSY                             (0x0008)
#define SPI_9BIT_VAL                         (0x0010)



/*===========================================================================*/
/* memory map Timer+3PWM */
/*===========================================================================*/
#define TIMER0_CTRL_REG                      (0x50003400) /* Timer0 control register */
#define TIMER0_ON_REG                        (0x50003402) /* Timer0 on control register */
#define TIMER0_RELOAD_M_REG                  (0x50003404) /* 16 bits reload value for Timer0 */
#define TIMER0_RELOAD_N_REG                  (0x50003406) /* 16 bits reload value for Timer0 */
#define PWM2_DUTY_CYCLE                      (0x50003408) /* Duty Cycle for PWM2 */
#define PWM3_DUTY_CYCLE                      (0x5000340A) /* Duty Cycle for PWM3 */
#define PWM4_DUTY_CYCLE                      (0x5000340C) /* Duty Cycle for PWM4 */
#define TRIPLE_PWM_FREQUENCY                 (0x5000340E) /* Frequency for PWM 2,3 and 4 */
#define TRIPLE_PWM_CTRL_REG                  (0x50003410) /* PWM 2 3 4 Control */



/*=============================*/
struct __TIMER0_CTRL_REG
/*=============================*/
{
    WORD BITFLD_TIM0_CTRL                            : 1;
    WORD BITFLD_TIM0_CLK_SEL                         : 1;
    WORD BITFLD_TIM0_CLK_DIV                         : 1;
    WORD BITFLD_PWM_MODE                             : 1;
};
#define TIM0_CTRL                            (0x0001)
#define TIM0_CLK_SEL                         (0x0002)
#define TIM0_CLK_DIV                         (0x0004)
#define PWM_MODE                             (0x0008)



/*=============================*/
struct __TIMER0_ON_REG
/*=============================*/
{
    WORD BITFLD_TIM0_ON                              : 16;
};
#define TIM0_ON                              (0xFFFF)



/*=============================*/
struct __TIMER0_RELOAD_M_REG
/*=============================*/
{
    WORD BITFLD_TIM0_M                               : 16;
};
#define TIM0_M                               (0xFFFF)



/*=============================*/
struct __TIMER0_RELOAD_N_REG
/*=============================*/
{
    WORD BITFLD_TIM0_N                               : 16;
};
#define TIM0_N                               (0xFFFF)



/*=============================*/
struct __PWM2_DUTY_CYCLE
/*=============================*/
{
    WORD BITFLD_DUTY_CYCLE                           : 14;
};
#define DUTY_CYCLE                           (0x3FFF)



/*=============================*/
struct __PWM3_DUTY_CYCLE
/*=============================*/
{
    WORD BITFLD_DUTY_CYCLE                           : 14;
};
#define DUTY_CYCLE                           (0x3FFF)



/*=============================*/
struct __PWM4_DUTY_CYCLE
/*=============================*/
{
    WORD BITFLD_DUTY_CYCLE                           : 14;
};
#define DUTY_CYCLE                           (0x3FFF)



/*=============================*/
struct __TRIPLE_PWM_FREQUENCY
/*=============================*/
{
    WORD BITFLD_FREQ                                 : 14;
};
#define FREQ                                 (0x3FFF)



/*=============================*/
struct __TRIPLE_PWM_CTRL_REG
/*=============================*/
{
    WORD BITFLD_TRIPLE_PWM_ENABLE                    : 1;
    WORD BITFLD_SW_PAUSE_EN                          : 1;
    WORD BITFLD_HW_PAUSE_EN                          : 1;
};
#define TRIPLE_PWM_ENABLE                    (0x0001)
#define SW_PAUSE_EN                          (0x0002)
#define HW_PAUSE_EN                          (0x0004)



/*===========================================================================*/
/* memory map UART */
/*===========================================================================*/
#define UART_RBR_THR_DLL_REG                 (0x50001000) /* Receive Buffer Register */
#define UART_IER_DLH_REG                     (0x50001004) /* Interrupt Enable Register */
#define UART_IIR_FCR_REG                     (0x50001008) /* Interrupt Identification Register/FIFO Control Register */
#define UART_LCR_REG                         (0x5000100C) /* Line Control Register */
#define UART_MCR_REG                         (0x50001010) /* Modem Control Register */
#define UART_LSR_REG                         (0x50001014) /* Line Status Register */
#define UART_MSR_REG                         (0x50001018) /* Modem Status Register */
#define UART_SCR_REG                         (0x5000101C) /* Scratchpad Register */
#define UART_LPDLL_REG                       (0x50001020) /* Low Power Divisor Latch Low */
#define UART_LPDLH_REG                       (0x50001024) /* Low Power Divisor Latch High */
#define UART_SRBR_STHR0_REG                  (0x50001030) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR1_REG                  (0x50001034) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR2_REG                  (0x50001038) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR3_REG                  (0x5000103C) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR4_REG                  (0x50001040) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR5_REG                  (0x50001044) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR6_REG                  (0x50001048) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR7_REG                  (0x5000104C) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR8_REG                  (0x50001050) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR9_REG                  (0x50001054) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR10_REG                 (0x50001058) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR11_REG                 (0x5000105C) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR12_REG                 (0x50001060) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR13_REG                 (0x50001064) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR14_REG                 (0x50001068) /* Shadow Receive/Transmit Buffer Register */
#define UART_SRBR_STHR15_REG                 (0x5000106C) /* Shadow Receive/Transmit Buffer Register */
#define UART_USR_REG                         (0x5000107C) /* UART Status register. */
#define UART_TFL_REG                         (0x50001080) /* Transmit FIFO Level */
#define UART_RFL_REG                         (0x50001084) /* Receive FIFO Level. */
#define UART_SRR_REG                         (0x50001088) /* Software Reset Register. */
#define UART_SRTS_REG                        (0x5000108C) /* Shadow Request to Send */
#define UART_SBCR_REG                        (0x50001090) /* Shadow Break Control Register */
#define UART_SDMAM_REG                       (0x50001094) /* Shadow DMA Mode */
#define UART_SFE_REG                         (0x50001098) /* Shadow FIFO Enable */
#define UART_SRT_REG                         (0x5000109C) /* Shadow RCVR Trigger */
#define UART_STET_REG                        (0x500010A0) /* Shadow TX Empty Trigger */
#define UART_HTX_REG                         (0x500010A4) /* Halt TX */
#define UART_CPR_REG                         (0x500010F4) /* Component Parameter Register */
#define UART_UCV_REG                         (0x500010F8) /* Component Version */
#define UART_CTR_REG                         (0x500010FC) /* Component Type Register */
#define UART2_RBR_THR_DLL_REG                (0x50001100) /* Receive Buffer Register */
#define UART2_IER_DLH_REG                    (0x50001104) /* Interrupt Enable Register */
#define UART2_IIR_FCR_REG                    (0x50001108) /* Interrupt Identification Register/FIFO Control Register */
#define UART2_LCR_REG                        (0x5000110C) /* Line Control Register */
#define UART2_MCR_REG                        (0x50001110) /* Modem Control Register */
#define UART2_LSR_REG                        (0x50001114) /* Line Status Register */
#define UART2_MSR_REG                        (0x50001118) /* Modem Status Register */
#define UART2_SCR_REG                        (0x5000111C) /* Scratchpad Register */
#define UART2_LPDLL_REG                      (0x50001120) /* Low Power Divisor Latch Low */
#define UART2_LPDLH_REG                      (0x50001124) /* Low Power Divisor Latch High */
#define UART2_SRBR_STHR0_REG                 (0x50001130) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR1_REG                 (0x50001134) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR2_REG                 (0x50001138) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR3_REG                 (0x5000113C) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR4_REG                 (0x50001140) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR5_REG                 (0x50001144) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR6_REG                 (0x50001148) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR7_REG                 (0x5000114C) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR8_REG                 (0x50001150) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR9_REG                 (0x50001154) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR10_REG                (0x50001158) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR11_REG                (0x5000115C) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR12_REG                (0x50001160) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR13_REG                (0x50001164) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR14_REG                (0x50001168) /* Shadow Receive/Transmit Buffer Register */
#define UART2_SRBR_STHR15_REG                (0x5000116C) /* Shadow Receive/Transmit Buffer Register */
#define UART2_USR_REG                        (0x5000117C) /* UART Status register. */
#define UART2_TFL_REG                        (0x50001180) /* Transmit FIFO Level */
#define UART2_RFL_REG                        (0x50001184) /* Receive FIFO Level. */
#define UART2_SRR_REG                        (0x50001188) /* Software Reset Register. */
#define UART2_SRTS_REG                       (0x5000118C) /* Shadow Request to Send */
#define UART2_SBCR_REG                       (0x50001190) /* Shadow Break Control Register */
#define UART2_SDMAM_REG                      (0x50001194) /* Shadow DMA Mode */
#define UART2_SFE_REG                        (0x50001198) /* Shadow FIFO Enable */
#define UART2_SRT_REG                        (0x5000119C) /* Shadow RCVR Trigger */
#define UART2_STET_REG                       (0x500011A0) /* Shadow TX Empty Trigger */
#define UART2_HTX_REG                        (0x500011A4) /* Halt TX */
#define UART2_CPR_REG                        (0x500011F4) /* Component Parameter Register */
#define UART2_UCV_REG                        (0x500011F8) /* Component Version */
#define UART2_CTR_REG                        (0x500011FC) /* Component Type Register */



/*=============================*/
struct __UART_RBR_THR_DLL_REG
/*=============================*/
{
    WORD BITFLD_RBR_THR_DLL                          : 8;
};
#define RBR_THR_DLL                          (0x00FF)



/*=============================*/
struct __UART_IER_DLH_REG
/*=============================*/
{
    WORD BITFLD_ERBFI_dlh0                           : 1;
    WORD BITFLD_ETBEI_dlh1                           : 1;
    WORD BITFLD_ELSI_dhl2                            : 1;
    WORD BITFLD_EDSSI_dlh3                           : 1;
    WORD                                             : 3;
    WORD BITFLD_PTIME_dlh7                           : 1;
};
#define ERBFI_dlh0                           (0x0001)
#define ETBEI_dlh1                           (0x0002)
#define ELSI_dhl2                            (0x0004)
#define EDSSI_dlh3                           (0x0008)
#define PTIME_dlh7                           (0x0080)



/*=============================*/
struct __UART_IIR_FCR_REG
/*=============================*/
{
    DWORD BITFLD_IIR_FCR                             : 32;
};
#define IIR_FCR                              (0xFFFFFFFF)



/*=============================*/
struct __UART_LCR_REG
/*=============================*/
{
    WORD BITFLD_UART_DLS                             : 2;
    WORD BITFLD_UART_STOP                            : 1;
    WORD BITFLD_UART_PEN                             : 1;
    WORD BITFLD_UART_EPS                             : 1;
    WORD                                             : 1;
    WORD BITFLD_UART_BC                              : 1;
    WORD BITFLD_UART_DLAB                            : 1;
};
#define UART_DLS                             (0x0003)
#define UART_STOP                            (0x0004)
#define UART_PEN                             (0x0008)
#define UART_EPS                             (0x0010)
#define UART_BC                              (0x0040)
#define UART_DLAB                            (0x0080)



/*=============================*/
struct __UART_MCR_REG
/*=============================*/
{
    WORD BITFLD_UART_DTR                             : 1;
    WORD BITFLD_UART_RTS                             : 1;
    WORD BITFLD_UART_OUT1                            : 1;
    WORD BITFLD_UART_OUT2                            : 1;
    WORD BITFLD_UART_LB                              : 1;
    WORD BITFLD_UART_AFCE                            : 1;
    WORD BITFLD_UART_SIRE                            : 1;
};
#define UART_DTR                             (0x0001)
#define UART_RTS                             (0x0002)
#define UART_OUT1                            (0x0004)
#define UART_OUT2                            (0x0008)
#define UART_LB                              (0x0010)
#define UART_AFCE                            (0x0020)
#define UART_SIRE                            (0x0040)



/*=============================*/
struct __UART_LSR_REG
/*=============================*/
{
    WORD BITFLD_UART_DR                              : 1;
    WORD BITFLD_UART_OE                              : 1;
    WORD BITFLD_UART_PE                              : 1;
    WORD BITFLD_UART_FE                              : 1;
    WORD BITFLD_UART_B1                              : 1;
    WORD BITFLD_UART_THRE                            : 1;
    WORD BITFLD_UART_TEMT                            : 1;
    WORD BITFLD_UART_RFE                             : 1;
};
#define UART_DR                              (0x0001)
#define UART_OE                              (0x0002)
#define UART_PE                              (0x0004)
#define UART_FE                              (0x0008)
#define UART_B1                              (0x0010)
#define UART_THRE                            (0x0020)
#define UART_TEMT                            (0x0040)
#define UART_RFE                             (0x0080)



/*=============================*/
struct __UART_MSR_REG
/*=============================*/
{
    WORD BITFLD_UART_DCTS                            : 1;
    WORD BITFLD_UART_DDSR                            : 1;
    WORD BITFLD_UART_TERI                            : 1;
    WORD BITFLD_UART_DDCD                            : 1;
    WORD BITFLD_UART_CTS                             : 1;
    WORD BITFLD_UART_DSR                             : 1;
    WORD BITFLD_UART_R1                              : 1;
    WORD BITFLD_UART_DCD                             : 1;
};
#define UART_DCTS                            (0x0001)
#define UART_DDSR                            (0x0002)
#define UART_TERI                            (0x0004)
#define UART_DDCD                            (0x0008)
#define UART_CTS                             (0x0010)
#define UART_DSR                             (0x0020)
#define UART_R1                              (0x0040)
#define UART_DCD                             (0x0080)



/*=============================*/
struct __UART_SCR_REG
/*=============================*/
{
    WORD BITFLD_UART_SCRATCH_PAD                     : 8;
};
#define UART_SCRATCH_PAD                     (0x00FF)



/*=============================*/
struct __UART_LPDLL_REG
/*=============================*/
{
    WORD BITFLD_UART_LPDLL                           : 8;
};
#define UART_LPDLL                           (0x00FF)



/*=============================*/
struct __UART_LPDLH_REG
/*=============================*/
{
    WORD BITFLD_UART_LPDLH                           : 8;
};
#define UART_LPDLH                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR0_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR1_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR2_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR3_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR4_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR5_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR6_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR7_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR8_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR9_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR10_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR11_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR12_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR13_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR14_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_SRBR_STHR15_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART_USR_REG
/*=============================*/
{
    WORD                                             : 1;
    WORD BITFLD_UART_TFNF                            : 1;
    WORD BITFLD_UART_TFE                             : 1;
    WORD BITFLD_UART_RFNE                            : 1;
    WORD BITFLD_UART_RFF                             : 1;
};
#define UART_TFNF                            (0x0002)
#define UART_TFE                             (0x0004)
#define UART_RFNE                            (0x0008)
#define UART_RFF                             (0x0010)



/*=============================*/
struct __UART_TFL_REG
/*=============================*/
{
    WORD BITFLD_UART_TRANSMIT_FIFO_LEVEL             : 16;
};
#define UART_TRANSMIT_FIFO_LEVEL             (0xFFFF)



/*=============================*/
struct __UART_RFL_REG
/*=============================*/
{
    WORD BITFLD_UART_RECEIVE_FIFO_LEVEL              : 16;
};
#define UART_RECEIVE_FIFO_LEVEL              (0xFFFF)



/*=============================*/
struct __UART_SRR_REG
/*=============================*/
{
    WORD BITFLD_UART_UR                              : 1;
    WORD BITFLD_UART_RFR                             : 1;
    WORD BITFLD_UART_XFR                             : 1;
};
#define UART_UR                              (0x0001)
#define UART_RFR                             (0x0002)
#define UART_XFR                             (0x0004)



/*=============================*/
struct __UART_SRTS_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_REQUEST_TO_SEND          : 1;
};
#define UART_SHADOW_REQUEST_TO_SEND          (0x0001)



/*=============================*/
struct __UART_SBCR_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_BREAK_CONTROL            : 1;
};
#define UART_SHADOW_BREAK_CONTROL            (0x0001)



/*=============================*/
struct __UART_SDMAM_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_DMA_MODE                 : 1;
};
#define UART_SHADOW_DMA_MODE                 (0x0001)



/*=============================*/
struct __UART_SFE_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_FIFO_ENABLE              : 1;
};
#define UART_SHADOW_FIFO_ENABLE              (0x0001)



/*=============================*/
struct __UART_SRT_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_RCVR_TRIGGER             : 2;
};
#define UART_SHADOW_RCVR_TRIGGER             (0x0003)



/*=============================*/
struct __UART_STET_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_TX_EMPTY_TRIGGER         : 2;
};
#define UART_SHADOW_TX_EMPTY_TRIGGER         (0x0003)



/*=============================*/
struct __UART_HTX_REG
/*=============================*/
{
    WORD BITFLD_UART_HALT_TX                         : 1;
};
#define UART_HALT_TX                         (0x0001)



/*=============================*/
struct __UART_CPR_REG
/*=============================*/
{
    DWORD BITFLD_CPR                                 : 32;
};
#define CPR                                  (0xFFFFFFFF)



/*=============================*/
struct __UART_UCV_REG
/*=============================*/
{
    DWORD BITFLD_UCV                                 : 32;
};
#define UCV                                  (0xFFFFFFFF)



/*=============================*/
struct __UART_CTR_REG
/*=============================*/
{
    DWORD BITFLD_CTR                                 : 32;
};
#define CTR                                  (0xFFFFFFFF)



/*=============================*/
struct __UART2_RBR_THR_DLL_REG
/*=============================*/
{
    WORD BITFLD_RBR_THR_DLL                          : 8;
};
//field masks defined earlier
//#define RBR_THR_DLL                          (0x00FF)



/*=============================*/
struct __UART2_IER_DLH_REG
/*=============================*/
{
    WORD BITFLD_ERBFI_dlh0                           : 1;
    WORD BITFLD_ETBEI_dlh1                           : 1;
    WORD BITFLD_ELSI_dhl2                            : 1;
    WORD BITFLD_EDSSI_dlh3                           : 1;
    WORD                                             : 3;
    WORD BITFLD_PTIME_dlh7                           : 1;
};
//field masks defined earlier
//#define ERBFI_dlh0                           (0x0001)
//#define ETBEI_dlh1                           (0x0002)
//#define ELSI_dhl2                            (0x0004)
//#define EDSSI_dlh3                           (0x0008)
//#define PTIME_dlh7                           (0x0080)



/*=============================*/
struct __UART2_IIR_FCR_REG
/*=============================*/
{
    DWORD BITFLD_IIR_FCR                             : 32;
};
//field masks defined earlier
//#define IIR_FCR                              (0xFFFFFFFF)



/*=============================*/
struct __UART2_LCR_REG
/*=============================*/
{
    WORD BITFLD_UART_DLS                             : 2;
    WORD BITFLD_UART_STOP                            : 1;
    WORD BITFLD_UART_PEN                             : 1;
    WORD BITFLD_UART_EPS                             : 1;
    WORD                                             : 1;
    WORD BITFLD_UART_BC                              : 1;
    WORD BITFLD_UART_DLAB                            : 1;
};
//field masks defined earlier
//#define UART_DLS                             (0x0003)
//#define UART_STOP                            (0x0004)
//#define UART_PEN                             (0x0008)
//#define UART_EPS                             (0x0010)
//#define UART_BC                              (0x0040)
//#define UART_DLAB                            (0x0080)



/*=============================*/
struct __UART2_MCR_REG
/*=============================*/
{
    WORD BITFLD_UART_DTR                             : 1;
    WORD BITFLD_UART_RTS                             : 1;
    WORD BITFLD_UART_OUT1                            : 1;
    WORD BITFLD_UART_OUT2                            : 1;
    WORD BITFLD_UART_LB                              : 1;
    WORD BITFLD_UART_AFCE                            : 1;
    WORD BITFLD_UART_SIRE                            : 1;
};
//field masks defined earlier
//#define UART_DTR                             (0x0001)
//#define UART_RTS                             (0x0002)
//#define UART_OUT1                            (0x0004)
//#define UART_OUT2                            (0x0008)
//#define UART_LB                              (0x0010)
//#define UART_AFCE                            (0x0020)
//#define UART_SIRE                            (0x0040)



/*=============================*/
struct __UART2_LSR_REG
/*=============================*/
{
    WORD BITFLD_UART_DR                              : 1;
    WORD BITFLD_UART_OE                              : 1;
    WORD BITFLD_UART_PE                              : 1;
    WORD BITFLD_UART_FE                              : 1;
    WORD BITFLD_UART_B1                              : 1;
    WORD BITFLD_UART_THRE                            : 1;
    WORD BITFLD_UART_TEMT                            : 1;
    WORD BITFLD_UART_RFE                             : 1;
};
//field masks defined earlier
//#define UART_DR                              (0x0001)
//#define UART_OE                              (0x0002)
//#define UART_PE                              (0x0004)
//#define UART_FE                              (0x0008)
//#define UART_B1                              (0x0010)
//#define UART_THRE                            (0x0020)
//#define UART_TEMT                            (0x0040)
//#define UART_RFE                             (0x0080)



/*=============================*/
struct __UART2_MSR_REG
/*=============================*/
{
    WORD BITFLD_UART_DCTS                            : 1;
    WORD BITFLD_UART_DDSR                            : 1;
    WORD BITFLD_UART_TERI                            : 1;
    WORD BITFLD_UART_DDCD                            : 1;
    WORD BITFLD_UART_CTS                             : 1;
    WORD BITFLD_UART_DSR                             : 1;
    WORD BITFLD_UART_R1                              : 1;
    WORD BITFLD_UART_DCD                             : 1;
};
//field masks defined earlier
//#define UART_DCTS                            (0x0001)
//#define UART_DDSR                            (0x0002)
//#define UART_TERI                            (0x0004)
//#define UART_DDCD                            (0x0008)
//#define UART_CTS                             (0x0010)
//#define UART_DSR                             (0x0020)
//#define UART_R1                              (0x0040)
//#define UART_DCD                             (0x0080)



/*=============================*/
struct __UART2_SCR_REG
/*=============================*/
{
    WORD BITFLD_UART_SCRATCH_PAD                     : 8;
};
//field masks defined earlier
//#define UART_SCRATCH_PAD                     (0x00FF)



/*=============================*/
struct __UART2_LPDLL_REG
/*=============================*/
{
    WORD BITFLD_UART_LPDLL                           : 8;
};
//field masks defined earlier
//#define UART_LPDLL                           (0x00FF)



/*=============================*/
struct __UART2_LPDLH_REG
/*=============================*/
{
    WORD BITFLD_UART_LPDLH                           : 8;
};
//field masks defined earlier
//#define UART_LPDLH                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR0_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR1_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR2_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR3_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR4_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR5_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR6_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR7_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR8_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR9_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR10_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR11_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR12_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR13_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR14_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_SRBR_STHR15_REG
/*=============================*/
{
    WORD BITFLD_SRBR_STHRx                           : 8;
};
//field masks defined earlier
//#define SRBR_STHRx                           (0x00FF)



/*=============================*/
struct __UART2_USR_REG
/*=============================*/
{
    WORD                                             : 1;
    WORD BITFLD_UART_TFNF                            : 1;
    WORD BITFLD_UART_TFE                             : 1;
    WORD BITFLD_UART_RFNE                            : 1;
    WORD BITFLD_UART_RFF                             : 1;
};
//field masks defined earlier
//#define UART_TFNF                            (0x0002)
//#define UART_TFE                             (0x0004)
//#define UART_RFNE                            (0x0008)
//#define UART_RFF                             (0x0010)



/*=============================*/
struct __UART2_TFL_REG
/*=============================*/
{
    WORD BITFLD_UART_TRANSMIT_FIFO_LEVEL             : 16;
};
//field masks defined earlier
//#define UART_TRANSMIT_FIFO_LEVEL             (0xFFFF)



/*=============================*/
struct __UART2_RFL_REG
/*=============================*/
{
    WORD BITFLD_UART_RECEIVE_FIFO_LEVEL              : 16;
};
//field masks defined earlier
//#define UART_RECEIVE_FIFO_LEVEL              (0xFFFF)



/*=============================*/
struct __UART2_SRR_REG
/*=============================*/
{
    WORD BITFLD_UART_UR                              : 1;
    WORD BITFLD_UART_RFR                             : 1;
    WORD BITFLD_UART_XFR                             : 1;
};
//field masks defined earlier
//#define UART_UR                              (0x0001)
//#define UART_RFR                             (0x0002)
//#define UART_XFR                             (0x0004)



/*=============================*/
struct __UART2_SRTS_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_REQUEST_TO_SEND          : 1;
};
//field masks defined earlier
//#define UART_SHADOW_REQUEST_TO_SEND          (0x0001)



/*=============================*/
struct __UART2_SBCR_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_BREAK_CONTROL            : 1;
};
//field masks defined earlier
//#define UART_SHADOW_BREAK_CONTROL            (0x0001)



/*=============================*/
struct __UART2_SDMAM_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_DMA_MODE                 : 1;
};
//field masks defined earlier
//#define UART_SHADOW_DMA_MODE                 (0x0001)



/*=============================*/
struct __UART2_SFE_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_FIFO_ENABLE              : 1;
};
//field masks defined earlier
//#define UART_SHADOW_FIFO_ENABLE              (0x0001)



/*=============================*/
struct __UART2_SRT_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_RCVR_TRIGGER             : 2;
};
//field masks defined earlier
//#define UART_SHADOW_RCVR_TRIGGER             (0x0003)



/*=============================*/
struct __UART2_STET_REG
/*=============================*/
{
    WORD BITFLD_UART_SHADOW_TX_EMPTY_TRIGGER         : 2;
};
//field masks defined earlier
//#define UART_SHADOW_TX_EMPTY_TRIGGER         (0x0003)



/*=============================*/
struct __UART2_HTX_REG
/*=============================*/
{
    WORD BITFLD_UART_HALT_TX                         : 1;
};
//field masks defined earlier
//#define UART_HALT_TX                         (0x0001)



/*=============================*/
struct __UART2_CPR_REG
/*=============================*/
{
    DWORD BITFLD_CPR                                 : 32;
};
//field masks defined earlier
//#define CPR                                  (0xFFFFFFFF)



/*=============================*/
struct __UART2_UCV_REG
/*=============================*/
{
    DWORD BITFLD_UCV                                 : 32;
};
//field masks defined earlier
//#define UCV                                  (0xFFFFFFFF)



/*=============================*/
struct __UART2_CTR_REG
/*=============================*/
{
    DWORD BITFLD_CTR                                 : 32;
};
//field masks defined earlier
//#define CTR                                  (0xFFFFFFFF)



/*===========================================================================*/
/* memory map Version */
/*===========================================================================*/
#define CHIP_ID1_REG                         (0x50003200) /* Chip identification register 1. */
#define CHIP_ID2_REG                         (0x50003201) /* Chip identification register 2. */
#define CHIP_ID3_REG                         (0x50003202) /* Chip identification register 3. */
#define CHIP_SWC_REG                         (0x50003203) /* Software compatibility register. */
#define CHIP_REVISION_REG                    (0x50003204) /* Chip revision register. */
#define CHIP_CONFIG1_REG                     (0x50003205) /* Chip configuration register 1. */
#define CHIP_CONFIG2_REG                     (0x50003206) /* Chip configuration register 2. */
#define CHIP_CONFIG3_REG                     (0x50003207) /* Chip configuration register 3. */
#define CHIP_TEST1_REG                       (0x5000320A) /* Chip test register 1. */
#define CHIP_TEST2_REG                       (0x5000320B) /* Chip test register 2. */



/*=============================*/
struct __CHIP_ID1_REG
/*=============================*/
{
    WORD BITFLD_CHIP_ID1                             : 8;
};
#define CHIP_ID1                             (0x00FF)



/*=============================*/
struct __CHIP_ID2_REG
/*=============================*/
{
    WORD BITFLD_CHIP_ID2                             : 8;
};
#define CHIP_ID2                             (0x00FF)



/*=============================*/
struct __CHIP_ID3_REG
/*=============================*/
{
    WORD BITFLD_CHIP_ID3                             : 8;
};
#define CHIP_ID3                             (0x00FF)



/*=============================*/
struct __CHIP_SWC_REG
/*=============================*/
{
    WORD BITFLD_CHIP_SWC                             : 4;
};
#define CHIP_SWC                             (0x000F)



/*=============================*/
struct __CHIP_REVISION_REG
/*=============================*/
{
    WORD BITFLD_REVISION_ID                          : 8;
};
#define REVISION_ID                          (0x00FF)



/*=============================*/
struct __CHIP_CONFIG1_REG
/*=============================*/
{
    WORD BITFLD_CHIP_CONFIG1                         : 8;
};
#define CHIP_CONFIG1                         (0x00FF)



/*=============================*/
struct __CHIP_CONFIG2_REG
/*=============================*/
{
    WORD BITFLD_CHIP_CONFIG2                         : 8;
};
#define CHIP_CONFIG2                         (0x00FF)



/*=============================*/
struct __CHIP_CONFIG3_REG
/*=============================*/
{
    WORD BITFLD_CHIP_CONFIG3                         : 8;
};
#define CHIP_CONFIG3                         (0x00FF)


#if 0
/*=============================*/
struct __CHIP_TEST1_REG
/*=============================*/
{
    WORD                                             : 4;
};



/*=============================*/
struct __CHIP_TEST2_REG
/*=============================*/
{
    WORD                                             : 8;
};

#endif

/*===========================================================================*/
/* memory map WakeUp and KBRD */
/*===========================================================================*/
#define WKUP_CTRL_REG                        (0x50000100) /* Control register for the wakeup counter */
#define WKUP_COMPARE_REG                     (0x50000102) /* Number of events before wakeup interrupt */
#define WKUP_RESET_IRQ_REG                   (0x50000104) /* Reset wakeup interrupt */
#define WKUP_COUNTER_REG                     (0x50000106) /* Actual number of events of the wakeup counter */
#define WKUP_RESET_CNTR_REG                  (0x50000108) /* Reset the event counter */



/*=============================*/
struct __WKUP_CTRL_REG
/*=============================*/
{
    WORD BITFLD_INPUT_SEL                            : 6;
    WORD BITFLD_WUP_DEB_VALUE                        : 6;
    WORD BITFLD_LEVEL                                : 1;
    WORD BITFLD_ENABLE_IRQ                           : 1;
};
#define INPUT_SEL                            (0x003F)
#define WUP_DEB_VALUE                        (0x0FC0)
#define LEVEL                                (0x1000)
#define ENABLE_IRQ                           (0x2000)



/*=============================*/
struct __WKUP_COMPARE_REG
/*=============================*/
{
    WORD BITFLD_COMPARE                              : 8;
};
#define COMPARE                              (0x00FF)



/*=============================*/
struct __WKUP_RESET_IRQ_REG
/*=============================*/
{
    WORD BITFLD_WKUP_IRQ_RST                         : 16;
};
#define WKUP_IRQ_RST                         (0xFFFF)



/*=============================*/
struct __WKUP_COUNTER_REG
/*=============================*/
{
    WORD BITFLD_EVENT_VALUE                          : 8;
};
#define EVENT_VALUE                          (0x00FF)



/*=============================*/
struct __WKUP_RESET_CNTR_REG
/*=============================*/
{
    WORD BITFLD_WKUP_CNTR_RST                        : 16;
};
#define WKUP_CNTR_RST                        (0xFFFF)



/*===========================================================================*/
/* memory map WDOG */
/*===========================================================================*/
#define WATCHDOG_REG                         (0x50003100) /* Watchdog timer register. */
#define WATCHDOG_CTRL_REG                    (0x50003102) /* Watchdog control register. */



/*=============================*/
struct __WATCHDOG_REG
/*=============================*/
{
    WORD BITFLD_WDOG_VAL                             : 8;
    WORD BITFLD_WDOG_VAL_NEG                         : 1;
    WORD BITFLD_WDOG_WEN                             : 7;
};
#define WDOG_VAL                             (0x00FF)
#define WDOG_VAL_NEG                         (0x0100)
#define WDOG_WEN                             (0xFE00)



/*=============================*/
struct __WATCHDOG_CTRL_REG
/*=============================*/
{
    WORD BITFLD_NMI_RST                              : 1;
};
#define NMI_RST                              (0x0001)
/*===========================================================================*/ 
/*===================== End of automatic generated code =====================*/ 
/*===========================================================================*/ 



/*===========================================================================*/ 
/*========================== Start of footer part ===========================*/ 
/*===========================================================================*/ 
/*----------------------------*/
/* Data access macros         */
/*----------------------------*/

#define SHIF16(a) ((a)&0x0001?0: (a)&0x0002?1: (a)&0x0004?2: (a)&0x0008?3:\
                   (a)&0x0010?4: (a)&0x0020?5: (a)&0x0040?6: (a)&0x0080?7:\
                   (a)&0x0100?8: (a)&0x0200?9: (a)&0x0400?10:(a)&0x0800?11:\
                   (a)&0x1000?12:(a)&0x2000?13:(a)&0x4000?14: 15)

#define SHIF32(a)((a)&0x00000001?0: (a)&0x00000002?1: (a)&0x00000004?2: (a)&0x00000008?3:\
                  (a)&0x00000010?4: (a)&0x00000020?5: (a)&0x00000040?6: (a)&0x00000080?7:\
                  (a)&0x00000100?8: (a)&0x00000200?9: (a)&0x00000400?10:(a)&0x00000800?11:\
                  (a)&0x00001000?12:(a)&0x00002000?13:(a)&0x00004000?14:(a)&0x00008000?15:\
                  (a)&0x00010000?16:(a)&0x00020000?17:(a)&0x00040000?18:(a)&0x00080000?19:\
                  (a)&0x00100000?20:(a)&0x00200000?21:(a)&0x00400000?22:(a)&0x00800000?23:\
                  (a)&0x01000000?24:(a)&0x02000000?25:(a)&0x04000000?26:(a)&0x08000000?27:\
                  (a)&0x10000000?28:(a)&0x20000000?29:(a)&0x40000000?30: 31)

#define SetWord8(a,d)       (* ( volatile uint8*) (a)=(d) )
#define SetWord16(a,d)      (* ( volatile uint16*)(a)=(d) )
#define SetWord32(a,d)      (* ( volatile uint32*)(a)=(d) )
#define SetWord64(a,d)      (* ( volatile uint64*)(a)=(d) )

#define GetWord8(a)         (* ( volatile uint8*) (a) )
#define GetWord16(a)        (* ( volatile uint16*)(a) )
#define GetWord32(a)        (* ( volatile uint32*)(a) )
#define GetWord64(a)        (* ( volatile uint64*)(a) )

/* Aliases for backwards compatibility (only the Byte versions). */
#define SetByte(a,d)        SetWord8((a),(d))
#define GetByte(a)          GetWord8((a))

#define SetBits16(a,f,d)    ( SetWord16( (a), (GetWord16(a)&(~(uint16)(f))) | (((uint16)(d))<<SHIF16((f))) ))
#define SetBits32(a,f,d)    ( SetWord32( (a), (GetWord32(a)&(~(uint32)(f))) | (((uint32)(d))<<SHIF32((f))) ))

//#define GetBits(a,f)      (( ( volatile struct __##a *)(a))->BITFLD_##f )

#define GetBits16(a,f)  ( (GetWord16(a)&( (uint16)(f) )) >> SHIF16(f) )
#define GetBits32(a,f)  ( (GetWord32(a)&( (uint32)(f) )) >> SHIF32(f) )


/*===========================================================================*/ 
/*=========================== End of footer part ============================*/ 
/*===========================================================================*/ 



// _IODA14580_INCLUDED
#endif
