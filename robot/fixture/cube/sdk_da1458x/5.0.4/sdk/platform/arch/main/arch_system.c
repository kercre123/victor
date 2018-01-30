/**
 ****************************************************************************************
 *
 * @file arch_system.c
 *
 * @brief System setup.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


/*
 * INCLUDES
 ****************************************************************************************
 */

#include "arch.h"
#include "arch_api.h"
#include "user_callback_config.h"
#include <stdlib.h>
#include <stddef.h>    // standard definitions
#include <stdint.h>    // standard integer definition
#include <stdbool.h>   // boolean definition
#include "rwip.h"     // BLE initialization
#include "llc.h"
#include "pll_vcocal_lut.h"
#include "gpio.h"
#include "rf_580.h"
#include "gtl_env.h"
#include "gtl_task.h"
#if HCI_OVER_SPI_ENABLED
#include "spi_hci.h"
#if HCI_OVER_UART_ENABLED
#include "user_periph_setup.h"
#endif
#endif

#include "hcic.h"

#if (USE_TRNG)
#include "trng.h"       // True random number generator API
#endif

#include "arch_wdg.h"
#include "arch_system.h"
#include "nvds.h"
#include "user_periph_setup.h"
#include "arch_patch.h"
#include "app.h"
#include "rwble.h"

#include "uart.h"
#include "reg_uart.h"   // uart register

/*
 * DEFINES
 ****************************************************************************************
 */

#if HW_CONFIG_PRO_DK
    #define DEFAULT_XTAL16M_TRIM_VALUE (850)
#else
    #define DEFAULT_XTAL16M_TRIM_VALUE (1302)
#endif

/**
 * @addtogroup DRIVERS
 * @{
 */

// Delay sleep entrance at system startup

uint32_t startup_sleep_delay = STARTUP_SLEEP_DELAY_DEFAULT;

#define LP_CLK_OTP_OFFSET       0x7f74     //OTP IQ_Trim offset

extern uint32_t last_temp_time;         // time of last temperature count measurement
extern uint16_t last_temp_count;        /// temperature counter

extern uint32_t lld_sleep_us_2_lpcycles_func(uint32_t us);
extern uint32_t lld_sleep_lpcycles_2_us_func(uint32_t lpcycles);

void platform_reset_func(uint32_t error);

extern bool sys_startup_flag;

#if !(BLE_HOST_PRESENT)

#if (HCI_OVER_UART_ENABLED)
// Creation of uart external interface api
extern const struct rwip_eif_api uart_api;
#endif //#if (HCI_OVER_UART_ENABLED)

#if (HCI_OVER_SPI_ENABLED)
// Creation of SPI external interface api
const struct rwip_eif_api spi_api =
{
    spi_hci_read_func,
    spi_hci_write_func,
    spi_hci_flow_on_func,
    spi_hci_flow_off_func,
};
#endif //#if (HCI_OVER_UART_ENABLED)
#endif //#if (!BLE_HOST_PRESENT)

uint32_t lp_clk_sel __attribute__((section("retention_mem_area0"),zero_init));   //low power clock selection
uint32_t rcx_freq __attribute__((section("retention_mem_area0"),zero_init));
uint8_t cal_enable  __attribute__((section("retention_mem_area0"),zero_init));
uint32_t rcx_period __attribute__((section("retention_mem_area0"),zero_init));
uint32_t rcx_slot_duration_num __attribute__((section("retention_mem_area0"), zero_init));
uint32_t rcx_slot_duration_den __attribute__((section("retention_mem_area0"), zero_init));

uint8_t force_rf_cal __attribute__((section("retention_mem_area0"),zero_init));

#ifndef __DA14581__
#if (BLE_CONNECTION_MAX_USER > 4)
volatile uint8_t cs_table[EM_BLE_CS_COUNT_USER * REG_BLE_EM_CS_SIZE] __attribute__((section("cs_area"), zero_init, used, aligned(4)));
#endif
#else
#if (BLE_CONNECTION_MAX_USER > 1)
volatile uint8_t cs_table[(BLE_CONNECTION_MAX + 2) * REG_BLE_EM_WPB_SIZE * 2] __attribute__((section("cs_area"), zero_init, used, aligned(4)));
#endif
#endif

