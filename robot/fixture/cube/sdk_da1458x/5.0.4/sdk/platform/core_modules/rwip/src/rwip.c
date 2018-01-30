/**
****************************************************************************************
*
* @file rwip.c
*
* @brief RW IP SW main module
*
* Copyright (C) RivieraWaves 2009-2013
*
*
****************************************************************************************
*/

/**
 ****************************************************************************************
 * @addtogroup RW IP SW main module
 * @ingroup ROOT
 * @brief The RW IP SW main module.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"    // RW SW configuration

#include <string.h>         // for mem* functions
#include "co_version.h"
#include "rwip.h"           // RW definitions
#include "arch.h"           // Platform architecture definition
#include "arch_api.h"


#if (BLE_EMB_PRESENT)
    #include "rwble.h"          // rwble definitions
#endif //BLE_EMB_PRESENT

#if (BLE_HOST_PRESENT)
    #include "rwble_hl.h"       // BLE HL definitions
    #include "gapc.h"
    #include "smpc.h"
    #include "gattc.h"
    #include "attc.h"
    #include "atts.h"
    #include "l2cc.h"
#endif //BLE_HOST_PRESENT

#if (BLE_APP_PRESENT)
    #include "app.h"            // Application definitions
#endif //BLE_APP_PRESENT



#if USE_POWER_OPTIMIZATIONS
    #include "syscntl.h"
#endif
#include "lld_sleep.h"      // definitions for sleep mode
#include "llc.h"

#include "plf.h"            // platform definition
#include "rf.h"             // RF definitions

#if (GTL_ITF)
    #include "gtl.h"
    #include "gtl_eif.h"
#endif //GTL_ITF

#if (BLE_EMB_PRESENT)
    #include "hcic.h"           // HCI definition
    #include "hcic_eif.h"
#endif // BLE_EMB_PRESENT

#if (KE_SUPPORT)
    #include "ke.h"             // kernel definition
    #include "ke_event.h"       // kernel event
    #include "ke_timer.h"       // definitions for timer
    #include "ke_mem.h"         // kernel memory manager
    #include "ke_env.h"
#endif //KE_SUPPORT

#include "dbg.h"            // debug definition

#if ((BLE_APP_PRESENT) || ((BLE_HOST_PRESENT && (!GTL_ITF))))
    #include "app.h"
#endif //BLE_APP_PRESENT

#include "em_map_ble_user.h"
#include "em_map_ble.h"
#include "reg_ble_em_rx.h"
#include "arch_system.h"


uint32_t sleep_lp_cycles __attribute__((section("retention_mem_area0"), zero_init));
uint32_t slp_period_retained __attribute__((section("retention_mem_area0"), zero_init)); // sleep counter value at wakeup



/*
 * FORWARD DECLARATION OF GLOBAL FUNCTIONS
 ****************************************************************************************
 */
void ble_regs_push(void);
void smpc_regs_push(void);


/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */
extern boost_overhead_st set_boost_low_vbat1v_overhead;
extern uint32_t rcx_freq;
extern uint32_t lp_clk_sel;
extern bool sys_startup_flag;
extern uint8_t func_check_mem_flag;
extern uint32_t rcx_slot_duration_num;
extern uint32_t rcx_slot_duration_den;
extern uint8_t ke_mem_heaps_used;
extern struct arch_sleep_env_tag sleep_env;


/*
 * PROFILING CODE
 ****************************************************************************************
 */
#if (USE_POWER_OPTIMIZATIONS) && defined(POWER_OPT_PROFILING)

#define __RETAINED __attribute__((section("retention_mem_area0"), zero_init))

uint32_t dbg_evt_times[PROFILE_LOG_SZ];         // evt->time
uint32_t dbg_check_times[PROFILE_LOG_SZ];       // time left (in usec) after all sleep_duration checks have been completed
uint32_t dbg_sleep_times[PROFILE_LOG_SZ];       // sleep_duration
uint32_t dbg_remaining_times[PROFILE_LOG_SZ];   // time left just before sleep() is called (should be greater than 4 x LP cycles plus the time for the SLP ISR)
uint8_t  dbg_sleep_flag[PROFILE_LOG_SZ];        // 0xFF is sleep_duration is greater than the average sleep duration of the previous 10 sleeps
uint32_t dbg_slp_rem[PROFILE_LOG_SZ];

