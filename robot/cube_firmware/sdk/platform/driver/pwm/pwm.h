/**
 ****************************************************************************************
 *
 * @file pwm.h
 *
 * @brief PWM driver header file.
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

#ifndef _PWM_H_
#define _PWM_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

typedef enum
{
    PWM_MODE_ONE,
    PWM_MODE_CLOCK_DIV_BY_TWO
} PWM_MODE_t;

typedef enum
{
    TIM0_CLK_DIV_BY_10,
    TIM0_CLK_NO_DIV
} TIM0_CLK_DIV_t;

typedef enum
{
    TIM0_CLK_32K,
    TIM0_CLK_FAST
} TIM0_CLK_SEL_t;

typedef enum
{
    TIM0_CTRL_OFF_RESET,
    TIM0_CTRL_RUNNING
} TIM0_CTRL_t;

typedef enum
{
    CLK_PER_REG_TMR_DISABLED,
    CLK_PER_REG_TMR_ENABLED,
} CLK_PER_REG_TMR_ENABLE_t;

typedef enum
{
    CLK_PER_REG_TMR_DIV_1,
    CLK_PER_REG_TMR_DIV_2,
    CLK_PER_REG_TMR_DIV_4,
    CLK_PER_REG_TMR_DIV_8
} CLK_PER_REG_TMR_DIV_t;

typedef enum
{
    HW_CAN_NOT_PAUSE_PWM_2_3_4,
    HW_CAN_PAUSE_PWM_2_3_4
} TRIPLE_PWM_HW_PAUSE_EN_t;

typedef enum
{
    PWM_2_3_4_SW_PAUSE_DISABLED,
    PWM_2_3_4_SW_PAUSE_ENABLED
} TRIPLE_PWM_SW_PAUSE_EN_t;

typedef enum
{
    TRIPLE_PWM_DISABLED,
    TRIPLE_PWM_ENABLED
} TRIPLE_PWM_ENABLE_t;

typedef void (timer0_handler_function_t)(void);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief  Enables/Disables TIMER0,TIMER2 clock.
 * @param[in] clk_per_reg_tmr_enable TMR_ENABLE status: CLK_PER_REG_TMR_DISABLED or CLK_PER_REG_TMR_ENABLED
 * @return void
 ****************************************************************************************
 */
void set_tmr_enable(CLK_PER_REG_TMR_ENABLE_t clk_per_reg_tmr_enable);

 /**
 ****************************************************************************************
 * @brief  Sets TIMER0,TIMER2 clock division factor.
 * @param[in] per_tmr_div TMR_ENABLE status: CLK_PER_REG_TMR_DISABLED or CLK_PER_REG_TMR_ENABLED
 * @return void
 ****************************************************************************************
 */
void set_tmr_div(CLK_PER_REG_TMR_DIV_t per_tmr_div);

/**
 ****************************************************************************************
 * @brief  Initializes TIMER0.
 * @param[in] tim0_clk_sel  Timer0 uses 16MHz (fast) or 32kHz (slow) clock frequency
 * @param[in] pwm_mode      (during high time) '1' or (fast) clock divided by 2
 * @param[in] tim0_clk_div  Division factor for TIMER0 "on" time in peripheral divider register
 * @return void
 ****************************************************************************************
 */
void timer0_init(TIM0_CLK_SEL_t tim0_clk_sel, PWM_MODE_t pwm_mode, TIM0_CLK_DIV_t tim0_clk_div);

/**
 ****************************************************************************************
 * @brief Starts TIMER0
 * @return void
 ****************************************************************************************
 */
void timer0_start(void);

/**
 ****************************************************************************************
 * @brief Stops TIMER0.
 * @return void
 ****************************************************************************************
 */
void timer0_stop(void);

/**
 ****************************************************************************************
 * @brief  Releases TIMER0. Does not disable the TIM clock, as it is shared with TIMER2.
 * @return void
 ****************************************************************************************
 */
void timer0_release(void);

/**
 ****************************************************************************************
 * @brief Configures TIMER0 ON counter reload value.
 * @param[in] pwm_on ON COUNTER reload value
 * @return void
 ****************************************************************************************
 */
void timer0_set_pwm_on_counter(uint16_t pwm_on);

/**
 ****************************************************************************************
 * @brief Configures TIMER0 M counter reload value (duration of PWM0 high part).
 * @param[in] pwm_high M COUNTER reload value
 * @return void
 ****************************************************************************************
 */
void timer0_set_pwm_high_counter(uint16_t pwm_high);

/**
 ****************************************************************************************
 * @brief Configures TIMER0 N counter reload value (duration of PWM0 low part).
 * @param[in] pwm_low N COUNTER reload value
 * @return void
 ****************************************************************************************
 */
