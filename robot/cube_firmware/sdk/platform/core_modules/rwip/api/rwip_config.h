/**
 ****************************************************************************************
 *
 * @file rwip_config.h
 *
 * @brief Configuration of the RW IP SW
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef _RWIP_CONFIG_H_
    #define _RWIP_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup ROOT
 * @{
 *
 *  Information about RW SW IP options and flags
 *
 *        BT_DUAL_MODE             BT/BLE Dual Mode
 *        BT_STD_MODE              BT Only
 *        BLE_STD_MODE             BLE Only
 *
 *        RW_DM_SUPPORT            Dual mode is supported
 *        RW_BLE_SUPPORT           Configured as BLE only
 *
 *        BLE_EMB_PRESENT          BLE controller exists
 *        BLE_HOST_PRESENT         BLE host exists
 *
 * @name RW Stack Configuration
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */


/******************************************************************************************/
/* --------------------------   GENERAL SETUP       --------------------------------------*/
/******************************************************************************************/

    /// Flag indicating if stack is compiled in dual or single mode
    #ifdef CFG_BT
        #define BLE_STD_MODE                0
         
        #ifdef CFG_BLE
            #define BT_DUAL_MODE            1
            #define BT_STD_MODE             0
        #else
            #define BT_DUAL_MODE            0
            #define BT_STD_MODE             1
        #endif
    #elif defined(CFG_BLE)
        #define BT_DUAL_MODE                0
        #define BT_STD_MODE                 0
        #define BLE_STD_MODE                1
    #endif // CFG_BT

    /// Flag indicating if Dual mode is supported
    #define RW_DM_SUPPORT                   BT_DUAL_MODE

    /// Flag indicating if BLE handles main parts of the stack
    #define RW_BLE_SUPPORT                  BLE_STD_MODE

    /// Flag indicating if stack is compiled for BLE1.0 HW or later
    #ifdef CFG_BLECORE_11
        #define BLE11_HW                    1
        #define BLE12_HW                    0
    #else
        #define BLE11_HW                    0
        #define BLE12_HW                    1
    #endif


/******************************************************************************************/
/* -------------------------   STACK PARTITIONING      -----------------------------------*/
/******************************************************************************************/

    #if BT_DUAL_MODE
        #define BT_EMB_PRESENT              1
        #define BLE_EMB_PRESENT             1
        #define BLE_HOST_PRESENT            0
        #define BLE_APP_PRESENT             0
    #elif BT_STD_MODE
        #define BT_EMB_PRESENT              1
        #define BLE_EMB_PRESENT             0
        #define BLE_HOST_PRESENT            0
        #define BLE_APP_PRESENT             0
    #elif BLE_STD_MODE
        #define BT_EMB_PRESENT              0
        
        #ifdef CFG_EMB
            #define BLE_EMB_PRESENT         1
        #else
            #define BLE_EMB_PRESENT         0
        #endif
        
        #ifdef CFG_HOST
            #define BLE_HOST_PRESENT        1
        #else
            #define BLE_HOST_PRESENT        0
        #endif
        
        #ifdef CFG_APP
            #define BLE_APP_PRESENT         1
        #else
            #define BLE_APP_PRESENT         0
        #endif
    #endif // BT_DUAL_MODE / BT_STD_MODE / BLE_STD_MODE

    /// Security Application
    #ifdef CFG_APP_SECURITY
        #define BLE_APP_SEC                 1
    #else
        #define BLE_APP_SEC                 0
    #endif

    /// Integrated processor host application with GTL iface
    #ifdef CFG_INTEGRATED_HOST_GTL
        #define BLE_INTEGRATED_HOST_GTL     1
    #else
        #define BLE_INTEGRATED_HOST_GTL     0
    #endif


/******************************************************************************************/
/* -------------------------   INTERFACES DEFINITIONS      -------------------------------*/
/******************************************************************************************/

    /// Generic Transport Layer
    #ifdef CFG_GTL
        #define GTL_ITF                     1
    #else
        #define GTL_ITF                     0
    #endif

    /// Host Controller Interface (Controller side)
    #define HCIC_ITF                        (!BLE_HOST_PRESENT)
    /// Host Controller Interface (Host side)
    #define HCIH_ITF                        (BLE_HOST_PRESENT && !BLE_EMB_PRESENT)

    #define TL_TASK_SIZE                    (GTL_ITF + HCIC_ITF + HCIH_ITF)