#if (RCX_MEASURE_ENABLED)
uint32_t rcx_freq_min __attribute__((section("retention_mem_area0"),zero_init));
uint32_t rcx_freq_max __attribute__((section("retention_mem_area0"),zero_init));
uint32_t rcx_period_last __attribute__((section("retention_mem_area0"),zero_init));
uint32_t rcx_period_diff __attribute__((section("retention_mem_area0"),zero_init));
#endif

#ifndef USE_ARCH_WKUPCT_DEB_TIME
#define USE_ARCH_WKUPCT_DEB_TIME
uint16_t arch_wkupct_deb_time    __attribute__((section("retention_mem_area0"), zero_init)); // Wakeup timer debouncing time
#endif

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Read low power clock selection from
 *
 * The Hclk and Pclk are set
 ****************************************************************************************
*/
void select_lp_clk()
{
    if (CFG_LP_CLK == LP_CLK_FROM_OTP)
    {

        uint16_t *lp_clk;
        int cnt = 100000;

        if (APP_BOOT_FROM_OTP)
        {
            #define XPMC_MODE_MREAD   0x1
            lp_clk = (uint16_t *)(0x40000 + LP_CLK_OTP_OFFSET);     //where in OTP header is IQ_Trim

            SetBits16(CLK_AMBA_REG, OTP_ENABLE, 1);                           //enable OTP clock
            while ((GetWord16(ANA_STATUS_REG) & LDO_OTP_OK) != LDO_OTP_OK && cnt--)
                /* Just wait */;

            // set OTP in read mode
            SetWord32(OTPC_MODE_REG, XPMC_MODE_MREAD);
        }
        else
        {
            lp_clk = (uint16_t *)(0x20000000 + LP_CLK_OTP_OFFSET);  //where in OTP header is IQ_Trim
        }

        lp_clk_sel  = (*lp_clk);
        SetBits16(CLK_AMBA_REG, OTP_ENABLE, 0);                           //disable OTP clock
    }
    else //CFG_LP_CLK
        lp_clk_sel = CFG_LP_CLK;

}

/**
 ****************************************************************************************
 * @brief Initialisation of ble core, pwr and clk
 *
 * The Hclk and Pclk are set
 ****************************************************************************************
 */
static __inline void init_pwr_and_clk_ble(void)
{
    SetBits16(CLK_RADIO_REG, BLE_DIV, 0);
    SetBits16(CLK_RADIO_REG, BLE_ENABLE, 1);
    SetBits16(CLK_RADIO_REG, RFCU_DIV, 1);
    SetBits16(CLK_RADIO_REG, RFCU_ENABLE, 1);

    /*
     * Power up BLE core & reset BLE Timers
    */
    SetBits16(CLK_32K_REG,  RC32K_ENABLE, 1);
    SetBits16(SYS_CTRL_REG, CLK32_SOURCE, 0);
    SetBits16(CLK_RADIO_REG, BLE_LP_RESET, 1);
    SetBits16(PMU_CTRL_REG, RADIO_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & RAD_IS_UP)); // Just wait for radio to truely wake up

    select_lp_clk();

    if ( arch_clk_is_XTAL32( ) )
    {
        SetBits16(CLK_32K_REG, XTAL32K_ENABLE, 1);  // Enable XTAL32KHz

        // Disable XTAL32 amplitude regulation in BOOST mode
        if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 0x1)
            SetBits16(CLK_32K_REG,  XTAL32K_DISABLE_AMPREG, 1);
        else
            SetBits16(CLK_32K_REG,  XTAL32K_DISABLE_AMPREG, 0);
        SetBits16(CLK_32K_REG,  XTAL32K_CUR, 5);
        SetBits16(CLK_32K_REG,  XTAL32K_RBIAS, 3);
        SetBits16(SYS_CTRL_REG, CLK32_SOURCE, 1);  // Select XTAL32K as LP clock

     }
    else if ( arch_clk_is_RCX20( ) )
    {
        SetBits16(CLK_RCX20K_REG, RCX20K_NTC, 0xB);
        SetBits16(CLK_RCX20K_REG, RCX20K_BIAS, 0);
        SetBits16(CLK_RCX20K_REG, RCX20K_TRIM, 0);
        SetBits16(CLK_RCX20K_REG, RCX20K_LOWF, 1);

        SetBits16(CLK_RCX20K_REG, RCX20K_ENABLE, 1);

        SetBits16(CLK_RCX20K_REG, RCX20K_SELECT, 1);

        SetBits16(SYS_CTRL_REG, CLK32_SOURCE, 0);

        SetBits16(CLK_32K_REG, XTAL32K_ENABLE, 0); // Disable Xtal32KHz
    }
    else
        ASSERT_WARNING(0);

    SetBits16(CLK_32K_REG,  RC32K_ENABLE, 0);   // Disable RC32KHz

    SetBits16(CLK_RADIO_REG, BLE_LP_RESET, 0);

    if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 0x1)
        SetWord16(DCDC_CTRL3_REG, 0x5);

    /*
     * Just make sure that BLE core is stopped (if already running)
     */
    SetBits32(BLE_RWBTLECNTL_REG, RWBLE_EN, 0);

    /*
     * Since BLE is stopped (and powered), set CLK_SEL
     */
    SetBits32(BLE_CNTL2_REG, BLE_CLK_SEL, 16);
    SetBits32(BLE_CNTL2_REG, BLE_RSSI_SEL, 1);
}



