/**
 ****************************************************************************************
 *
 * @file lld_data.c
 *
 * @brief Implementation of the functions for data transmission/reception
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LLDDATA
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "lld.h"
#include "lld_data.h"
#include "llc.h"
#include "reg_ble_em_txe.h"
#include "reg_ble_em_tx.h"
#include "reg_ble_em_rx.h"
#include "reg_ble_em_cs.h"
#include "rf_580.h"
#include "customer_prod.h"

#if (RW_BLE_WLAN_COEX)
#include "lld_wlcoex.h"
#endif //RW_BLE_WLAN_COEX

extern volatile uint16_t test_rx_packet_nr;
extern volatile uint16_t test_rx_packet_nr_syncerr;
extern volatile uint16_t test_rx_packet_nr_crcerr;
extern volatile uint16_t rx_test_rssi_1;
extern volatile uint16_t rx_test_rssi_2;		
 

// Function declarations
void spi_write_rx_data_back_2_pxi (uint8* rx_buf_address, uint8 len);
//bool check_data(uint8 mode, uint8* rx_buf_address, uint8 len) ;//mode select expected data pattern.
void lld_data_tx_check(struct lld_evt_tag *evt, struct lld_data_ind *msg);
extern struct co_buf_env_tag co_buf_env;



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 
 

/**
 ****************************************************************************************
 * @brief Check the packets that have been received and indicate them to the upper layers
 *
 * @param[in]  evt    Event for which received data have to be checked
 * @param[out] msg    Message structure to be filled with the number of received packets
 *                    and the pointer to the first RX descriptor
 * @param[in]  rx_cnt Number of buffers that have be handled
 *
 ****************************************************************************************
 */
static void lld_data_rx_check(struct lld_evt_tag *evt,
                              struct lld_data_ind *msg,
                              uint8_t rx_cnt)
{
    //uint8_t hdl = co_buf_rx_current_get();
	uint8_t hdl = co_buf_env.rx_current;

    // Increment the number of handled buffers in the event
    evt->rx_cnt += rx_cnt;

    // Initialize the message
    msg->rx_cnt = rx_cnt;
    msg->rx_hdl = hdl;

    //Get the event counter
    msg->evt_cnt = evt->counter;


    // If required, copy the received buffers from exchange memory to system RAM
    while(rx_cnt--)
    {
        #if (!BLE_EM_PRESENT || BLE_PERIPHERAL)
        struct co_buf_rx_desc *rxdesc = co_buf_rx_get(hdl);
        #endif // #if (!BLE_EM_PRESENT || BLE_PERIPHERAL)

        #if (!BLE_EM_PRESENT)
        uint16_t len_msk;

        // Set the length mask according to the handle
        len_msk = (evt->conhdl == LLD_ADV_HDL)?BLE_RXADVLEN_MASK:BLE_RXLEN_MASK;

        // First read the fixed part of the descriptor
        em_ble_burst_rd(rxdesc,
                        REG_BLE_EM_RX_ADDR_GET(hdl),
                        8);
        em_ble_burst_rd(rxdesc->data,
                        REG_BLE_EM_RX_ADDR_GET(hdl) + 8,
                        (rxdesc->rxheader & len_msk) >> BLE_RXLEN_LSB);
        #endif // !BLE_EM_PRESENT

#ifdef SEND_RX_DATA_BACK_BY_SPI
//send received buffer by SPI back to Tester/PXI system
spi_write_rx_data_back_2_pxi((uint8*)rxdesc->data,37); //len=37 bytes, and also write RSSI/PEAKHOLD_RF_REG AND RXRSSI-DESCRIPTOR
// if (!check_data(0,(uint8*)rxdesc->data,37))
// {
//    // packet_amount_with_bit_errors++;
// }
//packet_amount++;
#endif	

#if PRODUCTION_TEST
		
if(test_state ==STATE_START_RX  || test_state==STATE_DIRECT_RX_TEST )
{
	test_rx_packet_nr++;
			
	if( (rxdesc->rxstatus & 1)==1 ) //SYNCERR
	{
		
		test_rx_packet_nr_syncerr++;
		
	}
	if( (rxdesc->rxstatus & 8)==8 ) //CRCERR
	{
		test_rx_packet_nr_crcerr++;
	}

	//check_rx_production_test_data((uint8*)rxdesc->data);
	//if (!check_data(0,(uint8*)rxdesc->data,37))
	// {
	//    // packet_amount_with_bit_errors++;
	// }
#define RSSI_VALUE_ADDED
#ifdef RSSI_VALUE_ADDED
//	test_rx_rssi_averaged_1 = (0x00FF&rxdesc->rxchass );
//	test_rx_rssi_averaged_2 =  GetWord16(RF_RSSI_RESULT_REG);
	rx_test_rssi_1 = (uint16)(((uint32)(rx_test_rssi_1 + (0x00FF&rxdesc->rxchass       )) )>>1);
	rx_test_rssi_2 = (uint16)(((uint32)(rx_test_rssi_2 + (GetWord16(RF_RSSI_RESULT_REG ) >> 8) ) ) >>1);
#endif	//RSSI_VALUE_ADDED
}
#endif 	//PRODUCTION_TEST	
		
        #if (BLE_PERIPHERAL)
        // If we are waiting for the acknowledgment, and it is received, enable the slave
        // latency
        if ((evt->waiting_evt & LLD_EVT_WAITING_ACK) && !(rxdesc->rxstatus & BLE_NESN_ERR_BIT))
            lld_evt_ack_received(evt);
        #endif // BLE_PERIPHERAL

        // Go to the next descriptor
        hdl = co_buf_rx_next(hdl);
    };

    // Move the current RX buffer
    co_buf_rx_current_set(hdl);

    // Verify if we get a sync error on the first RX descriptor
    if ((evt->rx_sync_err) && (evt->rx_cnt != 0))
    {
        if (!(co_buf_rx_get(msg->rx_hdl)->rxstatus & BLE_SYNC_ERR_BIT))
            evt->rx_sync_err = false;
    }
}

