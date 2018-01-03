/**
 ****************************************************************************************
 *
 * @file rc5.c
 *
 * @brief RC-5 s/w encoder driver source file.
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

#if IR_SUPPORTED

#include "global_io.h"
#include "datasheet.h"
#include "pwm.h"
#include "rc5.h"
#include "periph_setup.h"
#include "arch_sleep.h"

extern timer0_handler_function_t* TIMER0_callback;

sw_timer_state_t sw_timer_state __RETAINED;

uint8_t rc5_burst_data_device_address __RETAINED;
uint8_t rc5_burst_data_command_code __RETAINED;
uint16_t rc5_repetitions __RETAINED;

uint16_t rc5_burst_data_bits __RETAINED;

uint16_t ir_signal_repetition_counter;

#define LOWNIBBLE   35
#define HIGHNIBBLE    74
#define TMRIRQ_NO    (HIGHNIBBLE+LOWNIBBLE+2)*32-1
uint8_t lasttoggle   __RETAINED;  

         
uint8_t rc5_next_burst_data_device_address __RETAINED;
uint8_t rc5_next_burst_data_command_code __RETAINED;
uint8_t rc5_next_burst_data_available __RETAINED;

uint8_t rc5_toggle_bit __RETAINED;
uint8_t rc5_burst_index __RETAINED;



#define RC5_CARRIER_BIT_HIGH_DURATION_IN_TICKS (222/3)
#define RC5_CARRIER_BIT_LOW_DURATION_IN_TICKS (222/3*2)

// ( (2 * 10) / 16MHz ) * 711 = 888.75 uS
#define RC5_HALF_LOGIC_BIT_DURATION_IN_ON_COUNTER_TICKS (711)

#define RC5_REPETITION_PERIOD_IN_LOGIC_BITS (100)



/**
 ****************************************************************************************
 * @brief  Initializes RC-5 
 *
 * @param[in]
 *
 * @return 
 ****************************************************************************************
 */
void rc5_init(void)
{
    // TIMER0 clock is divided by 2
    // (Only when the 16MHz(fast) clock source is selected)
    // The clock source selection is done in timer0_init, below
    set_tmr_div(CLK_PER_REG_TMR_DIV_2);

    // TIM0_CLK_FAST: Select the 16MHz as clock source to TIMER0
    // PWM_MODE_ONE: During the active periods of PWM, the driven outputs will be constantly high
    // TIM0_CLK_DIV_BY_10: TIMER0 uses the selected frequency divided by 10 for advancing the ON counter. (Only for the ON counter!)
    timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_DIV_BY_10);
    
    // Enable the TIMER0 clock
    set_tmr_enable(CLK_PER_REG_TMR_ENABLED);

    // Set IR signal HIGH duration
    timer0_set_pwm_high_counter(RC5_CARRIER_BIT_HIGH_DURATION_IN_TICKS);
    
    // Set IR signal LOW duration
    timer0_set_pwm_low_counter(RC5_CARRIER_BIT_LOW_DURATION_IN_TICKS);
    
    // Set IR signal IRQ period to half the period of a logic bit
    timer0_set_pwm_on_counter(RC5_HALF_LOGIC_BIT_DURATION_IN_ON_COUNTER_TICKS);
    
    //Enable the TIMER0 irq (SWTIM_IRQ)
    timer0_enable_irq(); //Note: Priority has been set to 254 in timer0_init
    
    //Start TIMER0
    timer0_start();   
}


/**
 ****************************************************************************************
 * @brief  starts RC-5
 ****************************************************************************************
 */

void rc5_start(uint8_t address,uint8_t cmd, uint16_t repetitions, uint16_t delay)
{ 
    // Deactivate IR output
    rc5_deactivate_output();
    
    rc5_burst_data_device_address = address;
    rc5_burst_data_command_code = cmd;
    rc5_repetitions = repetitions;
    
    if (delay == 0)
    {
        sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST;
    }
    else
    {
        ir_signal_repetition_counter = delay;        
        sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST_PRECEDED_BY_DELAY;
    }    
    
  //   // Activate IR output
 //   rc5_activate_output();    
}


