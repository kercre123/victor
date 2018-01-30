/**
 ****************************************************************************************
 *
 * @file gtl_eif.c
 *
 * @brief Transport module for the Generic Transport Layer
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GTL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"      // SW configuration

#if GTL_ITF
    #include "co_bt.h"            // BT standard definitions
    #include "co_endian.h"
    #include "co_utils.h"

    #include "ke_event.h"         // kernel event
    #include "ke_msg.h"

    #include "gtl.h"
    #include "gtl_env.h"
    #include "gtl_eif.h"
    #include "gtl_task.h"
    #include "gtl_hci.h"
    #include "co_list.h"

    #if DEEP_SLEEP
        #include "ke_timer.h"
        #include "rwip.h"             // stack definitions
    #endif //DEEP_SLEEP

    #if BLE_APP_NEB
        #include "app_neb_task.h"
    #endif //BLE_APP_NEB

    #include "arch.h"
    #include "pll_vcocal_lut.h"
    #include "rf_580.h"
    #include "llm_task.h"
    #include "rf_580.h"
    #include "customer_prod.h"


//struct gtl_env_tag gtl_env __attribute__((section("exchange_mem_case1"))); 
extern struct gtl_env_tag gtl_env;


/*
 * DEFINES
 ****************************************************************************************
 */

    #if DEEP_SLEEP
        /// Period of sleep preventing when an GTL event is emitted
        #define GTL_SLEEP_TIMEOUT                  4   // 4 * 10ms = 40ms (+/-10ms)
        /// Period for polling the sleep status of GTL interface
        #define GTL_SLEEP_POLLING_INTERVAL         2   // 2 * 10ms = 20ms (+/-10ms)
    #endif //DEEPSLEEP

    #define GTL_SYNC_PATTERN_SIZE              3


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern uint16_t last_temp_count; /// temperature counter

 
 extern bool rwip_sleep_enable (void);

/// Transport layer synchronization pattern
static const uint8_t gtl_sync_pattern[GTL_SYNC_PATTERN_SIZE] = "RW!";


/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

static void gtl_eif_read_start(void);
static void gtl_eif_read_hdr(uint8_t len);
static void gtl_eif_tx_done(uint8_t status);
static void gtl_eif_rx_done(uint8_t status);


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
******************************************************************************************
* @brief Local function : places GTL EIF in RX_START state
* and sets the external interface environment.
******************************************************************************************
*/
static void gtl_eif_read_start(void)
{
    //Initialize external interface in reception mode state
    gtl_env.rx_state = GTL_STATE_RX_START;

    //Set the external interface environment to message type 1 byte reception
    gtl_env.ext_if->read(&gtl_env.curr_msg_type, 1, &gtl_eif_rx_done);

    #if DEEP_SLEEP
    // No GTL reception is ongoing, so allow going to sleep
    rwip_prevent_sleep_clear(RW_GTL_RX_ONGOING);
    #endif
}


/**
****************************************************************************************
* @brief Local function : places GTL EIF in RX header state
* and sets the external interface env.
*
* @param[in] len Length of header to be received in the currently set buffer.
*****************************************************************************************
*/
static void gtl_eif_read_hdr(uint8_t len)
{
    //change Rx state - wait for header next
    gtl_env.rx_state = GTL_STATE_RX_HDR;

    //set external interface environment to header reception of len bytes
    gtl_env.ext_if->read((uint8_t *)&gtl_env.curr_hdr_buff[0], len, &gtl_eif_rx_done);
    
    #if DEEP_SLEEP
    // An GTL reception is ongoing
    rwip_prevent_sleep_set(RW_GTL_RX_ONGOING);
    #endif //DEEP_SLEEP
}


