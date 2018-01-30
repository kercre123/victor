/**
 ****************************************************************************************
 *
 * @file user_custs1_impl.c
 *
 * @brief All in one project Custom1 Server implementation source code.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "adc.h"
#include "app.h"
#include "app_api.h"
#include "gpio.h"
#include "pwm.h"
#include "user_all_in_one.h"
#include "user_custs1_def.h"
#include "user_custs1_impl.h"
#include "user_periph_setup.h"
#include "wkupct_quadec.h"

struct app_proj_env_tag user_app_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY


/*
 * DEFINES
 ****************************************************************************************
 */
#define APP_PWM_ON      1000
#define APP_PWM_HIGH    500
#define APP_PWM_LOW     200

#define APP_NOTES_NUM   26

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

ke_msg_id_t timer_used;
const uint16_t notes[APP_NOTES_NUM] =
{
    1046,987,767,932,328,880,830,609,783,991,739,989,698,456,659,255,622,254,587,554,365,523,251,493,466,440
};

/*
 * LOCAL FUNCTIONS DECLARATION
 ****************************************************************************************
*/

static void user_app_pwm_callback_function(void);
static void user_app_enable_pwm(void);
static void user_app_disable_pwm(void);
static void user_app_set_button_event(uint8_t);
static void user_app_disable_button(void);
static void user_app_disable_button(void);
static void user_app_button_press_cb(void);
static void user_app_get_adcval2_val(void);

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void user_custs1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);

    if (val == CUSTS1_CP_CMD_ADC_VAL1_ENABLE)
    {
        timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, user_app_adcval1_timer_cb_handler);
    }
    else if (val == CUSTS1_CP_CMD_ADC_VAL1_DISABLE)
    {
        if (timer_used != 0xFFFF)
        {
            app_easy_timer_cancel(timer_used);
            timer_used = 0xFFFF;
        }
    }
    else if (val == CUSTS1_CP_CMD_PWM_ENABLE)
    {
        user_app_enable_pwm();
    }
    else if (val == CUSTS1_CP_CMD_PWM_DISABLE)
    {
        user_app_disable_pwm();
    }
    else if (val == CUSTS1_CP_CMD_ADC_VAL_2_ENABLE)
    {
        user_app_env.custs1_adcval2_enabled = 1;
        user_app_get_adcval2_val();
    }
    else if (val == CUSTS1_CP_CMD_ADC_VAL_2_DISABLE)
    {
        user_app_env.custs1_adcval2_enabled = 0;
    }
}

void user_custs1_led_wr_ind_handler(ke_msg_id_t const msgid,
                                     struct custs1_val_write_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);

    if (val == CUSTS1_LED_ON)
    {
        GPIO_SetActive(GPIO_LED_PORT, GPIO_LED_PIN);
        arch_force_active_mode();
    }
    else if (val == CUSTS1_LED_OFF)
    {
        GPIO_SetInactive(GPIO_LED_PORT, GPIO_LED_PIN);
        arch_restore_sleep_mode();
    }
}

void user_custs1_long_val_cfg_ind_handler(ke_msg_id_t const msgid,
                                           struct custs1_val_write_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
}

void user_custs1_long_val_wr_ind_handler(ke_msg_id_t const msgid,
                                          struct custs1_val_write_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
}

void user_custs1_long_val_ntf_cfm_handler(ke_msg_id_t const msgid,
                                           struct custs1_val_write_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
}

void user_custs1_adc_val_1_cfg_ind_handler(ke_msg_id_t const msgid,
                                            struct custs1_val_write_ind const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
}

void user_custs1_adc_val_1_ntf_cfm_handler(ke_msg_id_t const msgid,
                                            struct custs1_val_write_ind const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
}

void user_custs1_button_cfg_ind_handler(ke_msg_id_t const msgid,
                                         struct custs1_val_write_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
}

void user_custs1_button_ntf_cfm_handler(ke_msg_id_t const msgid,
                                         struct custs1_val_write_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
}

void user_custs1_indicateable_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct custs1_val_write_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
}

void user_custs1_indicateable_ind_cfm_handler(ke_msg_id_t const msgid,
                                               struct custs1_val_write_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
}

