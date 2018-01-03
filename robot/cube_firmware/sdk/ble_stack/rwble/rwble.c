/**
 ****************************************************************************************
 *
 * @file rwble.c
 *
 * @brief Entry points the BLE software
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup ROOT
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "arch.h"    
#include <string.h>
#include "co_version.h"
#include "rwble.h"
#if BLE_HCIC_ITF
#include "hcic.h"
#endif //BLE_HCIC_ITF
#include "ke_event.h"
#include "ke_timer.h"
#include "co_buf.h"
#include "lld.h"
#include "llc.h"
#include "llm.h"
#include "dbg.h"
#include "lld_evt.h"
#include "reg_blecore.h"
#include "syscntl.h"

#include "nvds.h"         // NVDS definitions

#include "gtl_eif.h"
#include "hcic_eif.h"
#include "uart.h"
#include "reg_ble_em_rx.h"
#include "ke_mem.h"
#include "pll_vcocal_lut.h"
#include "gpio.h"
#include "rf_580.h"
#include "adc.h"
#include "user_periph_setup.h"
#include "arch_api.h"

#include "arch_system.h"
#include "arch_wdg.h"
#include "llc_util.h"

#include "reg_ble_em_et.h"

last_ble_evt        arch_rwble_last_event                __attribute__((section("retention_mem_area0"), zero_init));
boost_overhead_st   set_boost_low_vbat1v_overhead   __attribute__((section("retention_mem_area0"), zero_init));
uint16_t            clk_freq_trim_reg_value         __attribute__((section("retention_mem_area0"), zero_init));
uint16_t            wakeup_lp_comp                  __attribute__((section("retention_mem_area0"), zero_init));

#if (BLE_METRICS)
arch_ble_metrics_t            metrics                  __attribute__((section("retention_mem_area0"), zero_init));
#endif //(BLE_METRICS)


/*
 * FORWARD DECLARATION OF GLOBAL FUNCTIONS
 ****************************************************************************************
 */
void ble_regs_pop (void);


/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */
extern uint8_t func_check_mem_flag;
extern uint32_t lp_clk_sel;


extern uint32_t sleep_lp_cycles;
extern uint32_t slp_period_retained;


#if (WLAN_COEX_ENABLED)
extern bool wlan_coex_lld_evt_start;
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
//#define LOW_LEVEL_GPIO_DEBUGGING
#if defined(LOW_LEVEL_GPIO_DEBUGGING)
#define DEACTIVATE_PAD_LATCHES                                  \
    {                                                           \
        SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);               \
    }

#define TOGGLE_GPIO13_LONG                                      \
    {                                                           \
        SetWord16(P13_MODE_REG, PID_GPIO|OUTPUT);               \
        SetWord16(P1_SET_DATA_REG, 0x8);                        \
        for (volatile int i = 0; i < 3; i++);                   \
        SetWord16(P1_RESET_DATA_REG, 0x8);                      \
    }
    
#define TOGGLE_GPIO13_SHORT                                     \
    {                                                           \
        SetWord16(P13_MODE_REG, PID_GPIO|OUTPUT);               \
        SetWord16(P1_SET_DATA_REG, 0x8);                        \
        SetWord16(P1_RESET_DATA_REG, 0x8);                      \
    }
    
#else
    
#define DEACTIVATE_PAD_LATCHES {}
#define TOGGLE_GPIO13_LONG {}
#define TOGGLE_GPIO13_SHORT {}
    
#endif // LOW_LEVEL_GPIO_DEBUGGING

    
/*
 * PROFILING CODE
 ****************************************************************************************
 */
#if (USE_POWER_OPTIMIZATIONS) && defined(POWER_OPT_PROFILING)
extern uint32_t dbg_evt_timer_wr_ptr;
extern uint32_t dbg_slp_rem[PROFILE_LOG_SZ];
extern uint32_t dbg_slp_rem_min;
extern uint32_t dbg_slp_rem_max;
extern int32_t  dbg_slp_rem_diff_min;

#define POWER_PROFILE_SLP_BEFORE                                \
    {                                                           \
        int ptr;                                                \
                                                                \
        if (dbg_evt_timer_wr_ptr != 0)                          \
            ptr = dbg_evt_timer_wr_ptr - 1;                     \
        else                                                    \
            ptr = PROFILE_LOG_SZ - 1;                           \
                                                                \
        dbg_slp_rem[ptr] = ble_finetimecnt_get();               \
        if (dbg_slp_rem_min > dbg_slp_rem[ptr])                 \
            dbg_slp_rem_min = dbg_slp_rem[ptr];                 \
                                                                \
        if (dbg_slp_rem_max < dbg_slp_rem[ptr])                 \
            dbg_slp_rem_max = dbg_slp_rem[ptr];                 \
    }