/**
******************************************************************************************
* @brief Local function : places GTL EIF in RX payload state
* and sets the external interface env.
*
* @param[in] len      Length of payload to be received in the currently set buffer.
* @param[in] p_buf    Pointer to the payload buffer
******************************************************************************************
*/
static void gtl_eif_read_payl(uint16_t len, void* p_buf)
{
    // Change rx state to payload reception
    gtl_env.rx_state = GTL_STATE_RX_PAYL;

    // Start external interface reception of len bytes
    gtl_env.ext_if->read(p_buf, len, &gtl_eif_rx_done);
}


/**
******************************************************************************************
* @brief Local function : places GTL EIF in RX_START_OUT_OF_SYNC state.
******************************************************************************************
*/
static void gtl_eif_read_next_out_of_sync(void)
{
    //Set external interface reception state to GTL_STATE_RX_START_OUT_OF_SYNC
    gtl_env.rx_state = GTL_STATE_RX_OUT_OF_SYNC;

    //Set the external interface environment to 1 byte reception
    gtl_env.ext_if->read(&gtl_env.out_of_sync.byte, 1, &gtl_eif_rx_done);
}

/**
 ****************************************************************************************
 * @brief Check received byte in out of sync state
 *
 * This function is the algorithm to check that received byte stream in out of sync state
 * corresponds to GTL_reset command.
 *
 * Level of reception is incremented when bytes of GTL_reset_cmd are detected.
 *
 * @return   True if sync is completely received / False otherwise
 *****************************************************************************************
 */
static bool gtl_eif_out_of_sync_check(void)
{
    bool sync_found = false;

    // Check received byte according to current byte position
    if(gtl_env.out_of_sync.byte == gtl_sync_pattern[gtl_env.out_of_sync.index])
    {
        // Increment index
        gtl_env.out_of_sync.index++;
    }
    else
    {
        // Re-initialize index
        gtl_env.out_of_sync.index = 0;
    }

    if(gtl_env.out_of_sync.index >= GTL_SYNC_PATTERN_SIZE)
    {
        // Sync has been found
        sync_found = true;

        // Re-initialize index
        gtl_env.out_of_sync.index = 0;
    }

    return sync_found;
}

/**
 *****************************************************************************************
 *@brief Static function handling external interface out of synchronization detection.
 *
 * At external interface reception, when packet indicator opcode of a command is not
 * recognized.
 *
 *****************************************************************************************
 */
static void gtl_eif_out_of_sync(uint8_t rx_byte)
{
    // Initialize index
    gtl_env.out_of_sync.index = 0;

    // Check the RX byte
    gtl_env.out_of_sync.byte = rx_byte;
    gtl_eif_out_of_sync_check();

    // Start reception of new packet ID
    gtl_eif_read_next_out_of_sync();

    #if DEEP_SLEEP
    // No GTL reception is ongoing, so allow going to sleep
    rwip_prevent_sleep_clear(RW_GTL_RX_ONGOING);
    #endif // DEEP_SLEEP
}

/**
 ****************************************************************************************
 * @brief Actions after external interface TX.
 *
 * Analyzes the status value and sets the gtl environment state to TX_DONE/ERR
 * accordingly. This allows the higher function calling write to have feedback
 * and decide the following action (repeat/abort tx in case of error, continue otherwise).
 *
 * @param[in]  status external interface Tx status: ok or error.
 *****************************************************************************************
 */
static void gtl_eif_tx_done(uint8_t status)
{
    // Sanity check: Transmission should always work
    ASSERT_ERR(status == RWIP_EIF_STATUS_OK);

    // Defer the freeing of resources to ensure that it is done in background
    ke_event_set(KE_EVENT_GTL_TX_DONE);

    #if DEEP_SLEEP
    // The GTL transmission is finished, so allow going back to sleep
    rwip_prevent_sleep_clear(RW_GTL_TX_ONGOING);
    #endif // DEEP_SLEEP
}

static void gtl_eif_rx_free()
{
    if(gtl_env.p_msg_rx) {
        ke_msg_free(gtl_env.p_msg_rx);
        gtl_env.p_msg_rx = NULL;
    }
}
    
