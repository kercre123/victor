/**
 ****************************************************************************************
 *
 * @file wsss.h
 *
 * @brief Header file - Weight Scale Service Server.
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

#ifndef WSSS_H_
    #define WSSS_H_
    /**
     ************************************************************************************
     * @addtogroup WSSS Weight Scale Service Server
     * @ingroup WSS
     * @brief Weight Scale Service Server
     * @{
     ************************************************************************************
     */

    /// UserData Service Server Role
    #define BLE_WSS_SERVER              1
    #if !defined (BLE_SERVER_PRF)
        #define BLE_SERVER_PRF          1
    #endif

    /*
     * INCLUDE FILES
     ************************************************************************************
     */
    #if (BLE_WSS_SERVER)
        #include "wss_common.h"

        /*
         * DEFINES
         ********************************************************************************
         */

        /// Maximum length of Measurement Characteristic value
        #define WSS_MEAS_VAL_MAX_LEN               (15)

        /*
         * MACROS
         ********************************************************************************
         */

        #define WSSS_IS_SUPPORTED(mask) \
            (((wsss_env.features & mask) == mask))

        #define WSSS_IS_ENABLED(mask) \
            (((wsss_env.evt_cfg & mask) == mask))

        /*
         * ENUMERATIONS
         ********************************************************************************
         */

        ///Attributes State Machine
        enum
        {
            WSS_IDX_SVC,
            WSS_IDX_INCL_SVC,

            WSS_IDX_FEATURE_CHAR,
            WSS_IDX_FEATURE_VAL,

            WSS_IDX_WT_MEAS_CHAR,
            WSS_IDX_WT_MEAS_VAL,
            WSS_IDX_WT_MEAS_IND_CFG,

            WSS_IDX_NB,
        };

        ///Attribute Table Indexes
        enum
        {
            WSS_FEATURE_CHAR,
            WSS_MEASUREMENT_CHAR,
            
            WSS_CHAR_MAX,
        };


        /// Indication/notification configuration (put in feature flag to optimize memory usage)
        enum
        {
            /// Bit used to know if weight measurement indication is enabled
            WSSS_WT_MEAS_IND_CFG            = 0x02,
        };

        /*
         * TYPE DEFINITIONS
         ********************************************************************************
         */

        ///Weight Scale Service Server Environment Variable
        struct wsss_env_tag
        {
            /// Connection Info
            struct prf_con_info con_info;

            /// Service Start HandleVAL
            uint16_t shdl;

            ///Database Configuration
            uint8_t features;    

            ///Event (notification/indication) config
            uint8_t evt_cfg;
            
            /// Attribute Table
            uint8_t att_tbl[WSS_CHAR_MAX];
        };



        /*
         * GLOBAL VARIABLE DECLARATIONS
         ********************************************************************************
         */

        extern const struct attm_desc wsss_att_db[WSS_IDX_NB];

        extern const att_svc_desc_t wss_svc;

        extern const struct att_char_desc wsss_feature_char;
        extern const struct att_char_desc wsss_measurement_char;

        extern struct wsss_env_tag wsss_env;

        /*
         * FUNCTION DECLARATIONS
         ********************************************************************************
         */

        /**
         ********************************************************************************
         * @brief Initialization of the WSSS module.
         * This function performs all the initializations of the WSSS module.
         ********************************************************************************
         */
        void wsss_init(void);

        /**
         ********************************************************************************
         * @brief Check if an attribute shall be added or not in the database
         * @param features WSS features
         ********************************************************************************
         */
        uint32_t wsss_compute_cfg_flag(uint32_t features);

        /**
         ********************************************************************************
         * @brief Check if the provided value length matches characteristic requirements
         * @param char_code Characteristic Code
         * @param val_len   Length of the Characteristic value
         ********************************************************************************
         */
        uint8_t wsss_check_val_len(uint8_t char_code, uint8_t val_len);

        /**
         ********************************************************************************
         * @brief Send a WSSS_MEAS_SEND_CFM message to the application.
         * @param status Confirmation Status
         ********************************************************************************
         */
        void wsss_meas_send_cfm_send(uint8_t status);

        /**
         ********************************************************************************
         * @brief Pack Weight measurement value
         * @param p_meas_val Weight measurement value struct
         ********************************************************************************
         */
        void wsss_pack_meas_value(uint8_t *packed_ws, const struct wss_wt_meas* pmeas_val);

        /**
         ********************************************************************************
         * @brief Calculate Weight measurement size
         * @param p_meas_val Weight measurement value struct
         ********************************************************************************
         */
        uint8_t wsss_calc_meas_value_size(uint8_t flags);

        /**
         ********************************************************************************
         * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
         ********************************************************************************
         */
        void wsss_disable(uint16_t conhdl); 
    #endif //BLE_WSS_SERVER
    /// @} WSSS
#endif // WSSS_H_