/**
 ****************************************************************************************
 * @brief Check the packets that have been transmitted and confirm them to the upper layers
 *
 * @param[in] evt     Event for which transmitted data have to be checked
 * @param[out] msg    Message structure to be filled with the number of transmitted data
 *                    control and non-connected packets.
 *
 ****************************************************************************************
 */
// static void lld_data_tx_check(struct lld_evt_tag *evt, struct lld_data_ind *msg)
// {
//     struct co_list *prog = &evt->tx_prog;
//     uint8_t tx_done_cnt = ble_txdesccnt_getf(evt->conhdl);
//     msg->tx_cnt = 0;
//     msg->tx_cnt_cntl = 0;

//     // Check if an empty packet is chained on top of list
//     if (evt->empty_chained && tx_done_cnt)
//     {
//         // Decrease the number of TX done to remove the empty packet
//         tx_done_cnt--;

//         // Reset the flag
//         evt->empty_chained = false;
//     }

//     // Go through the programmed list to flush the transmitted data
//     for (int i = 0; i < tx_done_cnt; i++)
//     {
//         // Pop the first descriptor from the programmed list
//         #if (BLE_CENTRAL || BLE_PERIPHERAL || BLE_DEBUG)
//         struct co_buf_tx_node *txnode = (struct co_buf_tx_node *) co_list_pop_front(prog);

//         // Sanity check
//         ASSERT_ERR(txnode != NULL);
//         ASSERT_ERR(ble_txdone_getf(txnode->idx));
//         #else
//         co_list_pop_front(prog);
//         #endif // BLE_CENTRAL || BLE_PERIPHERAL || BLE_DEBUG

//         // and free it (only if it is not a static descriptor)
//         #if (BLE_CENTRAL || BLE_PERIPHERAL)
//         if (txnode->idx < BLE_TX_BUFFER_DATA)
//         {
//             msg->tx_cnt++;
//             GLOBAL_INT_DISABLE();
//             co_buf_tx_free(txnode);
//             GLOBAL_INT_RESTORE();
//         }
//         else
//         #endif //BLE_CENTRAL || BLE_PERIPHERAL
//         {
//             msg->tx_cnt_cntl++;
//         }
//     }