uint32_t dbg_slp_rem_min        __RETAINED;     // time remaining for SLP
uint32_t dbg_slp_rem_max        __RETAINED;
int32_t  dbg_slp_rem_diff_min   __RETAINED;     // time spent in SLP ISR until clock compensation has been requested
uint32_t dbg_evt_timer_wr_ptr   __RETAINED;     // write index to the dbg_ arrays
uint32_t dbg_sleep_dur_last     __RETAINED;     // last calculated sleep_duration
uint32_t dbg_sleep_dur_avg      __RETAINED;     // average sleep_duration of the previous 10 sleeps
uint32_t dbg_sleep_dur_sum      __RETAINED;     // sum of the sleep_duration of the previous 10 sleeps (used for averaging)
uint32_t dbg_sleep_dur_meas     __RETAINED;     // counter to know when we have 10 samples available for averaging
uint32_t dbg_remaining_time_min __RETAINED;     // minimum time remaining for sleep() and the SLP ISR 


void pow_profile_exec1(uint32_t sleep_duration)
{
    struct lld_evt_tag *evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);

    dbg_sleep_times[dbg_evt_timer_wr_ptr] = sleep_duration;
        
    dbg_sleep_dur_last = sleep_duration;
    if (dbg_sleep_dur_meas < 10)
    {
        dbg_sleep_dur_meas++;
        dbg_sleep_dur_sum += sleep_duration;
    }
    else
    {
        dbg_sleep_dur_avg = dbg_sleep_dur_sum / 10;
        dbg_sleep_dur_sum = 0;
        dbg_sleep_dur_meas = 0;
    }
    
    do
    {
        if (evt == NULL)
            break;

        if (evt->prog)
            break;
        
        dbg_evt_times[dbg_evt_timer_wr_ptr] = evt->time;
        if (sleep_duration > dbg_sleep_dur_avg)
            dbg_sleep_flag[dbg_evt_timer_wr_ptr] = 0xFF;
        else
            dbg_sleep_flag[dbg_evt_timer_wr_ptr] = 0;
    } while (0);        
}


void pow_profile_exec2(void)
{
    dbg_remaining_times[dbg_evt_timer_wr_ptr] = ble_finetimecnt_get();
    if (dbg_remaining_time_min > dbg_remaining_times[dbg_evt_timer_wr_ptr])
        dbg_remaining_time_min = dbg_remaining_times[dbg_evt_timer_wr_ptr];
    
    dbg_evt_timer_wr_ptr++;
    if (dbg_evt_timer_wr_ptr == PROFILE_LOG_SZ) 
        dbg_evt_timer_wr_ptr = 0;
}


// Initialize dbg_remaining_time_min
    #define POWER_PROFILE_INIT                                          \
    {                                                                   \
        if (sys_startup_flag)                                           \
        {                                                               \
            dbg_remaining_time_min = 0xFFFFFFFF;                        \
            dbg_slp_rem_min = 0xFFFFFFFF;                               \
            dbg_slp_rem_diff_min = 1000000;                             \
        }                                                               \
    }


// Update dbg_check_times
    #define POWER_PROFILE_CHECKS_COMPLETED                              \
    {                                                                   \
        dbg_check_times[dbg_evt_timer_wr_ptr] = ble_finetimecnt_get();  \
    }


    #define POWER_PROFILE_SLEEP_TIMES                                   \
    {                                                                   \
        pow_profile_exec1(sleep_duration);                              \
    }


    #define POWER_PROFILE_REMAINING_TIME                                \
    {                                                                   \
        pow_profile_exec2();                                            \
    }


