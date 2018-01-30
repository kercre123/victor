/**
 ****************************************************************************************
 *
 * @file lld_evt.c
 *
 * @brief Definition of the functions used for event scheduling
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LLDEVT
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "arch.h"
#include "co_bt.h"
#include "co_endian.h"
#include "ke_event.h"
#include "ke_timer.h"
#include "ke_mem.h"
#include "lld.h"
#include "lld_data.h"
#include "lld_evt.h"
#include "reg_ble_em_et.h"
#include "reg_ble_em_cs.h"
#include "reg_ble_em_rx.h"
#include "reg_access.h"
#include "llm.h"
#include "rwip.h"
#include "llc.h"
#include "led.h"

#if (NVDS_SUPPORT)
#include "nvds.h"         // NVDS definitions
#endif // NVDS_SUPPORT

#include "arch.h"



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// Environment of the LLD module
extern struct lld_evt_env_tag lld_evt_env;

void lld_evt_local_sca_init(void);




/*
 ****************************************************************************************
 *
 * PRIVATE FUNCTION DEFINITIONS
 *
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Reads the low power clock drift from NVDS, compute the resulting SCA and store
 * it in the environment.
 *
 ****************************************************************************************
 */
 
// static void lld_evt_local_sca_init(void)
// {
//     uint8_t sca;
//     uint16_t drift;
//     #if (NVDS_SUPPORT)
//     uint8_t length = NVDS_LEN_LPCLK_DRIFT;

//     // Get the sleep clock accuracy from the storage
//     if (nvds_get(NVDS_TAG_LPCLK_DRIFT, &length, (uint8_t *)&drift) != NVDS_OK)
//     #endif // NVDS_SUPPORT
//     {
//         // If no low power clock drift is found in NVDS, put the default value
//         #if BLE_STD_MODE
//         drift = DRIFT_BLE_DFT;
//         #else //BLE_STD_MODE
//         drift = DRIFT_BT_DFT;
//         #endif //BLE_STD_MODE
//     }

//     // Deduce the SCA from the drift
//     if (drift < 21)
//         sca = SCA_20PPM;
//     else if (drift < 31)
//         sca = SCA_30PPM;
//     else if (drift < 51)
//         sca = SCA_50PPM;
//     else if (drift < 76)
//         sca = SCA_75PPM;
//     else if (drift < 101)
//         sca = SCA_100PPM;
//     else if (drift < 151)
//         sca = SCA_150PPM;
//     else if (drift < 251)
//         sca = SCA_250PPM;
//     else
//         sca = SCA_500PPM;

//     // Put the SCA in the environment
//     lld_evt_env.sca = sca;
// }

// #endif
// /**
//  ****************************************************************************************
//  * @brief Initializes the fields of an event
//  *
//  * @param[in] evt          Pointer to the event to initialize
//  ****************************************************************************************
//  */
// static void lld_evt_init_fields(struct lld_evt_tag *const evt)
// {
//     // Initialize all fields to NULL values it also sets flag free to false.
//     memset(evt, 0, sizeof(struct lld_evt_tag));

//     // Initialize Max power
//     evt->tx_pwr = rwip_rf.txpwr_max;

//     // Initialize data lists
//     ASSERT_ERR(co_list_is_empty(&evt->tx_prog));
//     co_list_init(&evt->tx_prog);
//     ASSERT_ERR(co_list_is_empty(&evt->tx_rdy));
//     co_list_init(&evt->tx_rdy);
// }

// /**
//  ****************************************************************************************
//  * @brief Computes parameters of the parameter update (instant, offset, winsize)
//  *
//  * @param[in]  old_evt      Pointer to the old event (before connection update)
//  * @param[in]  new_evt      Pointer to the new event (after connection update)
//  * @param[out] upd_par      Pointer to the structure containing the parameters for the
//  *                          connection update
//  *
//  ****************************************************************************************
//  */
// #if 0
// static void lld_evt_update_param_compute(struct lld_evt_tag *old_evt,
//                                          struct lld_evt_tag *new_evt,
//                                          struct lld_evt_update_tag *upd_par)

// {
//     // The instant will be 7 wake-up times after the next event
//     uint16_t count_to_inst = old_evt->latency * 7;
//     uint32_t delay_old = old_evt->interval * count_to_inst;

//     // Compute the old event time at instant
//     uint32_t time_old = (old_evt->time + delay_old) & BLE_BASETIMECNT_MASK;

//     // Compute new event time immediately following old event time at instant
//     while (lld_evt_time_cmp(new_evt->time, time_old))
//     {
//         new_evt->time = (new_evt->time + new_evt->interval) & BLE_BASETIMECNT_MASK;
//     }

//     // Program the instant in the old event
//     old_evt->instant = old_evt->counter + count_to_inst - 1;

//     // Compute the time difference between new time and old time to get the offset
//     upd_par->win_offset = (uint16_t)(((new_evt->time - time_old) & BLE_BASETIMECNT_MASK) / 2);
//     upd_par->win_size = 1;
//     upd_par->instant = old_evt->instant + 1;
// }
// #endif
// /**
//  ****************************************************************************************
//  * @brief Computes the maximum drift according to the master clock accuracy and the delay
//  * passed as parameters
//  *
//  * @param[in] delay       Duration for which the drift is computed
//  * @param[in] master_sca  Sleep clock accuracy of the master
//  *
//  * @return The value of the RX window size formatted for the RXWINCNTL field of the
//  * control structure
//  *
//  ****************************************************************************************
//  */
// static uint16_t lld_evt_drift_compute(uint16_t delay,
//                                       uint8_t master_sca)
// {
//     uint32_t drift;
//     uint32_t delay32 = delay;
//     uint32_t accuracy;

//     // Compute the total accuracy in ppm
//     accuracy = co_sca2ppm[lld_evt_sca_get()] + co_sca2ppm[master_sca];

//     // Compute the drift from the interval and the accuracy
//     drift = ((delay32 * accuracy * 41) >> 16) + 1;

//     return (drift);
// }


// /**
//  ****************************************************************************************
//  * @brief Check if slave has to listen for the next connection event or not
//  *
//  * @param[in] evt The event for which the window size has changed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_slave_wakeup(struct lld_evt_tag *evt)
// {
//     uint16_t latency = evt->latency - 1;

//     // Correct the latency in case the total accuracy is 1000ppm
//     if ((evt->mst_sca == SCA_500PPM) && (lld_evt_env.sca == SCA_500PPM) &&
//         (latency > LLD_EVT_MAX_LATENCY))
//     {
//         // Latency is too high and may cause connection loss due to Window Widening, so
//         // limit it to the max value
//         latency = LLD_EVT_MAX_LATENCY;
//     }

//     // Check if we missed too many events and need to listen for the next one
//     if (evt->missed_cnt >= latency)
//         evt->waiting_evt |= LLD_EVT_WAITING_SYNC;

//     // Check if we have reached the instant
//     if ((evt->counter == evt->instant) && (evt->inst_action != LLD_NO_ACTION))
//         evt->waiting_evt |= LLD_EVT_WAITING_INSTANT;
// }


// /**
//  ****************************************************************************************
//  * @brief Computes the next unmapped channel index according to the hopping mechanism.
//  * The previous channel index is taken from the control structure, and once the next
//  * unmapped channel is computed, it is put back in the control structure.
//  *
//  * @param[in] evt The event for which the next channel has to be computed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_channel_next(struct lld_evt_tag *evt)
// {
//     uint16_t hopcntl = ble_hopcntl_get(evt->conhdl);
//     uint16_t last_ch = hopcntl & BLE_CH_IDX_MASK;
//     uint16_t hop = (hopcntl & BLE_HOP_INT_MASK) >> BLE_HOP_INT_LSB;
//     uint16_t next_ch;

//     // Compute the next channel
//     next_ch = (last_ch + hop) % 37;

//     // Set it in the control structure
//     ble_hopcntl_set(evt->conhdl, BLE_FH_EN_BIT | (hop << BLE_HOP_INT_LSB) | next_ch);
// }

// /**
//  ****************************************************************************************
//  * @brief Computes the window size formatted for the RXWINCNTL field of the control
//  * structure
//  * Once the value is computed, the function puts the new value into the control structure
//  *
//  * @param[in] evt The event for which the window size has changed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_winsize_change(struct lld_evt_tag *evt)
// {
//     uint32_t drift = evt->drift_current;
//     uint32_t window = evt->win_size;
//     uint32_t winsize;
//     uint16_t winsize_cs;

//     winsize = drift + window + 16;

//     // Convert to a CS compatible value
//     if (winsize > (BLE_RXWINSZ_MASK / 2))
//     {
//         // Compute the value in slot count
//         winsize_cs = winsize / 625 + 1;

//         // Put the half window size in the event
//         evt->win_size_cs = BLE_RXWIDE_BIT | winsize_cs;

//         // Double it to put in the control structure
//         winsize_cs = BLE_RXWIDE_BIT | (winsize_cs * 2);
//     }
//     else
//     {
//         // Put the half window size in the event
//         evt->win_size_cs = winsize;

//         // Double it to put in the control structure
//         winsize_cs = (uint16_t)(winsize * 2);
//     }