//     // If programmed list is not empty, then set again the more bit in the last descriptor
//     if ((evt->conhdl != LLD_ADV_HDL) && (!co_list_is_empty(prog)))
//     {
//         // Set the More Data bit of the last descriptor
//         ble_txmd_setf(((struct co_buf_tx_node *)prog->last)->idx, 1);
//     }

//     // Check if the core sent a non acknowledged NULL as last packet
//     if (ble_lastempty_getf(evt->conhdl))
//     {
//         // Set the flag indicating that an empty packet will have to be chained
//         evt->empty_chained = true;

//         // Reset the last empty flag
//         ble_lastempty_setf(evt->conhdl, 0);
//     }
// }

/**
 ****************************************************************************************
 * @brief Flush all the packets pending for transmission in the specified list
 *
 * @param[in] list   List of buffers that have to be flushed
 *
 * @return The number of TX buffers that have been flushed
 *
 ****************************************************************************************
 */
// static uint8_t lld_data_tx_flush_list(struct co_list *list)
// {
//     uint8_t tx_cnt = 0;

//     // Go through the list to flush the data
//     while(1)
//     {
//         // Pop the first descriptor from the list
//         struct co_buf_tx_node *txnode = (struct co_buf_tx_node *)co_list_pop_front(list);

//         // If we reach the end of the list, then we exit the loop
//         if (txnode == NULL)
//             break;

//         // Increment the TX counter only if this is a data buffer
//         #if (BLE_CENTRAL || BLE_PERIPHERAL)
//         if (txnode->idx < BLE_TX_BUFFER_DATA)
//         {
//             tx_cnt++;

//             // Free the buffer
//             GLOBAL_INT_DISABLE();
//             co_buf_tx_free(txnode);
//             GLOBAL_INT_RESTORE();
//         }
//         #endif // BLE_CENTRAL || BLE_PERIPHERAL
//     }
//     return(tx_cnt);
// }

// void lld_data_tx_loop(struct lld_evt_tag *evt)
// {
//     struct co_list *rdy = &evt->tx_rdy;

//     // Sanity check: There should not be any data in the programmed list
//     ASSERT_ERR(co_list_is_empty(&evt->tx_prog));
//     // Sanity check: There should be at least one element in the ready list
//     ASSERT_ERR(!co_list_is_empty(rdy));

//     // Link the last descriptor with the first descriptor
//     ble_nextptr_setf(((struct co_buf_tx_node *)(rdy->last))->idx,
//                  REG_BLE_EM_TX_ADDR_GET(((struct co_buf_tx_node *)(rdy->first))->idx));
// }


// void lld_data_tx_push(struct lld_evt_tag *evt, struct co_buf_tx_node *txnode)
// {
//     struct co_list *list = &evt->tx_rdy;
//     // Sanity check: TX node should be valid
//     ASSERT_ERR(txnode != NULL);
//     struct co_buf_tx_desc *txdesc = co_buf_tx_desc_get(txnode->idx);


//     // By default, if it is not an advertising packet, set the more data bit in the descriptor
//     if (evt->conhdl != LLD_ADV_HDL)
//     {
//         txdesc->txheader |= BLE_TXMD_BIT;
//     }

//     // If required, copy the buffer from system RAM to exchange memory
//     #if (BLE_EM_PRESENT)
//     // Reset the Control Field of the TX descriptor
//     ble_txcntl_set(txnode->idx, 0);
//     #else
//     uint16_t len_msk;
//     // Set the length mask according to the handle
//     len_msk = (evt->conhdl == LLD_ADV_HDL)?BLE_TXADVLEN_MASK:BLE_TXLEN_MASK;

//     em_ble_burst_wr(txdesc,
//                     REG_BLE_EM_TX_ADDR_GET(txnode->idx),
//                     ((txdesc->txheader & len_msk) >> BLE_TXLEN_LSB) + 4);
//     #endif // BLE_EM_PRESENT

