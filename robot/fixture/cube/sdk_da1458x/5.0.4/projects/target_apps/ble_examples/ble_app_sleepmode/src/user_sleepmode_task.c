/**
 ****************************************************************************************
 *
 * @file user_sleepmode_task.c
 *
 * @brief Sleep mode project source code.
 *
 * Copyright (C) 2016. Dialog Semiconductor Ltd, unpublished work. This computer
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_sleepmode_task.h"
#include "wkupct_quadec.h"
#include "gpio.h"
#include "pwm.h"
#include "adc.h"
#include "custs1_task.h"
#include "user_custs1_def.h"
#include "user_sleepmode.h"
#include "user_periph_setup.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define APP_PWM_ON      (1000)
#define APP_PWM_HIGH    (500)
#define APP_PWM_LOW     (200)
#define APP_NOTES_NUM   (26)

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct app_proj_env_tag app_sleep_env __attribute__((section("retention_mem_area0"), zero_init)); //@RETENTION MEMORY

const uint16_t notes[APP_NOTES_NUM] =
{
    1046, 987, 767, 932, 328, 880, 830, 609, 783, 991, 739, 989, 698, 456, 659, 255, 622,
    254, 587, 554, 365, 523, 251, 493, 466, 440
};

/*
 * LOCAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Timer callback function. Update PWM settings.
 * @return void
 ****************************************************************************************
 */
static void user_app_pwm_callback_function(void);

/**
 ****************************************************************************************
 * @brief Enable PWM timer.
 * @return void
 ****************************************************************************************
 */
static void user_app_enable_pwm(void);

/**
 ****************************************************************************************
 * @brief Disable PWM timer.
 * @return void
 ****************************************************************************************
 */
static void user_app_disable_pwm(void);

/**
 ****************************************************************************************
 * @brief Set button event configuration.
 * @param[in] next_event    Next event number
 * @return void
 ****************************************************************************************
 */
static void user_app_set_button_event(uint8_t next_event);

/**
 ****************************************************************************************
 * @brief Disable button action.
 * @return void
 ****************************************************************************************
 */
static void user_app_disable_button(void);

/**
 ****************************************************************************************
 * @brief Callback function for button action. Update button state charactersitc.
 * @return void
 ****************************************************************************************
 */
static void user_app_button_press_cb(void);

/**
 ****************************************************************************************
 * @brief Read ADC val and update ADC_VAL2 characteristic.
 * @return void
 ****************************************************************************************
*/
static void user_app_get_adc_val(void);

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void user_app_pwm_callback_function(void)
{
    static uint8_t change_delay = 0;
    static uint8_t notes_idx = 0;

    if (change_delay == 10)
    {
        change_delay = 0;
        timer0_set_pwm_on_counter(0xFFFF);
        timer0_set_pwm_high_counter(notes[notes_idx] / 3 * 2);
        timer0_set_pwm_low_counter(notes[notes_idx] / 3);
        notes_idx = ++notes_idx % (APP_NOTES_NUM - 1);
    }
    change_delay++;
}

static void user_app_enable_pwm(void)
{
    if(!app_sleep_env.custs1_pwm_enabled)
    {
        app_sleep_env.custs1_pwm_enabled = 1;

        // Disable sleep mode
        arch_force_active_mode();

        // Enable TIMER clock
        set_tmr_enable(CLK_PER_REG_TMR_ENABLED);
        // Set clock division
        set_tmr_div(CLK_PER_REG_TMR_DIV_8);
        // Init timer 0
        timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_NO_DIV);
        // Set timing parameters
        timer0_set(APP_PWM_ON, APP_PWM_HIGH, APP_PWM_LOW);
        // Register IRQ callback
        timer0_register_callback(user_app_pwm_callback_function);
        // Enable IRQ
        timer0_enable_irq();
        // Start timer
        timer0_start();
    }
}

