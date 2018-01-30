/**
 ****************************************************************************************
 *
 * @file wlan_coex.c
 *
 * @brief WLAN coexistence API. 
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

#include "lld.h"
#include "lld_evt.h"
#include "ke_event.h"
#include "em_map_ble.h"
#include "reg_ble_em_et.h"
#include "reg_ble_em_tx.h"
#include "reg_ble_em_cs.h"

#if WLAN_COEX_ENABLED 

/* The following must be defined to configure WLAN_COEX
 * #define WLAN_COEX_BLE_EVENT  6 or 7
 * #define WLAN_COEX_PORT      0-3
 * #define WLAN_COEX_PIN       depends on port
 * #define WLAN_COEX_IRQ       0-4
 
 * #define WLAN_COEX_PORT_2    0-3 if this is defined the second WEP will also be setup and used
 * #define WLAN_COEX_PIN_2     depends on port
 * #define WLAN_COEX_IRQ_2     0-4

 * #define WLAN_COEX_PRIO_PORT      0-3
 * #define WLAN_COEX_PRIO_PIN       depends on port

 * #define WLAN_COEX_DEBUG      //will setup and use P2_7 to show radio on/off, P2_8 for event end and P2_9 for event schedule
*/


#if WLAN_COEX_BLE_EVENT != 0 && WLAN_COEX_BLE_EVENT != 1 && WLAN_COEX_BLE_EVENT != 6 && WLAN_COEX_BLE_EVENT != 7
#error "WLAN_COEX_BLE_EVENT must be either 0, 1, 6 or 7"
#endif

#if WLAN_COEX_PORT > 3
#error "WLAN_COEX_PORT must be 0-3"
#endif

#if WLAN_COEX_PIN > 5
#if WLAN_COEX_PORT == 0 && WLAN_COEX_PIN > 7
#error "WLAN_COEX_PORT must be 0-7"
#endif
#if WLAN_COEX_PORT == 1
#error "WLAN_COEX_PORT must be 0-5"
#endif
#if WLAN_COEX_PORT == 2 && WLAN_COEX_PIN > 9
#error "WLAN_COEX_PORT must be 0-9"
#endif
#if WLAN_COEX_PORT == 3 && WLAN_COEX_PIN > 7
#error "WLAN_COEX_PORT must be 0-7"
#endif
#endif

#if WLAN_COEX_IRQ > 4
#error "WLAN_COEX_IRQ must be 0-4"
#endif

#ifdef WLAN_COEX_PORT_2
#if WLAN_COEX_PORT_2 > 3
#error "WLAN_COEX_PORT_2 must be 0-3"
#endif

#if WLAN_COEX_PIN_2 > 5
#if WLAN_COEX_PORT_2 == 0 && WLAN_COEX_PIN_2 > 7
#error "WLAN_COEX_PORT_2 must be 0-7"
#endif
#if WLAN_COEX_PORT_2 == 1
#error "WLAN_COEX_PORT_2 must be 0-5"
#endif
#if WLAN_COEX_PORT_2 == 2 && WLAN_COEX_PIN_2 > 9
#error "WLAN_COEX_PORT_2 must be 0-9"
#endif
#if WLAN_COEX_PORT_2 == 3 && WLAN_COEX_PIN_2 > 7
#error "WLAN_COEX_PORT_2 must be 0-7"
#endif
#endif

#if WLAN_COEX_IRQ_2 > 4
#error "WLAN_COEX_IRQ_2 must be 0-4"
#endif

#if WLAN_COEX_IRQ_1 == WLAN_COEX_IRQ_2
#error "WLAN_COEX_IRQ_1 must be different than WLAN_COEX_IRQ_2"
#endif
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "wlan_coex.h" 
#include "gpio.h"

uint32_t wlan_coex_criteria[BLE_CONNECTION_MAX_USER+1] __attribute__((section("retention_mem_area0")));


#if DEVELOPMENT_DEBUG
/**
 ****************************************************************************************
 * @brief Reserves the GPIOs used by the WLAN coexistence module.
 *
 * @return void 
 ****************************************************************************************
*/
void wlan_coex_reservations(void)
{
    RESERVE_GPIO( WLANCOEXGPIO_0, GPIO_PORT_0, WLAN_COEX_BLE_EVENT, PID_BLE_DIAG);
    RESERVE_GPIO( WLANCOEXGPIO_1, WLAN_COEX_PRIO_PORT,  WLAN_COEX_PRIO_PIN, PID_GPIO);
    RESERVE_GPIO( WLANCOEXGPIO_2, WLAN_COEX_PORT,  WLAN_COEX_PIN, PID_GPIO);
#ifdef WLAN_COEX_PORT_2
    RESERVE_GPIO( WLANCOEXGPIO_3, WLAN_COEX_PORT_2,  WLAN_COEX_PIN_2, PID_GPIO);
#endif
}