/**
 ****************************************************************************************
 * @brief Function called at each RX interrupt.
 *
 * According to GTL RX state, the received data is treated differently: message type,
 * header or payload. Once payload is obtained (if existing) the appropriate gtl unpacking
 * function is called thus generating a ke_msg for the appropriate task.
 *
 * @param[in]  status external interface RX status: ok or error
 *****************************************************************************************
 */
static void gtl_eif_rx_done(uint8_t status)
{
    // status is not the expected one.
    if(status != RWIP_EIF_STATUS_OK)
    {
        // free no more used buffers.
        if(gtl_env.rx_state == GTL_STATE_RX_PAYL)
        {
            // Check received packet indicator
            switch(gtl_env.curr_msg_type)
            {
    #if BLE_EMB_PRESENT
            case HCI_CMD_MSG_TYPE:
    #endif //BLE_EMB_PRESENT
            case GTL_KE_MSG_TYPE:
    #if BLE_APP_NEB
            case GTL_NEB_MSG_TYPE:
    #endif // BLE_APP_NEB
                // Send the kernel message
                gtl_eif_rx_free();
                break;

            default:
                ASSERT_ERR(0);
                break;
            }
        }

        // check occured event
        switch(status)
        {
        //detect external interface RX error and handle accordingly
        case RWIP_EIF_STATUS_ERROR:
            // external interface RX error -> enter in out of sync
            gtl_eif_out_of_sync(0);
            break;
        default:
            // Restart a new packet reception
            gtl_eif_read_start();
            return;
        }
    }

    //check GTL state to see what was received
    switch(gtl_env.rx_state)
    {
    /* RECEIVE MESSAGE TYPE STATE*/
    case GTL_STATE_RX_START:
        // Check received packet indicator
        switch(gtl_env.curr_msg_type)
        {
    #if BLE_EMB_PRESENT
        case HCI_CMD_MSG_TYPE:
            // Start header reception
            gtl_eif_read_hdr(HCI_CMD_HDR_LEN);
            break;
    #endif //BLE_EMB_PRESENT

        case GTL_KE_MSG_TYPE:
            // Start header reception
            gtl_eif_read_hdr(KE_MSG_HDR_LEN);
            break;

    #if BLE_APP_NEB
        case GTL_NEB_MSG_TYPE:
            // Start header reception
            gtl_eif_read_hdr(NEB_MSG_HDR_LEN);
            break;
    #endif // BLE_APP_NEB

        default:
            // Incorrect packet indicator -> enter in out of sync
            gtl_eif_out_of_sync(gtl_env.curr_msg_type);
            break;
        }
        break;
    /* RECEIVE MESSAGE TYPE STATE END*/

    /* RECEIVE HEADER STATE*/
    case GTL_STATE_RX_HDR:
        // Check received packet indicator
        switch(gtl_env.curr_msg_type)
        {
            struct gtl_kemsghdr *p_msg_hdr;
    #if BLE_EMB_PRESENT
        case HCI_CMD_MSG_TYPE:
            // Decode the header
            gtl_hci_rx_header();

            //no params
            if (gtl_env.p_msg_rx->param_len == 0)
            {
                if(gtl_env.p_msg_rx->id != HCI_LE_END_PROD_RX_TEST_CMD_OPCODE &&
                   gtl_env.p_msg_rx->id != HCI_TX_END_CONTINUE_TEST_CMD_OPCODE)
                {
                // Send message directly
                    ke_msg_send(ke_msg2param(gtl_env.p_msg_rx));
                }
                else //@WR coming in this else branch means
                     //the customer production test commands with parameter len==0
                     //are already handled in the gtl_hci_rx_header function.
                     //No ke_msg_send may be sent (cmd is not part of the Stack)
                     //and the used id and param_len must be cleared
                {
                    gtl_env.p_msg_rx->id = 0;
                    gtl_env.p_msg_rx->param_len = 0;
                    gtl_eif_rx_free();
                }

                // Restart a new packet reception
                gtl_eif_read_start();
            }
            else
            {
                // Start payload reception
                gtl_eif_read_payl(gtl_env.p_msg_rx->param_len, gtl_env.p_msg_rx->param);
            }
            break;
    #endif //BLE_EMB_PRESENT

        case GTL_KE_MSG_TYPE:
            p_msg_hdr = (struct gtl_kemsghdr*) gtl_env.curr_hdr_buff;

            // Allocate the kernel message
            gtl_env.p_msg_rx = ke_param2msg(ke_msg_alloc(p_msg_hdr->id,
                    p_msg_hdr->dest_id,
                    p_msg_hdr->src_id,
                    p_msg_hdr->param_len));

            //no params
            if (gtl_env.p_msg_rx->param_len == 0)
            {
                // Send message directly
                ke_msg_send(ke_msg2param(gtl_env.p_msg_rx));

                // Restart a new packet reception
                gtl_eif_read_start();
            }
            else
            {
                // Start payload reception
                gtl_eif_read_payl(gtl_env.p_msg_rx->param_len, gtl_env.p_msg_rx->param);
            }
            break;

    #if BLE_APP_NEB
        case GTL_NEB_MSG_TYPE:
            // Allocate the kernel message
            gtl_env.p_msg_rx = ke_param2msg(ke_msg_alloc(APP_NEB_MSG_RX, TASK_APP_NEB, TASK_GTL,
                                                         co_btohs(co_read16p(gtl_env.curr_hdr_buff))));

            //no params
            if (gtl_env.p_msg_rx->param_len == 0)
            {
                //send message directly
                ke_msg_send(ke_msg2param(gtl_env.p_msg_rx));
                //change gtl rx state to ke message header reception
                gtl_eif_read_start();
            }
            else
            {
                // Start payload reception
                gtl_eif_read_payl(gtl_env.p_msg_rx->param_len, gtl_env.p_msg_rx->param);
            }
            break;
    #endif // BLE_APP_NEB

        default:
            ASSERT_ERR(0);
            break;
        }
        break;
    /* RECEIVE HEADER STATE END*/

    /* RECEIVE PAYLOAD STATE */
    case GTL_STATE_RX_PAYL:
        // Check received packet indicator
        switch(gtl_env.curr_msg_type)
        {
    #if BLE_EMB_PRESENT
        case HCI_CMD_MSG_TYPE:
            // Unpack the parameters
            gtl_hci_rx_payload();
            // Send message directly, but not in case of a TX test with length added in the command
            if( gtl_env.p_msg_rx->id == LLM_RESET_CMD       ||
                gtl_env.p_msg_rx->id == LLM_LE_TEST_RX_CMD  ||
                gtl_env.p_msg_rx->id == LLM_LE_TEST_END_CMD ||
               (gtl_env.p_msg_rx->id == LLM_LE_TEST_TX_CMD && gtl_env.p_msg_rx->param_len == 3)
              )
            {
                ke_msg_send(ke_msg2param(gtl_env.p_msg_rx));
            }
            else //@WR coming in this else branch means
                 //the customer production test commands with parameter len!=0
                 //are already handled in the gtl_hci_rx_payload function.
                 //No ke_msg_send may be sent (cmd is not part of the Stack)
                 //and the used id and param_len must be cleared
            {
                gtl_env.p_msg_rx->id = 0;
                gtl_env.p_msg_rx->param_len = 0;
                gtl_eif_rx_free();
            }
            // Restart a new packet reception
            gtl_eif_read_start();
            break;
    #endif //BLE_EMB_PRESENT

        case GTL_KE_MSG_TYPE:
    #if BLE_APP_NEB
        case GTL_NEB_MSG_TYPE:
    #endif // BLE_APP_NEB
            // Send the kernel message
            ke_msg_send(ke_msg2param(gtl_env.p_msg_rx));

            // Restart a new packet reception
            gtl_eif_read_start();
            break;

        default:
            ASSERT_ERR(0);
            break;
        }
        break;
    /* RECEIVE PAYLOAD STATE END*/

    /* RX OUT OF SYNC STATE */
    case GTL_STATE_RX_OUT_OF_SYNC:
        // Check if sync pattern is fully received
        if(gtl_eif_out_of_sync_check())
        {
            //change gtl rx state to message type reception
            gtl_eif_read_start();
        }
        else
        {
            // Start a new byte reception
            gtl_eif_read_next_out_of_sync();
        }
        break;
    /* RX OUT OF SYNC STATE END*/
    }
    /* STATE SWITCH END */
}

