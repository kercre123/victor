/**
 ****************************************************************************************
 *
 * @file spi_439.h
 *
 * @brief spi 439 interface driver header file.
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
#ifndef _SPI_439_H_
#define _SPI_439_H_

#ifdef CFG_SPI_439
#define SPI_439_DRIVER_VERSION (2)
#define SPI_439_DRIVER_SUBVERSION (0)

// SC14439 REGISTERS
#define SC14439_BAT_CTRL_REG			(0x1212) 
#define SC14439_GPIO_DATA_REG			(0x1220)  /* GPIO DATA INPUT/OUTPUT REGISTER */
#define SC14439_GPIO_SET_DATA_REG		(0x1222)  /* GPIO SET PORT PINS REGISTER */
#define SC14439_GPIO_RESET_DATA_REG		(0x1224)  /* GPIO RESET PORT PINS REGISTER */
#define SC14439_GPIO_DIR_REG			(0x1226)  /* GPIO DIRECTION REGISTER */
#define SC14439_GPIO_MODE_REG			(0x1228)  /* GPIO MODE REGISTER */
#define SC14439_GPIO_PUPD_REG			(0x122A)  /* GPIO PULL-UP PULL-DOWN REGISTER */
#define SC14439_GPIO_PF_DEBOUNCE_REG	(0x122C)  /* GPIO DEBOUNCE CONTROL REGISTER */
    
#define SC14439_CLASSD_CTRL_REG			(0x1230)  /* CLASSD CONTROL REGISTER */
#define SC14439_CLASSD_CLEAR_INT_REG	(0x1232)  /* CLASSD CLEAR INTERRUPT REGISTER */
#define SC14439_CLASSD_BUZZER_REG		(0x1234)  /* CLASSD BUZZER REGISTER */
#define SC14439_CLASSD_TEST_REG			(0x1236)  /* CLASSD TEST REGISTER */
   	 
#define SC14439_DMA0_A_STARTL_REG		(0x1250)  /* START ADDRESS LOW A OF DMA CHANNEL 0 */
#define SC14439_DMA0_A_IDX_REG			(0x1254)  /* DMA RECEIVE INDEX A OF DMA CHANNEL 0 */
#define SC14439_DMA0_B_STARTL_REG		(0x1256)  /* START ADDRESS LOW B OF DMA CHANNEL 0 */
#define SC14439_DMA0_B_IDX_REG			(0x125A)  /* DMA RECEIVE INDEX B OF DMA CHANNEL 0 */
#define SC14439_DMA0_LEN_REG			(0x125E)  /* DMA RECEIVE LENGTH REGISTER CHANNEL 0 */
#define SC14439_DMA0_CTRL_REG			(0x1260)  /* CONTROL REGISTER FOR DMA CHANNEL 0    */

#define SC14439_CODEC_MIC_REG			(0x1280)  /* CODEC MICROPHONE CONTROL REGISTER */
#define SC14439_CODEC_LSR_REG			(0x1282)  /* CODEC LOUDSPEAKER CONTROL REGISTER */
#define SC14439_CODEC_VREF_REG			(0x1284)  /* CODEC VREF CONTROL REGISTER */
#define SC14439_CODEC_IN_OUT_REG		(0x1286)  /* CODEC INPUT/OUTPUT REGISTER */
#define SC14439_CODEC_ADDA_REG			(0x1288)  /* CODEC AD/DA CONTROL REGISTER */
#define SC14439_CODEC_OFFSET1_REG		(0x128A)  /* CODEC OFFSET ERROR AND COMPENSATION REGISTER */
#define SC14439_CODEC_TEST_CTRL_REG		(0x128C)  /* CODEC TEST CONTROL REGISTER */
#define SC14439_CODEC_OFFSET2_REG		(0x128E)  /* CODEC OFFSET COMPENSATION REGISTER */

