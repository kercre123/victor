/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
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

/*
 * INCLUDES
 ****************************************************************************************
 */
#include "da1458x_scatter_config.h"
#include "arch.h"
#include "arch_api.h"
#include <stdlib.h>
#include <stddef.h>     // standard definitions
#include <stdbool.h>    // boolean definition
#include "boot.h"       // boot definition
#include "rwip.h"       // BLE initialization
#include "syscntl.h"    // System control initialization
#include "emi.h"        // EMI initialization
#include "intc.h"       // Interrupt initialization
#include "em_map_ble.h"
#include "ke_mem.h"
#include "ke_event.h"
#include "user_periph_setup.h"

#include "uart.h"   // UART initialization
#include "nvds.h"   // NVDS initialization
#include "rf.h"     // RF initialization
#include "app.h"    // application functions
#include "dbg.h"    // For dbg_warning function

#include "global_io.h"

#include "datasheet.h"

#include "em_map_ble_user.h"
#include "em_map_ble.h"

#include "lld_sleep.h"
#include "rwble.h"
#include "rf_580.h"
#include "gpio.h"

#include "lld_evt.h"
#include "arch_console.h"

#include "arch_system.h"

#include "arch_patch.h"

#include "arch_wdg.h"

#include "user_callback_config.h"

/**
 * @addtogroup DRIVERS
 * @{
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

#ifdef __DA14581__
uint32_t error;              /// Variable storing the reason of platform reset
#endif

extern uint32_t error;              /// Variable storing the reason of platform reset

/// Reserve space for Exchange Memory, this section is linked first in the section "exchange_mem_case"
extern uint8_t func_check_mem_flag;
extern struct arch_sleep_env_tag sleep_env;

volatile uint8_t descript[EM_SYSMEM_SIZE] __attribute__((section("BLE_exchange_memory"), zero_init, used)); //CASE_15_OFFSET
#if ((EM_SYSMEM_START != EXCHANGE_MEMORY_BASE) || (EM_SYSMEM_SIZE > EXCHANGE_MEMORY_SIZE))
#error("Error in Exhange Memory Definition in the scatter file. Please correct da14580_scatter_config.h settings.");
#endif
bool sys_startup_flag __attribute__((section("retention_mem_area0"), zero_init));

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */
static inline void otp_prepare(uint32_t code_size);
static inline bool ble_is_powered(void);
static inline void ble_turn_radio_off(void);
static inline void schedule_while_ble_on(void);
static inline sleep_mode_t ble_validate_sleep_mode(sleep_mode_t current_sleep_mode);
static inline void arch_turn_peripherals_off(sleep_mode_t current_sleep_mode);
static inline void arch_goto_sleep(sleep_mode_t current_sleep_mode);
static inline void arch_switch_clock_goto_sleep(sleep_mode_t current_sleep_mode);
static inline void arch_resume_from_sleep(void);
static inline sleep_mode_t rwip_power_down(void);
static inline arch_main_loop_callback_ret_t app_asynch_trm(void);
static inline arch_main_loop_callback_ret_t app_asynch_proc(void);
static inline void app_asynch_sleep_proc(void);
static inline void app_sleep_prepare_proc(sleep_mode_t *sleep_mode);
static inline void app_sleep_exit_proc(void);
static inline void app_sleep_entry_proc(sleep_mode_t sleep_mode);

#if USE_POWER_OPTIMIZATIONS
extern bool fine_hit;
#endif

/*
 * MAIN FUNCTION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief BLE main function.
 *        This function is called right after the booting process has completed.
 *        It contains the main function loop.
 ****************************************************************************************
 */
int main_func(void) __attribute__((noreturn));

