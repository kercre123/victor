/**
 ****************************************************************************************
 *
 * @file trng.c
 *
 * @brief True Random Number Generator.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
/*
Revision History (ver. 1.1)

ver. 1.0  Initial version
ver. 1.1  Modified rfpt_init() , Added save/restore of the TEST_CTRL_REG in trng_acquire()

*/

#include "arch.h"

#if (USE_TRNG)
#include <stdlib.h>
#include "global_io.h"
#include "gpio.h"
#include "datasheet.h"
#include "arch_wdg.h"

bool DCoffsetCalibration(void);
bool IffCalibration(void);

// RFPT acquisition definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define NUM_POINTS 					0x200						// Number of points per acquisition
#define NUM_POINTS_MUL_2_M_4 		NUM_POINTS*2-4	// Number of iters to run the mean_calc loop

// Bul def...
#define WAIT100() for (wait=0; wait < 0x91; wait++) {}                              // wait about 100 us


/**
 ****************************************************************************************
 * @brief Initialize the rfpt
 ****************************************************************************************
 */
extern void rf_regs(void);
void rfpt_init(void)
{
    volatile uint16  wait;

	SetWord16(CLK_RADIO_REG, 0x0089);                          // En BLE clk, dis BLE timer, En RFCU, RFCU div by 2
	SetWord16(BLE_CNTL2_REG, 0x2000);                          //
	SetBits16 (CLK_AMBA_REG, OTP_ENABLE, 1);
	SetBits16 (CLK_AMBA_REG, HCLK_DIV, 0);
    SetBits16 (TEST_CTRL_REG, ENABLE_RFPT, 1);
     // Recommended settings
    //set_recommended_settings();
    rf_regs();

    IffCalibration();
    DCoffsetCalibration();
    WAIT100();                                  // Wait 100 us to finish all DCF signals
}

/**
 ****************************************************************************************
 * @brief Override RF preferred settings for TRNG mode
 ****************************************************************************************
 */
void trng_init(void)
{
    volatile uint16  wait;

	// Override preferred settings for TRNG mode
	SetWord16(RF_ENABLE_CONFIG1_REG, 	0x0000); 	// LNA off
	SetWord16(RF_ENABLE_CONFIG2_REG, 	0x0000); 	// Mixer off
	SetWord16(RF_DC_OFFSET_CTRL1_REG, 0x8080); 	// Fixed DC offset compensation values for I and Q
	SetWord16(RF_DC_OFFSET_CTRL2_REG, 0x01D1); 	// Use the manual DC offset compensation values
	SetWord16(RF_ENABLE_CONFIG4_REG, 	0x0000); 	// VCO_LDO_EN=0, MD_LDO_EN=0. You need this for more isolation from the RF input
	// SetWord16(RF_ENABLE_CONFIG14_REG, 	0x0000); 	// LOBUF_RXIQ_EN=0, DIV2_EN=0. This increases the noise floor for some reason. So you get more entropy. Need to understand it and then decide...
	// SetWord16(RF_SPARE1_REG, 					0x0001);	// Set the IFF in REAL transfer function, to remove I-Q correlation. But it affects the DC offsets!
	SetWord16(RF_AGC_CTRL2_REG, 	0x00C3); 			// AGC=0 i.e. max RX gain

	// Overrule-start the RX
	SetWord16(RF_OVERRULE_REG,0x8);   	// Start up receiver
	WAIT100();													// Wait for the DCFs to finish
	WAIT100();													// Wait for the DCFs to finish
	WAIT100();													// Wait for the DCFs to finish
}

/**
 ****************************************************************************************
 * @brief  Acquires the raw RFADC IQ samples
 ****************************************************************************************
 */
void trng_get_raw_data(uint32 address, uint32 length)
{
    SetWord16(RFPT_ADDR_REG, address);
    SetWord16(RFPT_LEN_REG, length);
    SetWord16(RFPT_CTRL_REG, 0x1);

    while ((GetWord16(RFPT_STAT_REG) & RFPT_ACTIVE)== RFPT_ACTIVE);
}

 /**
 ****************************************************************************************
 * @brief  Acquires 128 bits, saves them in trng_bits[]and restores the modified regs
 ****************************************************************************************
 */
