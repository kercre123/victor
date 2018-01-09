/**
 ****************************************************************************************
 *
 * @file llm_util.h
 *
 * @brief  Link layer manager utilities definitions
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef LLM_UTIL_H_
#define LLM_UTIL_H_

/**
 ****************************************************************************************
 * @addtogroup LLMUTIL
 * @ingroup LLM
 * @brief Link layer manager utilities definitions
 *
 * full description
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <string.h>
#include "compiler.h"
#include "co_buf.h"
#include "co_endian.h"
#include "co_error.h"
#include "reg_ble_em_rx.h"
#include "reg_ble_em_tx.h"
#include "reg_blecore.h"
#include "llm.h"


/*
 * DEFINES
 ****************************************************************************************
 */
///Constant nibble to use as top 4 MSBs, to have at least 2 transitions
extern const uint8_t LLM_AA_CT1[3];

///Constant nibble to use in AA to get more than 1 bit different from Advertising AA
extern const uint8_t LLM_AA_CT2[2];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Checks if the channel map contains at least 1 channel
 *
 * This function checks if advertising or data channel map contains at least 1 channel.
 *
 * @param[in] channel_map        Pointer on the channel map to be checked.
 * @param[in] nb_octet           number of octet(s) to be parsed.
 *
 * @return If the channel map is correct or not.
 *
 ****************************************************************************************
 */
uint8_t llm_util_check_map_validity(uint8_t *channel_map, uint8_t nb_octet);

/**
 ****************************************************************************************
 * @brief Checks if the device address is available in the white list.
 *
 * This function checks if the bd address if present in the WL and return the status
 * and position.
 *
 * @param[in] bd_address         Pointer on the device address to be checked.
 * @param[in] bd_address_type    Type of the address ..
 * @param[in/out] position       Position in the White List.
 *
 * @return If the device address has been found or not.
 *
 ****************************************************************************************
 */
bool llm_util_bd_addr_in_wl(struct bd_addr const *bd_address, uint8_t bd_addr_type ,
                            uint16_t *position);

/**
 ****************************************************************************************
 * @brief Checks the validity of the device address
 *
 * This function checks if the address type and value are available, no NULL, available
 * in the WL, not the local one.
 *
 * @param[in] bd_address           Pointer on the device address to be checked.
 * @param[in] addr_type            Type of the device address to be checked.
 *
 * @return If the device address is correct or not.
 *
 ****************************************************************************************
 */
uint8_t llm_util_check_address_validity( struct bd_addr *bd_address, uint8_t addr_type);

/**
 ****************************************************************************************
 * @brief Apply the BD address
 *
 * This function applies the BD address corresponding to the current address type on the
 * device.
 *
 * @param[in] addr_type           Type of the requested address
 *
 ****************************************************************************************
 */
void llm_util_apply_bd_addr(uint8_t addr_type);

/**
 ****************************************************************************************
 * @brief Set the public address
 *
 * @param[in]  bd_addr           Public address
 ****************************************************************************************
 */
void llm_util_set_public_addr(struct bd_addr *bd_addr);

#if (BLE_CENTRAL || BLE_OBSERVER)
/**
 ****************************************************************************************
 * @brief Clears the advertising report list
 *
 * This function removes the bd address and advertising type set in the list.
 *
 ****************************************************************************************
 */
void llm_util_clear_adv_report(void);

/**
 ****************************************************************************************
 * @brief Check if the advertiser address has been already received.
 *
 * This function checks in case where the filtering is enabled if the address is already
 * listed.
 *
 * @param[in] adv_bd_addr         Pointer on the device address to be checked.
 * @param[in] adv_type             Type of the advertising report (ADV_IND, ADV., SCANRSP)
 *
 * @return If the device address has been found or not.
 *
 ****************************************************************************************
 */