int main_func(void)
{
    sleep_mode_t sleep_mode;

    //global initialise
    system_init();

    /*
     ************************************************************************************
     * Platform initialization
     ************************************************************************************
     */
    while(1)
    {
        do {
            // schedule all pending events
            schedule_while_ble_on();
        }
        while (app_asynch_proc() != GOTO_SLEEP);    //grant control to the application, try to go to power down
                                                              //if the application returns GOTO_SLEEP
              //((STREAMDATA_QUEUE)&& stream_queue_more_data())); //grant control to the streamer, try to go to power down
                                                                //if the application returns GOTO_SLEEP

        //wait for interrupt and go to sleep if this is allowed
        if (((!BLE_APP_PRESENT) && (check_gtl_state())) || (BLE_APP_PRESENT))
        {
            //Disable the interrupts
            GLOBAL_INT_STOP();

            app_asynch_sleep_proc();

            // get the allowed sleep mode
            // time from rwip_power_down() to WFI() must be kept as short as possible!!
            sleep_mode = rwip_power_down();

            if ((sleep_mode == mode_ext_sleep) || (sleep_mode == mode_deep_sleep))
            {
                //power down the radio and whatever is allowed
                arch_goto_sleep(sleep_mode);

                // In extended or deep sleep mode the watchdog timer is disabled
                // (power domain PD_SYS is automatically OFF). Although, if the debugger
                // is attached the watchdog timer remains enabled and must be explicitly
                // disabled.
                if ((GetWord16(SYS_STAT_REG) & DBG_IS_UP) == DBG_IS_UP)
                {
                    wdg_freeze();    // Stop watchdog timer
                }
                
                //wait for an interrupt to resume operation
                WFI();
                
                //resume operation
                arch_resume_from_sleep();
            }
            else if (sleep_mode == mode_idle)
            {
                if (((!BLE_APP_PRESENT) && check_gtl_state()) || (BLE_APP_PRESENT))
                {   
                    //wait for an interrupt to resume operation
                    WFI();
                }
            }
            // restore interrupts
            GLOBAL_INT_START();
        }
        wdg_reload(WATCHDOG_DEFAULT_PERIOD);
    }
}

/**
 ****************************************************************************************
 * @brief Power down the BLE Radio and whatever is allowed according to the sleep mode and
 *        the state of the system and application
 * @param[in] current_sleep_mode The current sleep mode proposed by the application.
 * @return void
 ****************************************************************************************
 */
static inline void arch_goto_sleep (sleep_mode_t current_sleep_mode)
{
    sleep_mode_t sleep_mode = current_sleep_mode;

    ble_turn_radio_off ( );
    //turn the radio off and check if we can go into deep sleep
    sleep_mode = ble_validate_sleep_mode(sleep_mode);

    // grant access to the application to check if we can go to sleep
    app_sleep_prepare_proc(&sleep_mode);    //SDK Improvements for uniformity this one should be changed?

    //turn the peripherals off according to the current sleep mode
    arch_turn_peripherals_off(sleep_mode);

    #if (USE_POWER_OPTIMIZATIONS)
        fine_hit = false;
    #endif

    // hook for app specific tasks just before sleeping
    app_sleep_entry_proc(sleep_mode);

    #if ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT)) // external wake up, only in external processor designs
        ext_wakeup_enable(EXTERNAL_WAKEUP_GPIO_PORT, EXTERNAL_WAKEUP_GPIO_PIN, EXTERNAL_WAKEUP_GPIO_POLARITY);
    #endif

    // do the last house keeping of the clocks and go to sleep
    arch_switch_clock_goto_sleep (sleep_mode);
}

/**
 ****************************************************************************************
 * @brief Manage the clocks and go to sleep.
 * @param[in] current_sleep_mode The current sleep mode proposed by the system so far
 * @return void
 ****************************************************************************************
 */
static inline void arch_switch_clock_goto_sleep(sleep_mode_t current_sleep_mode)
{
    if ( (current_sleep_mode == mode_ext_sleep) || (current_sleep_mode == mode_deep_sleep) )
    {
        SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 0);      // Set BIAS to '0' if sleep has been decided

        if (USE_POWER_OPTIMIZATIONS)
        {
            clk_freq_trim_reg_value = GetWord16(CLK_FREQ_TRIM_REG); // store used trim value

            SetBits16(CLK_16M_REG, RC16M_ENABLE, 1);                // Enable RC16

            for (volatile int i = 0; i < 20; i++);

            SetBits16(CLK_CTRL_REG, SYS_CLK_SEL, 1);                // Switch to RC16

            while( (GetWord16(CLK_CTRL_REG) & RUNNING_AT_RC16M) == 0 );

            // Do not disable XTAL16M! It will be disabled when we sleep...
            SetWord16(CLK_FREQ_TRIM_REG, 0x0000);                   // Set zero value to CLK_FREQ_TRIM_REG
        }
    }
}

/**
 ****************************************************************************************
 * @brief  An interrupt came, resume from sleep.
 * @return void
 ****************************************************************************************
 */
static inline void arch_resume_from_sleep(void)
{
    // hook for app specific tasks just after waking up
    app_sleep_exit_proc( );

#if ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT)) // external wake up, only in external processor designs
    // Disable external wakeup interrupt
    ext_wakeup_disable();
#endif

    // restore ARM Sleep mode
    // reset SCR[2]=SLEEPDEEP bit else the mode=idle WFI will cause a deep sleep
    // instead of a processor halt
    SCB->SCR &= ~(1<<2);
}