/**
 ****************************************************************************************
 * @brief Creates sw cursor in power profiler tool. Used for Development/ Debugging
 *
 * @return void
 ****************************************************************************************
 */
void arch_set_pxact_gpio(void)
{
    if (DEVELOPMENT_DEBUG)
    {
        uint32_t i;

        SetWord16(P13_MODE_REG, PID_GPIO|OUTPUT);
        SetWord16(P1_SET_DATA_REG, 0x8);
        for (i = 0; i < 150; i++); //20 is almost 7.6usec of time.
        SetWord16(P1_RESET_DATA_REG, 0x8);
    }

    return;
}

void calibrate_rcx20(uint16_t cal_time)
{
    if ((CFG_LP_CLK == LP_CLK_FROM_OTP) || (CFG_LP_CLK == LP_CLK_RCX20))
    {
        SetWord16(CLK_REF_CNT_REG, cal_time);
        SetBits16(CLK_REF_SEL_REG, REF_CLK_SEL, 0x3); //RCX select
        SetBits16(CLK_REF_SEL_REG, REF_CAL_START, 0x1); //Start Calibration
        cal_enable = 1;
    }
}

void read_rcx_freq(uint16_t cal_time)
{
    if ( (cal_enable) && ((CFG_LP_CLK == LP_CLK_FROM_OTP) || (CFG_LP_CLK == LP_CLK_RCX20)) )
    {
        while(GetBits16(CLK_REF_SEL_REG, REF_CAL_START) == 1);
        volatile uint32_t high = GetWord16(CLK_REF_VAL_H_REG);
        volatile uint32_t low = GetWord16(CLK_REF_VAL_L_REG);
        volatile uint32_t value = ( high << 16 ) + low;
        volatile uint32_t f = (16000000 * cal_time) / value;

        cal_enable = 0;

        rcx_freq = f;
        rcx_period = (64 * value) / cal_time;
        rcx_slot_duration_num = (625 * cal_time * 16);
        rcx_slot_duration_den = value;

#if (RCX_MEASURE_ENABLED)
        if (rcx_period_last)
        {
            volatile int diff = rcx_period_last - rcx_period;
            if (abs(diff) > rcx_period_diff)
                rcx_period_diff = abs(diff);
        }
        rcx_period_last = rcx_period;

        if (rcx_freq_min == 0)
        {
            rcx_freq_min = rcx_freq;
            rcx_freq_max = rcx_freq;
        }
        if (rcx_freq < rcx_freq_min)
            rcx_freq_min = rcx_freq;
        else if (rcx_freq > rcx_freq_max)
            rcx_freq_max = rcx_freq;
#endif
    }
}

