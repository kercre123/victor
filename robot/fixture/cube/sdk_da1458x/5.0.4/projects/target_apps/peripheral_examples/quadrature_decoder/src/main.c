/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Quadrature decoder example for DA14580/581 SDK
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
#include <stdio.h>

#include "global_io.h"
#include "user_periph_setup.h"
#include "wkupct_quadec.h"
#include "gpio.h"
#include "common_uart.h"

void system_init(void);
void quad_decoder_test(void);

uint8_t terminate_quad_test;
uint8_t quad_started = 0;
uint8_t qdec_xcnt;
uint8_t qdec_ycnt;
uint8_t qdec_zcnt;
uint8_t qdec_new_data = false;

/**
 ****************************************************************************************
 * @brief  Main routine of the quadrature decoder example
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    quad_decoder_test();
    while(1);
}
/**
 ****************************************************************************************
 * @brief System Initiialization 
 *
 * 
 ****************************************************************************************
 */
void system_init(void)
{
    SetWord16(CLK_AMBA_REG, 0x00);                 // set clocks (hclk and pclk ) 16MHz
    SetWord16(SET_FREEZE_REG,FRZ_WDOG);            // stop watch dog    
    SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);        // open pads
    SetBits16(SYS_CTRL_REG,DEBUGGER_ENABLE,1);     // open debugger
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);       // exit peripheral power down
}
 

/**
 ****************************************************************************************
 * @brief  Quadrature Decoder callback function
 * 
 ****************************************************************************************
 */ 
void quad_decoder_user_callback_function(int16_t qdec_xcnt_reg, int16_t qdec_ycnt_reg, int16_t qdec_zcnt_reg)
{ 
    qdec_xcnt = qdec_xcnt_reg;
    qdec_ycnt = qdec_ycnt_reg;
    qdec_zcnt = qdec_zcnt_reg;
    qdec_new_data = true;

    quad_decoder_enable_irq(1);
}
 
/**
 ****************************************************************************************
 * @brief  WKUP callback function
 * 
 ****************************************************************************************
 */ 
void wkup_callback_function(void)
{
    if(!(GetWord16(P0_DATA_REG) & (1 << 6)))
    {
        terminate_quad_test = true;
    }
    
    if(!(GetWord16(P1_DATA_REG) & (1 << 1)))
    {
        quad_started ^= true;
    }
    
    wkupct_enable_irq(0x240, 0x240, 1, 0);

    return;
}
 
/**
 ****************************************************************************************
 * @brief  Quadrature Decoder test function
 * 
 ****************************************************************************************
 */
void quad_decoder_test(void)
{
    int16_t qdec_xcnt_reg;

    //Due to the fact that P06(PWM2_PORT/PIN) is also used by another test (TIMER2 test) in this h/w setup,
    //we need to ensure that it is configured correctly for this test
    GPIO_ConfigurePin(WKUP_TEST_BUTTON_1_PORT, WKUP_TEST_BUTTON_1_PIN, INPUT_PULLUP, PID_GPIO, true);

    printf_string("Quadrature Decoder / WKUP controller\n\r");
    printf_string("K1 button to start/stop Quadec polling\n\r");
    printf_string("K2 button to terminate Quadec test\n\r");
    
    printf_string("\n\r Quadrature Decoder Test started!.");
    printf_string("\n\r Press K2 button to terminate test.\n\r");

    QUAD_DEC_INIT_PARAMS_t quad_dec_init_param = {.chx_port_sel = QUAD_DEC_CHXA_P00_AND_CHXB_P01,
                                                  .chy_port_sel = QUAD_DEC_CHYA_NONE_AND_CHYB_NONE,
                                                  .chz_port_sel = QUAD_DEC_CHZA_NONE_AND_CHZB_NONE,
                                                  .qdec_clockdiv = 1,
                                                  .qdec_events_count_to_trigger_interrupt = 1,};
    quad_decoder_init(&quad_dec_init_param);
    quad_decoder_register_callback((uint32_t*)quad_decoder_user_callback_function);
    quad_decoder_enable_irq(1);

    wkupct_register_callback(wkup_callback_function);
    wkupct_enable_irq(0x240, 0x240, 1, 0);

    quad_started = false;
    terminate_quad_test = false;
    while(terminate_quad_test == false)
    {
        // polling
        if (quad_started)
        {
            qdec_xcnt_reg = quad_decoder_get_x_counter();
            printf_string("\n\r Quadec Polling DX: ");
            printf_byte(qdec_xcnt_reg >> 8);
            printf_byte(qdec_xcnt_reg & 0xFF);
        }
        
        if (qdec_new_data == true)
        {
            // interrupt has triggered
            qdec_new_data = false;
  
            printf_string("\n\n\rQuadec ISR report:");

            printf_string("\n\r DX: ");
            printf_byte(qdec_xcnt >> 8);
            printf_byte(qdec_xcnt & 0xFF);

            printf_string(" DY: ");
            printf_byte(qdec_ycnt >> 8);
            printf_byte(qdec_ycnt & 0xFF);

            printf_string(" DZ: ");
            printf_byte(qdec_zcnt >> 8);
            printf_byte(qdec_zcnt & 0xFF);
        }
    }
    printf_string("\n\r Quadrature Decoder Test terminated!");
    printf_string("\n\rEnd of test\n\r");
}
