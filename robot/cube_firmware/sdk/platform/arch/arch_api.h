/**
 ****************************************************************************************
 *
 * @file arch_api.h
 *
 * @brief Architecture functions API.
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

#ifndef _ARCH_API_H_
#define _ARCH_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "stdbool.h"
#include "nvds.h"
#include "arch.h"
#include "arch_wdg.h"
#include "rwble.h"

/*
##############################################################################
#   Sleep mode API                                                           #
##############################################################################
*/

typedef enum
{
	ARCH_SLEEP_OFF,
	ARCH_EXT_SLEEP_ON,
	ARCH_DEEP_SLEEP_ON,
} sleep_state_t;


/// Arch Sleep environment structure
struct arch_sleep_env_tag
{
 	sleep_state_t slp_state;
};

/**
 ****************************************************************************************
 * @brief       Disable all sleep modes. The system operates in active / idle modes only.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_disable_sleep(void);

/**
 ****************************************************************************************
 * @brief       Activates extended sleep mode. The system operates in idle / active / extended sleep modes.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_set_extended_sleep(void);

/**
 ****************************************************************************************
 * @brief       Activates deep sleep mode. The system operates in idle / active / deep sleep modes.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_set_deep_sleep(void);

/**
 ****************************************************************************************
 * @brief       Activates selected sleep mode.
 * @param       sleep_state     Selected sleep mode.
 * @return      void
 ****************************************************************************************
 */
void arch_set_sleep_mode(sleep_state_t sleep_state);

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
uint8_t arch_get_sleep_mode(void);

/**
 ****************************************************************************************
 * @brief       Restore the sleep mode to what it was before disabling. App should not modify the sleep mode directly.
 * @param       void
 * @details     Restores the sleep mode. 
 * @return      void
 ****************************************************************************************
 */
void arch_restore_sleep_mode(void);

/**
 ****************************************************************************************
 * @brief       Disable sleep but save the sleep mode status. Store the sleep mode used by the app.
 * @param       void
 * @return      void
 ****************************************************************************************
 */
void arch_force_active_mode(void);

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
void arch_ble_ext_wakeup_on(void);

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
void arch_ble_ext_wakeup_off(void);

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
bool arch_ble_ext_wakeup_get(void);

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
bool arch_ble_force_wakeup(void);

/**
 ****************************************************************************************
 * @brief       Modifies the system startup sleep delay. It can be called in app_init() to modify the default delay (2 sec);
 * @param[in]   Delay in BLE slots (0.625 usec)
 * @return      void
 ****************************************************************************************
 */

__INLINE void arch_startup_sleep_delay_set(uint32_t delay)
{
    startup_sleep_delay = delay;
}

/*
##############################################################################
#   BLE events API                                                           #
##############################################################################
*/

/*Last BLE event. Used for application's synchronisation with BLE activity */
typedef enum 
{
    BLE_EVT_SLP,
    BLE_EVT_CSCNT,
    BLE_EVT_RX,
    BLE_EVT_TX,
    BLE_EVT_END,

}last_ble_evt;

/**
 ****************************************************************************************
 * @brief       Used for application's tasks synchronisation with ble events. 
 * @param       void
 * @return      Last BLE event. See last_ble_evt enumaration
 ****************************************************************************************
 */
last_ble_evt arch_last_rwble_evt_get(void);

/*
##############################################################################
#   NVDS API                                                                 #
##############################################################################
*/

/**
 ****************************************************************************************
 * @brief Look for a specific tag and return, if found and matching (in length), the
 *        DATA part of the TAG. 
 *
 * If the length does not match, the TAG header structure is still filled, in order for
 * the caller to be able to check the actual length of the TAG.
 *
 * @param[in]  tag     TAG to look for whose DATA is to be retrieved
 * @param[in]  length  Expected length of the TAG
 * @param[out] buf     A pointer to the buffer allocated by the caller to be filled with
 *                     the DATA part of the TAG
 *
 * @return  NVDS_OK                  The read operation was performed
 *          NVDS_LENGTH_OUT_OF_RANGE The length passed in parameter is different than the TAG's
 ****************************************************************************************
 */
uint8_t nvds_get(uint8_t tag, nvds_tag_len_t * lengthPtr, uint8_t *buf);

/*
##############################################################################
#   DEVELOPMENT DEBUG API                                                    #
##############################################################################
*/