#define POWER_PROFILE_SLP_AFTER                                 \
    {                                                           \
        int ptr;                                                \
                                                                \
        if (dbg_evt_timer_wr_ptr != 0)                          \
            ptr = dbg_evt_timer_wr_ptr - 1;                     \
        else                                                    \
            ptr = PROFILE_LOG_SZ - 1;                           \
                                                                \
        int diff = dbg_slp_rem[ptr] - ble_finetimecnt_get();    \
        if (diff < dbg_slp_rem_diff_min)                        \
            dbg_slp_rem_diff_min = diff;                        \
    }
    
#else

#define POWER_PROFILE_SLP_BEFORE {}
#define POWER_PROFILE_SLP_AFTER  {}
    
#endif // (USE_POWER_OPTIMIZATIONS) && (POWER_OPT_PROFILING)


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
    
#if (!BLE_HOST_PRESENT)
/**
 ****************************************************************************************
 * @brief Initializes the diagnostic port.
 ****************************************************************************************
 */
static void rwble_diagport_init(void)
{
    uint8_t diag_cfg[NVDS_LEN_DIAG_BLE_HW];
    uint8_t length = NVDS_LEN_DIAG_BLE_HW;

    // Read diagport configuration from NVDS
    if(nvds_get(NVDS_TAG_DIAG_BLE_HW, &length, diag_cfg) == NVDS_OK)
    {
        ble_diagcntl_pack(1, diag_cfg[3], 1, diag_cfg[2], 1, diag_cfg[1], 1, diag_cfg[0]);
    }
    else
    {
        ble_diagcntl_set(0);
    }
}
 
void rwble_init(void)
{
    if (RW_BLE_SUPPORT)
        // Initialize buffers
        co_buf_init();

    if (BLE_HCIC_ITF)
        // In split mode, initialize the HCI
        hci_init(rwip_eif_get(RWIP_EIF_HCIC));
        
    // Initialize the Link Layer Driver
    lld_init(false);

    if (BLE_CENTRAL || BLE_PERIPHERAL)
        // Initialize the Link Layer Controller
        llc_init();
        
    // Initialize the Link Layer Manager
    llm_init();

    if (RW_BLE_SUPPORT)
        // Initialize the debug process
        dbg_init();
    

    // Initialize diagport - for test and debug only
    rwble_diagport_init();

    if (BLE_HOST_PRESENT)
        // Signal the completion of the boot process to the host layers
        llm_ble_ready();
        
}

#endif

#if RW_BLE_SUPPORT && HCIC_ITF

void rwble_send_message(uint32_t error)
{
    // Send HCI_Hw_Error_Evt message
    struct llm_hw_error_evt *evt;

    if(error == RESET_TO_ROM || error == RESET_AND_LOAD_FW)
    {
        // Send platform reset command complete if requested by user
        dbg_platform_reset_complete(error);
    }
    else
    {
        // Allocate a message structure for hardware error event
        evt = (struct llm_hw_error_evt *)KE_MSG_ALLOC(LLM_HW_ERROR_EVT, HOST_GAP_TASK,
                TASK_LLM, llm_hw_error_evt);

        // Fill the HW error code
        switch(error)
        {
            case RESET_MEM_ALLOC_FAIL: evt->hw_code = CO_ERROR_HW_MEM_ALLOC_FAIL; break;
            default: ASSERT_INFO(0, error, 0); break;
        }

        // Send the message
        ke_msg_send(evt);
    }
}

#endif //RW_BLE_SUPPORT && HCIC_ITF
 
unsigned char cpt_event = 0;
int transmitting_data = 0;

#if BLE_METRICS

/**
 ****************************************************************************************
 * @brief Hook function that is called upon each packet reception.
 *
 * An application that needs packet metrics has to define this hook function.
 *
 * @param[in] packet_error_status   The error status of the received packet.
 *                                  If 0 then the packet has no error,
 *                                  otherwise the packet has an error.
 *
 * @return void
 ****************************************************************************************
*/