#endif // DEVELOPMENT_DEBUG


/*
 * Name         : wlan_coex_init - Initialise and enable WLAN coexistence
 * Arguments    : none
 * Description  : Call the function to configure and enable WLAN coexistence.
 *
 * Returns      : void
 *
 */
void wlan_coex_init()
{
#if WLAN_COEX_DEBUG    
    SetWord16(P27_MODE_REG, 0x300);
    SetWord16(P28_MODE_REG, 0x300);
    SetWord16(P29_MODE_REG, 0x300);
#endif
    
    wlan_coex_enable();
}

/*
 * Name         : wlan_coex_prio_criteria_add - Add priority criteria to WLAN coexistence
 * Arguments    : none
 * Description  : Call the function to add a priority criteria for WLAN coexistence.
 *
 * Returns      : void
 *
 */
void wlan_coex_prio_criteria_add(uint16_t type, uint16_t conhdl, uint16_t missed)
{
    if(conhdl > BLE_CONNECTION_MAX_USER)  //if conhdl is higher than supported connections
        conhdl = BLE_CONNECTION_MAX_USER;   //assume it is general criteria
    
    wlan_coex_criteria[conhdl] |= type;
    if(type == BLEMPRIO_MISSED)
        wlan_coex_criteria[conhdl] |= missed << 16;
}

/*
 * Name         : wlan_coex_prio_criteria_del - Delete priority criteria to WLAN coexistence
 * Arguments    : none
 * Description  : Call the function to delete a priority criteria for WLAN coexistence.
 *
 * Returns      : void
 *
 */
void wlan_coex_prio_criteria_del(uint16_t type, uint16_t conhdl, uint16_t missed)
{
    if(conhdl > BLE_CONNECTION_MAX_USER)  //if conhdl is higher than supported connections
        conhdl = BLE_CONNECTION_MAX_USER;   //assume it is general criteria
    
    wlan_coex_criteria[conhdl] &= ~type;
    if(type == BLEMPRIO_MISSED)
        wlan_coex_criteria[conhdl] &= 0x00FF;
}


// forward declaration of local functions
void wlan_coex_eip_1_handler(void);
#ifdef WLAN_COEX_PORT_2
void wlan_coex_eip_2_handler(void);
#endif

/*
 * Name         : wlan_coex_enable - Configure and enable WLAN coexistence
 * Arguments    : none
 * Description  : Call the function to configure and enable WLAN coexistence.
 *
 * Returns      : void
 *
 */
void wlan_coex_enable()
{
    //enable diagnostic port for BLE event_in_process
#if WLAN_COEX_BLE_EVENT == 0
    SetBits32(BLE_DIAGCNTL_REG, DIAG2, 0x08);
    SetBits32(BLE_CNTL2_REG, DIAGPORT_REVERSE, 1);
#endif
#if WLAN_COEX_BLE_EVENT == 1
    SetBits32(BLE_DIAGCNTL_REG, DIAG2, 0x1F);
    SetBits32(BLE_CNTL2_REG, DIAGPORT_REVERSE, 1);
#endif
#if WLAN_COEX_BLE_EVENT == 6
    SetBits32(BLE_DIAGCNTL_REG, DIAG2, 0x1F);
#endif
#if WLAN_COEX_BLE_EVENT == 7
    SetBits32(BLE_DIAGCNTL_REG, DIAG2, 0x08);
#endif    
    SetBits32(BLE_DIAGCNTL_REG, DIAG2_EN, 1);  
    SetWord16(P00_MODE_REG+WLAN_COEX_BLE_EVENT*2,0x312);
    
    //enable BLE priority signal
    GPIO_ConfigurePin( (GPIO_PORT)WLAN_COEX_PRIO_PORT, (GPIO_PIN)WLAN_COEX_PRIO_PIN, OUTPUT, PID_GPIO, false );
    
    //enable WLAN incoming signal
    GPIO_ConfigurePin( (GPIO_PORT)WLAN_COEX_PORT, (GPIO_PIN)WLAN_COEX_PIN, INPUT, PID_GPIO, false );
    
#ifdef WLAN_COEX_PORT_2
    //enable WLAN 2 incoming signal
    GPIO_ConfigurePin( (GPIO_PORT)WLAN_COEX_PORT_2, (GPIO_PIN)WLAN_COEX_PIN_2, INPUT, PID_GPIO, false );
#endif


    wlan_coex_check_signals();

    // enable WLAN interrupt
    GPIO_RegisterCallback((IRQn_Type)(GPIO0_IRQn+WLAN_COEX_IRQ), wlan_coex_eip_1_handler);
    GPIO_EnableIRQ( (GPIO_PORT)WLAN_COEX_PORT, (GPIO_PIN)WLAN_COEX_PIN, (IRQn_Type)(GPIO0_IRQn+WLAN_COEX_IRQ), /*low_input*/ false, /*release_wait*/ false, /*debounce_ms*/ 0);
    
#ifdef WLAN_COEX_PORT_2
    // enable WLAN 2 interrupt
    GPIO_RegisterCallback((IRQn_Type)(GPIO0_IRQn + WLAN_COEX_IRQ_2), wlan_coex_eip_2_handler);
    GPIO_EnableIRQ( (GPIO_PORT)WLAN_COEX_PORT_2, (GPIO_PIN)WLAN_COEX_PIN_2, (IRQn_Type)(GPIO0_IRQn+WLAN_COEX_IRQ_2), /*low_input*/ false, /*release_wait*/ false, /*debounce_ms*/ 0);
#endif
}