#else
    
    #define POWER_PROFILE_INIT {}
    #define POWER_PROFILE_CHECKS_COMPLETED {}
    #define POWER_PROFILE_SLEEP_TIMES {}
    #define POWER_PROFILE_REMAINING_TIME {}
    
#endif // (USE_POWER_OPTIMIZATIONS) && (POWER_OPT_PROFILING)


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static uint32_t rwip_slot_2_lpcycles(uint32_t slot_cnt)
{
    uint32_t lpcycles;

    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(slot_cnt < 1000000);

    // Compute the low power clock cycles - case of a 32.768kHz clock
    lpcycles = (slot_cnt << 11)/100;
    
    if(lpcycles)
    {
       lpcycles--;
    }

    
    return(lpcycles);
}


static uint32_t rwip_slot_2_lpcycles_rcx(uint32_t slot_cnt)
{
    volatile uint32_t lpcycles;

    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(slot_cnt < 1000000);

    lpcycles = (uint32_t)((uint64_t)slot_cnt * rcx_slot_duration_num) / rcx_slot_duration_den;
    
    return(lpcycles);
}



sleep_mode_t rwip_sleep(void)
{
    sleep_mode_t proc_sleep = mode_active;
    uint32_t twirq_set_value;
    uint32_t twirq_reset_value;
    uint32_t twext_value;

    uint32_t sleep_duration = jump_table_struct[max_sleep_duration_external_wakeup_pos];//MAX_SLEEP_DURATION_EXTERNAL_WAKEUP;
    
    // We may lower the clock now while we are waiting the BLE to go to sleep...
    bool slow_system_clk = false;

    uint32_t xtal16m_settling_cycles;
    uint32_t tmp_dur;
    
#if (!DEVELOPMENT_DEBUG)
    uint32_t sleep_lp_cycles;
#endif

    DBG_SWDIAG(SLEEP, ALGO, 0);

    if (BLE_APP_PRESENT)
    {
        if ( arch_ble_ext_wakeup_get() || (rwip_env.ext_wakeup_enable == 2) )  // sleep forever!
            sleep_duration = 0;
    }
    else if (!EXTERNAL_WAKEUP) // sleep_duration will remain as it was set above....
    {
        if (rwip_env.ext_wakeup_enable == 2)
            sleep_duration = 0;
    }
        
    do
    {
        /************************************************************************
         **************            CHECK STARTUP FLAG             **************
         ************************************************************************/
        POWER_PROFILE_INIT;

        // Do not allow sleep if system is in startup period
        if (check_sys_startup_period())
            break;
        
        /************************************************************************
         **************            CHECK KERNEL EVENTS             **************
         ************************************************************************/
        // Check if some kernel processing is ongoing
        if (!ke_sleep_check())
            break;
        
        // Processor sleep can be enabled
        proc_sleep = mode_idle;

        DBG_SWDIAG(SLEEP, ALGO, 1);

        

        /************************************************************************
         **************             CHECK ENABLE FLAG              **************
         ************************************************************************/
        // Check sleep enable flag
        if(!rwip_env.sleep_enable)
            break;

        
        /************************************************************************
         **************           CHECK RADIO POWER DOWN           **************
         ************************************************************************/
        // Check if BLE + Radio are still sleeping
        if(GetBits16(SYS_STAT_REG, RAD_IS_DOWN)) {
            // If BLE + Radio are in sleep return the appropriate mode for ARM
            proc_sleep = mode_sleeping;
            break;
        }

        /************************************************************************
         **************              CHECK RW FLAGS                **************
         ************************************************************************/
        // First check if no pending procedure prevents us from going to sleep
        if (rwip_prevent_sleep_get() != 0)
            break;

        DBG_SWDIAG(SLEEP, ALGO, 2);

        
        /************************************************************************
         **************           CHECK EXT WAKEUP FLAG            **************
         ************************************************************************/
        /* If external wakeup is enabled, sleep duration can be set to maximum, otherwise
         * the system must be woken-up periodically to poll incoming packets from HCI */
        if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 )) // No need for periodic wakeup if we have full-hosted system
        {
            if(!rwip_env.ext_wakeup_enable)
                sleep_duration = jump_table_struct[max_sleep_duration_periodic_wakeup_pos]; // MAX_SLEEP_DURATION_PERIODIC_WAKEUP;
        }

        /************************************************************************
         *                                                                      *
         *                   CHECK DURATION UNTIL NEXT EVENT                    *
         *                                                                      *
         ************************************************************************/
        // If there's any timer pending, compute the time to wake-up to serve it
        if (ke_env.queue_timer.first != NULL)
            sleep_duration = jump_table_struct[max_sleep_duration_external_wakeup_pos];
            if (USE_POWER_OPTIMIZATIONS)
            {
                // Store sleep_duration calculated so far. Check below if sleep would be allowed.
                // If not, there's no reason to verify / ensure the available time for SLP...
                tmp_dur = sleep_duration;
            }
    

        /************************************************************************
         **************            CHECK KERNEL TIMERS             **************
         ************************************************************************/
        // Compute the duration up to the next software timer expires
        if (!ke_timer_sleep_check(&sleep_duration, rwip_env.wakeup_delay))
            break;
         
        // Check if a kernel timer has expired before the above ke_timer_sleep_check() 
        // function call. If that happens the ke_timer_sleep_check() will incorrectly 
        // allow the system to enter sleep, because the timeout of the first timer is 
        // already in the past, and consequently the timer event will not be serviced 
        // until the next system wake up (the rwip_sleep() function is being called 
        // while the interrupts are disabled).
        if (ble_intrawstat_get() & BLE_GROSSTGTIMINTRAWSTAT_BIT)
            break;

        DBG_SWDIAG(SLEEP, ALGO, 3);

        if (BLE_EMB_PRESENT)
        {
            /************************************************************************
             **************                 CHECK BLE                  **************
             ************************************************************************/
            // Compute the duration up to the next BLE event
            if (!lld_sleep_check(&sleep_duration, rwip_env.wakeup_delay))
                break;
        }
        
        DBG_SWDIAG(SLEEP, ALGO, 4);

        if (HCIC_ITF)
        /************************************************************************
         **************                 CHECK HCI                  **************
         ************************************************************************/
        {
            if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 ))
            {       
                // Try to switch off HCI
                if (!hci_enter_sleep())
                    break;
            }
        }
        
        if (GTL_ITF)
        /************************************************************************
         **************                 CHECK TL                   **************
         ************************************************************************/
        {
            if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 ))
            {
                // Try to switch off Transport Layer
                if (!gtl_enter_sleep())
                    break;
            }
        }

        DBG_SWDIAG(SLEEP, ALGO, 6);
        
        if (USE_POWER_OPTIMIZATIONS)
        {
            /************************************************************************
             ******      BLOCK UNTIL THERE'S TIME FOR sleep() AND SLP ISR      ******
             ************************************************************************/
            
            bool rcx_duration_corr = false;
            
            // Restore sleep_duration
            sleep_duration = tmp_dur;
            /*
             * Wait until there's enough time for SLP to restore clocks when the chip wakes up.
             * Then check again if sleep is possible.
             */
            if ( arch_clk_is_RCX20() )
            {
                xtal16m_settling_cycles = lld_sleep_us_2_lpcycles_sel_func(XTAL16M_SETTLING_IN_USEC);
                
                while ( (ble_finetimecnt_get() < 550) && (ble_finetimecnt_get() > 200) ); 
                // If we are close to the end of this slot then the actual sleep entry will
                // occur during the next one. But the sleep_duration will have been calculated
                // based on the current slot...
                if (ble_finetimecnt_get() <= 200)
                    rcx_duration_corr = true;
            }
            else if ( arch_clk_is_XTAL32( ) )
            {
                while (ble_finetimecnt_get() < 300);  
            }
            
            /************************************************************************
             *                                                                      *
             *                   CHECK DURATION UNTIL NEXT EVENT                    *
             *                       (this is the 2nd check)                        *
             *                                                                      *
             ************************************************************************/
            bool sleep_check = false;
            
            do
            {
                /************************************************************************
                 **************            CHECK KERNEL TIMERS (2)         **************
                 ************************************************************************/
                // Compute the duration up to the next software timer expires
                if (!ke_timer_sleep_check(&sleep_duration, rwip_env.wakeup_delay))
                    break;
                
                // Check if a kernel timer has expired before the above ke_timer_sleep_check() 
                // function call. If that happens the ke_timer_sleep_check() will incorrectly 
                // allow the system to enter sleep, because the timeout of the first timer is 
                // already in the past, and consequently the timer event will not be serviced 
                // until the next system wake up (the rwip_sleep() function is being called 
                // while the interrupts are disabled).
                if (ble_intrawstat_get() & BLE_GROSSTGTIMINTRAWSTAT_BIT)
                    break;

                DBG_SWDIAG(SLEEP, ALGO, 3);

                if (BLE_EMB_PRESENT)
                {
                    /************************************************************************
                     **************                 CHECK BLE (2)              **************
                     ************************************************************************/
                    // Compute the duration up to the next BLE event
                    if (!lld_sleep_check(&sleep_duration, rwip_env.wakeup_delay))
                        break;
                }
                
                sleep_check = true;
                
            } while(0);
            
            if (!sleep_check)
            {
                if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 ))
                {
                    if (BLE_HOST_PRESENT)		
                            gtl_eif_init();
                    else
                            hci_eif_init();
                    
                }
                // sleep is aborted and serial i/f communication is restored
                break;
            }
            
            if (sleep_duration && rcx_duration_corr)
                sleep_duration--;
            
            DBG_SWDIAG(SLEEP, ALGO, 4);
        }

        POWER_PROFILE_CHECKS_COMPLETED;
        
        /************************************************************************
         **************          PROGRAM CORE DEEP SLEEP           **************
         ************************************************************************/
        if ( arch_clk_is_RCX20() )
        {
            if (!USE_POWER_OPTIMIZATIONS)
            {
               twirq_set_value = lld_sleep_us_2_lpcycles_sel_func(XTAL_TRIMMING_TIME_USEC);
                
                twirq_reset_value = TWIRQ_RESET_VALUE;
                
                // TWEXT setting
                twext_value = TWEXT_VALUE_RCX;
            }
            else
            {
                // Calculate the time we need to wake-up before "time 0" to do XTAL16 settling,
                // call periph_init() and power-up the BLE core.
                uint32_t lpcycles = lld_sleep_us_2_lpcycles_sel_func(LP_ISR_TIME_USEC);
                
                // Set TWIRQ_SET taking into account that some LP cycles are needed for the power up FSM.
                twirq_set_value = RCX_POWER_UP_TIME + lpcycles;
                if (sleep_env.slp_state == ARCH_DEEP_SLEEP_ON)
                    twirq_set_value += RCX_OTP_COPY_OVERHEAD;
                
                // BOOST mode + RCX is not supported
                if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 1)
                    ASSERT_WARNING(0);

                // Program LP deassertion to occur when the XTAL16M has settled
                twirq_reset_value = lpcycles - xtal16m_settling_cycles;
                
                // TWEXT setting
                twext_value = lpcycles;
             }
        }
        else if ( arch_clk_is_XTAL32( ) )
        {
            if (!USE_POWER_OPTIMIZATIONS)
            {
                twirq_set_value = XTAL_TRIMMING_TIME;
                twirq_reset_value = TWIRQ_RESET_VALUE;
                twext_value = TWEXT_VALUE_XTAL32;
            }
            else       
            {
                // The time we need to wake-up before "time 0" to do XTAL16 settling,
                // call periph_init() and power-up the BLE core is LP_ISR_TIME_XTAL32_CYCLES in this case.
                
                // Set TWIRQ_SET taking into account that some LP cycles are needed for the power up FSM.
                twirq_set_value = XTAL32_POWER_UP_TIME + LP_ISR_TIME_XTAL32_CYCLES;
                if (sleep_env.slp_state == ARCH_DEEP_SLEEP_ON)
                    twirq_set_value += XTAL32_OTP_COPY_OVERHEAD;
                
                // Adjust TWIRQ_SET in case of BOOST mode, if needed
                if (set_boost_low_vbat1v_overhead == APPLY_OVERHEAD)
                    twirq_set_value += BOOST_POWER_UP_OVERHEAD;
                set_boost_low_vbat1v_overhead = NOT_MEASURED;

                // Program LP deassertion to occur when the XTAL16M has settled
                twirq_reset_value = LP_ISR_TIME_XTAL32_CYCLES - XTAL16M_SETTLING_IN_XTAL32_CYCLES;
                
                // TWEXT setting
                twext_value = LP_ISR_TIME_XTAL32_CYCLES;
            }
        }

       
        //Prepare BLE_ENBPRESET_REG for next sleep cycle
        SetBits32(BLE_ENBPRESET_REG, TWIRQ_RESET, twirq_reset_value);   // TWIRQ_RESET
        SetBits32(BLE_ENBPRESET_REG, TWIRQ_SET, twirq_set_value);       // TWIRQ_SET
        SetBits32(BLE_ENBPRESET_REG, TWEXT, twext_value);               // TWEXT

        //Everything ready for sleep!
        proc_sleep = mode_sleeping;
        
        if (USE_POWER_OPTIMIZATIONS)
        {
            // Eliminate any additional delays.
            if (sleep_duration) 
                sleep_duration += SLEEP_DURATION_CORR;
            
            POWER_PROFILE_SLEEP_TIMES;
        }

        if (BLE_EMB_PRESENT)
        {
            // Put BT core into deep sleep
            if ( arch_clk_is_XTAL32( ) )
                sleep_lp_cycles = rwip_slot_2_lpcycles(sleep_duration);
            else if ( arch_clk_is_RCX20() )
                sleep_lp_cycles = rwip_slot_2_lpcycles_rcx(sleep_duration);
            
            lld_sleep_enter(sleep_lp_cycles, rwip_env.ext_wakeup_enable);
        }
     
        DBG_SWDIAG(SLEEP, SLEEP, 1);

        /************************************************************************
         **************               SWITCH OFF RF                **************
         ************************************************************************/        
        POWER_PROFILE_REMAINING_TIME;
        
        rwip_rf.sleep();
        
        if (USE_POWER_OPTIMIZATIONS)
        {
            
            if (BLE_APP_PRESENT)            
            {
                if ( app_use_lower_clocks_check() )
                {
                    // It will save some power if you lower the clock while waiting for STAT...
                    SetBits16(CLK_AMBA_REG, PCLK_DIV, 3);  // lowest is 2MHz (div 8, source is @16MHz)
                    SetBits16(CLK_AMBA_REG, HCLK_DIV, 3);
                    
                    slow_system_clk = true;
                }
            }
        }
                
        while(!ble_deep_sleep_stat_getf());
                
        //check and wait till you may disable the radio. 32.768KHz XTAL must be running!
        //(debug note: use BLE_CNTL2_REG:MON_LP_CLK bit to check (write 0, should be set to 1 by the BLE))
        while ( !(GetWord32(BLE_CNTL2_REG) & RADIO_PWRDN_ALLOW) ) {};
        
        if (USE_POWER_OPTIMIZATIONS)
        {
            if (slow_system_clk)
            {
                // and restore clock rates (refer to a couple of lines above)
                syscntl_use_highest_amba_clocks();
            }
        }

        ble_regs_push();    // push the ble ret.vars to retention memory