void measure_errors_received(unsigned char pkts)
{
    uint8_t rx_cnt = pkts;
    uint8_t rx_hdl = co_buf_rx_current_get();
    struct lld_evt_tag *evt;
    evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);
    ke_task_id_t destid = (evt->conhdl == LLD_ADV_HDL) ? TASK_LLM : KE_BUILD_ID(TASK_LLC, evt->conhdl);
    uint16_t conhdl = KE_IDX_GET(destid);

    while(rx_cnt--)
    {
        struct co_buf_rx_desc *rxdesc = co_buf_rx_get(rx_hdl);
        unsigned char status = rxdesc->rxstatus & 0x7F;
        
        if ((status & (BLE_MIC_ERR_BIT | BLE_CRC_ERR_BIT | BLE_LEN_ERR_BIT | BLE_TYPE_ERR_BIT | BLE_SYNC_ERR_BIT))\
           && (llc_env[conhdl]->rssi > llm_get_min_rssi()))
        {
            metrics.rx_err++;
            if(status & BLE_SYNC_ERR_BIT)   metrics.rx_err_sync ++;
            if(status & BLE_CRC_ERR_BIT)    metrics.rx_err_crc ++;        
        }
        else
        {
            metrics.rx_pkt++;
        }
        
        rx_hdl = co_buf_rx_next(rx_hdl);
    }
}

#endif // BLE_METRICS


// ============================================================================================
// ==================== DEEP SLEEP PATCH - THIS CODE MUST STAY IN RAM =========================
// ============================================================================================
extern void rf_workaround_init(void);
extern void rf_reinit(void);

void lld_sleep_compensate_func_patched(void)
{
    uint32_t dur_us;
    uint32_t slot_corr;
    uint32_t fine_corr;
    
    // Get the number of low power sleep period
    uint32_t slp_period = ble_deepslstat_get();

    // Sanity check: The duration of a sleep is limited
    ASSERT_ERR(slp_period < LLD_EVT_MAX_SLEEP_DURATION);

    // Convert sleep duration into us
    dur_us = lld_sleep_lpcycles_2_us_sel_func(slp_period);
    slot_corr = dur_us / 625;
    fine_corr = 624 - (dur_us % 625);
    if (fine_corr == 0)
        fine_corr = 1;

    // The correction values are then deduced from the sleep duration in us
    ble_basetimecntcorr_set(slot_corr);
    ble_finecntcorr_set(fine_corr);

    // Start the correction
    ble_deep_sleep_corr_en_setf(1);

    if ((DEVELOPMENT_DEBUG) && (USE_POWER_OPTIMIZATIONS))
    {
        slp_period_retained = slp_period;
        // if this Assertion hits then the LP ISR lasts longer than the time
        // that has been reserved via LP_ISR_TIME_XTAL32_CYCLES and LP_ISR_TIME_USEC.
        if (sleep_lp_cycles && (sleep_lp_cycles < slp_period))
            ASSERT_WARNING(0);
    }

}




#if SKIP_SL_PATCH
/**
 ****************************************************************************************
 * @brief The following patch temporary drops the slave latency in the peripheral when
 *        the MD flag is set in the last received packet, or the last reception ended
 *        with an error.
 *        The function is called in the BLE_RX_Handler and the BLE_EVENT_Handler
 *        An application that needs to enable the patch has to define the SKIP_SL_PATCH
 *        in the project configuration file
 * @param[in] called_from   The handler that initiate the function call.
 *            RX_END_SERVICE or EVT_END_SERVICE.
 *
 * @return void
 ****************************************************************************************
*/

static uint8_t prev_rx __attribute__((section("retention_mem_area0"), zero_init));