//     // Write the value into the control structure
//     ble_rxwincntl_set(evt->conhdl, winsize_cs);
// }


// /**
//  ****************************************************************************************
//  * @brief Compute the RX window size according to the number of missed connection events
//  *
//  * The function also checks if the window size becomes more than half of the connection
//  * interval. In that case, the connection is terminated.
//  *
//  * @param evt       The event for which the RX window is increased
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_rxwin_compute(struct lld_evt_tag *evt)
// {
//     uint16_t delay;

//     // Compute the delay since last anchor point
//     delay = (evt->missed_cnt + 1) * evt->interval;

//     // Recompute the current drift
//     evt->drift_current = lld_evt_drift_compute(delay, evt->mst_sca);

//     // Change the value in the control structure
//     lld_evt_winsize_change(evt);

//     // Check if window value is still less than interval/2
//     if ((evt->drift_current * 2) >= ((uint32_t)(evt->interval * 625 - 150)))
//     {
//         // Window size is bigger than interval - T_IFS, so ask for link disconnection
//         evt->restart_pol = LLD_STOP_REQUESTED;
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Increase the RX window size after a sync error
//  *
//  * The function also checks if the window size becomes more than half of the connection
//  * interval. In that case, the connection is terminated.
//  *
//  * @param evt       The event for which the RX window is increased
//  * @param evt_end   Flag indicating if the increase is done after the end of event or
//  *                  before programming it (e.g. because of slave latency)
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_rxwin_increase(struct lld_evt_tag *evt)
// {
//     uint16_t delay;

//     // Increase the number of missed connection events
//     evt->missed_cnt++;

//     // Compute the delay since last anchor point
//     delay = (evt->missed_cnt + 1) * evt->interval;

//     // Recompute the current drift
//     evt->drift_current = lld_evt_drift_compute(delay, evt->mst_sca);

//     // Change the value in the control structure
//     lld_evt_winsize_change(evt);

//     // Check if window value is still less than interval/2
//     if ((evt->drift_current * 2) >= ((uint32_t)(evt->interval * 625 - 150)))
//     {
//         // Window size is bigger than interval - T_IFS, so ask for link disconnection
//         evt->restart_pol = LLD_STOP_REQUESTED;
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Reset the RX window size after a correct reception
//  *
//  * @param evt The event for which the RX window is reset
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_rxwin_reset(struct lld_evt_tag *evt)
// {
//     // Sync has been received
//     evt->waiting_evt &= ~LLD_EVT_WAITING_SYNC;

//     // Reset the number of missed connection events
//     evt->missed_cnt = 0;

//     // Check if window size is different from current value
//     if ((evt->drift_base != evt->drift_current) ||
//         (evt->win_size != 0))
//     {
//         // Reset the connection window
//         evt->win_size = 0;

//         // Put back the default value of the window
//         evt->drift_current = evt->drift_base;

//         // Change the value in the control structure
//         lld_evt_winsize_change(evt);
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Computes the time of programming of next slave connection event and the fine
//  * counter offset to be pushed in the control structure. The function then updates the
//  * event fields and control structure accordingly. The computing of the time is performed
//  * from the value of the next theoretical sync point and the RX window size
//  *
//  * @param[in] evt           Pointer to the event for which time is computed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_slave_time_compute(struct lld_evt_tag *evt, uint32_t next_basetimecnt)
// {
//     uint16_t slotcnt;
//     uint16_t finetimecnt = evt->anchor_point.finetime_cnt;
//     uint16_t finecnt;
//     uint16_t winsize = evt->win_size_cs;

//     // Split the window size in slot count and fine count
//     if (winsize & BLE_RXWIDE_BIT)
//     {
//         // If RX wide, then the value in slot is available immediately
//         slotcnt = winsize & BLE_RXWINSZ_MASK;

//         // And fine count is null
//         finecnt = 0;
//     }
//     else
//     {
//         // If RX is in us, then the value in slot has to be computed
//         slotcnt = winsize / 625;

//         // As well as fine count
//         finecnt = winsize % 625;
//     }

//     // Compute the fine counter corrected by the window size
//     if (finetimecnt < finecnt)
//     {
//         // Fine time count will wrap, so increase the number of slot by 1
//         slotcnt++;

//         // And increment the fine count by 1 slot in us
//         finetimecnt += 625;
//     }
//     finetimecnt -= finecnt;
//     if (finetimecnt < LLD_EVT_RX_WIN_DEFAULT_OFFSET)
//     {
//         finetimecnt += 625;
//         slotcnt++;
//     }
//     finetimecnt -= LLD_EVT_RX_WIN_DEFAULT_OFFSET;

//     // Compute the event time, corrected by the window size
//     evt->time = (next_basetimecnt - slotcnt) & BLE_BASETIMECNT_MASK;

//     // Set the fine time correction counter in the control structure
//     ble_fcntoffset_set(evt->conhdl, finetimecnt);

// }


// /**
//  ****************************************************************************************
//  * @brief Programs the fine counter at the time of event passed as parameter (minus
//  * programming latency)
//  *
//  * @param[in] evt           Pointer to the event for which time is programmed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_target_time_prog(struct lld_evt_tag *evt)
// {
//     // Program the event target time in HW
//     ble_finetarget_setf((evt->time - LLD_EVT_PROG_LATENCY) & BLE_BASETIMECNT_MASK);

//     // enable fine timer irq
//     if (!ble_finetgtimintmsk_getf())
//     {
//         // if timer is not enabled, it is possible that the irq is raised
//         // due to a spurious value, so ack it before
//         ble_intack_clear(BLE_FINETGTIMINTACK_BIT);
//         ble_finetgtimintmsk_setf(1);
//     }

//     // Sanity check
//     ASSERT_ERR(!lld_evt_time_past(evt->time - LLD_EVT_PROG_LATENCY));
// }

// /**
//  ****************************************************************************************
//  * @brief Insert an event at the right place in the event list
//  *
//  * @param[in]  evt  Pointer to the event to insert.
//  *
//  ****************************************************************************************
//  */
// void lld_evt_insert(struct lld_evt_tag *evt)
// {
//     struct co_list *list = &lld_evt_env.evt_prog;
//     struct lld_evt_tag *prev = NULL;
//     struct lld_evt_tag *scan = (struct lld_evt_tag *)list->first;

//     for(;;)
//     {
//         // scan the list until the end or cmp() returns true
//         if (scan)
//         {
//             // If there is already an event at this time, delay one of the events
//             if (evt->time == scan->time)
//             {
//                 // Check event priority to know which one to delay
//                 if (evt->restart_pol > scan->restart_pol)
//                 {
//                     uint8_t delay = 0;
//                     struct lld_evt_tag *tmp;

//                     // If old event is an advertising one, add the delay
//                     if (scan->restart_pol == LLD_ADV_RESTART)
//                         // Advertising delay is random between 0 and 10ms, and must have
//                         // an even value
//                         delay = CO_ALIGN2_HI(co_rand_byte() & 0x0F);

//                     // New event has a higher priority so delay old one
//                     scan->time = (scan->time + scan->interval + delay) & BLE_BASETIMECNT_MASK;

//                     // Insert new event at the place of old one
//                     if (prev)
//                         prev->hdr.next = &evt->hdr;
//                     else
//                         list->first = &evt->hdr;
//                     evt->hdr.next = scan->hdr.next;

//                     // Now we will look for a place for the delayed event
//                     tmp = evt;
//                     evt = scan;
//                     scan = tmp;
//                 }
//                 else
//                 {
//                     // New event has a lower priority than old one, so delay it
//                     evt->time = (evt->time + evt->interval) & BLE_BASETIMECNT_MASK;

//                     // In case event is a connection event, then perform a manual hop of
//                     // the channel
//                     if (evt->restart_pol >= LLD_MST_RESTART)
//                     {
//                         lld_evt_channel_next(evt);
//                         evt->counter++;
//                     }

//                     evt->missed_cnt++;
//                 }
//             }
//             // Compare new event time with current event time
//             else if (lld_evt_time_cmp(evt->time, scan->time))
//                 break;

//             prev = scan;
//             scan = (struct lld_evt_tag *)scan->hdr.next;
//         }
//         else
//         {
//             // end of list
//             list->last = &evt->hdr;
//             break;
//         }
//     }

//     evt->hdr.next = &scan->hdr;

//     if (prev)
//     {
//         // second or more
//         prev->hdr.next = &evt->hdr;
//     }
//     else
//     {
//         // first event
//         list->first = &evt->hdr;
//     }

//     #if (KE_PROFILING)
//     // Update the list count
//     list->cnt++;
//     #endif // KE_PROFILING

//     // Schedule events
//     ke_event_set(KE_EVENT_BLE_EVT_START);
// }


// #if (BLE_CENTRAL || BLE_OBSERVER)
// /**
//  ****************************************************************************************
//  * @brief Extracts the event passed as parameter from the interval list where it is
//  * chained
//  *
//  * @param[in] evt           Pointer to the event to be extracted
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_int_extract(struct lld_evt_tag *evt)
// {
//     struct lld_evt_int_tag *node = evt->int_list;
//     struct lld_evt_tag *scan = node->evt;