void trng_acquire(uint8_t *trng_bits_ptr)
{
	uint16_t i,i_acq;
 	uint16_t save_TEST_CTRL_REG, save_RF_ENABLE_CONFIG1_REG,save_RF_ENABLE_CONFIG2_REG,save_RF_DC_OFFSET_CTRL1_REG,save_RF_DC_OFFSET_CTRL2_REG;
	uint16_t save_RF_ENABLE_CONFIG4_REG,save_RF_ENABLE_CONFIG14_REG,save_RF_SPARE1_REG,save_RF_AGC_CTRL2_REG;
 	uint8_t		bit_cnt, rnd_byte, single_rnd_bit;
	uint16_t 	byte_idx;
    uint8_t rfadc_data[NUM_POINTS*2];
  	uint8_t * vq_uint8=(uint8_t *)&rfadc_data[0]; 	// The Q RFADC channel
	uint8_t * vi_uint8=(uint8_t *)&rfadc_data[2];   // The I RFADC channel

    save_TEST_CTRL_REG =GetWord16(TEST_CTRL_REG);

    rfpt_init();		// Init system and radio, set pref settings, and perform radio calibrations. Do not Radio Overrule yet...

	// Implement Save-Modify-Restore for the prefered settings that will be changed for the TRNG mode
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	save_RF_ENABLE_CONFIG1_REG	= GetWord16(RF_ENABLE_CONFIG1_REG); 	// LNA off
	save_RF_ENABLE_CONFIG2_REG	= GetWord16(RF_ENABLE_CONFIG2_REG); 	// Mixer off
	save_RF_DC_OFFSET_CTRL1_REG	= GetWord16(RF_DC_OFFSET_CTRL1_REG); // Fixed DC offset compensation values for I and Q
	save_RF_DC_OFFSET_CTRL2_REG	= GetWord16(RF_DC_OFFSET_CTRL2_REG); // Use the manual DC offset compensation values
	save_RF_ENABLE_CONFIG4_REG	= GetWord16(RF_ENABLE_CONFIG4_REG); 	// VCO_LDO_EN=0, MD_LDO_EN=0. You need this for more isolation from the RF input
	save_RF_ENABLE_CONFIG14_REG	= GetWord16(RF_ENABLE_CONFIG14_REG); // LOBUF_RXIQ_EN=0, DIV2_EN=0. This increases the noise floor for some reason. So you get more entropy. Need to understand it and then decide...
	save_RF_SPARE1_REG			= GetWord16(RF_SPARE1_REG);					// Set the IFF in REAL transfer function, to remove I-Q correlation. But it affects the DC offsets!
	save_RF_AGC_CTRL2_REG		= GetWord16(RF_AGC_CTRL2_REG); 			// AGC=0 i.e. max RX gain

	// Configure radio for TRNG mode (modifies some pref settings, starts RX in overrule)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	trng_init();
	// Start acquiring raw IQ RFADC data and then extract the random bits
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte_idx=0;
	for (i_acq=0; i_acq < 128/(NUM_POINTS*2/16);	i_acq++)
	{
		trng_get_raw_data((uint32)&rfadc_data[0], NUM_POINTS/2-1);	// acquires the raw RFADC IQ samples

		bit_cnt=0;
		rnd_byte=0;

		for (i=0;i<=NUM_POINTS_MUL_2_M_4;i=i+16)
		{
			single_rnd_bit = (vq_uint8[i] & 0x01)  ^  (vi_uint8[i] & 0x01) ; // This way it can pass ALL the NIST tests! This solves a small bias in 1s or 0s which appears due to the actual value of the DC offset...

			rnd_byte= rnd_byte | (single_rnd_bit<<bit_cnt++);
			if(bit_cnt==8)
			{
				trng_bits_ptr[byte_idx++] = rnd_byte;
				bit_cnt=0;
				rnd_byte=0;
			}
        if (USE_WDOG)
            wdg_reload(WATCHDOG_DEFAULT_PERIOD);          // Reset WDOG! 200 * 10.24ms active time for normal mode!
		}
	}

	// Restore the modified regs
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
   SetWord16(TEST_CTRL_REG,  save_TEST_CTRL_REG);

	SetWord16(RF_OVERRULE_REG,0x0);
	SetWord16(RF_ENABLE_CONFIG1_REG, 	save_RF_ENABLE_CONFIG1_REG); 	// LNA off
	SetWord16(RF_ENABLE_CONFIG2_REG, 	save_RF_ENABLE_CONFIG2_REG); 	// Mixer off
	SetWord16(RF_DC_OFFSET_CTRL1_REG, save_RF_DC_OFFSET_CTRL1_REG); // Fixed DC offset compensation values for I and Q
	SetWord16(RF_DC_OFFSET_CTRL2_REG, save_RF_DC_OFFSET_CTRL2_REG);	// Use the manual DC offset compensation values
	SetWord16(RF_ENABLE_CONFIG4_REG, 	save_RF_ENABLE_CONFIG4_REG); 	// VCO_LDO_EN=0, MD_LDO_EN=0. You need this for more isolation from the RF input
	SetWord16(RF_ENABLE_CONFIG14_REG, save_RF_ENABLE_CONFIG14_REG); // LOBUF_RXIQ_EN=0, DIV2_EN=0. This increases the noise floor for some reason. So you get more entropy. Need to understand it and then decide...
	SetWord16(RF_SPARE1_REG, 					save_RF_SPARE1_REG);					// Set the IFF in REAL transfer function, to remove I-Q correlation. But it affects the DC offsets!
	SetWord16(RF_AGC_CTRL2_REG, 			save_RF_AGC_CTRL2_REG); 			// AGC=0 i.e. max RX gain

	SetBits16 (CLK_AMBA_REG, OTP_ENABLE, 0);
    
}
#else
void trng_acquire(uint8_t *trng_bits_ptr)
{
    return;
}
#endif  //USE_TRNG