void skip_slave_latency_patch(uint8_t called_from)
{

#if (BLE_PERIPHERAL)

    volatile uint8_t status;
    int em_idx;
    uint8_t rx_cnt = 0;
    volatile uint8_t rx_pkts;
    uint8_t hdl;
    struct co_buf_rx_desc *rxdesc = 0;
    volatile unsigned char error_cnd;
    volatile unsigned char md_flag;
    volatile unsigned i;
    struct lld_evt_tag *evt;

    rx_pkts = 0;
    // Get next event in the list of programmed events
    evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);

    // If no more events, exit
    if ((evt == NULL) || (!evt->prog))
        return;

    if (called_from == RX_END_SERVICE)
    {
        rx_pkts = LLD_RX_IRQ_THRES;
        prev_rx = 0;
    }
    else
    {
        // Check if event has been handled by reading the exchange table status
        em_idx = evt->time & 0x0F;
        status = ble_etstatus_getf(em_idx);
        if (status == EM_BLE_ET_READY)
            // Event is not processed yet, so we can exit
            return;
        if (status == EM_BLE_ET_PROCESSED)
        {
            // Get the total RX count in the control structure
            rx_cnt = ble_rxdesccnt_getf(evt->conhdl);
            rx_pkts=rx_cnt - evt->rx_cnt;
        }
    }
    if (rx_pkts > 0)
    {
        hdl = co_buf_rx_current_get();
        rxdesc = co_buf_rx_get(hdl);
        while (--rx_pkts)
        {
            hdl = co_buf_rx_next(hdl);
            rxdesc = co_buf_rx_get(hdl);
        }
        status = rxdesc->rxstatus & 0x7F;
        error_cnd = (status & (BLE_MIC_ERR_BIT | BLE_CRC_ERR_BIT | BLE_LEN_ERR_BIT | BLE_TYPE_ERR_BIT | BLE_SYNC_ERR_BIT |  BLE_SN_ERR_BIT));
        md_flag = ble_rxmd_getf(hdl);

        if (called_from == RX_END_SERVICE)
        {
            if (md_flag || error_cnd)
                if ((evt->inst_action == LLD_NO_ACTION))
                    prev_rx = 1;
        }
        else
        {
            prev_rx = 0;
            if (md_flag || error_cnd)
                if ((evt->inst_action == LLD_NO_ACTION))
                {
                    evt->waiting_evt |= LLD_EVT_WAITING_INSTANT;
                }
        }
    }
    else
    {
        if (called_from != RX_END_SERVICE)
        {
            if (
                ((rx_cnt != 0) && prev_rx) // Packets are multiple of four the last RX should tell us what to do
                || (rx_cnt == 0)           // End evt with no receptions - missed the master?
            )
            {
                if ((evt->inst_action == LLD_NO_ACTION))
                        evt->waiting_evt |= LLD_EVT_WAITING_INSTANT;
            }
            prev_rx = 0;
        }
    }
    #endif // (BLE_PERIPHERAL)
}

#else
void skip_slave_latency_patch(uint8_t called_from)
{
    return;
}
#endif // SKIP_SL_PATCH

#if !(USE_POWER_OPTIMIZATIONS)
/*********************************************************************************
 *** WAKEUP_LP_INT ISR
 ***/
void BLE_WAKEUP_LP_Handler(void)
{    
	volatile long t=0;

    if (!USE_WDOG)
        wdg_freeze(); //Prepare WDOG, i.e. stop
    
//    // Gives 1dB higher sensitivity - UNTESTED
//    if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 0x1) 
//    { 
//        // Boost-mode
//        SetBits16(DCDC_CTRL2_REG, DCDC_CUR_LIM, 0x8); // 80mA
//    }
//    else 
//    { 
//        // Buck-mode
//        SetBits16(DCDC_CTRL2_REG, DCDC_CUR_LIM, 0x4); // 40mA
//    }
    
	/*
	 * Wait and Switch to XTAL 16MHz
	 * (default after each wake-up is RC 16MHz, but XTAL initialization sequence has been already initiated by PMU)
	 * NOTE 1: 
	 *       1. If app does not need XTAL16MHz but RC16MHz is enough then skip this section!
	 *       2. Wait-loop BEFORE activating PERIPH_PD in order to save some power...
     *
     * NOTE 2:
     *       1. It will save some power if you lower the clock while waiting for XTAL16 to settle.
     *       2. Could also switch to 32KHz, but then processing power is dramatically reduced (e.g. patching() routine may be too slow).
	 */
    syscntl_use_lowest_amba_clocks();

    while ( !GetBits16(SYS_STAT_REG, XTAL16_SETTLED) )  // this takes some mili seconds
        __NOP(), __NOP(), __NOP();          // reduce some APB activity

    SetBits16(CLK_CTRL_REG, SYS_CLK_SEL, 0);// select XTAL 16MHz
    SetBits16(CLK_16M_REG, RC16M_ENABLE, 0);// save power from RC 16MHz
    
    // and restore clock rates (refer to a couple of lines above)
    syscntl_use_highest_amba_clocks();
    
    /*
     * Init System Power Domain blocks: GPIO, WD Timer, Sys Timer, etc.
	 * Power up and init Peripheral Power Domain blocks, and finally release the pad latches.
	 */
    if(GetBits16(SYS_STAT_REG, PER_IS_DOWN))
        periph_init();

	/*
	 * Since XTAL 16MHz is activated, power-up the Radio Subsystem (including BLE)
	 *
	 * Note that BLE core clock is masked in order to handle the case where RADIO_PD does not get into power down state.
	 * The BLE clock should be active only as long as system is running at XTAL 16MHz (not at RC16 or 32KHz).
	 * Also BLE clock should be enabled before powering up the RADIO Power Domain !
	 */
	SetBits16(CLK_RADIO_REG, BLE_ENABLE, 1); // BLE clock enable
	SetBits16(PMU_CTRL_REG, RADIO_SLEEP, 0); // Power up! Note: System must run at 16MHz when powering up RADIO_PD.
	while (!(GetWord16(SYS_STAT_REG) & RAD_IS_UP)) {}; // this may take up to 1/2 of the 32KHz clock period


    /* 
     * Wait for at least one Low Power clock edge after the power up of the Radio Power Domain *e.g. with ble_wait_lp_clk_posedge() )
     * or even better check the BLE_CNTL2_REG[WAKEUPLPSTAT] !
     * Thus you assure that BLE_WAKEUP_LP_IRQ is deasserted and BLE_SLP_IRQ is asserted.
     * After this check exit this ISE in order to proceed with BLE_SLP_Handler().
     */
	while ( GetBits32(BLE_CNTL2_REG, WAKEUPLPSTAT) || !GetBits32(BLE_INTSTAT_REG, SLPINTSTAT))
		if (t) break;

	// Now BLE_WAKEUP_LP_IRQ is deasserted and BLE_SLP_IRQ is asserted, so exit in order to proceed with BLE_SLP_Handler().
	// NOTE: If returning from BLE_WAKEUP_LP_Handler() will not cause BLE_SLP_Handler() to start, 
	//       but the code after __WFI() is executed, then THERE WAS A SW SETUP PROBLEM !!! 
	//			 so it is recommended to place a check after __WFI().
}