//     // Check if the event to extract is the first of the list
//     if (evt == scan)
//     {
//         // Replace head of list with the event following the event to extract
//         node->evt = evt->next;
//     }
//     else
//     {
//         // Event is somewhere in the list, so go through the list until we find it
//         while (scan->next != evt)
//         {
//             // Go to next event
//             scan = scan->next;
//         }

//         // Extract the event
//         scan->next = evt->next;
//     }

//     // Now check if interval list has still some events or not
//     if (node->evt == NULL)
//     {
//         // The list is empty, so extract it from the list of interval nodes
//         co_list_extract(&lld_evt_env.int_used, &node->hdr);

//         // Free the interval structure
//         ke_free(node);
//     }
//     else
//     {
//         // Update the number of free slots
//         node->freeslot += evt->duration / 2;
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Check if the interval node passed as parameter fits the minimum and maximum
//  * interval requirements. If yes, it returns the interval that will be used for the
//  * event.
//  *
//  * @param[in] node          Pointer to the tested interval list
//  * @param[in] mininterval   Minimal allowed interval value
//  * @param[in] maxinterval   Maximal allowed interval value
//  *
//  * @return The interval to be used to fit in this node
//  *
//  ****************************************************************************************
//  */
// static uint16_t lld_evt_int_match(struct lld_evt_int_tag *node,
//                                   uint16_t mininterval,
//                                   uint16_t maxinterval)
// {
//     uint16_t interval = node->int_base;

//     // Check if a multiple of the base interval can match the requirements
//     for (int i=1; i<8; i++)
//     {
//         interval *= i;
//         // If computed interval is between min and max, then this node can match
//         if ((interval >= mininterval) && (interval <= maxinterval))
//         {
//             return (interval);
//         }
//     }

//     return LLD_EVT_INTERVAL_INVALID;
// }

// /**
//  ****************************************************************************************
//  * @brief Push the interval node passed as parameter in the list of interval nodes
//  *
//  * @param[in] node          Pointer to the interval node to be pushed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_int_node_push(struct lld_evt_int_tag *node)
// {
//     struct co_list *list = &lld_evt_env.int_used;
//     struct lld_evt_int_tag *curr = (struct lld_evt_int_tag *)co_list_pick(list);
//     struct lld_evt_int_tag *prev = NULL;

//     while(curr != NULL)
//     {
//         // Check if current interval is smaller than new node interval
//         if (node->int_base > curr->int_base)
//             break;

//         // Go to next node
//         prev = curr;
//         curr = (struct lld_evt_int_tag *)co_list_next(&curr->hdr);
//     }


//     // Insert the new node
//     #if (KE_PROFILING)
//     list->cnt++;
//     #endif //KE_PROFILING
//     if (prev == NULL)
//         list->first = &node->hdr;
//     else
//         prev->hdr.next = &node->hdr;
//     if (curr == NULL)
//         lld_evt_env.int_used.last = &node->hdr;

//     node->hdr.next = &curr->hdr;

// }

// /**
//  ****************************************************************************************
//  * @brief Get the closest future time of a slot in a specific interval node
//  *
//  * @param[in] node          Pointer to the interval node
//  * @param[in] slot          The slot index for which the time has to be computed
//  *
//  * @return The time for the slot passed as parameter
//  *
//  ****************************************************************************************
//  */
// static uint32_t lld_evt_slot_time_get(struct lld_evt_int_tag *node,
//                                       uint16_t slot)
// {
//     struct lld_evt_tag *base = node->evt;

//     // Get the delta of slots between the base time and the new event
//     int16_t delta = slot - base->slot;

//     // Apply the delta to base time to find the time for the new event
//     uint32_t time = base->time + (delta * 2);

//     // Check if computed time is in the past or too close in the future
//     if (lld_evt_time_past(time - LLD_EVT_PROG_LATENCY))
//     {
//         // Computed time is in the past, so add one base interval time
//         time += node->int_base;
//     }

//     // Return the time
//     return(time);
// }