/******************************************************************************************/
/* --------------------------   BLE COMMON DEFINITIONS      ------------------------------*/
/******************************************************************************************/

    /// Kernel Heap memory sized reserved for allocate dynamically connection environment
    #define KE_HEAP_MEM_RESERVED            4

    #ifdef CFG_BLE
        /// Application role definitions
        #define BLE_BROADCASTER             (CFG_BROADCASTER || CFG_ALLROLES)
        #define BLE_OBSERVER                (CFG_OBSERVER    || CFG_ALLROLES)
        #define BLE_PERIPHERAL              (CFG_PERIPHERAL  || CFG_ALLROLES)
        #define BLE_CENTRAL                 (CFG_CENTRAL     || CFG_ALLROLES)

        #if !BLE_BROADCASTER && !BLE_OBSERVER && !BLE_PERIPHERAL && !BLE_CENTRAL
            #error "No application role defined"
        #endif

        /// Maximum number of simultaneous connections
        #if BLE_CENTRAL
            #define BLE_CONNECTION_MAX      CFG_CON
        #elif BLE_PERIPHERAL
            #define BLE_CONNECTION_MAX      1
        #else
            #define BLE_CONNECTION_MAX      1
        #endif

        /// Number of tx data buffers
        #if BLE_CONNECTION_MAX == 1
            #if BLE_CENTRAL || BLE_PERIPHERAL
                #define BLE_TX_BUFFER_DATA  5
            #else
                #define BLE_TX_BUFFER_DATA  0
            #endif
        #else
            #define BLE_TX_BUFFER_DATA      (BLE_CONNECTION_MAX * 3)
        #endif // BLE_CONNECTION_MAX == 1

        #if BLE_CENTRAL || BLE_PERIPHERAL
            /// Number of tx advertising buffers
            #define BLE_TX_BUFFER_ADV       3
            /// Number of tx control buffers
            #define BLE_TX_BUFFER_CNTL      BLE_CONNECTION_MAX
        #else
            #if BLE_BROADCASTER
                /// Number of tx advertising buffers
                #define BLE_TX_BUFFER_ADV   2
                /// Number of tx control buffers
                #define BLE_TX_BUFFER_CNTL  0
            #else
                /// Number of tx advertising buffers
                #define BLE_TX_BUFFER_ADV   1
                /// Number of tx control buffers
                #define BLE_TX_BUFFER_CNTL  0
            #endif
        #endif // BLE_CENTRAL || BLE_PERIPHERAL

        /// Total number of elements in the TX buffer pool
        #define BLE_TX_BUFFER_CNT           (BLE_TX_BUFFER_DATA + BLE_TX_BUFFER_CNTL + BLE_TX_BUFFER_ADV)

        /// Number of receive buffers in the RX ring. This number defines the interrupt
        /// rate during a connection event. An interrupt is asserted every BLE_RX_BUFFER_CNT/2
        /// reception. This number has an impact on the size of the exchange memory. This number
        /// may have to be increased when CPU is very slow to free the received data, in order not
        /// to overflow the RX ring of buffers.
        #if BLE_CENTRAL || BLE_PERIPHERAL
            #define BLE_RX_BUFFER_CNT       8
        #elif BLE_BROADCASTER
            #define BLE_RX_BUFFER_CNT       1
        #else
            #define BLE_RX_BUFFER_CNT       4
        #endif

        /// Max advertising reports before sending the info to the host
        #define BLE_ADV_REPORTS_MAX         1

        /// Use of security manager block
        #define RW_BLE_USE_CRYPT  CFG_SECURITY_ON

    #endif //defined(CFG_BLE)

    /// Accelerometer Application
    #define BLE_APP_ACCEL                   0

    #if defined(CFG_PRF_ACCEL) || defined(CFG_PRF_PXPR) || defined(CFG_APP_KEYBOARD) || \
        defined(CFG_PRF_STREAMDATAH) || defined(CFG_PRF_SPOTAR)
        #define BLE_APP_SLAVE               1
    #else
        #define BLE_APP_SLAVE               0
    #endif

    #ifndef __DA14581__
        #define BLE_APP_TASK_SIZE           (BLE_APP_PRESENT + BLE_APP_SEC + BLE_APP_HT + BLE_APP_NEB + \
                                             BLE_APP_HID + BLE_APP_DIS + BLE_APP_BATT + BLE_APP_ACCEL)
    #else
        #define BLE_APP_TASK_SIZE           BLE_APP_PRESENT
    #endif