/*********************************************************************************
 *** CSCNT_INT ISR
 ***/
#ifdef __GNUC__
void __real_BLE_CSCNT_Handler(void);
#else
void $Super$$BLE_CSCNT_Handler(void);
#endif

#ifdef __GNUC__
void __wrap_BLE_CSCNT_Handler(void)
#else
void $Sub$$BLE_CSCNT_Handler(void)
#endif
{
    // Call the ROM handler
#ifdef __GNUC__
    __real_BLE_CSCNT_Handler();
#else
    $Super$$BLE_CSCNT_Handler();
#endif
    
    // Re-schedule any past events here
    lld_evt_start_isr();

    arch_rwble_last_event = BLE_EVT_CSCNT;
}

#if (WLAN_COEX_ENABLED)
/*********************************************************************************
 *** FINE_INT_ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_FINETGTIM_Handler(void)
#else
void $Sub$$BLE_FINETGTIM_Handler(void)
#endif
{
    uint32_t irq_stat = GetWord16(BLE_INTSTAT_REG);

    // FINE interrupt
    if (irq_stat & BLE_FINETGTIMINTSTAT_BIT)
    {
        wlan_coex_lld_evt_start = 1;

        lld_evt_start_isr();
    }
}
#endif // WLAN_COEX_ENABLED


/*********************************************************************************
 *** SLP_INT ISR
 ***/
void BLE_SLP_Handler(void)
{
	ble_regs_pop();

	SetBits16(GP_CONTROL_REG, BLE_WAKEUP_REQ, 0);   //just to be sure    

    if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 ))
	{
		// UART and pads have already been activated by periph_init() which is called
		// at initialization by main_func() and during wakeup by BLE_WAKEUP_LP_Handler().
            
        if (BLE_HOST_PRESENT)		
            gtl_eif_init();
        else
            hci_eif_init();

	}

	SetBits32(BLE_INTACK_REG, SLPINTACK, 1);

    if (RW_BLE_SUPPORT)
        rwip_wakeup();
    
	/*
	* Radio Subsystem initialization. Execute here after making sure that BLE core is awake.
	*/
	rf_workaround_init();
	rf_reinit();	

    if ( arch_clk_is_RCX20() )
        calibrate_rcx20(20);
    
    arch_rwble_last_event = BLE_EVT_SLP;
}