__INLINE bool llm_util_check_adv_report_list(struct bd_addr * adv_bd_addr, uint8_t adv_type)
{
    // checks if the bd address is not known in the list
    for (uint8_t idx = 0; idx < BLE_DUPLICATE_FILTER_MAX; idx++)
    {
#ifndef __DA14581__
        if(llm_le_env_lpus.adv_list[idx].adv_type == 0x00)
        {
            // saves the bd address in the adv filter list
            memcpy( &llm_le_env_lpus.adv_list[idx].adv_addr.addr[0],&adv_bd_addr->addr[0],
                    BD_ADDR_LEN);
            // save the type of the device address
            llm_le_env_lpus.adv_list[idx].adv_type |= (1 << adv_type);
            return true;
        }
        if (co_bdaddr_compare (adv_bd_addr, &llm_le_env_lpus.adv_list[idx].adv_addr))
        {
            if (llm_le_env_lpus.adv_list[idx].adv_type & (1 << adv_type))
                return false;
            else
            {
                llm_le_env_lpus.adv_list[idx].adv_type |= (1 << adv_type);
                return true;
            }
        }
#else
        if(llm_le_env.adv_list[idx].adv_type == 0x00)
        {
            // saves the bd address in the adv filter list
            memcpy( &llm_le_env.adv_list[idx].adv_addr.addr[0],&adv_bd_addr->addr[0],
                    BD_ADDR_LEN);
            // save the type of the device address
            llm_le_env.adv_list[idx].adv_type |= (1 << adv_type);
            return true;
        }
        if (co_bdaddr_compare (adv_bd_addr, &llm_le_env.adv_list[idx].adv_addr))
        {
            if (llm_le_env.adv_list[idx].adv_type & (1 << adv_type))
                return false;
            else
            {
                llm_le_env.adv_list[idx].adv_type |= (1 << adv_type);
                return true;
            }
        }
#endif
    }
    return true;
}
#endif // BLE_CENTRAL || BLE_OBSERVER

/**
 ****************************************************************************************
 * @brief Gets the size of the packet received.
 *
 * This function gets the length of the packet received.
 *
 * @param[in] rxdesc         Pointer on the reception descriptor.
 *
 * @return The length of the data.
 *
 ****************************************************************************************
 */
__INLINE uint8_t llm_util_rxlen_getf(struct co_buf_rx_desc *rxdesc)
{
    uint16_t localVal =  rxdesc->rxheader;
    return ((localVal & BLE_RXADVLEN_MASK) >> BLE_RXADVLEN_LSB);
}

/**
 ****************************************************************************************
 * @brief Gets the type of the packet received.
 *
 * This function gets the type of the packet received.
 *
 * @param[in] rxdesc         Pointer on the reception descriptor.
 *
 * @return The type of the packet.
 *
 ****************************************************************************************
 */
__INLINE uint16_t llm_util_rxtype_getf(struct co_buf_rx_desc *rxdesc)
{
    uint16_t localVal =  rxdesc->rxheader;
    return ((localVal & BLE_RXTYPE_MASK) >> BLE_RXTYPE_LSB);
}

/**
 ****************************************************************************************
 * @brief Gets the type of advertiser address.
 *
 * This function gets the the advertiser address type of the packet received.
 *
 * @param[in] rxdesc         Pointer on the reception descriptor.
 *
 * @return The type of the address.
 *
 ****************************************************************************************
 */
