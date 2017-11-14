/**
 ****************************************************************************************
 *
 * @file rc5.c
 *
 * @brief RC-5 s/w encoder driver header file.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _RC5_H_
#define _RC5_H_

#include "pwm.h"

#define __RETAINED __attribute__((section("retention_mem_area0"), zero_init))

typedef enum
{
    IR_GEN_STATE_IDLE = 0,
    IR_GEN_STATE_SEND_RC5_BURST_PRECEDED_BY_DELAY,
    IR_GEN_STATE_SEND_RC5_BURST,
    IR_GEN_STATE_SEND_RC5_BURST_BIT,
    IR_GEN_STATE_DELAY_BEFORE_REPEAT,
    IR_GEN_STATE_REPEAT_OR_STOP, 
    //IR_GEN_STATE_DELAY_AFTER_NEW_KEY_OR_REPEAT,   
    
} sw_timer_state_t;

#define RC5_START1_BIT_INDEX 13
#define RC5_START2_BIT_INDEX 12
#define RC5_TOGGLE_BIT_INDEX 11
#define RC5_ADDRESS_LSB_INDEX 6
#define RC5_COMMAND_LSB_INDEX 0

#define RC5_TOTAL_BITS_COUNT (14)

#define RC5_DELAY_BEFORE_BURST_IN_BITS_COUNT (2)

/**
 ****************************************************************************************
 * @brief  Initializes RC-5 
 ****************************************************************************************
 */
void rc5_init(void);

/**
 ****************************************************************************************
 * @brief  starts RC-5
 ****************************************************************************************
 */
void rc5_start(uint8_t address,uint8_t cmd, uint16_t repetitions, uint16_t delay);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void rc5_stop_forced(void);

/**
 ****************************************************************************************
 * @brief Deactivates only the RC-5 output
 ****************************************************************************************
 */
void rc5_deactivate_output(void);

/**
 ****************************************************************************************
 * @brief Activates the RC-5 output
 ****************************************************************************************
 */
void rc5_activate_output(void);

/**
 ****************************************************************************************
 * @brief  stops RC-5
 ****************************************************************************************
 */
void rc5_stop(void);

/**
 ****************************************************************************************
 * @brief  Releases RC-5.
 *
 ****************************************************************************************
 */
void rc5_release(void);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void rc5_register_callback(timer0_handler_function_t* callback);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void rc5_toggle(void);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void makeIRsignal(void);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void rc5_set_next_burst_data(uint8_t address,uint8_t cmd);

/**
 ****************************************************************************************
 * @brief  
 ****************************************************************************************
 */
void rc5_callback_function(void);


#endif // _RC5_H_