/**
 ****************************************************************************************
 * @brief Function called after sending message through external interface,
 * to free ke_msg space and push the next message for transmission if any.
 *
 * The message is popped from the tx queue kept in gtl_env and freed using ke_msg_free.
 *****************************************************************************************
 */
static void gtl_eif_tx_done_evt_handler(void)
{
    // Clear the event
    ke_event_clear(KE_EVENT_GTL_TX_DONE);


    // Free the kernel message space
    ke_msg_free(gtl_env.p_msg_tx);

    // check if there is something in TX queue
    if(! co_list_is_empty(&gtl_env.tx_queue))
    {
        //extract the ke_msg pointer from the param passed and push it in GTL queue
        struct ke_msg *msg = (struct ke_msg *) co_list_pop_front(&gtl_env.tx_queue);

        // send the queued message using GTL
        gtl_send_msg(msg);
    }
    else
    {

        // Set GTL task to TX IDLE state
        ke_state_set(TASK_GTL, GTL_TX_IDLE);
    #if DEEP_SLEEP
    // Prevent from going to deep sleep, in case Host has other commands to send
    if(rwip_sleep_enable() && !rwip_ext_wakeup_enable())
    {
        // Prevent from going to sleep until next polling time
        rwip_prevent_sleep_set(RW_GTL_TIMEOUT);

        // Program the unblocking of GTL interface
        ke_timer_set(GTL_SLEEP_TO, TASK_GTL, GTL_SLEEP_TIMEOUT);
    }
    #endif //DEEP_SLEEP
    }
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void gtl_eif_init(void)
{
    typedef void (*my_function)( void);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[gtl_eif_init_pos]);
    PtrFunc();
}