__INLINE uint8_t llm_util_rxtxadd_getf(struct co_buf_rx_desc *rxdesc)
{
    uint16_t localVal =  rxdesc->rxheader;
    return ((localVal & BLE_RXTXADD_BIT) >> BLE_RXTXADD_POS);
}
/**
****************************************************************************************
* @brief LLM function generating the Access Address for a new Link.
*
* The function will generate the following 4 bytes:
* |Byte 3           | Byte 2           |  Byte 1            | Byte 0            |
* |CT1 | PRAND_4MSB | CT2 | PRAND_4LSB | CT2 | INCRAND_4MSB | CT2 | INCRAND_4LSB|
*
* For each new generated AA,
*  - PRAND is a pseudo random 1 byte number.
*  - INCRAND is an initially generated random number, increased by 1 for each AA
*  - CT2 is toggled between the two values
*  - CT1 is again toggled between the 3 possible values
*
* Considering the formats of CT1 and CT2, the rules for AA generation are respected:
*  - since CT2 is put between bytes, and has two equal bits on each half,
*    it ensures no more than 6 consecutive bits inside the address. CT1 also.
*
*  - CT2 and CT1 ensure that the newly generated address is different and at least 1 bit
*    different from the advertising access address
*
*  - since CT1 != CT2 , the four bytes of the AA can never be equal
*
*  - Since in a 32 bit number there are 31 transitions, and the neighboring equal bits
*    in CT1 and CT2 represent 7 transitions, there can never be more than 31-7=24
*    transitions in the AA
*
*  - The format of CT1 ensures at least two transitions in the most significant 6 bits.
*
* @param acc_addr Pointer to the 4 byte array space for the AA.
*
*
*****************************************************************************************
*/
__INLINE void llm_util_aa_gen(uint8_t *acc_addr)
{
    //the pseudo random byte is generated every time - use a seed?
    uint8_t prand = co_rand_byte();

    //Create the AA - MSB to LSB

    acc_addr[0] = (LLM_AA_CT2[llm_le_env.aa.ct2_idx] << 4 ) | (llm_le_env.aa.intrand & 0x0F);
    acc_addr[1] = (LLM_AA_CT2[llm_le_env.aa.ct2_idx] << 4 ) | (llm_le_env.aa.intrand >> 4);
    acc_addr[2] = (LLM_AA_CT2[llm_le_env.aa.ct2_idx] << 4 ) | (prand & 0x0F);
    acc_addr[3] = (LLM_AA_CT1[llm_le_env.aa.ct1_idx] << 4 ) | (prand >> 4);

    //Toggle indexes for next AA
    //Toggle CT2 index  - only possible values are 0 and 1
    //llm_le_env.aa.ct2_idx = (llm_le_env.aa.ct2_idx + 1) %2 ;

    //Toggle CT1 only when CT2 back at 0
    //if (llm_le_env.aa.ct2_idx == 1)
    //{
        llm_le_env.aa.ct1_idx = (llm_le_env.aa.ct1_idx + 1) % 3;
    //}

    //Increase random
    llm_le_env.aa.intrand ++;

}

/**
 ****************************************************************************************
 * @brief Fulfills the payload for the transmit test mode.
 *
 * This function fulfills the payload for the transmit test mode.
 *
 * @param[in] pattern_type         type of the pattern.
 * @param[in] payload_len          length of the payload.
 *
 ****************************************************************************************
 */
__INLINE void llm_util_gen_pattern(uint8_t pattern_type, uint8_t payload_len ,
        uint8_t *payload)
{
    uint8_t pattern = 0;
    // get the pattern
    switch(pattern_type)
    {
        case PAYL_11110000:
            pattern = 0xF0;
            break;
        case PAYL_10101010:
            pattern = 0xAA;
            break;
        case PAYL_ALL_1:
            pattern = 0xFF;
            break;
        case PAYL_ALL_0:
            pattern = 0x00;
            break;
        case PAYL_00001111:
            pattern = 0x0F;
            break;
        case PAYL_01010101:
            pattern = 0x55;
            break;
        default:
            ASSERT_ERR(pattern_type < PAYL_END);
            break;
    }
    // fulfill the payload
    memset(payload, pattern, payload_len);

}

/**
 ****************************************************************************************
 * @brief Set the default scan parameters values
 *
 * This function set the default values for the scan parameters
 *
 ****************************************************************************************
 */
__INLINE void llm_util_set_param_scan_dft(void)
{
    // disable filtering
    llm_le_env.scanning_params.filterpolicy = SCAN_ALLOW_ADV_ALL;
    // set the scan interval to 10ms
    llm_le_env.scanning_params.interval = LLM_LE_SCAN_INTERV_DFLT;
    // set the scan interval to scan_interv/2
    llm_le_env.scanning_params.window = LLM_LE_SCAN_INTERV_DFLT/2;
    // set the scan mode to passive (no transmission)
    llm_le_env.scanning_params.type = SCAN_PASSIVE;
    // disable the duplicate filtering
    llm_le_env.scanning_params.filter_duplicate = SCAN_FILT_DUPLIC_DIS;
}

/**
 ****************************************************************************************
 * @brief Set the default advertising parameters values
 *
 * This function set the default values for the advertising parameters
 *
 ****************************************************************************************
 */
