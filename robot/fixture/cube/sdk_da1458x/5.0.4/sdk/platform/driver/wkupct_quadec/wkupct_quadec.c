/**
 ****************************************************************************************
 *
 * @file wucpt_quadec.c
 *
 * @brief Wakeup IRQ & Quadrature Decoder driver.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/* Important note: If, upon reception of interrupt from the wakeup timer or the quadrature
 *                  decoder, the system resumes from sleep mode and you wish to resume peripherals
 *                  functionality , it is necessary to include in your interrupt handler function(s)
 *                  - the ones you register using wkupct_register_callback() and/or
 *                  quad_decoder_register_callback() - the following lines:
 *
 *                  // Init System Power Domain blocks: GPIO, WD Timer, Sys Timer, etc.
 *                  // Power up and init Peripheral Power Domain blocks,
 *                  // and finally release the pad latches.
 *                  if(GetBits16(SYS_STAT_REG, PER_IS_DOWN))
 *                      periph_init();
 *
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdio.h>
#include "global_io.h"
#include "ARMCM0.h"
#include "syscntl.h"
#include "wkupct_quadec.h"

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

#ifdef USE_ARCH_WKUPCT_DEB_TIME
extern uint16_t arch_wkupct_deb_time;
#endif

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

wkupct_quad_IRQ_status_t wkupct_quad_IRQ_status __attribute__((section("retention_mem_area0"), zero_init));
static uint32_t* QUADDEC_callback               __attribute__((section("retention_mem_area0"), zero_init)); // Quadrature decoder handler callback
static void* WKUPCT_callback                    __attribute__((section("retention_mem_area0"), zero_init)); // Wakeup handler callback

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

wkupct_quadec_error_t wkupct_quad_disable_IRQ(void)
{
    if (wkupct_quad_IRQ_status.reservation_status == RESERVATION_STATUS_FREE)
    {
        NVIC_DisableIRQ(WKUP_QUADEC_IRQn);
        return WKUPCT_QUADEC_ERR_OK;
    }
    return WKUPCT_QUADEC_ERR_RESERVED;
}

void WKUP_QUADEC_Handler(void)
{
    int16_t x,y,z;
    wakeup_handler_function_t wakeupHandlerFunction;
    quad_encoder_handler_function_t quadEncoderHandlerFunction;

    // Restore clock
    syscntl_use_highest_amba_clocks();

    // Note: in case of simultaneous triggering of quadrature decoder and wakeup timer, the quadrature decoder interrupt will be handled.
    if ((GetBits16(CLK_PER_REG, QUAD_ENABLE) != 0) && (GetBits16(QDEC_CTRL_REG, QD_IRQ_STATUS) !=0 ))
    {// The Quadrature Decoder clock is enabled & the Quadrature Decoder interrupt has triggered
        if (wkupct_quad_IRQ_status.reservations.reserved_by_quad == true)
        {// The Quadrature Decoder has been registered as interrupt source
            SetBits16(QDEC_CTRL_REG, QD_IRQ_CLR, 1);  // write 1 to clear Quadrature Decoder interrupt
            SetBits16(QDEC_CTRL_REG, QD_IRQ_MASK, 0); // write 0 to mask the Quadrature Decoder interrupt
            if (QUADDEC_callback != NULL) // Quadrature Decoder callback function has been registered by the application
            {
                x = quad_decoder_get_x_counter();
                y = quad_decoder_get_y_counter();
                z = quad_decoder_get_z_counter();
                quadEncoderHandlerFunction = (quad_encoder_handler_function_t)(QUADDEC_callback);
                quadEncoderHandlerFunction(x,y,z);
            }
        }
        else
        {//ERROR (configuration by direct access to registers, not through the API)
             //   ASSERT_ERROR(0);
        }
    }
    else
    {
        // since the interrupt does not come from the Quadrature controller, it is from the wakeup timer
        if  ((GetBits16(CLK_PER_REG, WAKEUPCT_ENABLE) != 0))
        {
            if (wkupct_quad_IRQ_status.reservations.reserved_by_wkupct == true)
            {
                SetWord16(WKUP_RESET_IRQ_REG, 1); // Acknowledge it
                SetBits16(WKUP_CTRL_REG, WKUP_ENABLE_IRQ, 0); // No more interrupts of this kind
                if (WKUPCT_callback != NULL)
                {
                    wakeupHandlerFunction = (wakeup_handler_function_t)(WKUPCT_callback);
                    wakeupHandlerFunction();
                }
            }
            else
            {//ERROR (configuration by direct access to registers, not through the API)
             //   ASSERT_ERROR(0);
            }
        }
        else
        {//ERROR (should never end-up here)
            //ASSERT_ERROR(0);
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
////
////  WakeUP Capture Timer
////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void wkupct_enable_irq(uint32_t sel_pins, uint32_t pol_pins, uint16_t events_num, uint16_t deb_time)
{
    uint8_t temp;
    
    #ifdef USE_ARCH_WKUPCT_DEB_TIME
    arch_wkupct_deb_time = deb_time;                                // Store value in retention memory
    #endif
    
    SetBits16(CLK_PER_REG, WAKEUPCT_ENABLE, 1);                     // enable clock of Wakeup Controller

    SetWord16(WKUP_RESET_CNTR_REG, 0);                              // Clear event counter (for safety...)
    SetWord16(WKUP_COMPARE_REG, (events_num & 0xFF));               // Wait for 1 event and wakeup

    for(int i = 0; i < 4; ++i)
    {
        temp = (uint8_t)((sel_pins >> (8 * i)) & 0xFF);
        SetWord16(WKUP_SELECT_P0_REG + (2 * i), temp);
        temp = (uint8_t)((pol_pins >> (8 * i)) & 0xFF);
        SetWord16(WKUP_POL_P0_REG + (2 * i), temp);
    }

    // Set P28, P29 configuration
    SetBits16(WKUP_SELECT_P2_REG, (3<<8), (sel_pins>>14)&3);
    SetBits16(WKUP_POL_P2_REG, (3<<8), (pol_pins>>14)&3);

    SetWord16(WKUP_RESET_IRQ_REG, 1);                               // clear any garbagge
    NVIC_ClearPendingIRQ(WKUP_QUADEC_IRQn);                         // clear it to be on the safe side...

    SetWord16(WKUP_CTRL_REG, 0x80 | (deb_time & 0x3F));             // Setup IRQ: Enable IRQ, T ms debounce
    NVIC_SetPriority(WKUP_QUADEC_IRQn, 1);                          // Set irq Priority to 1
    wkupct_quad_IRQ_status.reservations.reserved_by_wkupct = true;  // Wakeup Timer registers IRQ use
    NVIC_EnableIRQ(WKUP_QUADEC_IRQn);
}

wkupct_quadec_error_t wkupct_disable_irq()
{
    wkupct_quad_IRQ_status.reservations.reserved_by_wkupct = false;         // Wakeup Timer unregisters IRQ use

    SetWord16(WKUP_RESET_IRQ_REG, 1); //Acknowledge it
    SetBits16(WKUP_CTRL_REG, WKUP_ENABLE_IRQ, 0);                           //No more interrupts of this kind

    return wkupct_quad_disable_IRQ();
}

void wkupct_register_callback(wakeup_handler_function_t callback)
{
    WKUPCT_callback = callback;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
////
////  Quadrature Decoder
////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void quad_decoder_register_callback(uint32_t* callback)
{
    QUADDEC_callback = callback;
}

void quad_decoder_init(QUAD_DEC_INIT_PARAMS_t *quad_dec_init_params)
{
   // Enable the Quadrature clock
    SetBits16(CLK_PER_REG, QUAD_ENABLE , true);
   // Setup Quadrature Decoder pin assignment
    SetWord16(QDEC_CTRL2_REG, quad_dec_init_params->chx_port_sel | quad_dec_init_params->chy_port_sel | quad_dec_init_params->chz_port_sel);
    SetWord16(QDEC_CLOCKDIV_REG, quad_dec_init_params->qdec_clockdiv);
}

void quad_decoder_release(void)
{
   // Reset Quadrature Decoder pin assignment (PLEASE REVIEW)
    SetWord16(QDEC_CTRL2_REG, QUAD_DEC_CHXA_NONE_AND_CHXB_NONE | QUAD_DEC_CHYA_NONE_AND_CHYB_NONE | QUAD_DEC_CHZA_NONE_AND_CHZB_NONE);
   // Disable the Quadrature clock
    SetBits16(CLK_PER_REG, QUAD_ENABLE , false);
    quad_decoder_disable_irq(); //Note: Will disable the IRQ only if Wakeup Timer has also unregistered from using it
}

inline int16_t quad_decoder_get_x_counter(void)
{
    return GetWord16(QDEC_XCNT_REG);
}

inline int16_t quad_decoder_get_y_counter(void)
{
    return GetWord16(QDEC_YCNT_REG);
}

inline int16_t quad_decoder_get_z_counter(void)
{
    return GetWord16(QDEC_ZCNT_REG);
}

void quad_decoder_enable_irq(uint8_t event_count)
{
    SetBits16(QDEC_CTRL_REG, QD_IRQ_CLR, 1);                               // clear any garbagge
    NVIC_ClearPendingIRQ(WKUP_QUADEC_IRQn);                                // clear it to be on the safe side...

    SetBits16(QDEC_CTRL_REG, QD_IRQ_THRES, event_count);                   // Set event counter
    SetBits16(QDEC_CTRL_REG, QD_IRQ_MASK, 1);                              // interrupt not masked
    NVIC_SetPriority(WKUP_QUADEC_IRQn, 1);                                 // Set irq Priority to 1
    wkupct_quad_IRQ_status.reservations.reserved_by_quad = true;           // IRQ freeing reserved by Quadrature Decoder
    NVIC_EnableIRQ(WKUP_QUADEC_IRQn);                                      // enable the WKUP_QUADEC_IRQn
}

wkupct_quadec_error_t quad_decoder_disable_irq(void)
{
    wkupct_quad_IRQ_status.reservations.reserved_by_quad = false;          // Quadrature Decoder unregisters IRQ use

    SetBits16(QDEC_CTRL_REG, QD_IRQ_CLR, 1);  // write 1 to clear Quadrature Decoder interrupt
    SetBits16(QDEC_CTRL_REG, QD_IRQ_MASK, 0); // write 0 to mask the Quadrature Decoder interrupt

    return wkupct_quad_disable_IRQ();
}