// /**
//  ****************************************************************************************
//  * @brief Find free slots in the node passed as parameters that fit the requested duration
//  *
//  * @param[in] node      Pointer to the interval node
//  * @param[in] evt       Pointer to the event for which slots are requested
//  * @param[in] duration  Requested duration
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_int_slot_find(struct lld_evt_int_tag *node,
//                                   struct lld_evt_tag *evt,
//                                   uint16_t duration)
// {
//     struct lld_evt_tag *curr = node->evt;
//     struct lld_evt_tag *prev = NULL;
//     struct lld_evt_tag *bestplace = NULL;
//     uint16_t slotcnt = 0xFFFF;
//     uint16_t prevslot = 0;
//     uint16_t deltasup = 0xFFFF;
//     uint16_t deltainf = 0xFFFF;
//     bool found = false;

//     // Check that requested duration is not too large for event interval
//     if (duration > (evt->interval - LLD_EVT_PROG_LATENCY))
//     {
//         // If too large, then reduce it
//         duration = evt->interval - LLD_EVT_PROG_LATENCY;
//     }

//     // duration is given in number of 625us, but slot is 1.25ms, so divide duration per 2
//     // to get the number of slots required for duration
//     duration /= 2;

//     // Go through the list to find enough slots to match the requested duration
//     while(1)
//     {
//         // Compute the number of free slots
//         if (curr == NULL)
//             slotcnt = (node->int_base / 2) - prevslot;
//         else
//             slotcnt = curr->slot - prevslot;

//         // Check if it is enough for the duration that is requested
//         if ((slotcnt >= duration))
//         {
//             if ((slotcnt - duration) < deltasup)
//             {
//                 // This place could be a good one, memorize it
//                 found = true;
//                 bestplace = prev;
//                 deltasup = slotcnt - duration;
//             }
//         }
//         else if ((slotcnt > 0) && !found)
//         {
//             if ((duration - slotcnt) < deltainf)
//             {
//                 // This place could be a good one, memorize it
//                 bestplace = prev;
//                 deltainf = duration - slotcnt;
//             }
//         }

//         // If the number of free slots matches exactly the requirement, exit the search
//         if (slotcnt == duration)
//             break;

//         // If current node is NULL, exit the search
//         if (curr == NULL)
//             break;

//         // Store current event and go to the next one
//         prev = curr;
//         prevslot = prev->slot + (prev->duration/2);
//         curr = curr->next;
//     }

//     // Save the duration of the event
//     evt->duration = ((slotcnt >= duration)?duration:slotcnt)*2;

//     // Update the number of free slots in the interval node
//     node->freeslot -= evt->duration / 2;

//     // Insert the event at the place found
//     if (bestplace != NULL)
//     {
//         // Get the time of the event according to the slot
//         evt->slot = bestplace->slot + (bestplace->duration/2);
//         evt->time = lld_evt_slot_time_get(node, evt->slot);
//         evt->next = bestplace->next;
//         bestplace->next = evt;
//     }
//     else
//     {
//         // New event will be placed on first position
//         evt->slot = 0;
//         // Check if there are other events already in this node
//         if (node->evt == NULL)
//             // No event yet, schedule the new one as soon as possible
//             evt->time = CO_ALIGN2_LO((lld_evt_time_get() + 4)) & BLE_BASETIMECNT_MASK;
//         else
//             // Already some event, so get the time of the new event according to the slot
//             evt->time = lld_evt_slot_time_get(node, evt->slot);
//         evt->next = node->evt;
//         node->evt = evt;
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Computes the scheduling parameters (master, initiator or scan)
//  *
//  * @param[in] evt         Pointer to the event
//  * @param[in] duration    Requested duration of the event
//  * @param[in] mininterval Minimal allowed interval
//  * @param[in] maxinterval Maximal allowed interval
//  ****************************************************************************************
//  */
// static void lld_evt_sched_param_compute(struct lld_evt_tag * evt,
//                                                   uint16_t duration,
//                                                   uint16_t mininterval,
//                                                   uint16_t maxinterval)
// {
//     uint16_t interval = LLD_EVT_INTERVAL_INVALID;
//     // Get first interval node
//     struct lld_evt_int_tag *node = (struct lld_evt_int_tag *)co_list_pick(&lld_evt_env.int_used);

//     while (node)
//     {
//         // Check if there is at least one free slot in this node
//         if (node->freeslot)
//         {
//             // Check if node interval can match with the requirements
//             interval = lld_evt_int_match(node, mininterval, maxinterval);
//             if (interval != LLD_EVT_INTERVAL_INVALID)
//             {
//                 break;
//             }
//         }
//         // Go to the next node
//         node = (struct lld_evt_int_tag *)co_list_next(&node->hdr);
//     }
//     // If no matching node is found, allocate one
//     if (node == NULL)
//     {
//         interval = maxinterval;
//         // Allocate a new interval
//         node = (struct lld_evt_int_tag *)ke_malloc(sizeof(struct lld_evt_int_tag), KE_MEM_KE_MSG);

//         // Sanity check: There should always be an available node
//         ASSERT_ERR(node != NULL);

//         // Initialize the node
//         node->hdr.next = NULL;
//         node->evt = NULL;
//         node->freeslot = maxinterval/2;  // One slot every 1.25ms
//         node->int_base = maxinterval;

//         // Push the node orderly in the used list
//         lld_evt_int_node_push(node);
//     }

//     // Save interval and node
//     evt->interval = interval;
//     #if (BLE_CENTRAL)
//     evt->int_list = node;
//     #endif // BLE_CENTRAL

//     // Search for free slots in the node
//     lld_evt_int_slot_find(node, evt, duration);

// }
// #endif // BLE_CENTRAL || BLE_OBSERVER

// #if (BLE_PERIPHERAL)
// /**
//  ****************************************************************************************
//  * @brief In slave mode, performs the parameter update (called at instant)
//  *
//  * @param[in] evt_old  Pointer to the event used before instant
//  ****************************************************************************************
//  */
// static void lld_evt_slave_param_update_perform(struct lld_evt_tag *evt_old, uint32_t *next_basetimecnt)
// {
//     // Get new event pointer
//     struct lld_evt_tag *evt_new = evt_old->alt_evt;

//     struct llc_env_tag *llc_env_ptr = llc_env[evt_old->conhdl];
//     // Retrieve some information from the old event
//     if((llc_env_ptr->evt->interval != evt_new->interval)
//     	    || (llc_env_ptr->evt->latency != evt_new->latency)
//     	    || (llc_env_ptr->sup_to != llc_env_ptr->n_sup_to))
//     {
//     	llc_env_ptr->cnx_update_evt_sent = true;
//     }
//     evt_new->counter = evt_old->counter;
//     evt_new->drift_base = evt_old->drift_base;
//     evt_new->anchor_point = evt_old->anchor_point;
//     evt_new->empty_chained = evt_old->empty_chained;
//     evt_new->tx_prog = evt_old->tx_prog;
//     evt_new->tx_rdy = evt_old->tx_rdy;

//     // Set the value of the window size in the control structure
//     lld_evt_winsize_change(evt_new);

//     // Recompute the max event time based on the new connection interval
//     ble_maxevtime_set(evt_new->conhdl, evt_new->interval - LLD_EVT_PROG_LATENCY + 1);

//     // Compute the new anchor point from the the window offset and size
//     *next_basetimecnt = (*next_basetimecnt + evt_old->update_size
//                                     + evt_old->update_offset * 2) & BLE_BASETIMECNT_MASK;

//     // Maintains in environment variable the time of the latest anchor point received
//     evt_new->anchor_point.basetime_cnt = (*next_basetimecnt - evt_new->interval) & BLE_BASETIMECNT_MASK;
//     
//     // Force the window size to be recomputed next time
//     evt_new->drift_current = 0;
//     
//     // Reset event interval (indicating that event is free)
//     evt_old->interval = 0;

//     #if (BLE_CENTRAL || BLE_OBSERVER)
//     // Extract the event from the interval list
//     if (evt_old->int_list != NULL)
//         lld_evt_int_extract(evt_old);
//     #endif // BLE_CENTRAL || BLE_OBSERVER

//     // Copy new event structure to old event structure
//     // Needs to be done as last action because old event parameters may be used by the LLC
//     memcpy(evt_old, evt_new, sizeof(struct lld_evt_tag));

//     // Free the new event structure
//     ke_free(evt_new);
// }
// #endif // BLE_PERIPHERAL

// #if (BLE_CENTRAL)
// /**
//  ****************************************************************************************
//  * @brief In master mode, performs the parameter update (called at instant)
//  *
//  * @param[in] evt_old  Pointer to the event used before instant
//  ****************************************************************************************
//  */
// static void lld_evt_master_param_update_perform(struct lld_evt_tag *evt_old)
// {
//     // Get new event pointer
//     struct lld_evt_tag *evt_new = evt_old->alt_evt;
//     struct llc_env_tag *llc_env_ptr = llc_env[evt_old->conhdl];

//     // Reset event interval (indicating that event is free)
//     evt_old->interval = 0;

//     #if (BLE_CENTRAL || BLE_OBSERVER)
//     // Extract the event from the interval list
//     if (evt_old->int_list != NULL)
//         lld_evt_int_extract(evt_old);
//     #endif // BLE_CENTRAL || BLE_OBSERVER

//     // Retrieve some information from the old event
//     if((llc_env_ptr->evt->interval != evt_new->interval)
//     	    || (llc_env_ptr->evt->latency != evt_new->latency)
//     	    || (llc_env_ptr->sup_to != llc_env_ptr->n_sup_to))
//     {
//     	llc_env_ptr->cnx_update_evt_sent = true;
//     }

//     // Retrieve some information from the old event
//     evt_new->counter = evt_old->counter + 1;
//     evt_new->empty_chained = evt_old->empty_chained;
//     evt_new->tx_prog = evt_old->tx_prog;
//     evt_new->tx_rdy = evt_old->tx_rdy;
//     lld_evt_channel_next(evt_new);

//     // Remove one interval period, it will be added back in the restart function
//     //evt_new->time = (evt_new->time - evt_new->interval) & BLE_BASETIMECNT_MASK;

//     // Copy new event structure to old event structure
//     // Needs to be done as last action because old event parameters may be used by the LLC
//     memcpy(evt_old, evt_new, sizeof(struct lld_evt_tag));

//     #if (BLE_CENTRAL || BLE_OBSERVER)
//     if (evt_old->int_list->evt == evt_new)
//     {
//         // Move the interval from temp event to permanent event
//         evt_old->int_list->evt = evt_old;
//     }
//     #endif //(BLE_CENTRAL || BLE_OBSERVER)

//     // Free the new event structure
//     ke_free(evt_new);
// }

// /**
//  ****************************************************************************************
//  * @brief In master mode, performs the actions at instant
//  *
//  * @param[in] evt  Pointer to the event used before instant
//  ****************************************************************************************
//  */
// static void lld_evt_instant(struct lld_evt_tag *evt)
// {

//     switch (evt->inst_action)
//     {
//         // A parameter update has to be performed
//         case LLD_PARAM_UPDATE:
//         {
//             // Switch to new parameters
//             lld_evt_master_param_update_perform(evt);

//             // Warn the LLC about the connection parameter update
//             llc_con_update_ind(evt->conhdl, evt);
//         }
//             break;

//         // A channel map update has to be performed
//         case LLD_CHMAP_UPDATE:
//             // Clear the action
//             evt->inst_action = LLD_NO_ACTION;
//             // Warn the LLC about the channel map update
//             llc_map_update_ind(evt->conhdl);
//             break;

//         default:
//             // Nothing is done
//             break;
//     }
// }
// #endif // BLE_CENTRAL

// // temporary

// void llm_delete_event(void)
// {
//     llm_le_env.evt = NULL;
// }



// /**
//  ****************************************************************************************
//  * @brief Reschedule an event according to its restarting policy.
//  *
//  * @param[in] evt     Pointer to the event to be restarted
//  *
//  ****************************************************************************************
//  */
// void lld_evt_restart(struct lld_evt_tag *evt)
// {
//     ke_task_id_t destid;
//     uint16_t delay = 0;
//     uint32_t next_basetimecnt = 0;
//     uint16_t evt_cnt_inc = 1;

//     // Reset the scanning flag if the event to be restarted is the scanning event
//     if (evt == lld_evt_env.scan_evt)
//         lld_evt_env.scan_evt = NULL;

//     switch (evt->restart_pol)
//     {
//         // Cases where no event restart has to be performed
//         case LLD_STOP_REQUESTED:
//             destid = (evt->conhdl==LLD_ADV_HDL)?TASK_LLM:KE_BUILD_ID(TASK_LLC,evt->conhdl);
//             // Confirm the stop to the host
//             ke_msg_send_basic(LLD_STOP_IND, destid, TASK_LLD);
//         case LLD_NO_RESTART:
//             // Reset event interval (indicating that event is free)
//             evt->interval = 0;
//             // Flush the event
//             {
//                 uint8_t nb_of_pkt_flushed = lld_data_tx_flush(evt);
//                 // if the number of packet flushed is not NULL send a number of packets
//                 if(nb_of_pkt_flushed > 0)
//                 {
//                     llc_common_nb_of_pkt_comp_evt_send(evt->conhdl, nb_of_pkt_flushed);
//                 }
//             }

//             #if (BLE_CENTRAL || BLE_OBSERVER)
//             // Extract the event from the interval list
//             if (evt->int_list != NULL)
//                 lld_evt_int_extract(evt);
//             #endif // BLE_CENTRAL || BLE_OBSERVER

//             // Free the event structure
//             evt->free = true;
//             if(evt->restart_pol == LLD_NO_RESTART)
//             {
//                 // Informs LLM that its event has been deleted my LLD
//                 llm_delete_event();
//             }
//             // The event is not reprogrammed, so schedule events ensure fine target time
//             // is correctly updated
//             ke_event_set(KE_EVENT_BLE_EVT_START);
//             break;

//         // Cases where event restart has to be performed
//         #if (BLE_PERIPHERAL || BLE_BROADCASTER)
//         case LLD_ADV_RESTART:
//             // Advertising delay is random between 0 and 10ms, and must have an even value
//             delay = CO_ALIGN2_HI(co_rand_byte() & 0x0F);
//             // Add the interval to the time to get the next target time
// 			evt->time = (evt->time + evt->interval + delay) & BLE_BASETIMECNT_MASK;
// 			// Insert back the event in the list
// 			lld_evt_insert(evt);
// 			break;
//         #endif // BLE_PERIPHERAL || BLE_BROADCASTER
//         #if (BLE_CENTRAL)
//         case LLD_MST_RESTART:
//         #endif // #if (BLE_CENTRAL)
//             // Check if a descriptor was received. If not, perform a manual hopping
//             #if (RW_DM_SUPPORT)
//             if ((evt->rx_cnt == 0) && !(ble_conflict_getf(evt->conhdl)))
//             #else
//             if (evt->rx_cnt == 0)
//             #endif // RW_DM_SUPPORT
//                 // Compute the next channel by software
//                 lld_evt_channel_next(evt);
//             #if (BLE_CENTRAL)
//             if ((evt->counter == evt->instant) && (evt->inst_action != LLD_NO_ACTION))
//                 lld_evt_instant(evt);
//             #endif // #if (BLE_CENTRAL)
//             // Increment the event counter
//             evt->counter++;

//         #if (BLE_CENTRAL || BLE_OBSERVER)
//         case LLD_SCN_RESTART:
//         #endif // BLE_CENTRAL || BLE_OBSERVER
//             // Add the interval to the time to get the next target time
//             evt->time = (evt->time + evt->interval + delay) & BLE_BASETIMECNT_MASK;
//             evt->missed_cnt = 0;
//             // Insert back the event in the list
//             lld_evt_insert(evt);
//             break;

//         #if (BLE_PERIPHERAL)
//         // Specific case of the slave connection event
//         case LLD_SLV_RESTART:
//             // Check if a descriptor was received. If not, perform a manual hopping
//             #if (RW_DM_SUPPORT)
//             if ((evt->rx_cnt == 0) && !(ble_conflict_getf(evt->conhdl)))
//             #else
//             if (evt->rx_cnt == 0)
//             #endif // RW_DM_SUPPORT
//                 // Compute the next channel by software
//                 lld_evt_channel_next(evt);

//             // Check if we got a sync error
//             if (evt->rx_sync_err)
//             {
//                 // Increase the number of missed connection events
//                 evt->missed_cnt++;

//             }
//             else
//             {
//                 // Reset the win size
//                 evt->win_size = 0;

//                 // Check if we have data to transmit or if a procedure is ongoing
//                 if (!co_list_is_empty(&evt->tx_rdy) || !co_list_is_empty(&evt->tx_prog)
//                         || (evt->waiting_evt & (LLD_EVT_WAITING_ACK | LLD_EVT_WAITING_INSTANT)))
//                 {
//                     // In such case we don't use slave latency
//                     evt->missed_cnt = 0;
//                 }
//                 else
//                 {
//                     uint16_t latency = evt->latency - 1;

//                     // Correct the latency in case the total accuracy is 1000ppm
//                     if ((evt->mst_sca == SCA_500PPM) && (lld_evt_env.sca == SCA_500PPM) &&
//                         (latency > LLD_EVT_MAX_LATENCY))
//                     {
//                         // Latency is too high and may cause connection loss due to Window Widening, so
//                         // limit it to the max value
//                         latency = LLD_EVT_MAX_LATENCY;
//                     }

//                     if ((evt->inst_action != LLD_NO_ACTION) && (evt->instant > evt->counter))
//                     {
//                         latency = co_min(evt->instant - evt->counter - 1, latency);
//                     }

//                     evt->missed_cnt = latency;
//                     evt_cnt_inc = latency + 1;
//                 }

//                 // Get the RX time from the control structure
//                 evt->anchor_point.basetime_cnt = ble_btcntsync0_get(evt->conhdl) |
//                                          (((uint32_t)ble_btcntsync1_get(evt->conhdl)) << 16);
//                 evt->anchor_point.finetime_cnt = 624 - ble_fcntsync_get(evt->conhdl);
//             }

//             // Recompute the sync window
//             lld_evt_rxwin_compute(evt);

//                 // Check if the event has to be reprogrammed
//                 if (evt->restart_pol == LLD_STOP_REQUESTED)
//                 {
//                     // Confirm the stop to the host
//                     ke_msg_send_basic(LLD_STOP_IND, KE_BUILD_ID(TASK_LLC,evt->conhdl),
//                                       TASK_LLD);

//                     // Reset event interval (indicating that event is free)
//                     evt->interval = 0;
//                     // Flush the event
//                     {
//                         uint8_t nb_of_pkt_flushed = lld_data_tx_flush(evt);
//                         // if the number of packet flushed is not NULL send a number of packets
//                         if(nb_of_pkt_flushed > 0)
//                         {
//                             llc_common_nb_of_pkt_comp_evt_send(evt->conhdl, nb_of_pkt_flushed);
//                         }
//                     }


//                     #if (BLE_CENTRAL || BLE_OBSERVER)
//                     // Extract the event from the interval list
//                     if (evt->int_list != NULL)
//                         lld_evt_int_extract(evt);
//                     #endif // BLE_CENTRAL || BLE_OBSERVER

//                     // Free the event structure
//                     evt->free = true;

//                     // The event is not reprogrammed, so schedule events to ensure that
//                     // the fine target time is correctly updated
//                     ke_event_set(KE_EVENT_BLE_EVT_START);
//                     break;
//                 }

//             // Move the Anchor point
//             next_basetimecnt = (evt->anchor_point.basetime_cnt +
//                                               evt->interval * (evt->missed_cnt + 1)) & BLE_BASETIMECNT_MASK;

//             // Check if an action has to be performed
//             if (evt->waiting_evt & LLD_EVT_WAITING_INSTANT)
//             {
//                 evt->waiting_evt &= ~LLD_EVT_WAITING_INSTANT;
//                 switch (evt->inst_action)
//                 {
//                     case LLD_NO_ACTION:
//                         break;
//                     case LLD_PARAM_UPDATE:
//                         // Switch to new parameters
//                         lld_evt_slave_param_update_perform(evt, &next_basetimecnt);
//                         // Warn the LLC about the connection parameter update
//                         llc_con_update_ind(evt->conhdl, evt);
//                         break;
//                     // A channel map update has to be performed
//                     case LLD_CHMAP_UPDATE:
//                         // Clear the action
//                         evt->inst_action = LLD_NO_ACTION;
//                         // Warn the LLC about the channel map update
//                         llc_map_update_ind(evt->conhdl);
//                         break;
//                 }
//             }

//             // Compute the time of the next event according to new Anchor point
//             lld_evt_slave_time_compute(evt, next_basetimecnt);

//             // Insert the event in the list of events
//             lld_evt_insert(evt);

//             // Increment the event counter
//             evt->counter += evt_cnt_inc;

//             if (evt_cnt_inc > 1)
//             {
//                 // Perform manual hopping in case we increment by more that one the event counter
//                 uint16_t hopcntl = ble_hopcntl_get(evt->conhdl);
//                 uint16_t last_ch = hopcntl & BLE_CH_IDX_MASK;
//                 uint16_t hop = (hopcntl & BLE_HOP_INT_MASK) >> BLE_HOP_INT_LSB;
//                 uint16_t next_ch;

//                 // Compute the next channel
//                 next_ch = (last_ch + (evt_cnt_inc - 1) * hop) % 37;

//                 // Set it in the control structure
//                 ble_hopcntl_set(evt->conhdl, BLE_FH_EN_BIT | (hop << BLE_HOP_INT_LSB) | next_ch);
//             }

//             // Ensure that event will be programmed next time
//             evt->waiting_evt |= LLD_EVT_WAITING_SYNC;
//             break;
//         #endif // BLE_PERIPHERAL


//         default:
//             // Memory corruption
//             ASSERT_ERR(0);
//             break;
//     }
// 		
// 		GLOBAL_INT_START();
// }


// /**
//  ****************************************************************************************
//  * @brief Program an event in the exchange table
//  *
//  * @param[in] evt   Pointer to the event to be programmed
//  *
//  ****************************************************************************************
//  */
// static void lld_evt_prog(struct lld_evt_tag *const evt)
// {
//     int em_idx = evt->time & 0x0F; // Get index in the table from the time of the event;

//     // By default we consider that we will get a sync error and no updated descriptors
//     evt->rx_sync_err = true;
//     evt->rx_cnt = 0;

//     // Check if the pointer in the exchange table is free or not
//     if (ble_extab_get(em_idx) != 0)
//     {
//         // Extract it from the list of events
//         co_list_extract(&lld_evt_env.evt_prog, &evt->hdr);

//         // And restart it
//         lld_evt_restart(evt);
//         return;
//     }

//     // Program the TX data buffers in the control structure
//     lld_data_tx_prog(evt);

//     #if (BLE_PERIPHERAL)
//     // Check if the event has to be programmed or if it can be skipped
//     if (evt->restart_pol == LLD_SLV_RESTART)
//     {
//         // Check if the event has to be programmed or not
//         lld_evt_slave_wakeup(evt);

//         if (!evt->waiting_evt)
//         {
//             // Extract the event from the list of events
//             co_list_extract(&lld_evt_env.evt_prog, &evt->hdr);

//             // The event is not programmed, restart it
//             lld_evt_restart(evt);
//             return;
//         }

//         // Event will be programmed, so now we have to receive a sync
//         evt->waiting_evt |= LLD_EVT_WAITING_SYNC;
//     }
//     #endif // BLE_PERIPHERAL

//     // Protect from interrupt preemption for programming in the past check
//     GLOBAL_INT_DISABLE();

//     // Sanity check: We should not program an event in the past
//     //ASSERT_ERR(!lld_evt_time_past(evt->time - 2);
//     if (lld_evt_time_past(evt->time - 2))
//     {
//         // Extract it from the list of events
//         co_list_extract(&lld_evt_env.evt_prog, &evt->hdr);

//         // And restart it
//         lld_evt_restart(evt);
//     }
//     else
//     {
//         // Program the event in the exchange table
//         ble_extab_set(em_idx, REG_BLE_EM_CS_ADDR_GET(evt->conhdl));

//         // Indicate that the event is now programmed
//         evt->prog = true;
//     }

//     // Restore the interrupts
//     GLOBAL_INT_RESTORE();

//     // Post programming checks
//     if (evt->prog)
//     {
//         // If event is a scanning event, set the scanning flag
//         if (evt->restart_pol == LLD_SCN_RESTART)
//         {
//             // Sanity check
//             ASSERT_ERR(lld_evt_env.scan_evt == NULL);
//             lld_evt_env.scan_evt = evt;
//         }
//         // Otherwise, if scan is programmed, check if it has to be aborted or not
//         else if (lld_evt_env.scan_evt)
//         {
//             struct lld_evt_tag *scn_evt = lld_evt_env.scan_evt;
//             uint32_t scn_end = (scn_evt->time + scn_evt->duration + 2) % BLE_BASETIMECNT_MASK;

//             // Check if scan will be finished upon new event time
//             if (lld_evt_time_cmp(evt->time, scn_end))
//             {
//                 // Scan is too long, abort it
//                 #if (BLE12_HW)
//                 ble_rwblecntl_set(ble_rwblecntl_get() | BLE_SCAN_ABORT_BIT);
//                 #else // (BLE12_HW)
//                 ble_rwbtlecntl_set(ble_rwbtlecntl_get() | BLE_SCAN_ABORT_BIT);
//                 #endif // (BLE12_HW)
//                 while(ble_scan_abort_getf());
//             }
//         }
//     }
// }

// /**
//  ****************************************************************************************
//  * @brief Flush all events / intervals
//  ****************************************************************************************
//  */
// static void lld_evt_flush(void)
// {
//     struct lld_evt_tag *evt_curr = (struct lld_evt_tag *)lld_evt_env.evt_prog.first;
//     struct lld_evt_tag *evt_temp = NULL;

//     struct lld_evt_int_tag *int_curr = (struct lld_evt_int_tag *)lld_evt_env.int_used.first;
//     struct lld_evt_int_tag *int_temp = NULL;

//     // Flush events
//     while(evt_curr)
//     {
//         // Get next
//         evt_temp = (struct lld_evt_tag *) evt_curr->hdr.next;

//         // Free element
//         ke_free(evt_curr);

//         // Move to next
//         evt_curr = evt_temp;
//     }

//     // Flush intervals
//     while(int_curr)
//     {
//         // Get next
//         int_temp = (struct lld_evt_int_tag *) int_curr->hdr.next;

//         // Free element
//         ke_free(int_curr);

//         // Move to next
//         int_curr = int_temp;
//     }
// }
// /*
//  ****************************************************************************************
//  *
//  * PUBLIC FUNCTION DEFINITIONS
//  *
//  ****************************************************************************************
//  */

void lld_evt_init(bool reset)
{
    typedef void (*my_function)( bool);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[lld_evt_init_pos]);
    PtrFunc(reset);
}


