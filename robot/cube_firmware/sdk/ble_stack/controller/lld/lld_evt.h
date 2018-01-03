/**
 ****************************************************************************************
 *
 * @file lld_evt.h
 *
 * @brief Declaration of functions used for event scheduling
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef LLD_EVT_H_
#define LLD_EVT_H_

/**
 ****************************************************************************************
 * @addtogroup LLDEVT
 * @ingroup LLD
 * @brief Event scheduling functions
 *
 * This module implements the primitives used for event scheduling
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "rwip_config.h"
#include "co_list.h"
#include "reg_blecore.h"
#include "llm.h"
#include "llc_task.h"


#include "arch.h"

// Forward declarations
struct llc_con_up_req;

/*
 * DEFINES
 ****************************************************************************************
 */
/// Size of the LLD event table. It shall be equal to the max number of supported
/// connections * 2 + 1 for the scanning/advertising
#define LLD_EVT_TAB_SIZE  (BLE_CONNECTION_MAX * 2 + 1)

/// Invalid value for an interval
#define LLD_EVT_INTERVAL_INVALID  0xFFFF

/// Minimal time required between event programming and event start
#if (RW_DM_SUPPORT)
#define LLD_EVT_PROG_LATENCY      jump_table_struct[lld_evt_prog_latency_pos] //6
#else
#define LLD_EVT_PROG_LATENCY      jump_table_struct[lld_evt_prog_latency_pos] //4
#endif // RW_DM_SUPPORT

/// Maximum value an interval can take. This value is for time comparison
#define MAX_INTERVAL_TIME  3193600  // Max number of 625us periods for a connection interval

/// Default RX window size
#define LLD_EVT_DEFAULT_RX_WIN_SIZE     14

// Sanity check
#if (LLD_EVT_DEFAULT_RX_WIN_SIZE > 15)
#error "LLD_EVT_DEFAULT_RX_WIN_SIZE is too large. It should be less than 16"
#endif // LLD_EVT_DEFAULT_RX_WIN_SIZE > 15
/// Default RX window offset
#define LLD_EVT_RX_WIN_DEFAULT_OFFSET   (40 - LLD_EVT_DEFAULT_RX_WIN_SIZE/2)

/// Flag forcing the slave to wait for the next acknowledgment
#define LLD_EVT_WAITING_ACK             0x01
/// Flag forcing the slave to wait for the next sync with the master
#define LLD_EVT_WAITING_SYNC            0x02
/// Flag forcing the slave to wake up for a programmed transmission
#define LLD_EVT_WAITING_TXPROG          0x04
/// Flag forcing the slave to wake up at instant
#define LLD_EVT_WAITING_INSTANT         0x08

/// Maximum duration of a sleep, in low power clock cycles (around 300s)
#define LLD_EVT_MAX_SLEEP_DURATION      12000000

/// Default sleep duration when no event is programmed (in slot count)
#define LLD_EVT_DEFAULT_SLEEP_DURATION  8000

/// Delay used for BLE to compensate the clock
#define LLD_CLOCK_CORR_LAT              1  // Maximum time for clock correction is 1 slot

/// Maximum slave latency supported when total SCA is 1000ppm
#define LLD_EVT_MAX_LATENCY             450



/// Type of events
enum
{
    LLD_MASTER_CONNECTED = 2,
    LLD_SLAVE_CONNECTED,
    LLD_ADVERTISER,
    LLD_PASSIVE_SCANNING = 8,
    LLD_ACTIVE_SCANNING,
    LLD_INITIATING = 15,
    LLD_TXTEST_MODE= 28,
    LLD_RXTEST_MODE ,
    LLD_TXRXTEST_MODE ,
};

/// Restart policies
enum
{
    LLD_NO_RESTART,
    LLD_STOP_REQUESTED,
    LLD_ADV_RESTART,
    LLD_SCN_RESTART,
    LLD_MST_RESTART,
    LLD_SLV_RESTART
};

