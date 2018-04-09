/**
 ****************************************************************************************
 *
 * @file mainloop_callbacks.h
 *
 * @brief Implementation of mainloop callbacks used by the production test application. 
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

#include "arch.h"
#if HAS_AUDIO
#include "audio_task.h"
#endif

#include "rf_580.h"
#include "pll_vcocal_lut.h"
#include "llm.h"
#include "gpio.h"
#include "uart.h"
#include "arch_api.h"

#include "customer_prod.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern uint32_t last_temp_time; // time of last temperature count measurement
extern uint16_t last_temp_count; /// temperature counter

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

arch_main_loop_callback_ret_t app_on_ble_powered(void)
{
    arch_main_loop_callback_ret_t ret = GOTO_SLEEP;
    static int cnt2sleep=100;

    do {
        if(test_state == STATE_START_TX)
        {
            if(llm_le_env.evt == NULL)
            {
                test_state=STATE_IDLE;
                set_state_start_tx();
            }
        }

        if((test_tx_packet_nr >= text_tx_nr_of_packets) &&  (test_state == STATE_START_TX) )
        {
            test_tx_packet_nr=0;
            struct msg_tx_packages_send_ready s;
            uint8_t* bufptr;
            s.packet_type	= HCI_EVT_MSG_TYPE;
            s.event_code 	= HCI_CMD_CMPL_EVT_CODE;
            s.length     	= 3;
            s.param0		= 0x01;
            s.param_opcode	= HCI_TX_PACKAGES_SEND_READY;
            bufptr = (uint8_t*)&s;
            uart_write(bufptr,sizeof(struct msg_tx_packages_send_ready),NULL);

            set_state_stop();
            //btw.state is STATE_IDLE after set_state_stop()
        }

        rwip_schedule();

        if( rf_calibration_request_cbt == 1)
        {
            last_temp_count = get_rc16m_count();

            #ifdef LUT_PATCH_ENABLED
                pll_vcocal_LUT_InitUpdate(LUT_UPDATE);
            #endif
            rf_calibration();
            rf_calibration_request_cbt = 0 ;
        }
    } while(0);

    if(arch_get_sleep_mode() == 0)
    {
        ret = KEEP_POWERED;
        cnt2sleep = 100;
    }
    else
    {
        if(cnt2sleep-- > 0)
        {
            ret = KEEP_POWERED;
        }
    }

#if HAS_AUDIO
    if (ret && ke_state_get(TASK_APP) == AUDIO_ON)
    {
        app_audio439_encode(8);
    }
#endif

    return ret;
}


arch_main_loop_callback_ret_t app_on_full_power(void)
{
    return GOTO_SLEEP;
}


void app_going_to_sleep(sleep_mode_t sleep_mode)
{
    if(sleep_mode == mode_deep_sleep)
    {
        SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE, 0);    // close debugger
        SetBits16(SYS_CTRL_REG, RET_SYSRAM, 0);         // turn System RAM off => all data will be lost!
    }
}


void app_resume_from_sleep(void)
{
    arch_disable_sleep();
}