/******************************************************************************************/
/* --------------------------      RTC SETUP         -------------------------------------*/
/******************************************************************************************/

    /// RTC enable/disable
    #ifdef CFG_RTC
        #define RTC_SUPPORT                 1
    #else
        #define RTC_SUPPORT                 0
    #endif


/******************************************************************************************/
/* -------------------------   DEEP SLEEP SETUP      -------------------------------------*/
/******************************************************************************************/

    /// DEEP SLEEP enable
    #define DEEP_SLEEP                              1


/******************************************************************************************/
/* -------------------------   COEXISTENCE SETUP      ------------------------------------*/
/******************************************************************************************/

    /// WLAN Coexistence
    #define RW_WLAN_COEX                    (defined(CFG_WLAN_COEX))

    ///WLAN test mode
    #ifdef CFG_WLAN_COEX
        #define RW_WLAN_COEX_TEST           (defined(CFG_WLAN_COEX_TEST))
    #else
        #define RW_WLAN_COEX_TEST           0
    #endif


/******************************************************************************************/
/* -------------------------   CHANNEL ASSESSMENT SETUP      -----------------------------*/
/******************************************************************************************/

    /// Channel Assessment
    #if defined(CFG_CHNL_ASSESS) && BLE_CENTRAL
        #define BLE_CHNL_ASSESS             1
    #else
        #define BLE_CHNL_ASSESS             0
    #endif


/******************************************************************************************/
/* --------------------------   DEBUG SETUP       ----------------------------------------*/
/******************************************************************************************/

    /// Number of tasks for DEBUG module
    #define DEBUG_TASK_SIZE                 1

    #define RW_DEBUG                        0
    #define RW_SWDIAG                       0
    #define KE_PROFILING                    0


/******************************************************************************************/
/* -------------------------   BLE / BLE HL CONFIG    ------------------------------------*/
/******************************************************************************************/

    #if BLE_EMB_PRESENT || BLE_HOST_PRESENT
        #include "rwble_config.h"   // ble stack configuration
    #endif

    #if BLE_HOST_PRESENT
        #include "rwble_hl_config.h"  // ble Host stack configuration
    #endif


/******************************************************************************************/
/* -------------------------   KERNEL SETUP          -------------------------------------*/
/******************************************************************************************/

    /// Flag indicating Kernel is supported
    #define KE_SUPPORT                      (BLE_EMB_PRESENT || BLE_HOST_PRESENT || BLE_APP_PRESENT)

/// Event types definition
enum KE_EVENT_TYPE
{
    #if DISPLAY_SUPPORT
    KE_EVENT_DISPLAY         ,
    #endif

    #if RTC_SUPPORT
    KE_EVENT_RTC_1S_TICK     ,
    #endif

    #if BLE_EMB_PRESENT
    KE_EVENT_BLE_CRYPT       ,
    #endif

    KE_EVENT_KE_MESSAGE      ,
    KE_EVENT_KE_TIMER        ,

    #if GTL_ITF
    KE_EVENT_GTL_TX_DONE     ,
    #endif

    #if HCIC_ITF || HCIH_ITF
    KE_EVENT_HCI_TX_DONE     ,
    #endif

    #if BLE_EMB_PRESENT
    KE_EVENT_BLE_EVT_END     ,
    KE_EVENT_BLE_RX          ,
    KE_EVENT_BLE_EVT_START   ,
    #endif

    KE_EVENT_MAX             ,
};

/// Tasks types definition
enum KE_TASK_TYPE
{
    TASK_NONE           = 0xFF,

    // Link Layer Tasks
    TASK_LLM            = 0   ,
    TASK_LLC            = 1   ,
    TASK_LLD            = 2   ,
    TASK_DBG            = 3   ,

    TASK_L2CM           = 4   ,
    TASK_L2CC           = 5   ,
    TASK_SMPM           = 6   ,
    TASK_SMPC           = 7   ,
    TASK_ATTM           = 8   ,   // Attribute Protocol Manager Task
    TASK_ATTC           = 9   ,   // Attribute Protocol Client Task
		