void set_sleep_delay(void)
{
    int16_t delay;

    if (!USE_POWER_OPTIMIZATIONS)
    {
        uint32_t sleep_delay;

        delay = 0;

        if ( arch_clk_is_RCX20() )
        {
            if (rcx_period > (RCX_PERIOD_MAX << 10) )
                ASSERT_ERROR(0);

            sleep_delay = SLP_PROC_TIME + SLEEP_PROC_TIME + (4 * RCX_PERIOD_MAX); // 400
        }
        else
        {
            sleep_delay = /*SLP_PROC_TIME + SLEEP_PROC_TIME + */(4 * XTAL32_PERIOD_MAX); // ~200
        }

        // Actual "delay" is application specific and is the execution time of the BLE_WAKEUP_LP_Handler(), which depends on XTAL trimming delay.
        // In case of OTP copy, this is done while the XTAL is settling. Time unit of delay is usec.
        delay += XTAL_TRIMMING_TIME_USEC;
        // Icrease time taking into account the time from the setting of DEEP_SLEEP_ON until the assertion of DEEP_SLEEP_STAT.
        delay += sleep_delay;
        // Add any application specific delay
        delay += APP_SLEEP_DELAY_OFFSET;
    }
    else // USE_POWER_OPTIMIZATIONS
    {
        delay = MINIMUM_SLEEP_DURATION;

        // if XTAL_TRIMMING_TIME_USEC changes (i.e. gets larger) then this
        // will make sure that the user gets notified to increase "delay" by 1 or more
        // slots so there's enough time for XTAL to settle
        # if ( (3125 + (MINIMUM_SLEEP_DURATION + 625)) < (LP_ISR_TIME_USEC + 1100)) //1.1ms max power-up time
        # error "Minimum sleep duration is too small for the 16MHz crystal used..."
        # endif
    }

    rwip_wakeup_delay_set(delay);
}


/**
 ****************************************************************************************
 * @brief get_rc16m_count_func_patched(). (Patch.
 *
 * @return void
 ****************************************************************************************
 */
#ifdef __GNUC__
extern uint16_t __real_get_rc16m_count_func(void);

uint16_t __wrap_get_rc16m_count_func(void)
{
    uint16_t count;
    uint16_t trim = GetBits16(CLK_16M_REG, RC16M_TRIM);

    count = __real_get_rc16m_count_func();

    SetBits16(CLK_16M_REG, RC16M_TRIM, trim);

    return count;
}

#else

extern uint16_t $Super$$get_rc16m_count_func(void);

uint16_t $Sub$$get_rc16m_count_func(void)
{
    uint16_t count;
    uint16_t trim = GetBits16(CLK_16M_REG, RC16M_TRIM);

    count = $Super$$get_rc16m_count_func();

    SetBits16(CLK_16M_REG, RC16M_TRIM, trim);

    return count;
}
#endif //__GNUC__

void conditionally_run_radio_cals(void)
{
    uint16_t count, count_diff;

    bool rf_cal_stat;

    uint32_t current_time = lld_evt_time_get();

    if (current_time < last_temp_time)
    {
        last_temp_time = 0;
    }

    if (force_rf_cal)
    {
        force_rf_cal = 0;

        last_temp_time = current_time;
        last_temp_count = get_rc16m_count();
        if (LUT_PATCH_ENABLED)
            pll_vcocal_LUT_InitUpdate(LUT_UPDATE);    //Update pll look up table

        rf_cal_stat = rf_calibration();
        if ( rf_cal_stat == false)
            force_rf_cal = 1;

        return;
    }

    if ( (current_time - last_temp_time) >= 3200) //2 sec
    {
        last_temp_time = current_time;
        count = get_rc16m_count();                  // Estimate the RC16M frequency

        if (count > last_temp_count)
            count_diff = count - last_temp_count;
        else
            count_diff = last_temp_count - count ;

        if (count_diff >= 24)// If corresponds to 5 C degrees difference
        {

            // Update the value of last_count
            last_temp_count = count;
            if (LUT_PATCH_ENABLED)
                pll_vcocal_LUT_InitUpdate(LUT_UPDATE);    //Update pll look up table

            rf_cal_stat = rf_calibration();

            if ( rf_cal_stat == false)
                force_rf_cal = 1;         // Perform the readio calibrations
        }
    }
}

/**
 ****************************************************************************************
 * @brief       Converts us to low power cycles for RCX20 clock.
 *
 * @param[in]   us. microseconds
 *
 * @return      uint32. Low power cycles
 ****************************************************************************************
 */
uint32_t lld_sleep_us_2_lpcycles_rcx_func(uint32_t us)
{
    volatile uint32_t lpcycles = 0;

    //RCX
    //Compute the low power clock cycles - case of a 16.342kHz clock
    if ((CFG_LP_CLK == LP_CLK_FROM_OTP) || (CFG_LP_CLK == LP_CLK_RCX20))
        lpcycles = (us * rcx_freq) / 1000000;

    return(lpcycles);
}

/**
 ****************************************************************************************
 * @brief       Converts low power cycles to us for RCX20 clock.
 *
 * @param[in]   lpcycles. Low power cycles
 *
 * @return      uint32. microseconds
 ****************************************************************************************
 */