/**
 ****************************************************************************************
 * @brief GPIO IRQ handler for WLAN EIP 1 
 ****************************************************************************************
 */
static void wlan_coex_eip_1_handler(void)
{
    IRQn_Type irq = (IRQn_Type) (GPIO0_IRQn + WLAN_COEX_IRQ);

    wlan_coex_check_signals();
    
    //Check current polarity and change accordingly
    if(GPIO_IRQ_INPUT_LEVEL_HIGH == GPIO_GetIRQInputLevel(irq))
    {
        //Change polarity to get an interrupt when WIFI event_in_process falls
        GPIO_SetIRQInputLevel(irq, GPIO_IRQ_INPUT_LEVEL_LOW);
    }
    else
    {
        //Change polarity to get an interrupt when WIFI event_in_process rises
        GPIO_SetIRQInputLevel(irq, GPIO_IRQ_INPUT_LEVEL_HIGH);
    }
}

#ifdef WLAN_COEX_PORT_2
/**
 ****************************************************************************************
 * @brief GPIO IRQ handler for WLAN EIP 2
 ****************************************************************************************
 */
static void wlan_coex_eip_2_handler(void)
{
    IRQn_Type irq = (IRQn_Type) (GPIO0_IRQn + WLAN_COEX_IRQ_2);
    
    wlan_coex_check_signals();
    
    //Check current polarity and change accordingly
    if(GPIO_IRQ_INPUT_LEVEL_HIGH == GPIO_GetIRQInputLevel(irq))
    {
        //Change polarity to get an interrupt when WIFI event_in_process falls
        GPIO_SetIRQInputLevel(irq, GPIO_IRQ_INPUT_LEVEL_LOW);
    }
    else
    {
        //Change polarity to get an interrupt when WIFI event_in_process rises
        GPIO_SetIRQInputLevel(irq, GPIO_IRQ_INPUT_LEVEL_HIGH);
    }
}
#endif

#include "gapm.h"

void wlan_coex_check_signals(void)
{
    uint32 radio_on_off = 0;

    GLOBAL_INT_STOP();
    
    //check BLE_PRIORITY current level
    if( 0 == GPIO_GetPinStatus(WLAN_COEX_PRIO_PORT, WLAN_COEX_PRIO_PIN) )
    {
        //if not BLE_PRIORITY then check WEPS
        
        //check current level
        if( 1 == GPIO_GetPinStatus(WLAN_COEX_PORT, WLAN_COEX_PIN) )
        {
            //turn off radio
            radio_on_off = 1;
        }
        else
        {
            //turn on radio
            radio_on_off = 0;
        }

#ifdef WLAN_COEX_PORT_2    
        if(radio_on_off == 0)
        {
            //check current level of 2nd WEP
            if( 1 == GPIO_GetPinStatus(WLAN_COEX_PORT_2, WLAN_COEX_PIN_2) )
            {
                //turn off radio
                radio_on_off = 1;
            }
            else
            {
                //turn on radio
                radio_on_off = 0;
            }
        }
#endif
    }
    
    //turn radio on/off
    if(gapm_env.role == GAP_PERIPHERAL_SLV) //turn of radio RX to get missed criteria
        SetBits16((0x50002020), (0x0004), radio_on_off);
    SetBits16((0x50002020), (0x0001), radio_on_off);

    GLOBAL_INT_START();
    
#if WLAN_COEX_DEBUG        
    if(radio_on_off)
        SetWord16(P2_RESET_DATA_REG, 1<<7);
    else
        SetWord16(P2_SET_DATA_REG, 1<<7);    
#endif    
}

void lld_evt_init_func(bool reset);

bool wlan_coex_lld_evt_start;

void wlan_coex_prio_level(int level)
{
    if(level)
        GPIO_SetActive((GPIO_PORT)WLAN_COEX_PRIO_PORT, (GPIO_PIN)WLAN_COEX_PRIO_PIN);
    else
        GPIO_SetInactive((GPIO_PORT)WLAN_COEX_PRIO_PORT, (GPIO_PIN)WLAN_COEX_PRIO_PIN);


    wlan_coex_check_signals();
}