/**
 ****************************************************************************************
 * @brief Creates sw cursor in power profiler tool. Used for Development/ Debugging 
 *
 * @return void 
 ****************************************************************************************
 */
 
void arch_set_pxact_gpio(void);

/**
 ****************************************************************************************
 * ASSERTION CHECK
 ****************************************************************************************
 */
#ifndef ASSERT_ERROR
#define ASSERT_ERROR(x)                                     \
{                                                           \
    do {                                                    \
        if (DEVELOPMENT_DEBUG)                              \
        {                                                   \
            if (!(x))                                       \
            {                                               \
                __asm("BKPT #0\n");                         \
            }                                               \
        }                                                   \
        else                                                \
        {                                                   \
            if (!(x))                                       \
            {                                               \
                wdg_reload(1);                              \
                wdg_resume();                               \
                while(1);                                   \
            }                                               \
        }                                                   \
    } while (0);                                            \
}

#define ASSERT_WARNING(x)                                   \
{                                                           \
    do {                                                    \
        if (DEVELOPMENT_DEBUG)                              \
        {                                                   \
            if (!(x))                                       \
            {                                               \
                __asm("BKPT #0\n");                         \
            }                                               \
        }                                                   \
    } while (0);                                            \
}
#endif

/*
##############################################################################
#   MISC                                                                     #
##############################################################################
*/

/*
 * LOW POWER CLOCK - Used for CFG_LP_CLK configuration in da14580_config
 ****************************************************************************************
 */
#define	LP_CLK_XTAL32       0x00
#define LP_CLK_RCX20        0xAA
#define LP_CLK_FROM_OTP     0xFF

/*
 * Scatterfile: Memory maps
 ****************************************************
 */
#define DEEP_SLEEP_SETUP    1
#define EXT_SLEEP_SETUP     2

/**
 ****************************************************************************************
 * @brief Calculates the minimum time between the beginning of two consecutive ADV_IND PDUs within an
advertising event. Used to optimise the power profile of advertisisng event. 
 *
 * @param[in]  adv_len          ADV_IND PDU size 
 * @param[in]  scan_rsp_len     SCAN_RSP PDU size
 *
 * @return     uint32_t         optimal ADV_IND PDUs interval in uSec   
 ****************************************************************************************
 */
uint32_t arch_adv_time_calculate(uint8_t adv_len, uint8_t scan_rsp_len);


/*
##############################################################################
#   MAIN LOOP CALLBACKS API                                                  #
##############################################################################
*/

typedef enum {
    GOTO_SLEEP = 0,
    KEEP_POWERED
} arch_main_loop_callback_ret_t;

/*
 * Main loop callback functions' structure. Developer must set application's callback functions in
 * the initialization of the structure in user_callback_config.h 
 ****************************************************
 */
struct arch_main_loop_callbacks {
    void (*app_on_init)(void);
    arch_main_loop_callback_ret_t (*app_on_ble_powered)(void);
    arch_main_loop_callback_ret_t (*app_on_system_powered)(void);
    void (*app_before_sleep)(void);
    sleep_mode_t (*app_validate_sleep)(sleep_mode_t sleep_mode);
    void (*app_going_to_sleep)(sleep_mode_t sleep_mode);
    void (*app_resume_from_sleep)(void);  
};    


/*
##############################################################################
#   BLE METRICS API                                                          #
##############################################################################
*/
#if (BLE_METRICS)

typedef struct
{
    uint32_t    rx_pkt;
    uint32_t    rx_err;
    uint32_t    rx_err_crc;
    uint32_t    rx_err_sync;
}arch_ble_metrics_t;

extern arch_ble_metrics_t            metrics;

/**
 ****************************************************************************************
 * @brief Returns a pointer to ble metrics structure keeping device's statistics. 
 *
 * @param[in]  void
 *
  @return     arch_ble_metrics_t          Pointer to ble metrics structure         
 ****************************************************************************************
 */
__INLINE arch_ble_metrics_t * arch_ble_metrics_get(void)
{
    return &metrics;
}

/**
 ****************************************************************************************
 * @brief Reset the counters of ble metrics structure. 
 *
 * @param[in]  void
 *
 * @return     void         
 ****************************************************************************************
 */
__INLINE void arch_ble_metrics_reset(void)
{
    memset(&metrics, 0, sizeof(arch_ble_metrics_t));
}

#endif

#endif // _ARCH_API_H_