/// Instant actions
enum
{
    LLD_NO_ACTION,
    LLD_PARAM_UPDATE,
    LLD_CHMAP_UPDATE
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
/// Structure describing an event
struct lld_evt_tag
{
    /// List element for chaining in the scheduling lists
    struct co_list_hdr hdr;
    #if (BLE_CENTRAL || BLE_OBSERVER)
    /// Element for chaining in the interval list
    struct lld_evt_tag *next;
    /// Pointer to the interval list the event is chained in
    struct lld_evt_int_tag *int_list;
    #endif // BLE_CENTRAL || BLE_OBSERVER
    /// Pointer to the alternate event (used for connection update mechanism)
    struct lld_evt_tag *alt_evt;
    /// List of TX descriptors programmed for transmission (i.e. chained with the CS)
    struct co_list tx_prog;
    /// List of TX descriptors ready for transmission (i.e. not yet chained with the CS)
    struct co_list tx_rdy;
    /// Time of next occurrence of the event
    uint32_t time;
    /// Current drift
    uint32_t drift_current;
    /// Anchor point
    struct
    {
        /// Base time counter value of the Anchor point
        uint32_t basetime_cnt;
        /// Fine time counter value of the Anchor point
        uint16_t finetime_cnt;
    } anchor_point;
    /// Drift for the base connection interval
    uint16_t drift_base;
    /// Window size used for the connection/parameter update
    uint16_t win_size;
    /// Window size put in the control structure
    uint16_t win_size_cs;
    /// Update offset
    uint16_t update_offset;
    /// Slot on which the event occurs
    uint16_t slot;
    /// Duration of the event (in slot count)
    uint16_t duration;
    /// Period of the event
    uint16_t interval;
    /// Slave latency for this event
    uint16_t latency;
    /// Connection Handle associated with the event
    uint16_t conhdl;
    /// Event counter
    uint16_t counter;
    /// Instant of next action (channel map update or connection parameter update)
    uint16_t instant;
    /// Number of connection events missed since last anchor point
    uint16_t missed_cnt;
    /// Update window size
    uint8_t update_size;
    /// Restart policy
    uint8_t restart_pol;
    /// Flag indicating if the event is currently programmed in the exchange table or not
    bool prog;
    /// Number of RX buffers already handled in the event
    uint8_t rx_cnt;
    /// Flag indicating if an action has to be performed on next instant
    uint8_t inst_action;
    /// Master sleep clock accuracy
    uint8_t mst_sca;
    /// Flag indicating if an empty packet is chained on top of TX list
    bool empty_chained;
    /// Flag indicating if an we are waiting for an event from the master
    uint8_t waiting_evt;
    /// Flag indicating if a sync error was detected on the first packet
    bool rx_sync_err;
    /// TX power used for this event
    uint8_t tx_pwr;
    /// Used to know if event should be free.
    bool free;
};

/// Structure describing the parameters for a connection update
struct lld_evt_update_tag
{
    /// Offset to be put in the connection update request
    uint16_t win_offset;
    /// Instant for the connection update
    uint16_t instant;
    /// Size of the window to be put in the connection update request
    uint8_t win_size;
};

/// Structure describing an event
struct lld_evt_int_tag
{
    /// List element for chaining
    struct co_list_hdr hdr;
    /// List of events attached to this interval
    struct lld_evt_tag *evt;
    /// Number of free slots
    uint32_t freeslot;
    /// Base interval time
    uint16_t int_base;
};

/// Environment structure for the LLD module
struct lld_evt_env_tag
{
    /// List of events currently programmed
    struct co_list evt_prog;
    /// List of used intervals
    struct co_list int_used;
    /// Pointer to the scanning event. If scan is not active, this pointer is NULL.
    struct lld_evt_tag *scan_evt;
    /// Accuracy of the low power clock connected to the BLE core
    uint8_t sca;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
/// Environment of the LLDEVT module
extern struct lld_evt_env_tag lld_evt_env;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Get sleep clock accuracy
 *
 * @return The sleep clock accuracy as defined in the standard
 *
 ****************************************************************************************
 */
__INLINE uint8_t lld_evt_sca_get(void)
{
    // Read sleep clock accuracy from the environment
    return (lld_evt_env.sca);
}

/**
 ****************************************************************************************
 * @brief Get current time value from HW
 *
 * @return The current time in units of 625us
 *
 ****************************************************************************************
 */
__INLINE uint32_t lld_evt_time_get(void)
{
    // Sample the base time count
    ble_sampleclk_set(BLE_SAMP_BIT);
    while(ble_sampleclk_get());
    // Read current time in HW
    return (ble_basetimecnt_get());
}

/**
 ****************************************************************************************
 * @brief Compare absolute times
 *
 * The absolute time difference between time1 and time2 is supposed to be less than the
 * maximum interval time
 *
 * @param[in] time1 First time to compare
 * @param[in] time2 Second time to compare
 *
 * @return true if time1 is smaller than time2.
 ****************************************************************************************
 */
__INLINE bool lld_evt_time_cmp(uint32_t time1,
                               uint32_t time2)
{
    return (((time1 - time2) & BLE_BASETIMECNT_MASK) > MAX_INTERVAL_TIME);
}

/**
 ****************************************************************************************
 * @brief Check if time passed as parameter is in the past
 *
 * @param[in] time Time to be compare with current time to see if it in the past
 *
 * @return true if time is in the past, false otherwise
 *
 ****************************************************************************************
 */
__INLINE bool lld_evt_time_past(uint32_t time)
{
    // Compare time and current time
    return(lld_evt_time_cmp(time & BLE_BASETIMECNT_MASK, lld_evt_time_get()));
}

/**
 ****************************************************************************************
 * @brief get the connection event counter for a dedicated event
 *
 * @param[in] evt   Pointer to the event for which the counter is requested
 *
 * @return The connection event counter
 *
 ****************************************************************************************
 */
__INLINE uint16_t lld_evt_con_count_get(struct lld_evt_tag *evt)
{
#ifndef __DA14581__
    return evt->counter;
#else
	return (evt->counter - evt->missed_cnt);
#endif
}

/**
 ****************************************************************************************
 * @brief Computes the compensation that has to be applied to the supervision timeout,
 * according to the drift between master and slave
 *
 * @param[in] evt The event for which the compensation has to be computed
 *
 * @return The computed compensation
 *
 ****************************************************************************************
 */
__INLINE uint16_t lld_evt_sup_to_compensate(struct lld_evt_tag *evt)
{
    uint32_t drift = evt->drift_base;
    uint32_t latency = evt->latency;
    uint32_t winsize = drift * latency;

    // Return the compensation in 10ms unit
    return ((winsize / 10000) + 1);
}


/**
 ****************************************************************************************
 * @brief Update the TX power field of the event passed as parameter with the requested
 * value of TX power
 *
 * @param[in] evt    The event for which the TX power is updated
 * @param[in] tx_pwr The TX power to be applied to this event
 *
 * @return The computed compensation
 *
 ****************************************************************************************
 */
__INLINE void lld_evt_txpwr_update(struct lld_evt_tag *evt, uint8_t tx_pwr)
{
    // Update Power in the event structure
    evt->tx_pwr = tx_pwr;
}


/**
 ****************************************************************************************
 * @brief Go through the current event list to find the one corresponding to the
 * connection handle passed as parameter
 *
 * @param[in] conhdl      Connection handle for which the corresponding event is searched
 *
 * @return The pointer to the found event (NULL if no event is attached to this handle)
 *
 ****************************************************************************************
 */
struct lld_evt_tag *lld_evt_conhdl2evt(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Create a connection or scanning event and chain it in the scheduling and
 * interval lists
 *
 * @param[in] handle      Connection handle for which the event is created (LLD_ADV_HDL if
 *                        non-connected event)
 * @param[in] duration    Expected duration of the event, in units of 625us. This duration
 *                        can be reduced if scheduling of other events is requiring that.
 * @param[in] mininterval Minimum interval for the connection event
 * @param[in] maxinterval Maximum interval for the connection event
 * @param[in] latency     Requested latency for the event
 *
 * @return The pointer to the event just created
 *
 ****************************************************************************************
 */
struct lld_evt_tag *lld_evt_scan_create(uint16_t handle,
                                        uint16_t duration,
                                        uint16_t mininterval,
                                        uint16_t maxinterval,
                                        uint16_t latency);

/**
 ****************************************************************************************
 * @brief Create an advertising event and chain it in the scheduling list
 *
 * @param[in] handle      Connection handle for which the event is created (LLD_ADV_HDL if
 *                        non-connected event)
 * @param[in] mininterval Minimum interval for the advertising event
 * @param[in] maxinterval Maximum interval for the advertising event
 * @param[in] restart_pol Requested restart policy, i.e. LLD_ADV_RESTART or LLD_NO_RESTART
 *
 * @return The pointer to the event just created
 *
 ****************************************************************************************
 */
struct lld_evt_tag *lld_evt_adv_create(uint16_t handle,
                                       uint16_t mininterval,
                                       uint16_t maxinterval,
                                       uint8_t  restart_pol);
#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @brief Create a connection event for parameter update
 *
 * @param[in]  old_evt     Pointer to the current connection event
 * @param[in]  mininterval Minimum interval for the connection event
 * @param[in]  maxinterval Maximum interval for the connection event
 * @param[in]  latency     Requested slave latency
 * @param[out] upd_par     Computed updated parameters, to be put in the LLCP frame
 *
 * @return The pointer to the new event created (used after instant)
 *
 ****************************************************************************************
 */
struct lld_evt_tag *lld_evt_update_create(struct lld_evt_tag *old_evt,
#ifdef __DA14581__
                                          uint16_t ce_len,
#endif
                                          uint16_t mininterval,
                                          uint16_t maxinterval,
                                          uint16_t latency,
                                          struct lld_evt_update_tag *upd_par);

/**
 ****************************************************************************************
 * @brief Create a slave connection event
 *
 * @param[in]  con_par     Pointer to the decoded connection parameters
 * @param[in]  con_req_pdu Pointer to the connection request frame as received
 * @param[in]  evt_adv     Pointer to the advertising event that triggered the connection
 * @param[in]  conhdl      Connection handle to the established connection
 *
 * @return The pointer to the slave event created
 *
 ****************************************************************************************
 */
struct lld_evt_tag *lld_evt_move_to_slave(struct llc_create_con_req_ind const *con_par,
                                          struct llm_pdu_con_req_rx *con_req_pdu,
                                          struct lld_evt_tag *evt_adv,
                                          uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Indicates to the LLD the occurrence of a connection parameter update.
 *
 * @param[in]  param_pdu   Pointer to the connection parameter update PDU
 * @param[in]  old_evt     Pointer to the current event used for this connection
 *
 ****************************************************************************************
 */
void lld_evt_slave_update(struct llcp_con_up_req const *param_pdu,
                          struct lld_evt_tag *old_evt);

/**
 ****************************************************************************************
 * @brief Indicates to the LLD to move from initiating to master connected state.
 *
 * @param[in]  evt      Pointer to the event used for initiation
 * @param[in]  conhdl   Handle of the new master connection
 *
 ****************************************************************************************
 */
void lld_evt_move_to_master(struct lld_evt_tag *evt, uint16_t conhdl);
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)

/**
 ****************************************************************************************
 * @brief Delete an event by removing it from all the lists it is in
 *
 * @param[in] evt        Event to be deleted
 * @param[in] abort_bit  Bit of the BLE core controlling the abort for the event
 *
 ****************************************************************************************
 */
void lld_evt_delete(struct lld_evt_tag *evt, uint32_t abort_bit);

/**
 ****************************************************************************************
 * @brief Program the next occurrence of the slave event passed as parameter
 * In case the slave event passed as parameter is far enough in the future (e.g. due to
 * slave latency), the event is canceled and replaced with the earliest possible one. This
 * function is called when a data has been pushed for transmission in order to send it as
 * soon as possible even if slave latency is used.
 *
 * @param[in] evt        Event to be deleted
 ****************************************************************************************
 */
void lld_evt_schedule_next(struct lld_evt_tag *evt);
/**
 ****************************************************************************************
 * @brief Initialization of the BLE event scheduler
 *
 * This function initializes the lists used for event scheduling.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 ****************************************************************************************
 */
void lld_evt_init(bool reset);

/**
 ****************************************************************************************
 * @brief Kernel event scheduled when BLE events has to be programmed in the HW
 *
 * This function is a deferred action of the BLE wake up interrupt. It programs the data
 * Tx/Rx exchange in the control structure corresponding to the event, and programs the
 * target time of the next event to be scheduled.
 *
 ****************************************************************************************
 */
void lld_evt_schedule(void);

/**
 ****************************************************************************************
 * @brief Kernel event scheduled when a BLE event has to be handled by the HW
 *
 * This function is a deferred action of the BLE end of event interrupt. It flushes the
 * data Tx/Rx exchange corresponding to the event, and programs the
 * target time of the next event to be scheduled.
 *
 ****************************************************************************************
 */
void lld_evt_end(void);

/**
 ****************************************************************************************
 * @brief Kernel event scheduled when a BLE RX has to be handled by the HW
 *
 * This function is a deferred action of the BLE RX interrupt. It flushes the
 * data Tx/Rx exchange corresponding to the event.
 *
 ****************************************************************************************
 */
void lld_evt_rx(void);

/**
 ****************************************************************************************
 * @brief End of sleep interrupt handler
 *
 * This function is called under interrupt when an end of sleep interrupt is generated by
 * the BLE core. It sets the associated kernel event in order to perform the handling as a
 * deferred action in background context.
 *
 ****************************************************************************************
 */
void lld_evt_start_isr(void);

/**
 ****************************************************************************************
 * @brief End of event interrupt handler
 *
 * This function is called under interrupt when an end of event interrupt is generated by
 * the BLE core. It sets the associated kernel event in order to perform the handling as a
 * deferred action in background context.
 *
 ****************************************************************************************
 */
void lld_evt_end_isr(void);

/**
 ****************************************************************************************
 * @brief RX interrupt handler
 *
 * This function is called under interrupt when an RX interrupt is generated by
 * the BLE core. It sets the associated kernel event in order to perform the handling as a
 * deferred action in background context.
 *
 ****************************************************************************************
 */
void lld_evt_rx_isr(void);

/**
 ****************************************************************************************
 * @brief General purpose timer interrupt handler
 *
 * This function is called under interrupt when a general purpose timer interrupt is
 * generated by the BLE core. It sets the associated kernel event in order to perform the
 * handling as a deferred action in background context.
 *
 ****************************************************************************************
 */
void lld_evt_timer_isr(void);

/**
 ****************************************************************************************
 * @brief Indication of an Acknowledgment received
 *
 * @param[in] evt  Pointer to the event for which the acknowledgment was received
 *
 ****************************************************************************************
 */
void lld_evt_ack_received(struct lld_evt_tag *evt);

/**
 ****************************************************************************************
 * @brief Indication of a channel map update reception
 *
 * @param[in] evt     Pointer to the event for which the request was received
 * @param[in] instant Instant at which the update will occur
 *
 ****************************************************************************************
 */
void lld_evt_ch_map_update_ind(struct lld_evt_tag *evt, uint16_t instant);

/**
 ****************************************************************************************
 * @brief Request to program a channel map update
 *
 * @param[in] evt     Pointer to the event for which the update has to be programmed
 *
 * @return The instant computed for the update
 *
 ****************************************************************************************
 */
uint16_t lld_evt_ch_map_update_req(struct lld_evt_tag *evt);
/**
 ****************************************************************************************
 * @brief Check if event is no more needed and can be freed.
 *
 * @param[in] evt          Pointer to the event to try to free
 ****************************************************************************************
 */
void lld_evt_try_free(struct lld_evt_tag ** evt);
#ifdef __DA14581__
/**
 ****************************************************************************************
 * @brief Check if event has an alternative event, if yes, delete it.
 *
 * @param[in] evt          Pointer to the event which can have alternative event
 ****************************************************************************************
 */
void lld_delete_alt_evt(struct lld_evt_tag * evt);
#endif
/// @} LLDEVT

#endif // LLD_EVT_H_