/*********************************************************************************
 *** RX_INT ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_RX_Handler(void)
#else
void $Sub$$BLE_RX_Handler(void)
#endif
{
    
    if (BLE_METRICS)
        measure_errors_received(LLD_RX_IRQ_THRES);

    if (SKIP_SL_PATCH)	
    skip_slave_latency_patch( RX_END_SERVICE);
	
    lld_evt_rx_isr();
}

/*********************************************************************************
 *** EVENT_INT ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_EVENT_Handler(void)
#else
void $Sub$$BLE_EVENT_Handler(void)
#endif
{
    // Check BLE interrupt status and call the appropriate handlers
    //uint32_t irq_stat = ble_intstat_get();
    uint32_t irq_stat = GetWord16(BLE_INTSTAT_REG);

    // End of event interrupt
    if (irq_stat & BLE_EVENTINTSTAT_BIT)
    {
                
         if (BLE_METRICS)
         {
            struct lld_evt_tag *evt;
            uint8_t rx_cnt;

             evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);
             rx_cnt = ble_rxdesccnt_getf(evt->conhdl);

            // Event has been processed, handle the transmitted and received data
             measure_errors_received( rx_cnt - evt->rx_cnt);
         }            
         if (SKIP_SL_PATCH)
            skip_slave_latency_patch( EVT_END_SERVICE);
        
        lld_evt_end_isr();
        
        // Do a SW reset
        // Reset the BLE state machines
        ble_master_soft_rst_setf(1);
        while(ble_master_soft_rst_getf());
        //If an encryption was on going restart it
        if( ble_aes_start_getf())
        {
            ble_aescntl_set(BLE_AES_START_BIT);
        }

        arch_rwble_last_event = BLE_EVT_END;
    }
}

/*******************************************************************************/
#else // USE_POWER_OPTIMIZATIONS
/*******************************************************************************/


/*********************************************************************************
 *** WAKEUP_LP_INT ISR
 ***/
void power_up(void)
{
    /*
     * Check if XTAL 16MHz is already being used.
     */
    if (GetBits16(CLK_CTRL_REG, SYS_CLK_SEL) != 0)
    {
        /*
         * Switch to XTAL 16MHz
         */
        SetBits16(CLK_CTRL_REG, SYS_CLK_SEL, 0); // select XTAL 16MHz
        SetBits16(CLK_16M_REG, RC16M_ENABLE, 0); // save power from RC 16MHz
    }

    /*
     * Since XTAL 16MHz is activated, power-up the Radio Subsystem (including BLE)
     *
     * Note that BLE core clock is masked in order to handle the case where RADIO_PD does not
     * get into power down state. The BLE clock should be active only as long as system is 
     * running at XTAL 16MHz (not at RC16 or 32KHz). Also the BLE clock should be enabled
     * before powering up the RADIO Power Domain!
     */
    SetBits16(CLK_RADIO_REG, BLE_ENABLE, 1); // BLE clock enable
    SetBits16(PMU_CTRL_REG, RADIO_SLEEP, 0); // Power up! Note: System must run at 16MHz when powering up RADIO_PD.
    while (!(GetWord16(SYS_STAT_REG) & RAD_IS_UP)) {}; // this may take up to 1/2 of the 32KHz clock period

    /*
     * BLE is up. The register status can be restored.
     */
    ble_regs_pop();

    /* 
     * Check if BLE_SLP_IRQ has already asserted. In this case, we delayed in periph_init().
     * Increase LP_ISR_TIME_XTAL32_CYCLES and LP_ISR_TIME_USEC values to give more execution time
     * to periph_init().
     */    
    if ( GetBits32(BLE_INTSTAT_REG, SLPINTSTAT) )
        ASSERT_WARNING(0);
        
    /* 
     * Wait for BLE_SLP_IRQ to be asserted. Use low clock until LP is deasserted to preserve power.
     * After this check, exit this Exception Handler in order to proceed with the BLE_SLP_Handler().
     */    
    while ( !GetBits32(BLE_INTSTAT_REG, SLPINTSTAT) ) {};
        
    POWER_PROFILE_SLP_BEFORE
}