void timer0_set_pwm_low_counter(uint16_t pwm_low);

/**
 ****************************************************************************************
 * @brief Configures TIMER0 pwm timing parameters.
 * @param[in] pwm_on   ON COUNTER reload value
 * @param[in] pwm_high M COUNTER reload value
 * @param[in] pwm_low  N COUNTER reload value
 * @return void
 ****************************************************************************************
 */
void timer0_set(uint16_t pwm_on, uint16_t pwm_high, uint16_t pwm_low);

/**
 ****************************************************************************************
 * @brief Enables SWTIM_IRQn.
 * @return void
 ****************************************************************************************
 */
void timer0_enable_irq(void);

/**
 ****************************************************************************************
 * @brief Disables SWTIM_IRQn.
 * @return void
 ****************************************************************************************
 */
void timer0_disable_irq(void);

/**
 ****************************************************************************************
 * @brief Registers a callback function to be called from within SWTIM_Handler
 *        when SWTIM_IRQn has triggered.
 * @param[in] callback Callback function
 * @return void
 ****************************************************************************************
 */
void timer0_register_callback(timer0_handler_function_t* callback);

/**
 ****************************************************************************************
 * @brief Enables/Disables TIMER2.
 * @param[in] triple_pwm_enable TRIPLE_PWM_DISABLED or TRIPLE_PWM_ENABLED
 * @return void
 ****************************************************************************************
 */
void timer2_enable(TRIPLE_PWM_ENABLE_t triple_pwm_enable);

/**
 ****************************************************************************************
 * @brief Enables/Disables TIMER2 pause by hw.
 * @param[in] hw_pause_en HW_CAN_NOT_PAUSE_PWM_2_3_4 or HW_CAN_PAUSE_PWM_2_3_4
 * @return void
 ****************************************************************************************
 */
void timer2_set_hw_pause(TRIPLE_PWM_HW_PAUSE_EN_t hw_pause_en);

/**
 ****************************************************************************************
 * @brief Pauses by sw / releases sw pause for TIMER2.
 * @param[in] sw_pause_en PWM_2_3_4_SW_PAUSE_DISABLED or PWM_2_3_4_SW_PAUSE_ENABLED
 * @return void
 ****************************************************************************************
 */
void timer2_set_sw_pause(TRIPLE_PWM_SW_PAUSE_EN_t sw_pause_en);

/**
 ****************************************************************************************
 * @brief Sets TIMER2 frequency.
 * @param[in] triple_pwm_frequency TIMER2 frequency
 * @return void
 ****************************************************************************************
 */
 void timer2_set_pwm_frequency(uint16_t triple_pwm_frequency);

/**
 ****************************************************************************************
 * @brief Initializes TIMER2.
 * @param[in] hw_pause_en          HW_CAN_NOT_PAUSE_PWM_2_3_4 or HW_CAN_PAUSE_PWM_2_3_4
 * @param[in] sw_pause_en          PWM_2_3_4_SW_PAUSE_DISABLED or PWM_2_3_4_SW_PAUSE_ENABLED
 * @param[in] triple_pwm_frequency TIMER2 frequency
 * @return void
 ****************************************************************************************
 */
void timer2_init(TRIPLE_PWM_HW_PAUSE_EN_t hw_pause_en, TRIPLE_PWM_SW_PAUSE_EN_t sw_pause_en, uint16_t triple_pwm_frequency);

/**
 ****************************************************************************************
 * @brief Stops timer2. Does not disable the TIM clock, as it is shared with TIMER0.
 * @param[in] timer2_duty_cycle PWM2 duty cycle
 * @return void
 ****************************************************************************************
 */
void timer2_stop(void);

/**
 ****************************************************************************************
 * @brief Sets PWM2 duty cycle.
 * @param[in] timer2_duty_cycle PWM2 duty cycle
 * @return void
 ****************************************************************************************
 */
void timer2_set_pwm2_duty_cycle(uint16_t pwm2_duty_cycle);

/**
 ****************************************************************************************
 * @brief Sets PWM3 duty cycle.
 * @param[in] timer3_duty_cycle PWM3 duty cycle.
 * @return void
 ****************************************************************************************
 */
void timer2_set_pwm3_duty_cycle(uint16_t pwm3_duty_cycle);

/**
 ****************************************************************************************
 * @brief Sets PWM4 duty cycle.
 * @param[in] timer3_duty_cycle PWM4 duty cycle.
 * @return void
 ****************************************************************************************
 */
void timer2_set_pwm4_duty_cycle(uint16_t pwm4_duty_cycle);

#endif // _PWM_H_