int wlan_coex_evt_prio(struct lld_evt_tag *evt)
{
    uint32_t prio = wlan_coex_criteria[evt->conhdl];
    
    if(evt->conhdl == LLD_ADV_HDL)  //general criteria
    {
        uint32_t chk = BLEMPRIO_SCAN;
        while(chk <= BLEMPRIO_CONREQ)
        {
            if(prio & chk)
            {
                switch (chk)
                {
                    case BLEMPRIO_SCAN:     //scan priority
                        if(evt->restart_pol == LLD_SCN_RESTART) //event is scan
                            return 1;
                        break;
                    case BLEMPRIO_ADV:     //advertise priority
                        if(evt->restart_pol == LLD_ADV_RESTART) //event is advertise
                            return 1;
                        break;
                    case BLEMPRIO_CONREQ:     //connection request priority
                        if(evt->restart_pol == LLD_SCN_RESTART) //event is scan
                        {
                            if(ble_cntl_get(LLD_ADV_HDL) == LLD_INITIATING) //connection request
                                return 1;
                        }
                }
            }
            chk = chk << 1;
        }
    }
    else    //per connection criteria
    {
        uint32_t chk = BLEMPRIO_LLCP;
        while(chk <= BLEMPRIO_MISSED)
        {
            if(prio & chk)
            {
                switch (chk)
                {
                    case BLEMPRIO_LLCP:
                    {
                        struct co_list *rdy = &evt->tx_rdy;
                        struct co_list *prog = &evt->tx_prog;

                        //check criteria
                        if (!co_list_is_empty(rdy))
                            if(ble_txllid_getf(((struct co_buf_tx_node *)rdy->first)->idx) == BLE_TXLLID_MASK)
                                return 1;

                        if (!co_list_is_empty(prog))
                            if(ble_txllid_getf(((struct co_buf_tx_node *)prog->first)->idx) == BLE_TXLLID_MASK)
                                return 1;
                    }
                    break;
                    case BLEMPRIO_DATA:
                        return 1;
                    case BLEMPRIO_MISSED:
                        if(evt->missed_cnt >= prio >> 16)
                            return 1;
                        break;
                }
            }
            chk = chk << 1;
        }
    }
    
    return 0;
}

void wlan_coex_evt_prio_set()
{
    struct lld_evt_tag *evt = (struct lld_evt_tag *)co_list_pick(&lld_evt_env.evt_prog);
    uint32_t curr_time = (lld_evt_time_get() + LLD_EVT_PROG_LATENCY + 1) & BLE_BASETIMECNT_MASK;
    uint8_t status;
    int em_idx;

    if(evt == NULL)
    {
        wlan_coex_prio_level(0);
        return;
    }
    
    // Check if event has been handled by reading the exchange table status
    em_idx = evt->time & 0x0F;
    status = ble_etstatus_getf(em_idx);
    if (status == EM_BLE_ET_PROCESSED)
    {
        wlan_coex_prio_level(0);
        return;
    }
    
    if(wlan_coex_evt_prio(evt))
    {
        wlan_coex_prio_level(1);
    }
    else
    {
        wlan_coex_prio_level(0);
    }
}

void wlan_coex_lld_evt_schedule(void)
{
#if WLAN_COEX_DEBUG    
    SetWord16(P2_SET_DATA_REG, 1<<9);
#endif    
    lld_evt_schedule();     //call rom func
    if(wlan_coex_lld_evt_start)
        wlan_coex_evt_prio_set();
    wlan_coex_lld_evt_start = 0;
#if WLAN_COEX_DEBUG    
    SetWord16(P2_RESET_DATA_REG, 1<<9);
#endif    
}

void wlan_coex_lld_evt_end(void)
{
#if WLAN_COEX_DEBUG
    SetWord16(P2_SET_DATA_REG, 1<<8);
#endif
    
    lld_evt_end();     //call rom func

    //wlan_coex_evt_prio_set();
    wlan_coex_prio_level(0);
    
#if WLAN_COEX_DEBUG
    SetWord16(P2_RESET_DATA_REG, 1<<8);
#endif
}

void wlan_coex_lld_evt_init_func(bool reset)
{
    lld_evt_init_func(reset);

#if WLAN_COEX_ENABLED 
    // Register BLE start event kernel event
    ke_event_callback_set(KE_EVENT_BLE_EVT_START, &wlan_coex_lld_evt_schedule);
    // Register BLE end event kernel event
    ke_event_callback_set(KE_EVENT_BLE_EVT_END,   &wlan_coex_lld_evt_end);    
#endif
}

#endif // WLAN_COEX_ENABLED 