uint32_t lld_sleep_lpcycles_2_us_rcx_func(uint32_t lpcycles)
{
    volatile uint32_t us = 0;

    // Sanity check: The number of lp cycles should not be too high to avoid overflow
    ASSERT_ERR(lpcycles < 1000000);

    // Compute the sleep duration in us - case of a 16.342kHz clock (61.19202)
    if ((CFG_LP_CLK == LP_CLK_FROM_OTP) || (CFG_LP_CLK == LP_CLK_RCX20)) {
        if (lpcycles >= 0x8000) {
            // lpcycles MUST be very large!
            uint64_t res = (uint64_t)lpcycles * rcx_period;
            us = res >> 10;
        }
        else {
            us = (lpcycles * rcx_period) >> 10;
        }
    }

    return(us);
}

uint32_t lld_sleep_us_2_lpcycles_sel_func(uint32_t us)
{
    volatile uint32_t lpcycles;

    if ( arch_clk_is_XTAL32( ) )
        lpcycles = lld_sleep_us_2_lpcycles_func(us);
    else if ( arch_clk_is_RCX20( ) )
        lpcycles = lld_sleep_us_2_lpcycles_rcx_func(us);

    return(lpcycles);
}

uint32_t lld_sleep_lpcycles_2_us_sel_func(uint32_t lpcycles)
{
    volatile uint32_t us;
    if ( arch_clk_is_XTAL32( ) )
        us = lld_sleep_lpcycles_2_us_func(lpcycles);
    else  if ( arch_clk_is_RCX20() )
        us = lld_sleep_lpcycles_2_us_rcx_func(lpcycles);

    return(us);
}


#if (!BLE_APP_PRESENT)
bool check_gtl_state(void)
{
    uint8_t ret = true;

    #if (BLE_HOST_PRESENT)
    {
        if ((ke_state_get(TASK_GTL) != GTL_TX_IDLE) ||
                ((gtl_env.rx_state != GTL_STATE_RX_START) &&
                (gtl_env.rx_state != GTL_STATE_RX_OUT_OF_SYNC)) )
                ret = false;
    }
    #else
    {
        if ((hci_env.tx_state != HCI_STATE_TX_IDLE) ||
                ((hci_env.rx_state != HCI_STATE_RX_START) &&
                (hci_env.rx_state != HCI_STATE_RX_OUT_OF_SYNC)) )
                ret = false;
    }
    #endif

    return ret;
}
#endif


#ifndef __DA14581__
/*
 ****************************************************************************************
 * Replacement of ROM function uart_flow_off_func
 ****************************************************************************************
 */

static bool uart_is_rx_fifo_empty(void)
{
    return (uart_data_rdy_getf() == 0);
}

bool uart_flow_off_func(void)
{
    bool flow_off = true;
    volatile unsigned int i;

    do
    {
        // First check if no transmission is ongoing
        if ((uart_temt_getf() == 0) || (uart_thre_getf() == 0))
        {
            flow_off = false;
            break;
        }

        // Configure modem (HW flow control disable, 'RTS flow off')
        SetWord32(UART_MCR_REG, 0);

        // Wait for 1 character duration to ensure host has not started a transmission at the
        // same time
        for (i = 0; i < 350; i++);

        // Check if data has been received during wait time
        if(!uart_is_rx_fifo_empty())
        {
            // Re-enable UART flow
            uart_flow_on();

            // We failed stopping the flow
            flow_off = false;
        }
    }while(false);

    return (flow_off);
}
#endif


#if ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT)) // only in full embedded designs
void ext_wakeup_enable(uint32_t port, uint32_t pin, uint8_t polarity)
{
    rwip_env.ext_wakeup_enable = 2;
    if (DEVELOPMENT_DEBUG)
        RESERVE_GPIO(EXT_WAKEUP_GPIO, (GPIO_PORT) port, (GPIO_PIN) pin, PID_GPIO);

    if ( polarity == 0 ) // active low
        GPIO_ConfigurePin((GPIO_PORT) port, (GPIO_PIN) pin, INPUT_PULLUP, PID_GPIO, false ); // active low. Set default to input high.
    else // active high
        GPIO_ConfigurePin((GPIO_PORT) port, (GPIO_PIN) pin, INPUT_PULLDOWN, PID_GPIO, false); // active high. Set default to input low.

    wkupct_register_callback(ext_wakeup_cb);

    wkupct_enable_irq(1 << pin, (uint32_t)(polarity == 0) ? (1 << pin) : (0), 1, 0); // pin, active low/high, 1 event, debouncing time = 0ms
}

