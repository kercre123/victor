 /**
 ****************************************************************************************
 *
 * @file app_audio439.c
 *
 * @brief Code related with the SC14439 control.
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

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#if HAS_AUDIO

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
    #include "app_audio439.h"
    #include "app_audio_codec.h"
    #include "arch_api.h"
    #include "audio_test.h"
    #include "pwm.h"
    #include "rwip_config.h"               // SW configuration
    #include "spi_439.h"


    #define CLICK_STARTUP_CLEAN     //enable the DC_BLOCK filter and drop of packages

    #ifdef CFG_SPI_439_BLOCK_BASED
        #define AUDIO439_SYSTICK_TIME 40000   // 40000 is 16 Khz/40 samples, NOTE, you must set systick to N-1 to get it exactly every N cycles
    #else
        #define AUDIO439_SYSTICK_TIME 999    // 1000 is 16 Khz/1 samples. NOTE, you must set systick to N-1 to get it exactly every N cycles
    #endif

    #ifdef CLICK_STARTUP_CLEAN
        #define DROP_PACKAGES_NO 25             //how many packages to drop
        #define DC_BLOCK_PACKAGES_START 10      //when to start DC_BLOCK calculation
        #define DC_BLOCK_PACKAGES_STOP 150      //when to stop DC_BLOCK calculation
        int click_packages;
    #endif

bool app_audio439_timer_started = false;
bool stop_when_buffer_empty = false;
t_app_audio439_env app_audio439_env;
uint8_t session_swtim_ints = 0; //count the first two ticks of the timer.

    #ifdef CFG_AUDIO439_ADAPTIVE_RATE
        #define _RETAINED __attribute__((section("retention_mem_area0"), zero_init))
volatile app_audio439_ima_mode_t app_audio439_imamode _RETAINED;
volatile int app_audio439_imaauto _RETAINED;
int app_audio439_imacnt;
int app_audio439_imatst = 0;
    #endif

void declare_audio439_gpios(void)
{                                 
    #ifdef HAS_AUDIO_MUTE
    RESERVE_GPIO(AUDIO_MUTE, AUDIO_MUTE_PORT, AUDIO_MUTE_PIN, PID_439_MUTE_LDO);
    #endif
    #ifdef HAS_AUDIO_VDDIO_CONTROL
    RESERVE_GPIO(AUDIO_VDDIO_CONTROL, AUDIO_VDDIO_CONTROL_PORT, AUDIO_VDDIO_CONTROL_PIN, PID_AUDIO_MUTE);
    #endif
    RESERVE_GPIO(CLK_16MHZOUT,   GPIO_PORT_0,        GPIO_PIN_5,         PID_16MHZ_CLK);
    RESERVE_GPIO(CLK_16MHZOUT,   GPIO_PORT_0,        GPIO_PIN_6,         PID_16MHZ_CLK);
    RESERVE_GPIO(CLK_16MHZOUT,   GPIO_PORT_0,        GPIO_PIN_7,         PID_16MHZ_CLK);
    RESERVE_GPIO(CLK_16MHZOUT,   GPIO_PORT_1,        GPIO_PIN_0,         PID_16MHZ_CLK);
    RESERVE_GPIO(AUDIO_SPI_EN,   AUDIO_SPI_EN_PORT,  AUDIO_SPI_EN_PIN,   PID_AUDIO_SPI_EN);  
    RESERVE_GPIO(AUDIO_SPI_CLK,  AUDIO_SPI_CLK_PORT, AUDIO_SPI_CLK_PIN,  PID_AUDIO_SPI_CLK); 
    RESERVE_GPIO(AUDIO_SPI_DO,   AUDIO_SPI_DO_PORT,  AUDIO_SPI_DO_PIN,   PID_AUDIO_SPI_DO);  
    RESERVE_GPIO(AUDIO_SPI_DI,   AUDIO_SPI_DI_PORT,  AUDIO_SPI_DI_PIN,   PID_AUDIO_SPI_DI);  
}
  
void init_audio439_gpios(bool enable_power)   
{                                       
    GPIO_SetPinFunction(AUDIO_SPI_EN_PORT,  AUDIO_SPI_EN_PIN,  INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction(AUDIO_SPI_CLK_PORT, AUDIO_SPI_CLK_PIN, INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction(AUDIO_SPI_DO_PORT,  AUDIO_SPI_DO_PIN,  INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction(AUDIO_SPI_DI_PORT,  AUDIO_SPI_DI_PIN,  INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction(AUDIO_CLK_PORT,     AUDIO_CLK_PIN,     INPUT_PULLDOWN, PID_GPIO);    
    #ifdef HAS_AUDIO_MUTE
    GPIO_SetPinFunction(AUDIO_MUTE_PORT, AUDIO_MUTE_PIN,
                        (enable_power && AUDIO_MUTE_POLARITY == GPIO_ACTIVE_HIGH) ||
                       (!enable_power && AUDIO_MUTE_POLARITY == GPIO_ACTIVE_LOW) ? INPUT_PULLUP
                                                                                 : INPUT_PULLDOWN, PID_GPIO);
    #endif
    #ifdef HAS_AUDIO_VDDIO_CONTROL
    GPIO_SetPinFunction(AUDIO_VDDIO_CONTROL_PORT, AUDIO_VDDIO_CONTROL_PIN,
                        (enable_power && AUDIO_VDDIO_CONTROL_POLARITY == GPIO_ACTIVE_HIGH) ||
                       (!enable_power && AUDIO_VDDIO_CONTROL_POLARITY == GPIO_ACTIVE_LOW) ? INPUT_PULLUP
                                                                                          : INPUT_PULLDOWN, PID_GPIO);
    #endif
}

#ifdef CFG_AUDIO439_IMA_ADPCM
/**
 ****************************************************************************************
 * @brief Set the correct IMA encoding parameters 
 *
 * The ima-mode is stored in global retention value app_audio439_ima_mode;
 * The Data Rate will be:
 * 0: 64 Kbit/s = ima 4Bps, 16 Khz.
 * 1: 48 Kbit/s = ima 3Bps, 16 Khz.
 * 2: 32 Kbit/s = ima 4Bps, 8 Khz (downsample).
 * 3: 24 Kbit/s = ima 3Bps, 8 Khz (downsample).
 ****************************************************************************************
 */