    TASK_ATTS           = 10  ,   // Attribute Protocol Server Task
    TASK_GATTM          = 11  ,   // Generic Attribute Profile Manager Task
    TASK_GATTC          = 12  ,   // Generic Attribute Profile Controller Task
    TASK_GAPM           = 13  ,   // Generic Access Profile Manager
    TASK_GAPC           = 14  ,   // Generic Access Profile Controller
    
    TASK_PROXM          = 15  ,   // Proximity Monitor Task
    TASK_PROXR          = 16  ,   // Proximity Reporter Task
    TASK_FINDL          = 17  ,   // Find Me Locator Task
    TASK_FINDT          = 18  ,   // Find Me Target Task
    TASK_HTPC           = 19  ,   // Health Thermometer Collector Task
    TASK_HTPT           = 20  ,   // Health Thermometer Sensor Task
    TASK_ACCEL          = 21  ,   // Accelerometer Sensor Task
    TASK_BLPS           = 22  ,   // Blood Pressure Sensor Task
    TASK_BLPC           = 23  ,   // Blood Pressure Collector Task
    TASK_HRPS           = 24  ,   // Heart Rate Sensor Task
    TASK_HRPC           = 25  ,   // Heart Rate Collector Task
    TASK_TIPS           = 26  ,   // Time Server Task
    TASK_TIPC           = 27  ,   // Time Client Task
    TASK_DISS           = 28  ,   // Device Information Service Server Task
    TASK_DISC           = 29  ,   // Device Information Service Client Task
    TASK_SCPPS          = 30  ,   // Scan Parameter Profile Server Task
    TASK_SCPPC          = 31  ,   // Scan Parameter Profile Client Task
    TASK_BASS           = 32  ,   // Battery Service Server Task
    TASK_BASC           = 33  ,   // Battery Service Client Task
    TASK_HOGPD          = 34  ,   // HID Device Task
    TASK_HOGPBH         = 35  ,   // HID Boot Host Task
    TASK_HOGPRH         = 36  ,   // HID Report Host Task
    TASK_GLPS           = 37  ,   // Glucose Profile Sensor Task
    TASK_GLPC           = 38  ,   // Glucose Profile Collector Task
    TASK_NBPS           = 39  ,   // Nebulizer Profile Server Task
    TASK_NBPC           = 40  ,   // Nebulizer Profile Client Task
    TASK_RSCPS          = 41  ,   // Running Speed and Cadence Profile Server Task
    TASK_RSCPC          = 42  ,   // Running Speed and Cadence Profile Collector Task
    TASK_CSCPS          = 43  ,   // Cycling Speed and Cadence Profile Server Task
    TASK_CSCPC          = 44  ,   // Cycling Speed and Cadence Profile Client Task
    TASK_ANPS           = 45  ,   // Alert Notification Profile Server Task
    TASK_ANPC           = 46  ,   // Alert Notification Profile Client Task
    TASK_PASPS          = 47  ,   // Phone Alert Status Profile Server Task
    TASK_PASPC          = 48  ,   // Phone Alert Status Profile Client Task

    TASK_LANS           = 49  ,   // Location and Navigation Profile Server Task
    TASK_APP            = 50  ,   // Do not Alter. 

    TASK_LANC           = 51  ,   // Location and Navigation Profile Client Task

    TASK_CPPS           = 52  ,   // Cycling Power Profile Server Task
    TASK_CPPC           = 53  ,   // Cycling Power Profile Client Task
    
    // Start of conditionally assigned task types
    
    #if (BLE_BM_SERVER)
    TASK_BMSS           ,   // BMSS Task
    #endif

    #if (BLE_BM_CLIENT)
    TASK_BMSC         ,   // BMSC Task
    #endif

    #if BLE_SPOTA_RECEIVER
    TASK_SPOTAR         ,   // SPOTA Receiver task
    #endif

    #if BLE_STREAMDATA_DEVICE
    TASK_STREAMDATAD    ,   // Stream Data Device Server task
    #endif

    #if BLE_STREAMDATA_HOST
    TASK_STREAMDATAH    ,   // Stream Data Device Server task
    #endif

    #if BLE_ANC_CLIENT
    TASK_ANCC           ,   // ANCS Client Task
    #endif