void lld_evt_init_func(bool reset)
{
    if(reset)
    {
        // Flush all events
//        lld_evt_flush();
    }
    // Clear the environment
    memset(&lld_evt_env, 0, sizeof(lld_evt_env));

    // Initialize lists
    co_list_init(&lld_evt_env.evt_prog);
    co_list_init(&lld_evt_env.int_used);


    // Initialize the local SCA
    lld_evt_local_sca_init();

    // Register BLE start event kernel event
    ke_event_callback_set(KE_EVENT_BLE_EVT_START, &lld_evt_schedule);
    // Register BLE RX event kernel event
    ke_event_callback_set(KE_EVENT_BLE_RX,        &lld_evt_rx);
    // Register BLE end event kernel event
    ke_event_callback_set(KE_EVENT_BLE_EVT_END,   &lld_evt_end);
}


// #if 0
// struct lld_evt_tag *lld_evt_conhdl2evt(uint16_t conhdl)
// {
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);

//     // Loop into the event list to search for the good one
//     while (evt != NULL)
//     {
//         // Check if current event has the connection handle that we search
//         if (evt->conhdl == conhdl)
//             break;

//         // Go to next event in the list
//         evt = (struct lld_evt_tag *)co_list_next(&evt->hdr);
//     }
//     return (evt);
// }