void app_audio439_set_ima_mode(void)
{
    int AUDIO_IMA_SIZE;    
    
    #ifdef CFG_AUDIO439_ADAPTIVE_RATE    
    switch (app_audio439_imamode) {
    case IMA_MODE_24KBPS_3_8KHZ:
    case IMA_MODE_32KBPS_4_8KHZ:
        app_audio439_env.sample_mode = 1;
        break;
    case IMA_MODE_48KBPS_3_16KHZ:
    case IMA_MODE_64KBPS_4_16KHZ:
        app_audio439_env.sample_mode = 0;
        break;
    default:
        break;
    }
    app_audio439_env.ima_mode  = app_audio439_imamode;
    app_audio439_imacnt = 0;
    #endif

    #ifdef CFG_AUDIO439_ADAPTIVE_RATE    
    switch (app_audio439_imamode) {
    #else        
    switch (IMA_DEFAULT_MODE) {
    #endif
    case IMA_MODE_24KBPS_3_8KHZ:
    case IMA_MODE_48KBPS_3_16KHZ:
        AUDIO_IMA_SIZE = 3;
        break;
    case IMA_MODE_64KBPS_4_16KHZ:
    case IMA_MODE_32KBPS_4_8KHZ:
    default:
        AUDIO_IMA_SIZE = 4;
        break;
    }

    t_IMAData *ima = &app_audio439_env.imaState;
    ima->imaSize            = AUDIO_IMA_SIZE;
    ima->imaOr              = (1 << (4 - AUDIO_IMA_SIZE)) - 1;
    ima->imaAnd             = 0xF - ima->imaOr;
    ima->len                = 160 / AUDIO_IMA_SIZE; // 20*8/AUDIO_IMA_SIZE - Number of samples needed to encode in one 20 byte packet, after possible downsampling  
    ima->index              = 0;
    ima->predictedSample    = 0;

    app_audio439_env.sbuf_min = ima->len;
}
#endif
    
/**
 ****************************************************************************************
 * @brief Initiliaze the 439 state variable.
 *
 * This function is called at every start of new Audio Command.
 ****************************************************************************************
 */
static void app_audio439_init(void)
{
    app_audio439_env.audio439SlotWrNr   = 0;
    app_audio439_env.audio439SlotRdNr   = 0;
    app_audio439_env.audio439SlotIdx    = 0;
    app_audio439_env.audio439SlotSize   = 0;

    t_audio439_slot *slot = app_audio439_env.audioSlots;
    for (int i = 0; i < AUDIO439_NR_SLOT; i++) {
        slot++->hasData = false;
    }

    app_audio439_env.sbuf_len           = 0;
    app_audio439_env.sbuf_min           = AUDIO439_NR_SAMP;   // Number of samples needed to encode in one 20 byte packet, after possible downsampling
    app_audio439_env.sbuf_avail         = AUDIO439_SBUF_SIZE; // Number of bytes available

    #ifdef DC_BLOCK
    t_DCBLOCKData *dcBlock  = &app_audio439_env.dcBlock;
    dcBlock->len            = AUDIO439_NR_SAMP;
    dcBlock->beta           = APP_AUDIO_DCB_BETA;
    dcBlock->xn1            = 0;
    dcBlock->yyn1           = 0;
    dcBlock->fade_step      = 16;   // about 1000 samples fade-in   
    dcBlock->fcnt           = 25;   // block input for first 25 frames of 40 samples
    #endif

    app_audio439_env.buffer_errors      = 0;
    app_audio439_env.spi_errors         = 0;
    app_audio439_env.errors_sent        = 100;

    #ifdef CFG_AUDIO439_IMA_ADPCM
    t_IMAData *ima = &app_audio439_env.imaState;
    ima->index              = 0;
    ima->predictedSample    = 0;
    app_audio439_set_ima_mode();  // set IMA adpcm encoding parameters
    #endif
}

/**
 ****************************************************************************************
 * @brief Fill work buffer with new packet from SPI439, with optional downsampling
 * This function will add a 439 packet to the work buffer. If downsampling is selective
 * then the a 2x downsampling is performed using FIR filter.
 *
 * @param[in] ptr: pointer to the data in 439-packet.
 *
 * @return void
 ****************************************************************************************
 */
static void app_audio439_fill_buffer(int16_t *ptr)
{
    int16_t *dst = &app_audio439_env.sbuffer[app_audio439_env.sbuf_len];
    int tot = AUDIO439_NR_SAMP;

#ifdef CFG_AUDIO439_ADAPTIVE_RATE    
    if (!app_audio439_env.sample_mode) {
        for (int i = 0; i < AUDIO439_NR_SAMP; i++) {
            *dst++ = *ptr++;
        }
    } else {
        audio439_downSample(AUDIO439_NR_SAMP, ptr, dst, app_audio439_env.FilterTaps);
        tot = AUDIO439_NR_SAMP / 2;
    }
#elif IMA_DEFAULT_MODE != 3 && IMA_DEFAULT_MODE != 4
    for (int i = 0; i < AUDIO439_NR_SAMP; i++) {
        *dst++ = *ptr++;
    }
#else
    audio439_downSample(AUDIO439_NR_SAMP, ptr, dst, app_audio439_env.FilterTaps);
    tot = AUDIO439_NR_SAMP / 2;
#endif        

    app_audio439_env.sbuf_len += tot;
    app_audio439_env.sbuf_avail -= tot;
}

/**
 ****************************************************************************************
 * @brief Drop the next package from the FIFO.
 *
 * @param  None
 *
 * @return void
 ****************************************************************************************
 */
static void app_audio439_next_package(void)
{
    app_audio439_env.audioSlots[app_audio439_env.audio439SlotRdNr].hasData = false;
    app_audio439_env.audio439SlotRdNr++;
    if (app_audio439_env.audio439SlotRdNr >= AUDIO439_NR_SLOT) {
        app_audio439_env.audio439SlotRdNr = 0;
    }
    app_audio439_env.audio439SlotSize--;
}

/**
 ****************************************************************************************
 * @brief Remove len samples from the work buffer. 
 * The len samples to be removed are always at the start of the buffer. Shift the complete
 * buffer, and update the sbuf_len and sbuf_avail states.
 *
 * @param[in] len: number of samples to remove
 *
 * @return void
 ****************************************************************************************
 */
static void app_audio439_empty_buffer(int len)
{
    app_audio439_env.sbuf_len -= len;
    app_audio439_env.sbuf_avail += len;

    int16_t *src = app_audio439_env.sbuffer+len;
    int16_t *dst = app_audio439_env.sbuffer;
    /* Move the buffer.. */
    for (int i = 0; i < app_audio439_env.sbuf_len; i++) {
        *dst++ = *src++;
    }   
}

/**
 ****************************************************************************************
 * @brief Encode the audio, read one packet from buffer, encode and store in stream buffer
 * This function is the interface between the raw sample packets from the 439 (stored in
 * spi439 fifo in groups of 40 samples) and streaming packets (with compressed audio) which
 * are currently 20 bytes (but this could change).
 * The function uses an internal working buffer (app_audio439_env.sbuffer) for this purpose.
 * - if sbuffer has enough space, add spi439 sample block.
 * - if sbuffer has enough samples, compress a block of samples to make one HID stream packet.
 *   The amount of samples needed for one HID packet depends on the IMA compression mode
 *   selected. Default 4 bits/sample = 40 samples makes 20 bytes. For 3 bits/sample, we 
 *   need 50 samples (5 IMA codes will occupy 2 bytes).
 * 
 * @param[in] maxNr maximum number of packets to encode
 *
 * @return void
 ****************************************************************************************
 */
void app_audio439_encode(int maxNr)
{
    while (maxNr--) {
        if (app_audio439_env.audioSlots[app_audio439_env.audio439SlotRdNr].hasData == true) {
#ifdef DC_BLOCK
        /* 
        ** Additional DC Blocking Filter 
        ** It will do the whole block and stores output inplace (same array as input).
        */
#ifdef CLICK_STARTUP_CLEAN
            if (click_packages > DC_BLOCK_PACKAGES_START)
#endif
            {
                app_audio439_env.dcBlock.inp = &app_audio439_env.audioSlots[app_audio439_env.audio439SlotRdNr].samples[AUDIO439_SKIP_SAMP];
                app_audio439_env.dcBlock.out = app_audio439_env.dcBlock.inp;  /* IN-PLACE operation !! */
                app_audio_dcblock(&app_audio439_env.dcBlock);
            }
#endif   
#ifdef CLICK_STARTUP_CLEAN            
            if (click_packages < DROP_PACKAGES_NO) {
                //if DROP_PACKAGES_NO>DC_BLOCK_PACKAGES_START DC-blocking updated but not used
                click_packages++;
                app_audio439_next_package();
                continue;
            } else if (click_packages < DC_BLOCK_PACKAGES_STOP) {
                click_packages++;
            }      
#endif
            /* Now add the sample block to our SBuffer */
            app_audio439_fill_buffer(&app_audio439_env.audioSlots[app_audio439_env.audio439SlotRdNr].samples[AUDIO439_SKIP_SAMP]);
            app_audio439_next_package();
        }
    
        /*
        ** Check if we have enough samples in the Sbuffer.
        ** sbuf_min is number of samples needed for encoding 20 output bytes (one ble packet).
        ** For IMA-ADPCM @ 8/16 Khz, this value is 40. For IMA-ADPCM 3 bits, this is 52.
        */
        if (app_audio439_env.sbuf_len >= app_audio439_env.sbuf_min) {
            audio_send(app_audio439_env.sbuffer);
            app_audio439_empty_buffer(app_audio439_env.sbuf_min);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Local function for stoping the Timer
 ****************************************************************************************
 */
static void swtim_stop(void)
{
    timer0_stop();
}

/**
 ****************************************************************************************
 * @brief Systick Handler, Interrupt handler to fetch the audio samples from the 439 over SPI
 * 
 * This function will either run at 16 Khz for Sample Based processing, 
 * or at 16Khs/40, to read block of data (40) samples from 439.
 ****************************************************************************************
 */
void SWTIM_Callback(void)
{
    #ifdef CFG_SPI_439_SAMPLE_BASED
    app_audio439_env.audio439SlotIdx++;
    if (app_audio439_env.audio439SlotIdx == AUDIO439_NR_SAMP) {
        app_audio439_env.audioSlots[app_audio439_env.audio439SlotWrNr].hasData = true;
        app_audio439_env.audio439SlotIdx = 0;
        app_audio439_env.audio439SlotWrNr++;
        if (app_audio439_env.audio439SlotWrNr == AUDIO439_NR_SLOT) {
                app_audio439_env.audio439SlotWrNr = 0;
        }
    }
    spi_439_buf_ptr = (int16_t*)&app_audio439_env.audioSlots[app_audio439_env.audio439SlotWrNr].samples[app_audio439_env.audio439SlotIdx];
    spi_439_get_codec_sample();
    #endif
    
    #ifdef CFG_SPI_439_BLOCK_BASED
    if (session_swtim_ints > 1) {
        //this if statement drops two packages 
        //the first when the session starts is empty and never filled from 439
        //the second package has unkwown data (0-10 samples caused by spi_439_codec_restart and "click" data )
        //the second package is also required to re-allign the DMA read operation of 439 and the timer 
        app_audio439_env.audioSlots[app_audio439_env.audio439SlotWrNr].hasData = true;
        app_audio439_env.audio439SlotWrNr++;
        if (app_audio439_env.audio439SlotWrNr == AUDIO439_NR_SLOT) {
            app_audio439_env.audio439SlotWrNr = 0;
        }
    }
    spi_439_buf_ptr = (int16_t*)&app_audio439_env.audioSlots[app_audio439_env.audio439SlotWrNr].samples;
    spi_439_getblock(app_audio439_env.audio439SlotWrNr);
    app_audio439_env.audio439SlotSize++;    
    #endif
    if (session_swtim_ints < 2) {
        session_swtim_ints++;   //increase this way to avoid overflow.
    }
}
    
/**
 ****************************************************************************************
 * @brief Local function for configuring the Timer
 *
 * @param[in] ticks
 * @param[in] mode
 ****************************************************************************************
 */
static void swtim_configure(const int ticks)
{
    //Enables TIMER0, TIMER2 clock
    set_tmr_enable(CLK_PER_REG_TMR_ENABLED);
    //Sets TIMER0,TIMER2 clock division factor to 8
    set_tmr_div(CLK_PER_REG_TMR_DIV_1);
    
    timer0_disable_irq();
    // initilalize PWM with the desired settings
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_NO_DIV);
    // set pwm Timer0 On, Timer0 'high' and Timer0 'low' reload values
    timer0_set(1000, (ticks/2)-1, (ticks-ticks/2)-1);
    // register callback function for SWTIM_IRQn irq
    timer0_register_callback(SWTIM_Callback);
}

/**
 ****************************************************************************************
 * @brief Local function for starting the Timer
 ****************************************************************************************
 */
static void swtim_start(void)
{
    // start pwm0
    timer0_start();
    while (!NVIC_GetPendingIRQ(SWTIM_IRQn));
    NVIC_ClearPendingIRQ(SWTIM_IRQn);
    // enable SWTIM_IRQn irq
    timer0_enable_irq();
}

/**
 ****************************************************************************************
 * @brief Start the Audio processing from 439
 ****************************************************************************************
 */
void app_audio439_start(void)
{
    stop_when_buffer_empty = false;
    if (app_audio439_timer_started) {
        return;
    }
    if (arch_get_sleep_mode()) {
        arch_force_active_mode();
    }
      
    spi_439_init();         //initialize 439
  
    app_audio439_init();    //clear the state data before setting the timer0

    swtim_configure(AUDIO439_SYSTICK_TIME);   //initialize the timer
    spi_439_codec_restart();    //SET THE DMA TO A GIVEN OFFSET -- May introduce artifacts on the first packet
    swtim_start();              //start the timer
    session_swtim_ints = 0;
    app_audio439_timer_started = true;
    #ifdef CLICK_STARTUP_CLEAN
    click_packages = 0;   //changing logic
    #endif
}

/**
 ****************************************************************************************
 * @brief Stop the Audio processing from 439
 ****************************************************************************************
 */
void app_audio439_stop(void)
{
    swtim_stop();
        
    if (app_audio439_timer_started) {
        arch_restore_sleep_mode();
    }

    app_audio439_timer_started = false;

    #ifdef HAS_AUDIO_VDDIO_CONTROL // Power down the 439
    GPIO_ConfigurePin(AUDIO_VDDIO_CONTROL_PORT, AUDIO_VDDIO_CONTROL_PIN,
                      AUDIO_VDDIO_CONTROL_POLARITY == GPIO_ACTIVE_HIGH ? INPUT_PULLDOWN
                                                                       : INPUT_PULLUP, PID_GPIO, 0);
    #endif

    #ifdef HAS_AUDIO_MUTE // Power down the 439 by setting MUTE_LDO inactive
    GPIO_ConfigurePin(AUDIO_MUTE_PORT, AUDIO_MUTE_PIN,
                      AUDIO_MUTE_POLARITY == GPIO_ACTIVE_HIGH ? INPUT_PULLDOWN
                                                              : INPUT_PULLUP, PID_GPIO, 0);
    #endif

    GPIO_SetPinFunction(AUDIO_SPI_EN_PORT,  AUDIO_SPI_EN_PIN,  INPUT_PULLDOWN, PID_GPIO);
    GPIO_SetPinFunction(AUDIO_SPI_CLK_PORT, AUDIO_SPI_CLK_PIN, INPUT_PULLDOWN, PID_GPIO);
    GPIO_SetPinFunction(AUDIO_SPI_DO_PORT,  AUDIO_SPI_DO_PIN,  INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction(AUDIO_SPI_DI_PORT,  AUDIO_SPI_DI_PIN,  INPUT_PULLDOWN, PID_GPIO);

    SetWord32(TEST_CTRL_REG, 0);
    GPIO_SetPinFunction(AUDIO_CLK_PORT, AUDIO_CLK_PIN, INPUT_PULLDOWN, PID_GPIO);
}

#endif // HAS_AUDIO
 
/// @} APP