#define SC14439_PCM0_IN_OUT_REG			(0x12B0)  /* PCM CHANNEL 0 INPUT/OUTPUT REGISTER */
#define SC14439_PCM1_IN_OUT_REG			(0x12B2)  /* PCM CHANNEL 1 INPUT/OUTPUT REGISTER */
#define SC14439_PCM_CTRL_REG			(0x12B4)  /* PCM CONTROL REGISTER 1 */
#define SC14439_PCM_CTRL2_REG			(0x12B6)  /* PCM CONTROL REGISTER 2 */

#define SC14439_TAM_CTRL_REG			(0x12D0)  /* TAM CONTROL REGISTER */
#define SC14439_TAM_STATUS_REG			(0x12D2)  /* TAM STATUS REGISTER */

#define SC14439_CLK_CTRL_REG			(0x12E2)  /* PLL CONTROL REGISTER */
#define SC14439_PLL_DIV_REG 			(0x12E4)  /* PLL CONTROL REGISTER */
#define SC14439_PER_DIV_REG				(0x12E6)  /* PERIPHERAL DIVIDER REGISTER */
#define SC14439_CODEC_DIV_REG			(0x12E8)  /* CODEC DIVIDER REGISTER */
#define SC14439_PHASE_INFO_REG			(0x12EA)  /* PHASE DIFFERENCE BETWEEN PCM_FSC AND 8/16KHZ */
#define SC14439_VERSION_REG0			(0x1FFA)  /* Version Register should be 0x34 */
#define SC14439_VERSION_REG1			(0x1FFB)  /* Version Register should be 0x33 */
#define SC14439_VERSION_REG2			(0x1FFC)  /* Version Register should be 0x39 */


#define SC14439_MEM_WR	(0x01)
#define SC14439_MEM_RD	(0x02)
#define SC14439_MEM_BWR (0x03)
#define SC14439_MEM_BRD (0x04)



/**
 ****************************************************************************************
 * @brief SPI_439 initialization. 
 *
 * Should only be called once after power reset of 439, do not call this twice.
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_init(void);

/**
 ****************************************************************************************
 * @brief SPI_439 release
 *
 * Should be called for powering-down 439 and releasing SPI interface.
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_release(void);

//void spi_439_test(void);

/**
****************************************************************************************
* @brief SPI transaction to slave with CS handling (works in all bit modes)
* @param[in] dataToSend: data to send
* @return  data read 
****************************************************************************************
*/
uint32_t spi_439_getword(uint32_t dataToSend);
void spi_439_setword(uint32_t address, uint32_t data);


/**
****************************************************************************************
* @brief SPI 439 Read Audio Sample from 439 CODEC
*
* This functions uses POLLING and is time consuming. 
*
* @return audio data read from the 439
****************************************************************************************
*/
int32_t spi_439_getsample(void);

/* Some global pointers used by app_audio439_task.c */
extern int spi_439_buf_idx;
extern int16_t *spi_439_buf_ptr;

//void spi_439_codec_setup(int16_t *data_ptr);

/**
 ****************************************************************************************
 * @brief Start or Restart the 439 SPI to get it ready for fetching Audio Data
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_codec_restart(void);


#ifdef CFG_SPI_439_SAMPLE_BASED
/**
 ****************************************************************************************
 * @brief Get one codec sample from 439, using Interupt to fetch result
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_get_codec_sample(void);
#endif


#ifdef CFG_SPI_439_BLOCK_BASED
/**
 ****************************************************************************************
 * @brief Start an SPI read block transfer of 40 audio samples from 439
 *
 * Note that we do 32 bits SPI processing, so the 580 SPI thinks it is loading 32 bits values,
 * and the 439 thinks it is doing 2 x 16 bits values.
 * 
 * @return void
 ****************************************************************************************
 */
void spi_439_getblock(int i);
 void spi439_delay (unsigned int delay);
#endif

#endif

#endif // _SPI_439_H_