/**
 ****************************************************************************************
 * @brief  terminates RC-5. The Timer is disabled
 ****************************************************************************************
 */

void rc5_terminate(void)
{
    // Deactivate IR output
    rc5_deactivate_output();

    timer0_stop();                                                      // Stop Timer0 (timer is off and in reset state) 
    timer0_disable_irq();                                               // disable interrupt
    set_tmr_enable(CLK_PER_REG_TMR_DISABLED);                           // Disable TIMER0(and TIMER2) clock
    timer0_set(0, 0, 0);    		                            	    // set M reg, N reg and ON reg to 0   
}


/**
 ****************************************************************************************
 * @brief Deactivates the RC-5 output. The FSM continues normally
 ****************************************************************************************
 */

void rc5_deactivate_output(void)
{
    // Deactivate IR output
    GPIO_SetInactive(IR_LED_PORT, IR_LED_PIN);    
    GPIO_SetPinFunction(IR_LED_PORT, IR_LED_PIN, OUTPUT, PID_GPIO);
}

/**
 ****************************************************************************************
 * @brief Activates the RC-5 output
 ****************************************************************************************
 */

void rc5_activate_output(void)
{
    // Activate IR output
    GPIO_SetPinFunction(IR_LED_PORT, IR_LED_PIN, OUTPUT, PID_PWM0);
}


/**
 ****************************************************************************************
 * @brief  stops RC-5
 ****************************************************************************************
 */

void rc5_stop(void)
{
    rc5_repetitions = 0;
}


/**
 ****************************************************************************************
 * @brief  Releases RC-5.
 *
 ****************************************************************************************
 */
void rc5_release(void)
{

}


/**
 ****************************************************************************************
 * @brief  registers a callback function to be called from within SWTIM_Handler
           when SWTIM_IRQn has triggered
 ****************************************************************************************
 */

void rc5_register_callback(timer0_handler_function_t* callback)
{
   TIMER0_callback = callback;
}


/**
 ****************************************************************************************
 * @brief  RC5 'toggle' bit toggling
 ****************************************************************************************
 */
void rc5_toggle(void)
{
    rc5_toggle_bit ^= true;    
}


/**
 ****************************************************************************************
 * @brief
 ****************************************************************************************
 */
void rc5_set_next_burst_data(uint8_t address,uint8_t cmd)
{
    rc5_next_burst_data_device_address = address;
    rc5_next_burst_data_command_code = cmd;
    rc5_next_burst_data_available = true;
}

 /**
 ****************************************************************************************
 * @brief the callback function for SWTIM_IRQ to implement the RC-5 IR protocol
 ****************************************************************************************
 */