void BLE_WAKEUP_LP_Handler(void)
{
    if (wakeup_lp_comp == 0)
    {
        wakeup_lp_comp = 1;
        
        if (!USE_WDOG)
            wdg_freeze(); // Prepare WDOG, i.e. stop

        DEACTIVATE_PAD_LATCHES
        TOGGLE_GPIO13_LONG
        
        /*
         * Check if XTAL 16MHz is already being used.
         */
        if (GetBits16(CLK_CTRL_REG, SYS_CLK_SEL) != 0)
        {
            // 1. Power up peripherals' power domain
            if(GetBits16(SYS_STAT_REG, PER_IS_DOWN))
                periph_init();
            
            // 2. Enable RC32
            SetBits16(CLK_32K_REG, RC32K_ENABLE, 1);
            
            // 3. XTAL16M Trim Procedure
            // 3a. Start counting
            SetWord16(CLK_REF_CNT_REG, 255); // 255/32kHz = 11.1ms
            SetWord16(CLK_REF_SEL_REG, REF_CAL_START); // select RC32kHz oscillator, start!

            // 3b. Lower clocks
            syscntl_use_lowest_amba_clocks();
            
            // 3c. Wait until num_clk_cycles have been measured
            while(1)
            {
                uint32_t cnt = (GetWord16(CLK_REF_VAL_H_REG) << 16) + GetWord16(CLK_REF_VAL_L_REG);
                if (cnt > CLK_TRIM_WAIT_CYCLES)
                    break;
            }
            SetWord16(CLK_REF_SEL_REG, (REF_CAL_START | 1)); // stop!
            
            // 3d. Apply Trim value
            SetWord16(CLK_FREQ_TRIM_REG, clk_freq_trim_reg_value);
            
            // 4. Disable RC32
            SetBits16(CLK_32K_REG, RC32K_ENABLE, 0);
        }
        // Trigger SLP (by SW!) to continue (it will be executed when the LP_IRQ goes down)
        NVIC_SetPendingIRQ(BLE_SLP_IRQn);
    }
}


/*********************************************************************************
 *** SLP_INT ISR
 ***/
void BLE_SLP_Handler(void)
{
    syscntl_use_highest_amba_clocks();
    
    TOGGLE_GPIO13_SHORT
    
    power_up(); // power-up the BLE core
    
    wakeup_lp_comp = 0; // clear flag for next time

    SetBits16(GP_CONTROL_REG, BLE_WAKEUP_REQ, 0);   //just to be sure    

    if((BLE_APP_PRESENT == 0) || (BLE_INTEGRATED_HOST_GTL == 1 ))
    {
        // UART and pads have already been activated by periph_init() which is called
        // at initialization by main_func() and during wakeup by BLE_WAKEUP_LP_Handler().
		
        if (BLE_HOST_PRESENT)		
            gtl_eif_init();
        else
            hci_eif_init();
    }

    SetBits32(BLE_INTACK_REG, SLPINTACK, 1);

    if (RW_BLE_SUPPORT)
        rwip_wakeup();

    NVIC_ClearPendingIRQ(BLE_SLP_IRQn); // Still pending because it has been re-asserted within SLP ISR

    TOGGLE_GPIO13_LONG
    
    POWER_PROFILE_SLP_AFTER
    
    /*
     * Radio Subsystem initialization.
     */
    rf_workaround_init();
    rf_reinit();	

    /*
     * Start RCX calibration (if applicable).
     */
    if ( arch_clk_is_RCX20() )
        calibrate_rcx20(20);
    
    /*
     * Mark event type.
     */
    arch_rwble_last_event = BLE_EVT_SLP;

    /*
     * When in BOOST mode, sample VBAT1V to handle longer startup delays at low voltages.
     */
    if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 1)
    {
        if (set_boost_low_vbat1v_overhead == NOT_MEASURED)
        {
            // default is to apply the overhead
            set_boost_low_vbat1v_overhead = APPLY_OVERHEAD;
            
            // Use ADC to measure VBAT1V (approx. 40usec delay)
            if (GetBits16(GP_ADC_CTRL_REG, GP_ADC_EN) == 0)
            {
                uint32_t adc_sample = adc_get_vbat_sample(true);

                if (adc_sample > 0x22C) // higher than 1V
                    set_boost_low_vbat1v_overhead = NO_OVERHEAD;
            }
        }
    }
}


/*********************************************************************************
 *** CSCNT ISR
 ***/
bool fine_hit __attribute__((section("retention_mem_area0"), zero_init));

static void ble_start(void)
{
    fine_hit = true;
        
    lld_evt_start_isr();
}

#ifdef __GNUC__
void __real_BLE_CSCNT_Handler(void);
#else
void $Super$$BLE_CSCNT_Handler(void);
#endif

#ifdef __GNUC__
void __wrap_BLE_CSCNT_Handler(void)
#else
void $Sub$$BLE_CSCNT_Handler(void)
#endif
{
    // Call the ROM handler
#ifdef __GNUC__
    __real_BLE_CSCNT_Handler();
#else
    $Super$$BLE_CSCNT_Handler();
#endif
    
    ble_start();

    arch_rwble_last_event = BLE_EVT_CSCNT;
}