    #if BLE_WPT_CLIENT
    TASK_WPTC           ,   // A4WP Wireless Power Transfer Client Profile Task
    #endif

    #if BLE_WPTS
    TASK_WPTS           ,   // A4WP Wireless Power Transfer Server Profile Task
    #endif

    #if BLE_APP_PTU
     TASK_APP_PTU       ,   // A4WP Wireless Power Transfer Client App Task
    #endif

    #if BLE_IEU
    TASK_IEU            ,   // Integrated Environmantal Unit Task
    #endif

    #if BLE_MPU
    TASK_MPU            ,   // Motion Processing Unit Task
    #endif

    #if BLE_WSS_SERVER
    TASK_WSSS           ,   // Weight Scale Server Task
    #endif

    #if BLE_WSS_COLLECTOR
    TASK_WSSC           ,   // Weight Scale Collector Task
    #endif

    #if BLE_UDS_SERVER
    TASK_UDSS           ,   // User Data Server Task
    #endif

    #if BLE_UDS_CLIENT
    TASK_UDSC           ,   // User Data Server Task
    #endif

    #if BLE_SPS_SERVER
    TASK_SPS_SERVER     ,   // Serial Proert Service Server Task
    #endif

    #if BLE_SPS_CLIENT
    TASK_SPS_CLIENT     ,   // Serial Proert Service Server Task
    #endif

    #if BLE_ADC_NOTIFY
    TASK_ADC_NOTIFY     ,   // Serial Proert Service Server Task
    #endif

    #if BLE_DEVICE_CONFIG
    TASK_DEVICE_CONFIG  ,   // Serial Proert Service Server Task
    #endif

    #if (BLE_BCS_SERVER)
    TASK_BCSS          ,   // Body Composition Server Task
    #endif

    #if (BLE_BCS_CLIENT)
    TASK_BCSC          ,   // Body Composition Client Task
    #endif

    #if (BLE_CTS_SERVER)
    TASK_CTSS          ,   // Current Time Server Task
    #endif

    #if (BLE_CTS_CLIENT)
    TASK_CTSC          ,   // Current Time Client Task
    #endif

    #if BLE_CUSTOM2_SERVER
	TASK_CUSTS2		    ,	// 2nd Custom profile server
    #endif

    #if BLE_CUSTOM1_SERVER
	TASK_CUSTS1		    ,	// 1st Custom profile server
    #endif

    // End of conditionally assigned task types
    
    TASK_HCI            = 60  ,
    TASK_HCIH           = 61  ,

    TASK_GTL            = 63  ,

    #if USE_AES
    TASK_AES            = 62  ,   // Task for asynchronous AES API
    #endif    

    TASK_MAX            = 64,  //MAX is 64. Do  not exceed. 
};

/// Kernel memory heaps types.
enum
{
    /// Memory allocated for environment variables
    KE_MEM_ENV,
    
    #if 1//(BLE_HOST_PRESENT)
    /// Memory allocated for Attribute database
    KE_MEM_ATT_DB,
    #endif
    
    /// Memory allocated for kernel messages
    KE_MEM_KE_MSG,
    /// Non Retention memory block
    KE_MEM_NON_RETENTION,
    KE_MEM_BLOCK_MAX,
};

    #if BLE_EMB_PRESENT
        #define BLE_TASK_SIZE_      BLE_TASK_SIZE
    #else
        #define BLE_TASK_SIZE_      0
    #endif

    #if BLE_HOST_PRESENT
        #define BLEHL_TASK_SIZE_    BLEHL_TASK_SIZE
    #else
        #define BLEHL_TASK_SIZE_    0
    #endif


/******************************************************************************************/
/* --------------------------   DISPLAY SETUP        -------------------------------------*/
/******************************************************************************************/

    /// Number of tasks for DISPLAY module
    #define DISPLAY_TASK_SIZE       1

    /// Display controller enable/disable
    #ifdef CFG_DISPLAY
        #define DISPLAY_SUPPORT     1
    #else
        #define DISPLAY_SUPPORT     0
    #endif


    /// Number of Kernel tasks
    #define KE_TASK_SIZE            (DISPLAY_TASK_SIZE     + \
                                     DEBUG_TASK_SIZE       + \
                                     TL_TASK_SIZE          + \
                                     BLE_APP_TASK_SIZE     + \
                                     BLE_TASK_SIZE_        + \
                                     BLEHL_TASK_SIZE_         )

    #define KE_USER_TASK_SIZE       5

    // Heap header size is 12 bytes
    #define RWIP_HEAP_HEADER        (12 / sizeof(uint32_t)) //header size in uint32_t
    // ceil(len/sizeof(uint32_t)) + RWIP_HEAP_HEADER
    #define RWIP_CALC_HEAP_LEN(len) (((len + sizeof(uint32_t) - 1) / sizeof(uint32_t)) + RWIP_HEAP_HEADER)

