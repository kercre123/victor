/**
 ****************************************************************************************
 *
 * @file    arch_sleep.c
 *
 * @brief   Sleep control functions.
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

#include "arch.h"
#include "arch_api.h"
#include "app.h"
#include "rwip.h"

/// Application Environment Structure
extern struct arch_sleep_env_tag    sleep_env;    // __attribute__((section("retention_mem_area0")));
uint8_t sleep_md                    __attribute__((section("retention_mem_area0"), zero_init));
uint8_t sleep_pend                  __attribute__((section("retention_mem_area0"), zero_init));
uint8_t sleep_cnt                   __attribute__((section("retention_mem_area0"), zero_init));
bool sleep_ext_force                __attribute__((section("retention_mem_area0"), zero_init));

extern last_ble_evt arch_rwble_last_event;

/**
 ****************************************************************************************
 * @brief       Disable all sleep modes. The system operates in active / idle modes only.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_disable_sleep(void)
{
    if (sleep_md == 0)
    {
        sleep_env.slp_state = ARCH_SLEEP_OFF;

        rwip_env.sleep_enable = false;

    }
    else
        sleep_pend = 0x80 | 0x00;
}

/**
 ****************************************************************************************
 * @brief       Activates extended sleep mode. The system operates in idle / active / extended sleep modes.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_set_extended_sleep(void)
{
    if (sleep_md == 0)
    {
        sleep_env.slp_state = ARCH_EXT_SLEEP_ON;

        rwip_env.sleep_enable = true;

    }
    else
        sleep_pend = 0x80 | 0x01;
}

/**
 ****************************************************************************************
 * @brief       Activates deep sleep mode. The system operates in idle / active / deep sleep modes.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_set_deep_sleep(void)
{
    if (sleep_md == 0)
    {
        
#if (USE_MEMORY_MAP == EXT_SLEEP_SETUP)
        //Warning. Deep sleep is set while memory map configuration is set to extended sleep configuration
        ASSERT_WARNING(0);
#endif
        
        sleep_env.slp_state = ARCH_DEEP_SLEEP_ON;

        rwip_env.sleep_enable = true;

    }
    else
        sleep_pend = 0x80 | 0x02;
}

/**
 ****************************************************************************************
 * @brief       Activates selected sleep mode.
 * @param       sleep_state     Selected sleep mode.
 * @return      void
 ****************************************************************************************
 */