// void lld_evt_ack_received(struct lld_evt_tag *evt)
// {
//     // We received the acknowledgment
//     evt->waiting_evt &= ~LLD_EVT_WAITING_ACK;
// }
// #endif //if 0


// void lld_evt_schedule_next(struct lld_evt_tag *evt)
// {
//     
//     uint32_t curr_time = (lld_evt_time_get() + LLD_EVT_PROG_LATENCY) & BLE_BASETIMECNT_MASK;
//     uint32_t delay;
//     uint32_t next_basetimecnt = 0;
//     uint16_t old_missed_cnt;
//     uint16_t hopcntl = ble_hopcntl_get(evt->conhdl);
//     int16_t next_ch = hopcntl & BLE_CH_IDX_MASK;
//     uint16_t hop = (hopcntl & BLE_HOP_INT_MASK) >> BLE_HOP_INT_LSB;


//     // Sanity check - the event must be active
//     ASSERT_ERR(evt->interval != 0);

//     do
//     {
//         // Check if the event is a slave one
//         if (evt->restart_pol != LLD_SLV_RESTART)
//             break;

//         // Check if the event is not already programmed to exchange memory
//         if (evt->prog)
//             break;

//         // Check if a parameter update procedure is ongoing or not
//         if (evt->win_size)
//             break;

//         // Check if next programmed occurrence of the event is not too close in the future
//         if (lld_evt_time_cmp(evt->time, (curr_time + evt->interval) & BLE_BASETIMECNT_MASK))
//             break;

//         // Extract it from the list of events
//         co_list_extract(&lld_evt_env.evt_prog, &evt->hdr);

//         // Save the missed_cnt to reuse it later for counter and channel re-computing
//         old_missed_cnt = evt->missed_cnt;

//         // Compute the number of missed connection events since last anchor point
//         delay = (curr_time - evt->anchor_point.basetime_cnt) & BLE_BASETIMECNT_MASK;
//         evt->missed_cnt = delay / evt->interval;

//         // Compute anchor point of next occurrence
//         next_basetimecnt = (evt->anchor_point.basetime_cnt +
//                                            ((uint32_t)evt->missed_cnt + 1) * (uint32_t)evt->interval) &
//                                                                 BLE_BASETIMECNT_MASK;

//         while (1)
//         {
//             // Compute the window size according to the number of missed events
//             lld_evt_rxwin_compute(evt);

//             // Compute the next event programming time
//             lld_evt_slave_time_compute(evt, next_basetimecnt);

//             // Check if this time is far enough in the future
//             if (lld_evt_time_cmp(curr_time, evt->time))
//                 break;

//             // Next anchor point is too close in the future due to window widening, so
//             // schedule the event 1 interval later
//             evt->missed_cnt++;
//             next_basetimecnt = (next_basetimecnt + evt->interval) & BLE_BASETIMECNT_MASK;
//         }
//         // Re-compute the event counters and channel
//         evt->counter -= (old_missed_cnt - evt->missed_cnt);
//         next_ch = (next_ch - ((old_missed_cnt - evt->missed_cnt) * hop)) % 37;
//         if (next_ch < 0)
//             next_ch += 37;

//         // Set it in the control structure
//         ble_hopcntl_set(evt->conhdl, BLE_FH_EN_BIT | (hop << BLE_HOP_INT_LSB) | next_ch);

//         // Insert back the event to the list
//         lld_evt_insert(evt);
//     } while (0);
// }

// #if 0
// void lld_evt_schedule(void)
// {
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);
//     uint32_t curr_time = (lld_evt_time_get() + LLD_EVT_PROG_LATENCY + 1) & BLE_BASETIMECNT_MASK;
//     uint32_t end_time = (curr_time + 14) & BLE_BASETIMECNT_MASK;

//     // Clear kernel event
//     ke_event_clear(KE_EVENT_BLE_EVT_START);

//     // Disable target timer
//     ble_finetgtimintmsk_setf(0);

//     // Go through the events and program the ones that are ready to
//     while (evt != NULL)
//     {
//         // Skip all already programmed events
//         if (evt->prog)
//         {
//             // Get next event and continue looping
//             evt = (struct lld_evt_tag *)co_list_next(&evt->hdr);
//             continue;
//         }