void ext_wakeup_disable(void)
{
    SetWord16(WKUP_RESET_IRQ_REG, 1); // Acknowledge it
    SetBits16(WKUP_CTRL_REG, WKUP_ENABLE_IRQ, 0); // No more interrupts of this kind
}

void ext_wakeup_cb(void)
{
    if (GetBits16(SYS_STAT_REG, PER_IS_DOWN))
    {
        // Return GPIO functionality from external wakeup GPIO
        if (DEVELOPMENT_DEBUG)
            GPIO_reservations();
        set_pad_functions();
    }
    SetBits32(GP_CONTROL_REG, BLE_WAKEUP_REQ, 1);
}
#endif // ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT))

#if (!BLE_HOST_PRESENT)

const struct rwip_eif_api* rwip_eif_get_func(uint8_t type)
{
    const struct rwip_eif_api* ret = NULL;
    switch(type)
    {
        case RWIP_EIF_AHI:
        case RWIP_EIF_HCIC:
        {
            if (HCI_BOTH_EIF_ENABLED)
            {
                        if (GPIO_GetPinStatus(HCI_EIF_SELECT_PORT,HCI_EIF_SELECT_PIN))
                            ret = &uart_api;
                        else
                            ret = &spi_api;
            }
            else
            {
              if (HCI_OVER_SPI_ENABLED)
                        ret = &spi_api;     // select spi api
              else
                        ret = &uart_api;    // select uart api

            }
        }
        break;
        default:
        {
            ASSERT_ERR(0);
        }
        break;
    }
    return ret;
}
#endif // (!BLE_HOST_PRESENT)


/**
 ****************************************************************************************
 * @brief Calculates optimal ADV_IND interval in uSec
 * @param[in] Advertise data length. Max value = 28
 * @param[in] Scan Response. Max value = 31
 * @return uint32   optimal ADV_IND interval in uSec
 ****************************************************************************************
 */
uint32_t arch_adv_time_calculate (uint8_t adv_len, uint8_t scan_rsp_len)
{
    uint32_t arch_adv_int;

    arch_adv_int = (ARCH_ADV_MIN_DUR + adv_len*8) +  (ARCH_SCAN_RSP_MIN_DUR + scan_rsp_len*8) + ARCH_SCAN_REQ_DUR + (3 * ARCH_INT_FRAME_TIME);

    return arch_adv_int;
}

#if USE_TRNG
void init_rand_seed_from_trng(void)
{
    uint8_t trng_bits[16];
    uint32_t seed = 0;

    trng_acquire(trng_bits);
    seed = *( (uint32_t *) trng_bits );
    srand(seed);
}
#endif // USE_TRNG

uint8_t check_sys_startup_period(void)
{
    uint8_t ret_value = 0;

    if (sys_startup_flag)
    {
        uint32_t current_time;

        current_time = lld_evt_time_get();

        // startup_sleep_delay after system startup to allow system to sleep
        if (current_time < startup_sleep_delay)
        {
            ret_value = 1;
        }
        else // After 2 seconds system can sleep
        {
            sys_startup_flag = false;
        }
    }
    return ret_value;
}

__weak bool app_use_lower_clocks_check(void)
{
    return false;
}

void set_xtal16m_trim_value(uint16_t trim_value)
{
    SetBits16(CLK_16M_REG, RC16M_ENABLE, 1);                        // enable RC 16MHz
    for (volatile int i = 0; i < 20; i++);

    SetBits16(CLK_CTRL_REG, SYS_CLK_SEL, 1);                        // switch to  RC16
    while( (GetWord16(CLK_CTRL_REG) & RUNNING_AT_RC16M) == 0 );     // wait for actual switch

    SetBits16(CLK_CTRL_REG, XTAL16M_DISABLE, 1);                    // disable XTAL16
    SetWord16(CLK_FREQ_TRIM_REG, trim_value);                       // set default trim value
    SetBits16(CLK_CTRL_REG, XTAL16M_DISABLE, 0);                    // enable XTAL16
    while( (GetWord16(SYS_STAT_REG) & XTAL16_SETTLED) == 0 );       // wait for XTAL16 settle

    SetBits16(CLK_CTRL_REG , SYS_CLK_SEL ,0);                       // switch to  XTAL16
    while( (GetWord16(CLK_CTRL_REG) & RUNNING_AT_XTAL16M) == 0 );   // wait for actual switch
}