/// @} BT Stack Configuration
/// @} ROOT

enum
{
    FLAG_FULLEMB_POS = 0,
    main_pos,                    
    rf_init_pos,                    
    prf_init_pos,                   
    calibrate_rc32KHz_pos,         
    calibrate_RF_pos,               
    uart_init_pos,                  
    uart_flow_on_pos,               
    uart_flow_off_pos,              
    uart_finish_transfers_pos,      
    uart_read_pos,                  
    uart_write_pos,                 
    UART_Handler_pos,               
    lld_sleep_compensate_pos,       
    lld_sleep_init_pos,             
    lld_sleep_us_2_lpcycles_pos,
    lld_sleep_lpcycles_2_us_pos,   
    hci_tx_done_pos,   
    hci_enter_sleep_pos,            
    app_sec_task_pos,           	
    rwip_heap_non_ret_pos,          
    rwip_heap_non_ret_size,         
    rwip_heap_env_pos,				
    rwip_heap_env_size,				
    rwip_heap_db_pos,				
    rwip_heap_db_size,				
    rwip_heap_msg_pos,				
    rwip_heap_msg_size,				
    offset_em_et,                    		         
    offset_em_ft,                   
    offset_em_enc_plain,            
    offset_em_enc_cipher,           
    offset_em_cs,                   
    offset_em_wpb,                  
    offset_em_wpv,                  
    offset_em_cnxadd,               
    offset_em_txe,                  
    offset_em_tx,                   
    offset_em_rx,                   
    nb_links_user,					
    sleep_wake_up_delay_pos,		
    lld_assessment_stat_pos,		
    lld_evt_init_pos,				
    gtl_eif_init_pos,				
    ke_task_init_pos,				
    ke_timer_init_pos,				
    llm_encryption_done_pos,		
    nvds_get_pos,					
    lld_evt_prog_latency_pos,		
    rwip_eif_get_pos,				
    prf_cleanup_pos,				
    lld_rx_irq_thres,			
    lld_init_pos,				
    enable_BLE_core_irq_pos,		
    platform_reset_pos,			
    gtl_hci_rx_header_func_pos,	
    gtl_hci_rx_payload_func_pos,		
    gtl_hci_tx_evt_func_pos,			
    GPADC_init_func_pos,				
    meas_precharge_freq_func_pos,	
    lut_cfg_pos,						
    check_pll_lock_func_pos,			
    update_calcap_min_channel_func_pos,	
    update_calcap_max_channel_func_pos,	
    write_one_SW_LUT_entry_func_pos,	
    write_HW_LUT_func_pos,				
    clear_HW_LUT_func_pos,				
    update_LUT_func_pos,					
    save_configure_restore_func_pos,		
    update_calcap_ranges_func_pos,		
    find_initial_calcap_ranges_func_pos,	
    pll_vcocal_LUT_InitUpdate_func_pos,	
    set_rf_cal_cap_func_pos,				
    enable_rf_diag_irq_func_pos,			
    modulationGainCalibration_func_pos,	
    DCoffsetCalibration_func_pos,		
    IffCalibration_func_pos,				
    rf_calibration_func_pos,				
    get_rc16m_count_func_pos,			
    set_gauss_modgain_func_pos,			
    lld_test_stop_func_pos,				   
    lld_test_mode_tx_func_pos,			   
    lld_test_mode_rx_func_pos,			   
    ret_rxwinsize_pos,					   
    max_sleep_duration_periodic_wakeup_pos, 
    max_sleep_duration_external_wakeup_pos, 
    #ifdef __DA14581__
    host_tester_task_pos,					
    host_gap_task_pos,						
    ke_task_size_pos,						
    #endif
};

#endif // _RWIP_CONFIG_H_