#pragma O0
void rc5_callback_function(void)
{
    uint8_t debug__last_rwble_evt_in_rc5_callback_function;
    debug__last_rwble_evt_in_rc5_callback_function = app_last_rwble_evt_get();
    static bool rc5_interrupted_by_ble __RETAINED;
    static uint16_t rc5_delay_before_burst __RETAINED;
    
    
    
    if (debug__last_rwble_evt_in_rc5_callback_function != BLE_EVT_END)
    {
        rc5_delay_before_burst = RC5_DELAY_BEFORE_BURST_IN_BITS_COUNT;
        
        if (sw_timer_state != IR_GEN_STATE_IDLE)
        {
            rc5_interrupted_by_ble = true;
            sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST_PRECEDED_BY_DELAY;
            rc5_delay_before_burst = RC5_REPETITION_PERIOD_IN_LOGIC_BITS;
/*
#if DEVELOPMENT_DEBUG    
    uint32_t i;

    SetWord16(P10_MODE_REG, PID_GPIO|OUTPUT);
    SetWord16(P1_SET_DATA_REG, 0x1);
    for ( i=0;i<40;i++); //20 is almost 7.6usec of time.
    SetWord16(P1_RESET_DATA_REG, 0x1);
#endif            
    */        
            
        }               
        return;     
    }
    
    
    
/*
    static uint8_t temp1;
    SetWord16(P35_MODE_REG, PID_GPIO|OUTPUT);    
    if (temp1)
    {
        SetWord16(P3_SET_DATA_REG, 1<<5);
    }
    else
    {
        SetWord16(P3_RESET_DATA_REG, 1<<5);        
    }
    temp1 ^= 1;
*/
    
    switch (sw_timer_state)
    {
       
        case IR_GEN_STATE_IDLE:
            

        break;
        
        case IR_GEN_STATE_SEND_RC5_BURST_PRECEDED_BY_DELAY:  
            if (rc5_delay_before_burst)
                rc5_delay_before_burst--;
                 
            if(rc5_delay_before_burst == 0)
                sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST;
        break;

        case IR_GEN_STATE_SEND_RC5_BURST:       
            rc5_burst_data_bits = (1 << RC5_START1_BIT_INDEX) | (1 << RC5_START2_BIT_INDEX) |
                                  (rc5_toggle_bit << RC5_TOGGLE_BIT_INDEX) |
                                  ( (rc5_burst_data_device_address & 0x1F) << RC5_ADDRESS_LSB_INDEX) |
                                  ( (rc5_burst_data_command_code & 0x3F) << RC5_COMMAND_LSB_INDEX);
            rc5_burst_index = 0;        
            sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST_BIT;     
        
        break;
        
        case IR_GEN_STATE_SEND_RC5_BURST_BIT:
            // Generate Manchester encoding
            // 0:mark - space
            // 1:space - mark
            // So, mark is sent during the first part of a '0' bit and during the second part of a '1' bit
            // and space is sent during the second part of a '0' bit and during the first part of a '1' bit
            // 
            // The part of the bit (first part or second part) is XORed with the MSB of the rc5_burst_data_bits         
            if ((rc5_burst_index % 2) ^ ((rc5_burst_data_bits>>13)&0x01) )
            {
                //SPACE (pin is driven low to disable IR LED - SPACE)
                rc5_deactivate_output();
            }
            else
            {
                //MARK (pin will be driven by PWM0 to send the modulated RC5 MARK signal - MARK)
                rc5_activate_output();
            }
            
            // Advance to the next bit
            if (rc5_burst_index % 2)
            {
               rc5_burst_data_bits <<= 1;                 
            }
            rc5_burst_index++;
            
            if (rc5_burst_index == 2 * RC5_TOTAL_BITS_COUNT + 1)
            {
                //All the bits of the signal have been sent out
                rc5_deactivate_output();
                
                sw_timer_state = IR_GEN_STATE_DELAY_BEFORE_REPEAT;                  // Switch to the 'delay before repeat' state
                ir_signal_repetition_counter = RC5_REPETITION_PERIOD_IN_LOGIC_BITS; 
            }
      
        break;     
            

        case IR_GEN_STATE_DELAY_BEFORE_REPEAT:
                if (ir_signal_repetition_counter)
                    ir_signal_repetition_counter--;
                if(ir_signal_repetition_counter == 0)
                    sw_timer_state = IR_GEN_STATE_REPEAT_OR_STOP; //Switch to the 'new key or repeat' state  
        break;

                
        case IR_GEN_STATE_REPEAT_OR_STOP: //repeat delay over,start to send repeat signal/or new key      
            if (rc5_repetitions)
            {   
                rc5_repetitions--;
                rc5_burst_index = 0;
                sw_timer_state = IR_GEN_STATE_SEND_RC5_BURST;                                                                                    
            }
            else
            {
                rc5_stop();
                sw_timer_state = IR_GEN_STATE_IDLE;                                                                             
            }
		break;          
    }        
}

#endif //IR_SUPPORTED