static void user_app_disable_pwm(void)
{
    if(app_sleep_env.custs1_pwm_enabled)
    {
        app_sleep_env.custs1_pwm_enabled = 0;

        arch_restore_sleep_mode();

        timer0_stop();
        set_tmr_enable(CLK_PER_REG_TMR_DISABLED);
    }
}

static void user_app_set_button_event(uint8_t next_event)
{

    wkupct_register_callback(user_app_button_press_cb);

    wkupct_enable_irq(WKUPCT_PIN_SELECT(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN),
                      WKUPCT_PIN_POLARITY(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, next_event), // polarity
                      1, // 1 event
                      10); // debouncing time = 10 ms

}

static void user_app_disable_button(void)
{
    wkupct_disable_irq();
}

static void user_app_button_press_cb(void)
{
    uint8_t next_btn_event = 0;

    // Read button state
    if(GPIO_GetPinStatus(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN))
    {
        app_sleep_env.custs1_btn_state = CUSTS1_BTN_STATE_RELEASED;
        next_btn_event = WKUPCT_PIN_POLARITY_LOW;
    }
    else
    {
        app_sleep_env.custs1_btn_state = CUSTS1_BTN_STATE_PRESSED;
        next_btn_event = WKUPCT_PIN_POLARITY_HIGH;
    }

    // Update button characterstic
    struct custs1_val_ntf_req *req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                      TASK_CUSTS1,
                                                      TASK_APP,
                                                      custs1_val_ntf_req,
                                                      DEF_CUST1_BUTTON_STATE_CHAR_LEN);

    req->conhdl = app_env->conhdl;
    req->handle = CUST1_IDX_BUTTON_STATE_VAL;
    req->length = DEF_CUST1_BUTTON_STATE_CHAR_LEN;
    req->value[0] = app_sleep_env.custs1_btn_state;

    ke_msg_send(req);

    // Configure next button event
    user_app_set_button_event(next_btn_event);
}

static void user_app_get_adc_val(void)
{
    if (app_sleep_env.custs1_adcval2_enabled)
    {
        uint16_t adc_sample;

        adc_calibrate();
        adc_sample = (uint16_t)adc_get_vbat_sample(false);

        struct custs1_val_set_req *req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_SET_REQ,
                                                          TASK_CUSTS1,
                                                          TASK_APP,
                                                          custs1_val_set_req,
                                                          DEF_CUST1_ADC_VAL_2_CHAR_LEN);

       req->conhdl = app_env->conhdl;
       req->handle = CUST1_IDX_ADC_VAL_2_VAL;
       req->length = DEF_CUST1_ADC_VAL_2_CHAR_LEN;
       memcpy(req->value, &adc_sample, DEF_CUST1_ADC_VAL_1_CHAR_LEN);

       ke_msg_send(req);
    }
}

void user_app_enable_periphs(void)
{
    // Update button state characteristic
    user_app_button_press_cb();
    // Get initial ADC value if enabled
    user_app_get_adc_val();
}

void user_app_disable_periphs(void)
{
    user_app_disable_pwm();
    user_app_disable_button();
}

void user_custs1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                     struct custs1_val_write_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    if(param->handle == CUST1_IDX_CONTROL_POINT_VAL)
    {
        if(param->value[0] == CUSTS1_CP_CMD_PWM_ENABLE)
        {
            user_app_enable_pwm();
        }
        else if(param->value[0] == CUSTS1_CP_CMD_PWM_DISABLE)
        {
            user_app_disable_pwm();
        }
        else if(param->value[0] == CUSTS1_CP_CMD_ADC_VAL_2_ENABLE)
        {
            app_sleep_env.custs1_adcval2_enabled = 1;
            user_app_get_adc_val();
        }
        else if(param->value[0] == CUSTS1_CP_CMD_ADC_VAL_2_DISABLE)
        {
            app_sleep_env.custs1_adcval2_enabled = 0;
        }
    }
}

/// @} APP