void gtl_eif_init_func(void)
{
    // Register GTL TX DONE kernel event
    ke_event_callback_set(KE_EVENT_GTL_TX_DONE, &gtl_eif_tx_done_evt_handler);

    // Enable external interface
    gtl_env.ext_if->flow_on();

    //start external interface reception
    gtl_eif_read_start();
}

void gtl_eif_write(uint8_t type, uint8_t *buf, uint16_t len)
{
    #if DEEP_SLEEP
    // An GTL transmission is ongoing - The bit has to be set prior to call to write
    // as this function may call gtl_eif_tx_done immediately
    rwip_prevent_sleep_set(RW_GTL_TX_ONGOING);
    #endif // DEEP_SLEEP

    //pack event type message (external interface header)
    buf -= HCI_TRANSPORT_HDR_LEN;
    *buf = type;

    // Send data over external interface
    gtl_env.ext_if->write(buf, len + HCI_TRANSPORT_HDR_LEN, &gtl_eif_tx_done);
}

#if DEEP_SLEEP
void gtl_eif_start(void)
{
    // Enable external interface flow
    gtl_env.ext_if->flow_on();
}

bool gtl_eif_stop(void)
{
    uint8_t sleep_ok = false;

    sleep_ok = gtl_env.ext_if->flow_off();

    if(!sleep_ok && rwip_ext_wakeup_enable())
    {
        // Program another checking of the GTL interface
        ke_timer_set(GTL_POLLING_TO, TASK_GTL, GTL_SLEEP_POLLING_INTERVAL);
    }

    return (sleep_ok);
}
#endif //DEEP_SLEEP


#endif //GTL_ITF

/// @} GTL_EIF