#if (BLE_PERIPHERAL || BLE_BROADCASTER)
__INLINE void llm_util_set_param_adv_dft(void)
{
    struct co_buf_tx_desc *txdesc;
    // disable filtering
    llm_le_env.advertising_params.filterpolicy = ADV_ALLOW_SCAN_ANY_CON_ANY;
    // set the advertising type to connectable undirected
    llm_le_env.advertising_params.type = ADV_CONN_UNDIR;
    // set the interval max to 1.28s
    llm_le_env.advertising_params.intervalmax = LLM_LE_ADV_INTERV_DFLT;
    // set the interval min to 1.28s
    llm_le_env.advertising_params.intervalmin = LLM_LE_ADV_INTERV_DFLT;
    // set all adv channels available
    llm_le_env.advertising_params.channelmap = LLM_LE_ADV_CH_MAP_DFLT;
    // reset data length
    llm_le_env.advertising_params.datalen = 0;
    // reset scan response length
    llm_le_env.advertising_params.scanrsplen = 0;
#ifdef __DA14581__
    llm_le_env.advertising_params.adv_data_req = (struct ke_msg*)NULL;
#endif

    // Set the adv connectable undirected
    txdesc = co_buf_tx_desc_get(LLM_LE_ADV_IDX);
    // sets the type of advertising, the length(only AdvA len = 6 bytes)
    txdesc->txheader =(uint16_t)(
                      ((llm_le_env.advertising_params.type << BLE_TXTYPE_LSB) & BLE_TXTYPE_MASK)
                                             | ((0x6 << BLE_TXADVLEN_LSB) & BLE_TXADVLEN_MASK));
    // sets the scan response
    txdesc = co_buf_tx_desc_get(LLM_LE_SCAN_RSP_IDX);
    txdesc->txheader =(uint16_t)(((LLM_SCAN_RSP << BLE_TXTYPE_LSB) & BLE_TXTYPE_MASK)
                                             | ((6 << BLE_TXADVLEN_LSB) & BLE_TXADVLEN_MASK));
}
#endif // BLE_PERIPHERAL || BLE_BROADCASTER

/**
 ****************************************************************************************
 * @brief Chek the status of the test mode
 *
 * This function sends an event to the host when the TX or RX test mode is finished
 *
 ****************************************************************************************
 */
__INLINE void llm_util_chk_tst_mode(void)
{
    //if the current state is not IDLE
    if((llm_le_env.test_mode.directtesttype != TEST_END)&&(llm_le_env.test_mode.end_of_tst==true))
    {

        // structure type for the complete command event
        struct llm_test_end_cmd_complete *event;

        // allocate the complete event message
        event = KE_MSG_ALLOC(LLM_LE_TEST_END_CMP_EVT, HOST_TESTER_TASK, TASK_LLM, llm_test_end_cmd_complete);

        llm_le_env.test_mode.end_of_tst=false;
        // enable the whitening
        ble_whit_dsb_setf(0);

        // Number_Of_Packets for a transmitter test is reported as NULL.
        if (llm_le_env.test_mode.directtesttype == TEST_TX)
        {
            event->nb_packet_received = 0;
        }
        else
        {
            event->nb_packet_received = llm_le_env.test_mode.nb_pkt_rcv;
            llm_le_env.test_mode.nb_pkt_rcv = 0;
        }
        // set the env variable,
        llm_le_env.test_mode.directtesttype = TEST_END;
        // update the status
        event->status = CO_ERROR_NO_ERROR;
        // send the message
        ke_msg_send(event);
    }
}

/**
 ****************************************************************************************
 * @brief Get the device role
 *
 * @return  Device role
 ****************************************************************************************
 */
uint8_t llm_util_get_role(void);

/**
 ****************************************************************************************
 * @brief Set the device role
 *
 * @param[in]  Device role
 ****************************************************************************************
 */
void llm_util_set_role(uint8_t role);

/**
 ****************************************************************************************
 * @brief Check the event mask
 *
 * @param[in]     event_id     event to check
 *
 * @return     True: event can be sent / False: event is masked
 ****************************************************************************************
 */
bool llm_util_check_evt_mask(uint8_t event_id);

/**
 ****************************************************************************************
 * @brief Get the channel map
 *
 * @param[out] map    Pointer to channel map buffer
 ****************************************************************************************
 */
void llm_util_get_channel_map(struct le_chnl_map *map);

/**
 ****************************************************************************************
 * @brief Get supported features
 *
 * @param[out] feats    Pointer to supported features buffer
 ****************************************************************************************
 */
void llm_util_get_supp_features(struct le_features *feats);

/// @} LLM

#endif // LLM_UTIL_H_