/**
 ****************************************************************************************
 * @brief Check that XTAL 16 MHz clock is calibrated.
 * @return void
 * About: Check if XTAL 16 MHz clock is calibrated and if not make corrective actions.
 ****************************************************************************************
 */
static __inline void xtal16_calibration_check(void)
{
    if(DEFAULT_XTAL16M_TRIM_VALUE_USED)
    {
    // Apply the default XTAL16 trim value if a trim value has not been programmed in OTP
        if (0 == GetWord16(CLK_FREQ_TRIM_REG))
        {
            set_xtal16m_trim_value(DEFAULT_XTAL16M_TRIM_VALUE);
        }
    }
}

/*
 ********************* System initialisation related functions *******************
*/

extern uint32_t error;              /// Variable storing the reason of platform reset
extern struct lld_sleep_env_tag lld_sleep_env;

void set_system_clocks(void);
/**
 ****************************************************************************************
 * @brief  If a Firmware reset occured, send a message to the Host with the error
 * @return void
 ****************************************************************************************
 */
static __inline  void hcic_reset_message (void)
{
    // If FW initializes due to FW reset, send the message to Host
    if(error != RESET_NO_ERROR)
    {
        rwble_send_message(error);
    }
}

/**
 ****************************************************************************************
 * @brief  Clear all pending ble interrupts from NVIC
 * @return void
 ****************************************************************************************
 */

static __inline void rwip_clear_interrupts (void)
{
    NVIC_ClearPendingIRQ(BLE_SLP_IRQn);
    NVIC_ClearPendingIRQ(BLE_EVENT_IRQn);
    NVIC_ClearPendingIRQ(BLE_RF_DIAG_IRQn);
    NVIC_ClearPendingIRQ(BLE_RX_IRQn);
    NVIC_ClearPendingIRQ(BLE_CRYPT_IRQn);
    NVIC_ClearPendingIRQ(BLE_FINETGTIM_IRQn);
    NVIC_ClearPendingIRQ(BLE_GROSSTGTIM_IRQn);
    NVIC_ClearPendingIRQ(BLE_WAKEUP_LP_IRQn);
}

void lld_sleep_init_func(void)
{
    // Clear the environment
    memset(&lld_sleep_env, 0, sizeof(lld_sleep_env));

    // Set wakeup_delay
    set_sleep_delay();

    // Enable external wake-up by default
    ble_extwkupdsb_setf(0);
}

/**
 ****************************************************************************************
 * @brief  Global System Init
 * @return void
 ****************************************************************************************
 */
void system_init(void)
{
    sys_startup_flag = true;
    /*
     ************************************************************************************
     * Platform initialization
     ************************************************************************************
     */
    //initialise the Watchdog unit
    wdg_init(0);

    //confirm XTAL 16 MHz calibration
    xtal16_calibration_check ();

    //set the system clocks
    set_system_clocks();

    //Initiliaze the GPIOs
    GPIO_init();

    // Initialize NVDS module
    nvds_init((uint8_t *)NVDS_FLASH_ADDRESS, NVDS_FLASH_SIZE);

    // Check and read BD address
    nvds_read_bdaddr();

    //Peripherals initilization
    periph_init();

    // Initialize random process
    srand(1);

    //Trim the radio from the otp
    iq_trim_from_otp();

    /*
     ************************************************************************************
     * BLE initialization
     ************************************************************************************
     */
    init_pwr_and_clk_ble();

    // Initialize BLE stack
    rwip_clear_interrupts ();

    // Initialize rw
    rwip_init(error);

    //Patch llm task
    patch_llm_task();

    //Patch gtl task
    if (BLE_GTL_PATCH)
        patch_gtl_task();

#if !defined( __DA14581__)
    //Patch llc task
    patch_llc_task();
#endif

#if !defined(__DA14581__)
    patch_atts_task();
#endif

#if (BLE_HOST_PRESENT)
    patch_gapc_task();
#endif

    //Enable the BLE core
    SetBits32(BLE_RWBTLECNTL_REG, RWBLE_EN, 1);

    // Initialise random number generator seed using random bits acquired from TRNG
    if (USE_TRNG)
        init_rand_seed_from_trng();

    //send a message to the host if an error occured
    if (BLE_HCIC_ITF)
        hcic_reset_message();

    rcx20_calibrate ();

    arch_disable_sleep();

    /*
     ************************************************************************************
     * Application initializations
     ************************************************************************************
     */
    // Initialise APP

#if (BLE_APP_PRESENT)
        app_init();         // Initialize APP
#endif

     if (user_app_main_loop_callbacks.app_on_init !=NULL)
          user_app_main_loop_callbacks.app_on_init();

    //Initialise lld_sleep
    lld_sleep_init_func();

    /*
     ************************************************************************************
     * XTAL16M trimming settings
     ************************************************************************************
     */
    //set trim and bias of xtal16
    xtal16__trim_init();

     // Enable the TX_EN/RX_EN interrupts, depending on the RF mode of operation (PLL-LUT and MGC_KMODALPHA combinations)
    enable_rf_diag_irq(RF_DIAG_IRQ_MODE_RXTX);

    /*
     ************************************************************************************
     * Watchdog
     ************************************************************************************
     */
    if(USE_WDOG)
    {
        wdg_reload(WATCHDOG_DEFAULT_PERIOD);
        wdg_resume ();
    }

#ifndef __DA14581__
# if (BLE_CONNECTION_MAX_USER > 4)
    cs_table[0] = cs_table[0];
# endif

#else //DA14581

# if (BLE_CONNECTION_MAX_USER > 1)
    cs_table[0] = cs_table[0];
# endif
#endif //__DA14581__

}