//        smpc_regs_push();   // push smpc ble ret.vars to retention memory
        
        //BLE CLK must be turned off when DEEP_SLEEP_STAT is set
        SetBits16(CLK_RADIO_REG, BLE_ENABLE, 0);   												

    } while(0);

    return proc_sleep;
}


#if (!BLE_HOST_PRESENT)
void rwip_init(uint32_t error)
{
    uint8_t length = 1;
    uint8_t sleep_enable;
    uint8_t ext_wakeup_enable;

    // Reset RW environment
    memset(&rwip_env, 0, sizeof(rwip_env));
	
	ke_mem_heaps_used = KE_MEM_BLOCK_MAX;

    #if (KE_SUPPORT)
    // Initialize kernel
    ke_init();
    // Initialize memory heap used by kernel.
    #if (KE_MEM_RW)
    // Memory allocated for environment variables
    ke_mem_init(KE_MEM_ENV,(uint8_t*)(jump_table_struct[rwip_heap_env_pos]),     jump_table_struct[rwip_heap_env_size]);	
    // Memory allocated for Attribute database
    ke_mem_init(KE_MEM_ATT_DB,(uint8_t*)(jump_table_struct[rwip_heap_db_pos]),      jump_table_struct[rwip_heap_db_size]);	        
    // Memory allocated for kernel messages
    ke_mem_init(KE_MEM_KE_MSG,(uint8_t*)(jump_table_struct[rwip_heap_msg_pos]),     jump_table_struct[rwip_heap_msg_size]);
    // Non Retention memory block
    ke_mem_init(KE_MEM_NON_RETENTION, (uint8_t*)(jump_table_struct[rwip_heap_non_ret_pos]), jump_table_struct[rwip_heap_non_ret_size]);

    #endif // (KE_MEM_RW)
    #endif //KE_SUPPORT

    #if (GTL_ITF)
    // Initialize the Generic Transport Layer
    gtl_init(rwip_eif_get(RWIP_EIF_AHI));
    #endif //GTL_ITF

    // Initialize RF
    rf_init(&rwip_rf);
    SetBits32(BLE_RADIOCNTL1_REG, XRFSEL, 3);

    #if (BLE_EMB_PRESENT)
    // Initialize BLE
    rwble_init();
    #endif //BLE_EMB_PRESENT

    #if (BLE_HOST_PRESENT)
    // Initialize BLE Host stack
    rwble_hl_init();
    #endif //BLE_HOST_PRESENT

    // Activate deep sleep feature if enabled in NVDS
    if(nvds_get(NVDS_TAG_SLEEP_ENABLE, &length, &sleep_enable) == NVDS_OK)
    {
        if(sleep_enable != 0)
        {
            rwip_env.sleep_enable = true;

            // Set max sleep duration depending on wake-up mode
            if(nvds_get(NVDS_TAG_EXT_WAKEUP_ENABLE, &length, &ext_wakeup_enable) == NVDS_OK)
            {
                if(ext_wakeup_enable != 0)
                {
                    rwip_env.ext_wakeup_enable = ext_wakeup_enable;
                }
            }
        }
    }

    // If FW initializes due to FW reset, send the message to Host
    if(error != RESET_NO_ERROR)
    {

#if (BLE_EMB_PRESENT && HCIC_ITF)
        rwble_send_message(error);
#elif (BLE_HOST_PRESENT && GTL_ITF)
        rwble_hl_send_message(error);
#endif // BLE_EMB_PRESENT
    }

    /*
     ************************************************************************************
     * Application initialization
     ************************************************************************************
     */
    // moved to main
    //    if(jump_table_struct[0]==TASK_APP)
    // Initialize APP
    //    app_init();

    func_check_mem_flag = 0;//false;
}


void rwip_wakeup(void)
{
    DBG_SWDIAG(SLEEP, SLEEP, 0);

    // Prevent going to deep sleep until a slot interrupt is received
    rwip_prevent_sleep_set(RW_WAKE_UP_ONGOING);

    #if (BLE_EMB_PRESENT)
    // Wake-up BLE core
    lld_sleep_wakeup();
    #endif //BLE_EMB_PRESENT

    #if (HCIC_ITF)
    // Restart the flow on the HCI
    hci_exit_sleep();
    #endif //HCIC_ITF

    #if (GTL_ITF)
    // Restart the flow on the GTL
    gtl_exit_sleep();
    #endif //GTL
}
#endif // (!BLE_HOST_PRESENT)