//     // Disable the interrupts as the list can be modified under interrupt
//     GLOBAL_INT_DISABLE();

//     // Link the new descriptor in exchange memory
//     if (!co_list_is_empty(list))
//     {
//         ble_nextptr_setf(((struct co_buf_tx_node *)(list->last))->idx,
//                                                     REG_BLE_EM_TX_ADDR_GET(txnode->idx));
//     }

//     // Push the descriptor at the end of the TX list
//     co_list_push_back(list, (struct co_list_hdr *)txnode);

//     // Try to re-schedule the event immediately
//     lld_evt_schedule_next(evt);
//     // Restore the interrupts
//     GLOBAL_INT_RESTORE();
// }

// already in rom


// void lld_data_tx_prog(struct lld_evt_tag *evt)
// {
//     uint16_t conhdl = evt->conhdl;
//     struct co_list *rdy = &evt->tx_rdy;
//     struct co_list *prog = &evt->tx_prog;
//     uint16_t tx_add = 0;
//     uint16_t txe_hdr = 0x01;

//     // By default we consider that no data has to be transmitted
//     evt->waiting_evt &= ~LLD_EVT_WAITING_TXPROG;

//     // Append the tx_rdy list to the tx_prog list
//     if (!co_list_is_empty(rdy))
//     {
//         #if (RW_BLE_WLAN_COEX)
//         if(ble_txllid_getf(((struct co_buf_tx_node *)rdy->first)->idx) == BLE_TXLLID_MASK)
//         {
//             //if Tx packet is LLCP
//             ble_pti_setf(conhdl,BLEMPRIO_LLCP);
//         }
//         else
//         {
//             //if Tx packet is data
//             ble_pti_setf(conhdl,BLEMPRIO_DATA);
//         }
//         #endif // RW_BLE_WLAN_COEX
//         if (!co_list_is_empty(prog))
//         {
//             // Chain the HW descriptors
//             ble_nextptr_setf(((struct co_buf_tx_node *)prog->last)->idx,
//                       REG_BLE_EM_TX_ADDR_GET(((struct co_buf_tx_node *)rdy->first)->idx));

//             // Append ready list to programmed list
//             co_list_merge(prog, rdy);
//         }
//         else
//         {
//             // Programmed list is empty, so copy ready list to programmed list
//             *prog = *rdy;
//         }
//         // Reset ready list
//         co_list_init(rdy);
//     }

//     // Program the control structure with the first tx descriptor pointer
//     if (!co_list_is_empty(prog))
//     {
//         // Reset the More Data bit from the last descriptor
//         ble_txmd_setf(((struct co_buf_tx_node *)prog->last)->idx, 0);

//         // Set the newly pushed descriptor pointer in the control structure
//         tx_add = REG_BLE_EM_TX_ADDR_GET(((struct co_buf_tx_node *)prog->first)->idx);
//         txe_hdr |= BLE_TXEMD_BIT;

//         // Signal that data has to be transmitted
//         evt->waiting_evt |= LLD_EVT_WAITING_TXPROG;
//     }
//     if (evt->empty_chained)
//     {
//         // Link the first descriptor to be transmitted with the NULL packet
//         ble_txecntl_set(conhdl, tx_add);

//         // Set the header field to an empty packet
//         ble_txephce_set(conhdl, txe_hdr);

//         // Program the empty packet first
//         ble_txdescptr_set(conhdl, REG_BLE_EM_TXE_ADDR_GET(conhdl));
//     }
//     else
//     {
//         // No empty packet to be chained, so chain the user data
//         ble_txdescptr_set(conhdl, tx_add);
//     }

// }