void user_app_adcval1_timer_cb_handler()
{
    struct custs1_val_ntf_req* req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                      TASK_CUSTS1,
                                                      TASK_APP,
                                                      custs1_val_ntf_req,
                                                      DEF_CUST1_ADC_VAL_1_CHAR_LEN);

    // ADC value to be sampled
    static uint16_t sample;
    sample = (sample <= 0xffff) ? (sample + 1) : 0;

    req->conhdl = app_env->conhdl;
    req->handle = CUST1_IDX_ADC_VAL_1_VAL;
    req->length = DEF_CUST1_ADC_VAL_1_CHAR_LEN;
    memcpy(req->value, &sample, DEF_CUST1_ADC_VAL_1_CHAR_LEN);

    ke_msg_send(req);

    if (ke_state_get(TASK_APP) == APP_CONNECTED)
    {
        // Set it once again until Stop command is received in Control Characteristic
        timer_used = app_easy_timer(APP_PERIPHERAL_CTRL_TIMER_DELAY, user_app_adcval1_timer_cb_handler);
    }
}

/**
 ****************************************************************************************
 * @brief Disable pwm timer
 * @return void
 ****************************************************************************************
*/
static void user_app_disable_pwm(void)
{
    if(user_app_env.custs1_pwm_enabled)
    {
        user_app_env.custs1_pwm_enabled = 0;

        arch_restore_sleep_mode();

        timer0_stop();
        set_tmr_enable(CLK_PER_REG_TMR_DISABLED);
    }
}

/**
 ****************************************************************************************
 * @brief Tiemer callback function. Update PWM settings
 * @return void
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
        timer0_set_pwm_high_counter(notes[notes_idx]/3 * 2);
        timer0_set_pwm_low_counter(notes[notes_idx]/3);
        notes_idx = ++notes_idx % (APP_NOTES_NUM -1);
    }
    change_delay++;
}

/**
 ****************************************************************************************
 * @brief Enable pwm timer
 * @return void
 ****************************************************************************************
*/
static void user_app_enable_pwm(void)
{
    if(!user_app_env.custs1_pwm_enabled)
    {
        user_app_env.custs1_pwm_enabled = 1;

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

/**
 ****************************************************************************************
 * @brief Set button event configuration
 * @return void
 ****************************************************************************************
*/
static void user_app_set_button_event(uint8_t next_event)
{

    wkupct_register_callback(user_app_button_press_cb);

    wkupct_enable_irq(WKUPCT_PIN_SELECT(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN),
                      WKUPCT_PIN_POLARITY(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, next_event), // polarity
                      1, // 1 event
                      10); // debouncing time = 10 ms

}

/**
 ****************************************************************************************
 * @brief Disable button action.
 * @return void
 ****************************************************************************************
*/
static void user_app_disable_button(void)
{
    wkupct_disable_irq();
}

/**
 ****************************************************************************************
 * @brief Callback function for button action. Update button state charactersitc.
 * @return void
 ****************************************************************************************
*/
static void user_app_button_press_cb(void)
{
    uint8_t next_btn_event = 0;

    // Read button state
    if(GPIO_GetPinStatus( GPIO_BUTTON_PORT, GPIO_BUTTON_PIN ))
    {
        user_app_env.custs1_btn_state = CUSTS1_BTN_STATE_RELEASED;
        next_btn_event = WKUPCT_PIN_POLARITY_LOW;
    }
    else
    {
        user_app_env.custs1_btn_state = CUSTS1_BTN_STATE_PRESSED;
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
    req->value[0] = user_app_env.custs1_btn_state;

    ke_msg_send(req);
    // Configure next button event
    user_app_set_button_event(next_btn_event);
}

/**
 ****************************************************************************************
 * @brief Read ADC val and update ADCVAL2 characteristic
 * @return void
 ****************************************************************************************
*/
static void user_app_get_adcval2_val(void)
{
    if(user_app_env.custs1_adcval2_enabled)
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

/**
 ****************************************************************************************
 * @brief Enable peripherals used by application.
 * @return void
 ****************************************************************************************
*/
void user_app_enable_periphs(void)
{
    // Update button state characteristic
    user_app_button_press_cb();
    // Get initial ADC value if enabled
    user_app_get_adcval2_val();
}

/**
 ****************************************************************************************
 * @brief Disable peripherals used by application.
 * @return void
 ****************************************************************************************
*/
void user_app_disable_periphs(void)
{
    user_app_disable_pwm();
    user_app_disable_button();
}