void wrap_platform_reset(uint32_t error)
{
    ASSERT_WARNING(error==RESET_AFTER_SPOTA_UPDATE);   //do not break in case of a SPOTA reset
    platform_reset_func(error);
}

void arch_wkupct_tweak_deb_time(bool tweak)
{
    if (arch_clk_is_RCX20())
    {
        if (tweak)
        {
            // Recalculate debouncing time to correct silicon bug
            uint32_t temp = (rcx_freq * arch_wkupct_deb_time) / 32768;
            SetBits16(WKUP_CTRL_REG, WKUP_DEB_VALUE, temp & 0x3F);
        }
        else
        {
            SetBits16(WKUP_CTRL_REG, WKUP_DEB_VALUE, arch_wkupct_deb_time & 0x3F);
        }
    }
}

void arch_uart_init_slow(uint16_t baudr, uint8_t mode)
{
    /// UART TX RX Channel
    struct uart_txrxchannel
    {
        /// size
        uint32_t  size;
        /// buffer pointer
        uint8_t  *bufptr;
        /// call back function pointer
        void (*callback) (uint8_t);
    };
    /// UART environment structure
    struct uart_env_tag
    {
        /// tx channel
        struct uart_txrxchannel tx;
        /// rx channel
        struct uart_txrxchannel rx;
        /// error detect
        uint8_t errordetect;
    };
    // ROM variable
    extern struct uart_env_tag uart_env;

    //ENABLE FIFO, REGISTER FCR IF UART_LCR_REG.DLAB=0
    // XMIT FIFO RESET, RCVR FIFO RESET, FIFO ENABLED
    SetBits16(UART_LCR_REG, UART_DLAB, 0);
    SetWord16(UART_IIR_FCR_REG, 7);

    //DISABLE INTERRUPTS, REGISTER IER IF UART_LCR_REG.DLAB=0
    SetWord16(UART_IER_DLH_REG, 0);

    // ACCESS DIVISORLATCH REGISTER FOR BAUDRATE, REGISTER UART_DLH/DLL_REG IF UART_LCR_REG.DLAB=1
    SetBits16(UART_LCR_REG, UART_DLAB, 1);
    SetWord16(UART_IER_DLH_REG, (baudr >> 8) & 0xFF);
    SetWord16(UART_RBR_THR_DLL_REG, baudr & 0xFF);

    // NO PARITY, 1 STOP BIT, 8 DATA LENGTH
    SetWord16(UART_LCR_REG,mode);

    //ENABLE TX INTERRUPTS, REGISTER IER IF UART_LCR_REG.DLAB=0
    SetBits16(UART_LCR_REG, UART_DLAB, 0);

    NVIC_EnableIRQ(UART_IRQn);
    NVIC->ICPR[UART_IRQn] = 1; //clear eventual pending bit, but not necessary becasuse this is already cleared automatically in HW

    // Configure UART environment
    uart_env.errordetect = UART_ERROR_DETECT_DISABLED;
    uart_env.rx.bufptr = NULL;
    uart_env.rx.size = 0;
    uart_env.tx.bufptr = NULL;
    uart_env.tx.size = 0;
}


/// @}