void lld_data_check(struct lld_evt_tag *evt, uint8_t rx_cnt, bool rx_only)
{
    ke_task_id_t destid = (evt->conhdl==LLD_ADV_HDL)?TASK_LLM:KE_BUILD_ID(TASK_LLC,evt->conhdl);
    struct lld_data_ind *msg = KE_MSG_ALLOC(LLD_DATA_IND, destid, TASK_LLD, lld_data_ind);

    // Check received data
    lld_data_rx_check(evt, msg, rx_cnt);

    #if (BLE_CENTRAL || BLE_PERIPHERAL)
    if (rx_only)
        msg->tx_cnt = 0;
    else
        // Confirm transmitted data or cntl
        lld_data_tx_check(evt, msg);
    #endif // BLE_CENTRAL || BLE_PERIPHERAL

    // Send the message
    ke_msg_send(msg);

}


__INLINE void pol_spi(void)
{
    do{
    }while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);  	// polling to wait for spi have data
    SetWord16(SPI_CLEAR_INT_REG, 1);   					// clear pending flag	
}

void spi_write_rx_data_back_2_pxi (uint8* rx_buf_address, uint8 len)
{
    SetBits16(SPI_CTRL_REG,SPI_WORD,1);  			// set to 16bit mode

    SetBits16(SPI_CTRL_REG,SPI_ON,1);    	  			// enable SPI block
    SetBits16(SPI_CTRL_REG,SPI_CLK,2);    	  			// fastest clock

//	SetWord16(P1_SET_DATA_REG, 0x01); //SET P1.0  USED AS CS   

	//transmit first 36 bytes
	uint16* spi_buf = (uint16*)rx_buf_address;

// 	for(uint8 i=0;i<18;i++)
// 	{
// 		SetWord16(SPI_RX_TX_REG0, spi_buf[i]);
// 		pol_spi();
// 	}
	SetWord16(SPI_RX_TX_REG0, spi_buf[0]);
	pol_spi();
	SetWord16(SPI_RX_TX_REG0, spi_buf[1]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[2]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[3]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[4]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[5]);
	pol_spi();
	SetWord16(SPI_RX_TX_REG0, spi_buf[6]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[7]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[8]);
	pol_spi();		
	SetWord16(SPI_RX_TX_REG0, spi_buf[9]);
	pol_spi();
	SetWord16(SPI_RX_TX_REG0, spi_buf[10]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[11]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[12]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[13]);
	pol_spi();		
	SetWord16(SPI_RX_TX_REG0, spi_buf[14]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[15]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[16]);
	pol_spi();	
	SetWord16(SPI_RX_TX_REG0, spi_buf[17]);
	pol_spi();
	
	//transmit last byte (number 37)
    SetBits16(SPI_CTRL_REG,SPI_WORD,0);  			// set to 8bit mode
    SetWord16(SPI_RX_TX_REG0, 0x00FF&spi_buf[18]);
    pol_spi();

	//transmit RF_RSSI_RESULT_REG
	SetBits16(SPI_CTRL_REG,SPI_WORD,1);  			// set to 16bit mode
	SetWord16(SPI_RX_TX_REG0, GetWord16(RF_RSSI_RESULT_REG));
    pol_spi();

	//transmit RXRSSI OF RXDESCRIPTOR (8BITS)
	uint8* spi_buf_1 = (uint8*)(rx_buf_address-2); //SO POINTS TO RXRSSI[7:0]
	SetBits16(SPI_CTRL_REG,SPI_WORD,0);  			// set to 8bit mode
    SetWord16(SPI_RX_TX_REG0, 0x00FF&spi_buf_1[0]);
    pol_spi();
	
	//transmit AGC and AFC
	SetBits16(SPI_CTRL_REG,SPI_WORD,1);  			// set to 16bit mode
	SetWord16(SPI_RX_TX_REG0, GetWord16(RF_AGC_RESULT_REG));
	pol_spi();
	
	SetWord16(SPI_RX_TX_REG0, GetWord16(RF_DC_OFFSET_RESULT_REG));
	pol_spi();

    
//	SetWord16(P1_RESET_DATA_REG, 0x01); //RESET P2.0 USED AS cs

    SetBits16(SPI_CTRL_REG,SPI_ON,0);    	  			// disable SPI block
 
}


/// @} LLDDATA