//         // Check if event is too far in the future to be programmed
//         if (!lld_evt_time_cmp(evt->time - 2, curr_time) ||
//             !lld_evt_time_cmp(evt->time, end_time))
//         {
//             // Event is too far in the future, we program the wake-up time and break
//             lld_evt_target_time_prog(evt);
//             break;
//         }

//         // Program the event in the exchange table
//         lld_evt_prog(evt);

//         // Move current time
//         curr_time = evt->time;
//         // check if event should be free
//         lld_evt_try_free(&(evt));

//         // Get next event
//         if(evt != NULL)
//         {
//         evt = (struct lld_evt_tag *)co_list_next(&evt->hdr);
//     }
// }
// }
// #endif


// void lld_evt_delete(struct lld_evt_tag *evt, uint32_t abort_bit)
// {
//     // Check if event is still active or not
//     if (evt->interval != 0)
//     {
//         // Indicate that the deletion is requested
//         evt->restart_pol = LLD_STOP_REQUESTED;

//         // Check if event is programmed or not
//         if (evt->prog)
//         {
//             // Event programmed, abort it if possible
//             #if (BLE12_HW)
//             ble_rwblecntl_set(ble_rwblecntl_get() | abort_bit);
//             #else
//             ble_rwbtlecntl_set(ble_rwbtlecntl_get() | abort_bit);
//             #endif
//             while(ble_scan_abort_getf());
//         }
//         else
//         {
//             // If event is the first of the list
//             if (lld_evt_env.evt_prog.first == &evt->hdr)
//             {
//                 // disable the target timer
//                 ble_finetgtimintmsk_setf(0);

//                 // force a rescheduling
//                 ke_event_set(KE_EVENT_BLE_EVT_START);
//             }

//             // Extract it from the list of events
//             co_list_extract(&lld_evt_env.evt_prog, &evt->hdr);

//             // And simulate a restart to stop it immediately
//             lld_evt_restart(evt);
//         }
//     }
//     else
//     {
//         ke_task_id_t destid = (evt->conhdl==LLD_ADV_HDL)?TASK_LLM:KE_BUILD_ID(TASK_LLC,evt->conhdl);
//         // Confirm the stop to the host
//         ke_msg_send_basic(LLD_STOP_IND, destid, TASK_LLD);
//     }
// }


// #if (BLE_CENTRAL || BLE_OBSERVER)
// struct lld_evt_tag *lld_evt_scan_create(uint16_t handle,
//                                         uint16_t duration,
//                                         uint16_t mininterval,
//                                         uint16_t maxinterval,
//                                         uint16_t latency)
// {
//     // Allocate event structure
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)ke_malloc(sizeof(struct lld_evt_tag), KE_MEM_KE_MSG);

//     // Sanity check: There should always be a free event
//     ASSERT_ERR(evt != NULL);

//     // Fill in event properties
//     lld_evt_init_fields(evt);
//     evt->conhdl = handle;
//     evt->latency = latency + 1;
//     evt->restart_pol = LLD_SCN_RESTART;

//     // Compute the scheduling parameters
//     lld_evt_sched_param_compute(evt, duration, mininterval, maxinterval);

//     // Insert the event in the event list
//     lld_evt_insert(evt);

//     // Return pointer to the created event
//     return(evt);
// }
// #endif // BLE_CENTRAL || BLE_OBSERVER

// #endif //if 0

// #if (BLE_CENTRAL)
// void lld_evt_move_to_master(struct lld_evt_tag *evt, uint16_t conhdl)
// {
//     uint16_t ce_len = CO_ALIGN2_HI(ble_minevtime_get(conhdl));
//     // Sanity check: This procedure normally occurs far enough from the next occurrence
//     ASSERT_ERR(!lld_evt_time_past(evt->time - LLD_EVT_PROG_LATENCY));

//     // Update connection handle
//     evt->conhdl = conhdl;

//     // Update restart policy
//     evt->restart_pol = LLD_MST_RESTART;

//     // Reset connection counter
//     evt->counter = 0;

//     // Adjust the duration of the event and the number of free slots
//     evt->int_list->freeslot += (evt->duration - ce_len) / 2;
//     evt->duration = ce_len;

//     // In case the initiator event was skipped, perform hopping manually
//     while(evt->missed_cnt--)
//     {
//         lld_evt_channel_next(evt);
//         evt->counter++;
//     }

//     // If the event is already programmed, force it to be reprogrammed
//     if (evt->prog)
//     {
//         int em_idx = evt->time & 0x0F;
//         // Reset the event in the exchange table
//         ble_extab_set(em_idx, 0);

//         // Reprogram it
//         lld_evt_prog(evt);
//     }

// }

// // already in rom
// #if 0
// struct lld_evt_tag *lld_evt_update_create(struct lld_evt_tag *old_evt,
//                                           uint16_t mininterval,
//                                           uint16_t maxinterval,
//                                           uint16_t latency,
//                                           struct lld_evt_update_tag *upd_par)
// {
//     // Allocate temporary event structure
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)ke_malloc(sizeof(struct lld_evt_tag), KE_MEM_NON_RETENTION);

//     // Sanity check: There should always be a free event
//     ASSERT_ERR(evt != NULL);

//     // Fill in event properties
//     lld_evt_init_fields(evt);
//     evt->conhdl = old_evt->conhdl;
//     evt->latency = latency + 1;
//     evt->restart_pol = LLD_MST_RESTART;
//     evt->drift_base = old_evt->drift_base;

//     // Compute the new scheduling parameters
//     lld_evt_sched_param_compute(evt, old_evt->duration, mininterval, maxinterval);

//     // Compute the parameters of the connection update
//     lld_evt_update_param_compute(old_evt, evt, upd_par);

//     // Store the new event pointer into the old one, in order to program it at instant
//     old_evt->alt_evt = evt;
//     old_evt->inst_action = LLD_PARAM_UPDATE;

//     // Return pointer to the created event
//     return(evt);
// }

// uint16_t lld_evt_ch_map_update_req(struct lld_evt_tag *evt)
// {
//     // The instant will be 7 wake-up times after the next event
//     uint16_t count_to_inst = evt->latency * 7;

//     // Program the instant in the event
//     evt->instant = evt->counter + count_to_inst;

//     // Indicate the action that will be performed at instant
//     evt->inst_action = LLD_CHMAP_UPDATE;

//     // Return the instant to be put in the frame
//     return (evt->instant + 1);
// }
// #endif //if 0
// #endif // BLE_CENTRAL

// //#if (BLE_APP_SLAVE)
// //#if (BLE_PERIPHERAL || BLE_CENTRAL)
// struct lld_evt_tag *lld_evt_move_to_slave_p(struct llc_create_con_req_ind const *con_par,
//                                           struct llm_pdu_con_req_rx *con_req_pdu,
//                                           struct lld_evt_tag *evt_adv,
//                                           uint16_t conhdl)
// {
//     struct lld_evt_tag *evt;
//     uint8_t winsize_pdu = con_req_pdu->winsize;
//     uint16_t winoff_pdu = co_btohs(con_req_pdu->winoffset);
//     uint8_t mst_sca = con_req_pdu->hop_sca >> 5;
//     if(evt_adv != NULL)
//     {
//         // Delete the advertising event
//         // Sanity check: The event should never be programmed at this stage
//         ASSERT_ERR(evt_adv->prog == 0);
//         lld_evt_delete(evt_adv, BLE_ADVERT_ABORT_BIT);
//         lld_evt_try_free(&llm_le_env.evt); // delete event message
//     }
//     // Allocate a new event for the slave connection
//     evt = (struct lld_evt_tag *)ke_malloc(sizeof(struct lld_evt_tag), KE_MEM_KE_MSG);

//     // Sanity check: There should always be an event available
//     ASSERT_ERR(evt != NULL);

//     if(evt_adv != NULL)
//     {
//         //back to IDLE state
//         rom_ke_state_set(TASK_LLM, LLM_IDLE);
//     }
// 	else
// 	{
//         // go to stopping state
//         rom_ke_state_set(TASK_LLM, LLM_STOPPING);
//     }

// 		// use new allocated event to be sure that it will not be freed after.
//     // without patch and out of corner case, llm_le_env.evt was already pointing on this event due
//     // to memory allocation mechanism.
//     llm_le_env.evt = evt;

// 		
//     // Fill in event properties
//     lld_evt_init_fields(evt);
//     evt->conhdl = conhdl;
//     evt->interval = con_par->con_int * 2;
//     evt->latency = con_par->con_lat + 1;
//     evt->restart_pol = LLD_SLV_RESTART;
//     evt->duration = 0xFFFF;
//     evt->mst_sca = mst_sca;

//     // Compute the theoretical drift according to the interval and the accuracy
//     evt->drift_base = lld_evt_drift_compute(evt->interval, mst_sca);

//     // Check if window offset is too small
//     if (winoff_pdu < 4)
//     {
//         // Move the offset by one interval
//         winoff_pdu += con_par->con_int;

//         // Update the channel index
//         lld_evt_channel_next(evt);

//         // Update the event counter
//         evt->counter = 1;
//     }

//     // Compute the possible drift to the connection window
//     evt->drift_current = lld_evt_drift_compute((winoff_pdu + winsize_pdu + 1) * 2, mst_sca);

