/**
 ****************************************************************************************
 *
 * @file app_audio439.h
 *
 * @brief AudioStreamer Application entry point file.
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

#ifndef APP_AUDIO439_H_
#define APP_AUDIO439_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief AudioStreamer Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
    #include "app_audio_codec.h"
    #include "app_audio439_config.h"
    #include "gpio.h"
    
    #if !defined(CFG_AUDIO439_ADAPTIVE_RATE) && !defined(IMA_DEFAULT_MODE)
        #error "IMA_DEFAULT_MODE must be defined"
    #endif

    #ifdef CFG_SPI_439_BLOCK_BASED
        #define AUDIO439_SKIP_SAMP 2
    #else
        #define AUDIO439_SKIP_SAMP 0
    #endif

    #define AUDIO439_NR_SAMP 40
    #define AUDIO439_NR_SLOT   10
    #define AUDIO439_SBUF_SIZE 100

    #ifdef CFG_AUDIO439_IMA_ADPCM
        #define AUDIO_ENC_SIZE (AUDIO439_NR_SAMP / 2)
    #elif defined(CFG_AUDIO439_ALAW)
        #define AUDIO_ENC_SIZE AUDIO439_NR_SAMP
    #else
        #define AUDIO_ENC_SIZE (AUDIO439_NR_SAMP * 2)
    #endif



/**
 ****************************************************************************************
 * Local buffers for storing Audio Samples
 * Audio is fetched in blocks of 40 samples.
 ****************************************************************************************
 */
typedef struct s_audio439 {
    int16_t samples[AUDIO439_NR_SAMP + AUDIO439_SKIP_SAMP];  // Extra dummy value at start of buffer...
    bool hasData;
} t_audio439_slot;

typedef enum {
    IMA_MODE_64KBPS_4_16KHZ = 0,
    IMA_MODE_48KBPS_3_16KHZ = 1,
    IMA_MODE_32KBPS_4_8KHZ  = 2,
    IMA_MODE_24KBPS_3_8KHZ  = 3,
} app_audio439_ima_mode_t;

/*
 * APP_AUDIO439 Env DataStructure
 ****************************************************************************************
 */
typedef  struct s_app_audio439_env
{
    t_audio439_slot audioSlots[AUDIO439_NR_SLOT];
    int audio439SlotWrNr;
    int audio439SlotIdx;
    int audio439SlotRdNr;
    int audio439SlotSize;

    #ifdef CFG_AUDIO439_IMA_ADPCM
        t_IMAData imaState;
        #if defined(CFG_AUDIO439_ADAPTIVE_RATE) || IMA_DEFAULT_MODE == 3 \
            || IMA_DEFAULT_MODE == 4
            int16_t FilterTaps[FILTER_LENGTH];  
        #endif    
        #ifdef CFG_AUDIO439_ADAPTIVE_RATE    
            bool sample_mode;
            app_audio439_ima_mode_t ima_mode;
        #endif    
    #endif // CFG_AUDIO439_IMA_ADPCM

    #ifdef DC_BLOCK
        t_DCBLOCKData dcBlock;
    #endif
    
    unsigned int    errors_sent;
    int             spi_errors;
    int             buffer_errors;

    /* States for internal working buffer, to deal with changing rates */
    int     sbuf_len;
    int     sbuf_min;
    int     sbuf_avail;
    int16_t sbuffer[AUDIO439_SBUF_SIZE];
}t_app_audio439_env;


/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */
extern t_app_audio439_env app_audio439_env;
extern bool app_audio439_timer_started;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Encode the audio 
 *
 * This test function will process all incoming Audio Packets (40 Samples) and 
 * encode them with selected coder (IMA, ALAW, LIN).
 * This function does not return, it runs indefinetly. 
 *
 * @param[in] maxNr
 ****************************************************************************************
 */
void app_audio439_encode(int maxNr);

/**
 ****************************************************************************************
 * @brief Start the Audio processing from 439
 ****************************************************************************************
 */
void app_audio439_start(void);

/**
 ****************************************************************************************
 * @brief Stop the Audio processing from 439
 ****************************************************************************************
 */
void app_audio439_stop(void);

/* RESERVE_GPIO( SC14439_MUTE_LDO,  MUTE_AUDIO_PORT, MUTE_AUDIO_PIN, PID_439_MUTE_LDO);    \
** Reserve the GPIO pins for 16 Mhz clock output.
** Clock is on P0_5, which also prohibits use of P0_6, P0_7, P1_0
*/
void declare_audio439_gpios(void);
  
void init_audio439_gpios(bool enable_power);

/// @} APP

#endif // APP_AUDIO439_H_