/**
 ****************************************************************************************
 * @brief Check if the BLE module is powered on.
 * @return void
 ****************************************************************************************
 */
static inline bool ble_is_powered()
{
    return ((GetBits16(CLK_RADIO_REG, BLE_ENABLE) == 1) &&
            (GetBits32(BLE_DEEPSLCNTL_REG, DEEP_SLEEP_STAT) == 0) &&
            !(rwip_prevent_sleep_get() & RW_WAKE_UP_ONGOING));
}

/**
 ****************************************************************************************
 * @brief Call the scheduler if the BLE module is powered on.
 * @return void
 ****************************************************************************************
 */
static inline void schedule_while_ble_on(void)
{
    // BLE clock is enabled
    while (ble_is_powered())
    {
        // BLE event end is set. conditional RF calibration can run.
        uint8_t ble_evt_end_set = ke_event_get(KE_EVENT_BLE_EVT_END);

        //execute messages and events
        rwip_schedule();

        if (ble_evt_end_set)
        {
           uint32_t sleep_duration = 0;
           rcx20_read_freq();

            //if you have enough time run a temperature calibration of the radio
            if (lld_sleep_check(&sleep_duration, 4)) //6 slots -> 3.750 ms
                // check time and temperature to run radio calibrations.
                conditionally_run_radio_cals();
        }

        //grant control to the application, try to go to sleep
        //if the application returns GOTO_SLEEP
        if (app_asynch_trm() != GOTO_SLEEP)
        {
            continue; // so that rwip_schedule() is called again
        }
        else
        {
            arch_printf_process();
            break;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Power down the ble ip if possible.
 * @return sleep_mode_t return the current sleep mode
 ****************************************************************************************
 */
static inline sleep_mode_t rwip_power_down(void)
{
    sleep_mode_t sleep_mode;
    // if app has turned sleep off, rwip_sleep() will act accordingly
    // time from rwip_sleep() to WFI() must be kept as short as possible!
    sleep_mode = rwip_sleep();

    // BLE is sleeping ==> app defines the mode
    if (sleep_mode == mode_sleeping) {
        if (sleep_env.slp_state == ARCH_EXT_SLEEP_ON) {
            sleep_mode = mode_ext_sleep;
        } else {
            sleep_mode = mode_deep_sleep;
        }
    }
    return (sleep_mode);
}

/**
 ****************************************************************************************
 * @brief Turn the radio off according to the current sleep_mode and check if we can go
 *        into deep sleep.
 * @param[in] current_sleep_mode The current sleep mode proposed by the system so far
 * @return sleep_mode_t return the allowable sleep mode
 ****************************************************************************************
 */
static inline void ble_turn_radio_off(void)
{
    SetBits16(PMU_CTRL_REG, RADIO_SLEEP, 1); // turn off radio
}

/**
 ****************************************************************************************
 * @brief Validate that we can use the proposed sleep mode.
 * @param[in] current_sleep_mode The current sleep mode proposed by the system so far
 * @return sleep_mode_t return the allowable sleep mode
 ****************************************************************************************
 */
static inline sleep_mode_t ble_validate_sleep_mode(sleep_mode_t current_sleep_mode)
{
    sleep_mode_t sleep_mode = current_sleep_mode;

    if (jump_table_struct[nb_links_user] > 1)
    {
        if ((sleep_mode == mode_deep_sleep) && func_check_mem() && test_rxdone() && ke_mem_is_empty(KE_MEM_NON_RETENTION))
        {
            func_check_mem_flag = 2; //true;
        }
        else
        {
            sleep_mode = mode_ext_sleep;
        }
    }
    else
    {
        if ((sleep_mode == mode_deep_sleep) && ke_mem_is_empty(KE_MEM_NON_RETENTION))
        {
            func_check_mem_flag = 1; //true;
        }
        else
        {
            sleep_mode = mode_ext_sleep;
        }
    }
    return (sleep_mode);
}

/**
 ****************************************************************************************
 * @brief  Turn the peripherals off according to the current sleep mode.
 * @param[in] current_sleep_mode The current sleep mode proposed by the system so far
 * @return void
 ****************************************************************************************
 */
static inline void arch_turn_peripherals_off (sleep_mode_t current_sleep_mode)
{
    if (current_sleep_mode == mode_ext_sleep || current_sleep_mode == mode_deep_sleep)
    {
        SCB->SCR |= 1<<2; // enable deep sleep  mode bit in System Control Register (SCR[2]=SLEEPDEEP)

        SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 0);           // activate PAD latches
        SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 1);           // turn off peripheral power domain
        if (current_sleep_mode == mode_ext_sleep)
        {
            SetBits16(SYS_CTRL_REG, RET_SYSRAM, 1);         // retain System RAM
            SetBits16(SYS_CTRL_REG, OTP_COPY, 0);           // disable OTP copy
        }
        else
        {
            // mode_deep_sleep
#if DEVELOPMENT_DEBUG
            SetBits16(SYS_CTRL_REG, RET_SYSRAM, 1);         // retain System RAM
#else
            SetBits16(SYS_CTRL_REG, RET_SYSRAM, 0);         // turn System RAM off => all data will be lost!
#endif
            otp_prepare(0x1FC0);                            // this is 0x1FC0 32 bits words, so 0x7F00 bytes
        }
    }
}

/**
 ****************************************************************************************
 * @brief Prepare OTP Controller in order to be able to reload SysRAM at the next power-up.
 ****************************************************************************************
 */
static inline void otp_prepare(uint32_t code_size)
{
    // Enable OPTC clock in order to have access
    SetBits16 (CLK_AMBA_REG, OTP_ENABLE, 1);

    // Wait a little bit to start the OTP clock...
    for(uint8_t i = 0 ; i<10 ; i++); //change this later to a defined time

    SetBits16(SYS_CTRL_REG, OTP_COPY, 1);

    // Copy the size of software from the first word of the retention mem.
    SetWord32 (OTPC_NWORDS_REG, code_size - 1);

    // And close the OPTC clock to save power
    SetBits16 (CLK_AMBA_REG, OTP_ENABLE, 0);
}

/**
 ****************************************************************************************
 * @brief Used for sending messages to kernel tasks generated from
 *        asynchronous events that have been processed in app_asynch_proc.
 * @return KEEP_POWERED to force calling of schedule_while_ble_on(), else GOTO_SLEEP
 ****************************************************************************************
 */
static inline arch_main_loop_callback_ret_t app_asynch_trm(void)
{
    if (user_app_main_loop_callbacks.app_on_ble_powered != NULL)
    {
        return user_app_main_loop_callbacks.app_on_ble_powered();
    }
    else
    {
        return GOTO_SLEEP;
    }
}

/**
 ****************************************************************************************
 * @brief Used for processing of asynchronous events at “user” level. The
 *        corresponding ISRs should be kept as short as possible and the
 *        remaining processing should be done at this point.
 * @return KEEP_POWERED to force calling of schedule_while_ble_on(), else GOTO_SLEEP
 ****************************************************************************************
 */
static inline arch_main_loop_callback_ret_t app_asynch_proc(void)
{
    if (user_app_main_loop_callbacks.app_on_system_powered != NULL)
    {
        return user_app_main_loop_callbacks.app_on_system_powered();
    }
    else
    {
        return GOTO_SLEEP;
    }
}

/**
 ****************************************************************************************
 * @brief Used for updating the state of the application just before sleep checking starts.
 * @return void
 ****************************************************************************************
 */
static inline void app_asynch_sleep_proc(void)
{
    if (user_app_main_loop_callbacks.app_before_sleep != NULL)
        user_app_main_loop_callbacks.app_before_sleep();
}

/**
 ****************************************************************************************
 * @brief Used to disallow extended or deep sleep based on the current application state.
 *        BLE and Radio are still powered off.
 * @param[in] sleep_mode     Sleep Mode
 * @return void
 ****************************************************************************************
 */
static inline void app_sleep_prepare_proc(sleep_mode_t *sleep_mode)
{
    if (user_app_main_loop_callbacks.app_validate_sleep != NULL)
        (*sleep_mode) = user_app_main_loop_callbacks.app_validate_sleep(*sleep_mode);
}

/**
 ****************************************************************************************
 * @brief Used for application specific tasks just before entering the low power mode.
 * @param[in] sleep_mode     Sleep Mode
 * @return void
 ****************************************************************************************
 */
static inline void app_sleep_entry_proc(sleep_mode_t sleep_mode)
{
    if (user_app_main_loop_callbacks.app_going_to_sleep != NULL)
        user_app_main_loop_callbacks.app_going_to_sleep(sleep_mode);
}

/**
 ****************************************************************************************
 * @brief Used for application specific tasks immediately after exiting the low power mode.
 * @param[in] sleep_mode     Sleep Mode
 * @return void
 ****************************************************************************************
 */
static inline void app_sleep_exit_proc(void)
{
    if (user_app_main_loop_callbacks.app_resume_from_sleep != NULL)
        user_app_main_loop_callbacks.app_resume_from_sleep();
}

/// @} DRIVERS