//     // Set the half connect window size according to the value received in the connect_req
//     evt->win_size = winsize_pdu * 625 + 40;

//     // Set the value of the window size in the control structure
//     lld_evt_winsize_change(evt);

//     // Compute the time of first occurrence of the event
//     // Get the time of the CONNECT_REQ reception
//     uint32_t basetimecnt = (uint32_t)ble_btcntsync0_get(LLD_ADV_HDL) |
//                                     (((uint32_t)ble_btcntsync1_get(LLD_ADV_HDL)) << 16);
//     uint16_t finetimecnt = 624 - ble_fcntsync_get(LLD_ADV_HDL);
//     // Add the duration of the CONNECT_REQ
//     finetimecnt += 312;     // CONNECT_REQ duration is 312us from SYNC
//     if (finetimecnt > 624)
//     {
//         // Fine counter has wrapped
//         finetimecnt -= 625;
//         // Increment Base time counter
//         basetimecnt++;
//     }

//     // Compute now the time of the window center (current + 1.25ms + WinOffset + WinSize/2)
//     basetimecnt = (basetimecnt + 2 * (1 + winoff_pdu) + winsize_pdu) & BLE_BASETIMECNT_MASK;

//     // Get the RX time from the control structure
//     evt->anchor_point.basetime_cnt = basetimecnt;
//     evt->anchor_point.finetime_cnt = finetimecnt;

//     // Compute the time of the event according to the parameters
//     lld_evt_slave_time_compute(evt, basetimecnt);

//     // Trick to avoid losing the 2nd anchor point in case where the 1st is lost, we are saving teh virtual
//     // last anchor point value.
//     evt->anchor_point.basetime_cnt = (evt->anchor_point.basetime_cnt - evt->interval) & BLE_BASETIMECNT_MASK;
//     
//     // Insert the event in the list of events
//     lld_evt_insert(evt);

//     // Indicate that we are waiting for the initial synchronization with the master
//     evt->waiting_evt |= LLD_EVT_WAITING_ACK;

//     return(evt);
// }
// //#endif // BLE_APP_SLAVE

// #if 0

// void lld_evt_slave_update(struct llcp_con_up_req const *param_pdu,
//                           struct lld_evt_tag *old_evt)
// {
//     // Allocate a new temporary event for the slave connection
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)ke_malloc(sizeof(struct lld_evt_tag), KE_MEM_NON_RETENTION);

//     // Sanity check: There should always be an event available
//     ASSERT_ERR(evt != NULL);

//     // Fill in event properties
//     lld_evt_init_fields(evt);
//     evt->conhdl = old_evt->conhdl;
//     evt->interval = param_pdu->interv * 2;
//     evt->latency = param_pdu->latency + 1;
//     evt->restart_pol = LLD_SLV_RESTART;
//     evt->duration = 0xFFFF;
//     evt->mst_sca = old_evt->mst_sca;

//     // Compute the theoretical RX half window size according to the interval and the accuracy
//     evt->drift_base = lld_evt_drift_compute(evt->interval, evt->mst_sca);

//     // Compute the possible drift to the update window
//     evt->drift_current = lld_evt_drift_compute((co_btohs(param_pdu->win_off) +
//                              param_pdu->win_size) * 2 + old_evt->interval, evt->mst_sca);

//     // Set the half connect window size according to the value received in the connect_req
//     evt->win_size = param_pdu->win_size * 625 + 40;
//     evt->waiting_evt |= LLD_EVT_WAITING_ACK;

//     // Store the new event in the old one while waiting for instant
//     old_evt->alt_evt = evt;
//     old_evt->instant = param_pdu->instant - 1;
//     old_evt->inst_action = LLD_PARAM_UPDATE;
//     old_evt->update_offset = param_pdu->win_off;
//     old_evt->update_size = param_pdu->win_size;
//     old_evt->waiting_evt |= LLD_EVT_WAITING_ACK;
// }

// void lld_evt_ch_map_update_ind(struct lld_evt_tag *evt, uint16_t instant)
// {
//     // Store the channel map update information in the event
//     evt->instant = instant - 1;
//     evt->inst_action = LLD_CHMAP_UPDATE;
//     evt->waiting_evt |= LLD_EVT_WAITING_ACK;
// }
// #endif
// //#endif // BLE_PERIPHERAL || BLE_CENTRAL

// #if 0
// struct lld_evt_tag *lld_evt_adv_create(uint16_t handle,
//                                        uint16_t mininterval,
//                                        uint16_t maxinterval,
//                                        uint8_t  restart_pol)
// {
//     // Allocate event structure
//     struct lld_evt_tag *evt = (struct lld_evt_tag *)ke_malloc(sizeof(struct lld_evt_tag), KE_MEM_KE_MSG);

//     // Sanity check: There should always be a free event
//     ASSERT_ERR(evt != NULL);

//     // Fill in event properties
//     lld_evt_init_fields(evt);
//     evt->conhdl = handle;
//     evt->interval = maxinterval;
//     evt->restart_pol = restart_pol;

//     // Schedule event as soon as possible
//     evt->time = (lld_evt_time_get() + LLD_EVT_PROG_LATENCY) & BLE_BASETIMECNT_MASK;
//     evt->duration = 1;

//     // Insert the event in the event list
//     lld_evt_insert(evt);

//     // Return pointer to the created event
//     return(evt);
// }

// void lld_evt_end(void)
// {
//     struct lld_evt_tag *evt;

//     // Clear kernel event
//     ke_event_clear(KE_EVENT_BLE_EVT_END);

//     while(1)
//     {
//         uint8_t status;
//         int em_idx;

//         // Get next event in the list of programmed events
//         evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);

//         // If no more events, exit
//         if ((evt == NULL) || (!evt->prog))
//             break;

//         // Check if event has been handled by reading the exchange table status
//         em_idx = evt->time & 0x0F;
//         status = ble_etstatus_getf(em_idx);
//         if (status == EM_BLE_ET_READY)
//             // Event is not processed yet, so we can exit
//             break;
//         if (status == EM_BLE_ET_PROCESSED)
//         {
//             // Get the total RX count in the control structure
//             uint8_t rx_cnt = ble_rxdesccnt_getf(evt->conhdl);

//             // Event has been processed, handle the transmitted and received data
//             lld_data_check(evt, rx_cnt - evt->rx_cnt, false);

//             // Update the RX count in the event
//             evt->rx_cnt = rx_cnt;
//         }

//         // Display a warning if event was not processed due to scheduling conflict
//         ASSERT_WARN((status == EM_BLE_ET_READY) || (status == EM_BLE_ET_PROCESSED));

//         // Update the TX power in the control structure
//         ble_txpwr_setf(evt->conhdl, evt->tx_pwr);

//         // Pop the event from the programmed list
//         co_list_pop_front(&lld_evt_env.evt_prog);

//         // Indicate that event is not programmed anymore
//         evt->prog = false;

//         // Clear the pointer in the exchange table
//         ble_extab_set(em_idx, 0);

//         // Restart the event
//         lld_evt_restart(evt);
//         // check if event should be free
//         lld_evt_try_free(&(evt));
//     }
// }

void lld_evt_rx(void)
{
    struct lld_evt_tag *evt;

    // Clear kernel event
    ke_event_clear(KE_EVENT_BLE_RX);

    // Get next event in the list of programmed events
    evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);

    // If no pending event, exit
 
	   if (evt != NULL)
        // Handle the received data
        lld_data_check(evt, LLD_RX_IRQ_THRES, true);
}

// void lld_evt_start_isr(void)
// {
//     // Clear the interrupt
//     ble_intack_clear(BLE_FINETGTIMINTACK_BIT);

//     // Set kernel event for deferred handling
//     ke_event_set(KE_EVENT_BLE_EVT_START);
// }

// void lld_evt_end_isr(void)
// {
//     // Sanity check: The event start ISR should not occur if there is an event pending
//     ASSERT_ERR(!(ke_event_get(KE_EVENT_BLE_EVT_END)));

//     // Clear the interrupt
//     ble_intack_clear(BLE_EVENTINTACK_BIT);

//     // Set kernel event for deferred handling
//     ke_event_set(KE_EVENT_BLE_EVT_END);
// }

// void lld_evt_rx_isr(void)
// {
//     // Clear the interrupt
//     ble_intack_clear(BLE_RXINTACK_BIT);

//     // Set kernel event for deferred handling
//     ke_event_set(KE_EVENT_BLE_RX);
// }

// #if (RW_BLE_SUPPORT)
// void lld_evt_timer_isr(void)
// {
//     // Clear the interrupt
//     ble_intack_clear(BLE_GROSSTGTIMINTACK_BIT);

//     // Set kernel event for deferred handling
//     ke_event_set(KE_EVENT_KE_TIMER);
// }
// #endif // RW_BLE_SUPPORT
// void lld_evt_try_free(struct lld_evt_tag ** evt)
// {
//     // check if event exist and should be free
//     if(((*evt) != NULL) && ((*evt)->free == true))
//     {
//         // free the event
//         ke_free(*evt);

//         // set pointer to NULL to be sure that heap data will no more be used.
//         *evt = NULL;
//     }
// }


/// @} LLDEVT