/*********************************************************************************
 *** FINE_INT_ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_FINETGTIM_Handler(void)
#else
void $Sub$$BLE_FINETGTIM_Handler(void)
#endif
{
    uint32_t irq_stat = GetWord16(BLE_INTSTAT_REG);

    // FINE interrupt
    if (irq_stat & BLE_FINETGTIMINTSTAT_BIT)
    {
        #if (WLAN_COEX_ENABLED)
            wlan_coex_lld_evt_start = 1;
        #endif
        
        ble_start();
        //SetBits32(BLE_INTACK_REG, FINETGTIMINTACK, 1);

        if ( arch_clk_is_RCX20() )
            calibrate_rcx20(20);
    }
}

/*********************************************************************************
 *** RX_INT ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_RX_Handler(void)
#else
void $Sub$$BLE_RX_Handler(void)
#endif
{
    if (BLE_METRICS)
        measure_errors_received(LLD_RX_IRQ_THRES);

    if (SKIP_SL_PATCH)
        skip_slave_latency_patch(RX_END_SERVICE);

    lld_evt_rx_isr();
    //SetBits32(BLE_INTACK_REG, RXINTACK, 1);
}



/*********************************************************************************
 *** EVENT_INT ISR
 ***/
#ifdef __GNUC__
void __wrap_BLE_EVENT_Handler(void)
#else
void $Sub$$BLE_EVENT_Handler(void)
#endif
{
    // Check BLE interrupt status and call the appropriate handlers
    //uint32_t irq_stat = ble_intstat_get();
    uint32_t irq_stat = GetWord16(BLE_INTSTAT_REG);

    // End of event interrupt
    if (irq_stat & BLE_EVENTINTSTAT_BIT)
    {

        if (BLE_METRICS)
        {
            struct lld_evt_tag *evt;
            uint8_t rx_cnt;

            evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);
            rx_cnt = ble_rxdesccnt_getf(evt->conhdl);

            // Event has been processed, handle the transmitted and received data
            measure_errors_received(rx_cnt - evt->rx_cnt);
        }            

        if (SKIP_SL_PATCH)
            skip_slave_latency_patch( EVT_END_SERVICE);

        lld_evt_end_isr();

        // Do a SW reset
        // Reset the BLE state machines
        ble_master_soft_rst_setf(1);
        while(ble_master_soft_rst_getf());
        //If an encryption was on going restart it
        if( ble_aes_start_getf())
        {
            ble_aescntl_set(BLE_AES_START_BIT);
        }

        arch_rwble_last_event = BLE_EVT_END;
    }
}

#endif // USE_POWER_OPTIMIZATIONS

#ifdef __GNUC__
void __wrap_BLE_RF_DIAG_Handler(void)
#else
void $Sub$$BLE_RF_DIAG_Handler(void)
#endif
{
    uint32 irq_scr;
    uint16_t cn;    
    cn = GetWord16(RF_BMCW_REG) & 0x003F;    

    irq_scr = GetWord32(BLE_RF_DIAGIRQ_REG);  // read BLE_RF_DIAGIRQ_REG so that you clear DIAGIRQ_STAT_0 (otherwise interrupt is activated again and again!)

    if (LUT_PATCH_ENABLED)
    {
        const volatile struct LUT_CFG_struct *pLUT_CFG;	// = (const volatile struct LUT_CFG_struct *)(jump_table_struct[lut_cfg_pos]);
        pLUT_CFG = (const volatile struct LUT_CFG_struct *)(jump_table_struct[lut_cfg_pos]);
        if(!pLUT_CFG->HW_LUT_MODE)
        { 
            set_rf_cal_cap(cn); 
        }
    }
  
    if (MGCKMODA_PATCH_ENABLED) 
    {
        if(GetBits16(RF_MGAIN_CTRL_REG, GAUSS_GAIN_SEL) && (irq_scr & DIAGIRQ_STAT_0) && (irq_scr & DIAGIRQ_MASK_0))  // If GAUSS_GAIN_SEL==0x1 AND it is an TX_EN interrupt (for RX_EN int it is not necessary to run)
        { 
            set_gauss_modgain(cn); 
        }
    }
  
	if(( irq_scr & DIAGIRQ_STAT_0) && ( irq_scr & DIAGIRQ_MASK_0))//check TXEN posedge 
	{	

 
        arch_rwble_last_event = BLE_EVT_TX;
        
        #if (PRODUCTION_TEST)  
            test_tx_packet_nr++;
        #endif    
	}
    
	if(( irq_scr & DIAGIRQ_STAT_1) && ( irq_scr & DIAGIRQ_MASK_1))//check RXEN posedge 
	{	
        arch_rwble_last_event = BLE_EVT_RX;
        
        #if (PRODUCTION_TEST)          
            test_rx_irq_cnt++;
        #endif
	}  
}

/// @} RWBLE