void arch_set_sleep_mode(sleep_state_t sleep_state)
{
    switch (sleep_state)
    {
        case ARCH_SLEEP_OFF:
            arch_disable_sleep();
            break;
        case ARCH_EXT_SLEEP_ON:
            arch_set_extended_sleep();
            break;
        case ARCH_DEEP_SLEEP_ON:
            arch_set_deep_sleep();
            break;
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief       Get the current sleep mode of operation.
 * @param       void
 * @return      The current sleep mode of operation. 
 * @retval      The sleep mode.
 *              <ul>
 *                  <li> 0 if sleep is disabled
 *                  <li> 1 if extended sleep is enabled
 *                  <li> 2 if deep sleep is enabled
 *              </ul>
 ****************************************************************************************
 */
uint8_t arch_get_sleep_mode(void)
{
    uint8_t ret = 0;

	switch(sleep_env.slp_state)
	{
		case ARCH_SLEEP_OFF: 
            ret = 0; break;
		case ARCH_EXT_SLEEP_ON:
            ret = 1; break;
		case ARCH_DEEP_SLEEP_ON: 
            ret = 2; break;
	}

	return ret;
}


/**
 ****************************************************************************************
 * @brief       Restore the sleep mode to what it was before disabling. App should not modify the sleep mode directly.
 * @param       void
 * @details     Restores the sleep mode. 
 * @return      void
 ****************************************************************************************
 */
void arch_restore_sleep_mode(void)
{
    uint8_t cur_mode;
    
    if (sleep_cnt > 0)
        sleep_cnt--;
    
    if (sleep_cnt > 0)
        return;     // cannot restore it yet. someone else has requested active mode and we'll wait him to release it.
        
    if (sleep_pend != 0)
    {
        sleep_md = sleep_pend & 0x3;
        sleep_pend = 0;
    }
    else if (sleep_md)
        sleep_md--;
    
    cur_mode = sleep_md;
    sleep_md = 0;
    
    switch(cur_mode) 
    {
       case 0:  break;
       case 1:  arch_set_extended_sleep(); break;
       case 2:  arch_set_deep_sleep(); break;
       default: break;
    }
}


/**
 ****************************************************************************************
 * @brief       Disable sleep but save the sleep mode status. Store the sleep mode used by the app.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_force_active_mode(void)
{
    uint8_t cur_mode;
    
    sleep_cnt++;
    
    if (sleep_md == 0)  // add this check for safety! If it's called again before restore happens then sleep_md won't get overwritten
    {
        cur_mode = arch_get_sleep_mode();
        cur_mode++;     // 1: disabled, 2: extended, 3: deep sleep (!=0 ==> sleep is in forced state)
        arch_disable_sleep();
        sleep_md = cur_mode;
    }
}

/**
 ****************************************************************************************
 * @brief       Puts the BLE core to permanent sleep. Only an external event can wake it up.\
 *              BLE sleeps forever waiting a forced wakeup. After waking up from an external 
 *              event, if the system has to wake BLE up it needs to restore the default mode 
 *              of operation by calling app_ble_ext_wakeup_off() or the BLE won't be able to
 *              wake up in order to serve BLE events!
 * @param       void
 * @return      void
 * @todo        Check why an external request wakes up the BLE core, even when 
 *              rwip_env.ext_wakeup_enable is 0.
 * @exception   Warning Assertion
 *              if rwip_env.ext_wakeup_enable is 0 since it wouldn't be possible to wake-up the 
 *              BLE core in this case.
 ****************************************************************************************
 */
void arch_ble_ext_wakeup_on(void)
{
//    ASSERT_WARNING(rwip_env.ext_wakeup_enable != 0);
    sleep_ext_force = true;
}

/**
 ****************************************************************************************
 * @brief       Takes the BLE core out of the permanent sleep mode.Restore BLE cores' operation to default mode. The BLE core will wake up every 
 *              10sec even if no BLE events are scheduled. If an event is to be scheduled 
 *              earlier, then BLE will wake up sooner to serve it.
 * @param       void
 * @details     
 * @return      void
 ****************************************************************************************
 */
void arch_ble_ext_wakeup_off(void)
{
    sleep_ext_force = false;
}

/**
 ****************************************************************************************
 * @brief       Checks whether the BLE core is in permanent sleep mode or not.
 *              Returns the current mode of operation of the BLE core (external wakeup or default).
 * @param       void
 * @return      The BLE core's mode of operation (extended wakeup or normal).
 * @retval      The BLE core's sleep mode.
 *              <ul>
 *                  <li> false, if default mode is selected
 *                  <li> true, if BLE sleeps forever waiting for a forced wakeup
 *              </ul>
 ****************************************************************************************
 */
bool arch_ble_ext_wakeup_get(void)
{
    return sleep_ext_force;
}

/**
 ****************************************************************************************
 * @brief       Wake the BLE core via an external request. If the BLE core is sleeping (permanently or not and external wake-up is enabled)
 *              then this function wakes it up. 
 *              A call to app_ble_ext_wakeup_off() should follow if the BLE core was in permanent sleep.
 * @param       void
 * @return      status
 * @retval      The status of the requested operation.
 *              <ul>
 *                  <li> false, if the BLE core is not sleeping
 *                  <li> true, if the BLE core was woken-up successfully
 *              </ul>
 ****************************************************************************************
 */
bool arch_ble_force_wakeup(void)
{
    bool retval = false;
    
    // If BLE is sleeping, wake it up!
    GLOBAL_INT_DISABLE();
    if (GetBits16(CLK_RADIO_REG, BLE_ENABLE) == 0) { // BLE clock is off
        SetBits16(GP_CONTROL_REG, BLE_WAKEUP_REQ, 1);
        retval = true;
    }
    GLOBAL_INT_RESTORE();
    
    return retval;
}

/**
 ****************************************************************************************
 * @brief       Used for application's tasks synchronisation with ble events. 
 * @param       void
 * @return      rwble_last_event
 ****************************************************************************************
 */
last_ble_evt arch_last_rwble_evt_get(void)
{
    return arch_rwble_last_event;
}
