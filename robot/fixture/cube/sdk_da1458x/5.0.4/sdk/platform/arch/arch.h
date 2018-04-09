/**
 ****************************************************************************************
 *
 * @file arch.h
 *
 * @brief This file contains the definitions of the macros and functions that are
 * architecture dependent.  The implementation of those is implemented in the
 * appropriate architecture directory.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev:  $
 *
 ****************************************************************************************
 */

#ifndef _ARCH_H_
#define _ARCH_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>        // standard integer definition
#include "compiler.h"      // inline functions
#include "stdbool.h"

#if ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT)) // external wake up, only in full embedded designs
#include "wkupct_quadec.h"
#include "gpio.h"
#endif

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef enum
{
    mode_active = 0,
    mode_idle,
    mode_ext_sleep,
    mode_deep_sleep,
    mode_sleeping,
} sleep_mode_t;

typedef enum {
    NOT_MEASURED = 0,
    APPLY_OVERHEAD,
    NO_OVERHEAD,
} boost_overhead_st;

/// Drift values in ppm as expected by 'lld_evt.c'
typedef enum {
    DRIFT_20PPM  = (20),
    DRIFT_30PPM  = (30),
    DRIFT_50PPM  = (50),
    DRIFT_75PPM  = (75),
    DRIFT_100PPM = (100),
    DRIFT_150PPM = (150),
    DRIFT_250PPM = (250),
    DRIFT_500PPM = (500), // Default value
} drift_value_in_ppm;

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

/// LLD ROM defines
struct lld_sleep_env_tag
{
    uint32_t irq_mask;
};

/// Memory usage logging structure
struct mem_usage_log
{
    /// Maximum used size
    uint16_t max_used_sz;
    /// Heap used size
    uint16_t used_sz;
    /// Maximum size used in other heaps
    uint16_t max_used_other_sz;
    /// Size used in other heaps
    uint16_t used_other_sz;
};

extern uint16_t clk_freq_trim_reg_value;

extern uint32_t startup_sleep_delay;

#ifndef __DA14581__
extern const uint32_t * const jump_table_base[88];
#else
extern const uint32_t * const jump_table_base[92];
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

/// Object allocated in shared memory - check linker script
#define __SHARED __attribute__ ((section("shram")))

#define jump_table_struct (uint32_t)jump_table_base

/// ARM is a 32-bit CPU
#define CPU_WORD_SIZE               (4)

/// ARM is little endian
#define CPU_LE                      (1)

#define ARCH_INT_FRAME_TIME         (150)
#define ARCH_SCAN_REQ_DUR           (150)
#define ARCH_SCAN_RSP_MIN_DUR       (130)
#define ARCH_ADV_MIN_DUR            (150)
#define ARCH_ADV_INT_DIRECT         (1250)
#define ARCH_ADV_INT_UNDIRECT       (1500)
#define ARCH_ADV_INT_NONCONNECTABLE (500)

#ifdef __DA14581__
#define ARCH_ADV_INT_CONNECTABLE    (1250)
#define ARCH_ADV_INT_NONCONNECTABLE (500)
#endif

/******************************************
 * da1458x_config_basic/advanced.h SETTINGS
 ******************************************
 */

#if defined (CFG_MEM_MAP_DEEP_SLEEP)
    #if defined (CFG_MEM_MAP_EXT_SLEEP)
        #error "CFG_MEM_MAP_DEEP_SLEEP defined with CFG_MEM_MAP_EXT_SLEEP! Select one of them"
    #endif
#endif

#if defined (CFG_LUT_PATCH)
    #define LUT_PATCH_ENABLED                           1
#else
    #define LUT_PATCH_ENABLED                           0
#endif /* CFG_LUT_PATCH */

#if defined (CFG_TRNG)
    #define USE_TRNG                                    1
#else
    #define USE_TRNG                                    0
#endif /* CFG_TRNG */

#if defined (CFG_SKIP_SL_PATCH)
    #define SKIP_SL_PATCH                               1
#else
    #define SKIP_SL_PATCH                               0
#endif /* CFG_TRNG */

#if defined (CFG_POWER_OPTIMIZATIONS)
    #define USE_POWER_OPTIMIZATIONS                     1
#else
    #define USE_POWER_OPTIMIZATIONS                     0
#endif

#if defined (CFG_DEVELOPMENT_DEBUG)
    #define DEVELOPMENT_DEBUG                           1
#else
    #define DEVELOPMENT_DEBUG                           0
#endif

#if defined (CFG_BOOT_FROM_OTP)
    #define APP_BOOT_FROM_OTP                           1
#else
    #define APP_BOOT_FROM_OTP                           0
#endif

#if defined (CFG_USE_DEFAULT_XTAL16M_TRIM_VALUE_IF_NOT_CALIBRATED)
    #define DEFAULT_XTAL16M_TRIM_VALUE_USED             1
#else
    #define DEFAULT_XTAL16M_TRIM_VALUE_USED             0
#endif

/// Alternate pairing mechanism
#if defined(CFG_MULTI_BOND)
#define HAS_MULTI_BOND                                  1
#else
#define HAS_MULTI_BOND                                  0
#endif

#if defined(CFG_HCI_BOTH_EIF)
#define CFG_HCI_UART
#define CFG_HCI_SPI
#define HCI_BOTH_EIF_ENABLED                            1
#else
#define HCI_BOTH_EIF_ENABLED                            0
#endif

/// HCI over UART enabled
#if defined(CFG_HCI_UART)
#define HCI_OVER_UART_ENABLED                           1
#else
#define HCI_OVER_UART_ENABLED                           0
#endif

/// HCI over SPI enabled
#if defined(CFG_HCI_SPI)
#define HCI_OVER_SPI_ENABLED                            1
#else
#define HCI_OVER_SPI_ENABLED                            0
#endif

/// RCX MEAS
#if defined(CFG_RCX_MEASURE)
#define RCX_MEASURE_ENABLED                             1
#else
#define RCX_MEASURE_ENABLED                             0
#endif

#if defined(CFG_LOG_MEM_USAGE)
#define LOG_MEM_USAGE                                   1
#else
#define LOG_MEM_USAGE                                   0
#endif

#if defined(CFG_WLAN_COEX)
#define WLAN_COEX_ENABLED                               1
#else
#define WLAN_COEX_ENABLED                               0
#endif

#if defined(CFG_GTL_SPI)
#define GTL_SPI_SUPPORTED                               1
#else
#define GTL_SPI_SUPPORTED                               0
#endif

#if defined(CFG_EXTERNAL_WAKEUP)
#define EXTERNAL_WAKEUP                                 1
#else
#define EXTERNAL_WAKEUP                                 0
#endif

#if defined(CFG_WAKEUP_EXT_PROCESSOR)
#define WKUP_EXT_PROCESSOR                              1
#else
#define WKUP_EXT_PROCESSOR                              0
#endif

#if defined(CFG_BLE_METRICS)
#define BLE_METRICS                                     1
#else
#define BLE_METRICS                                     0
#endif

#if defined(CFG_PRODUCTION_DEBUG_OUTPUT)
#define PRODUCTION_DEBUG_OUTPUT                         1
#else
#define PRODUCTION_DEBUG_OUTPUT                         0
#endif

#ifdef __DA14583__
#if defined (CFG_READ_BDADDR_FROM_DA14583_FLASH)
# define   BDADDR_FROM_DA14583_FLASH_DISABLED           0
#else //CFG_READ_BDADDR_FROM_DA14583_FLASH
# define   BDADDR_FROM_DA14583_FLASH_DISABLED           1
#endif //CFG_READ_BDADDR_FROM_DA14583_FLASH
#else //__DA14583__
# define   BDADDR_FROM_DA14583_FLASH_DISABLED           1
#endif //__DA14583__

#if defined(CFG_PRODUCTION_TEST)
#define PRODUCTION_TEST                                 1
#else
#define PRODUCTION_TEST                                 0
#endif

#if defined(CFG_MGCKMODA_PATCH_ENABLED)
#define MGCKMODA_PATCH_ENABLED                          1
#else
#define MGCKMODA_PATCH_ENABLED                          0
#endif

/*
 * Deep sleep threshold. Application specific. Control if during deep sleep the system RAM will be powered off and if OTP copy will be required.
 ****************************************************************************************
 */

#define MS_TO_SLOTS_CONVERT(x)  ((int)((1000 * x) / 625))

/// Sleep Duration Value in periodic wake-up mode
#define MAX_SLEEP_DURATION_PERIODIC_WAKEUP      MS_TO_SLOTS_CONVERT(CFG_MAX_SLEEP_DURATION_PERIODIC_WAKEUP_MS)
/// Sleep Duration Value in non external wake-up mode
#define MAX_SLEEP_DURATION_EXTERNAL_WAKEUP      MS_TO_SLOTS_CONVERT(CFG_MAX_SLEEP_DURATION_EXTERNAL_WAKEUP_MS)

#define DEEP_SLEEP_THRESHOLD    (800000)

/*
 * Duration from XTAL active to XTAL settle.
 ****************************************************************************************
 */
#define XTAL16_TRIM_DELAY_SETTING   (0xA2)  // TRIM_CTRL_REG value ==> 14 cycles of 250usec each = 3.5ms (note: RC16 is used)

/*
 * XTAL16 settling time measured in low power clock cycles.
 * Cannot be less than 1.9ms due to the OTP copy in deep sleep, which may take quite long
 * if RC16 is too low (i.e. 10MHz).
 ****************************************************************************************
 */
#define XTAL16M_SETTLING_IN_XTAL32_CYCLES       (95)    // count for 95 LP cycles, (x 30.5usec = ~2.9ms)
#define XTAL16M_SETTLING_IN_USEC                (2900)  // when RCX is used

/*
 * Wakeup timing parameters.
 ****************************************************************************************
 */

// (USE_POWER_OPTIMIZATIONS) definitions

#define MINIMUM_SLEEP_DURATION      (1250)  // 2 slots (3 or more slots will be allowed (note: not for timer wakeups))
#define LP_ISR_TIME_USEC            (3357)  // 110 * (1000000/32768); must be greater than XTAL16M_SETTLING_IN_USEC
#define LP_ISR_TIME_XTAL32_CYCLES   (110)
#define XTAL32_POWER_UP_TIME        (21)
#define RCX_POWER_UP_TIME           (18)
#define XTAL32_OTP_COPY_OVERHEAD    (50)
#define RCX_OTP_COPY_OVERHEAD       (17)
#define BOOST_POWER_UP_OVERHEAD     (33)    // 1msec

#define CLK_TRIM_WAIT_CYCLES        (1000)  // How many 16MHz cycles we will wait before applying the correct FREQ TRIM value

/* Note
 * --------------------------
 * After XTAL16M settling, the code needs ~10usec + 1/2 LP cycle to power-up BLE and pop BLE registers.
 * This translates to ~26usec for XTAL32 and ~57usec for RCX.
 *
 * Thus, LP_ISR_TIME_USEC - XTAL16 settling time = 3357 - 2900 = 457usec and 457 - 57 = 400usec
 * are left as a safety margin in the worst case (RCX) (best case is 3357 - 2900 - 26 = 431 for XTAL32).
 *
 * In principle, this time can be reduced to be at least 2 low power cycles.
 *
 * If the remaining time is not enough for periph_init() to complete then an ASSERTION will hit in
 * BLE_WAKEUP_LP_Handler (when DEVELOPMENT_DEBUG is 1). In this case, since the settling time of the
 * XTAL16M is fixed, the user should increase the LP_ISR_TIME_USEC (and, consequently, the
 * LP_ISR_TIME_XTAL32_CYCLES).
 */

// (!USE_POWER_OPTIMIZATIONS) definitions
#define XTAL_TRIMMING_TIME_USEC     (5798)  // 190 * (1000000/32768)
#define XTAL_TRIMMING_TIME          (190)

/*
 * Minimum and maximum LP clock periods. Measured in usec.
 ****************************************************************************************
 */
#define XTAL32_PERIOD_MIN           (30)    // 30.5usec
#define XTAL32_PERIOD_MAX           (31)
#define RCX_PERIOD_MIN              (85)    // normal range: 91.5 - 93usec
#define RCX_PERIOD_MAX              (200)

/*
 * DEEP SLEEP: Power down configuration
 ****************************************************************************************
 */
#define TWIRQ_RESET_VALUE           (1)
#define TWEXT_VALUE_RCX             (TWIRQ_RESET_VALUE + 2) // ~190usec (LP ISR processing time) / 85usec (LP clk period) = 2.23 => 3 LP cycles
#define TWEXT_VALUE_XTAL32          (TWIRQ_RESET_VALUE + 6) // ~190usec (LP ISR processing time) / 30usec (LP clk period) = 6.3 => 7 LP cycles


// The times below have been measured for RCX. For XTAL32 will be less. But this won't make much difference...
#define SLP_PROC_TIME               (60)    // 60 usec for SLP ISR to request Clock compensation
#define SLEEP_PROC_TIME             (30)    // 30 usec for rwip_sleep() to program Sleep time and drive DEEP_SLEEP_ON

// Change this if the application needs to increase the sleep delay limit from 9375usec that is now.
#define APP_SLEEP_DELAY_OFFSET      (0)

// Used to eliminated additional delays in the sleep duration
#define SLEEP_DURATION_CORR         (4)

/// Possible errors detected by FW
#define    RESET_NO_ERROR           (0x00000000)
#define    RESET_MEM_ALLOC_FAIL     (0xF2F2F2F2)

/// Reset platform and stay in ROM
#define    RESET_TO_ROM             (0xA5A5A5A5)
/// Reset platform and reload FW
#define    RESET_AND_LOAD_FW        (0xC3C3C3C3)

#define    RESET_AFTER_SPOTA_UPDATE (0x11111111)

#define STARTUP_SLEEP_DELAY_DEFAULT (3200)

/*
 * EXPORTED FUNCTION DECLARATION
 ****************************************************************************************
 */

/*
 * External wake up declarations and includes
 ****************************************************************************************
 */

#if ((EXTERNAL_WAKEUP) && (!BLE_APP_PRESENT)) // external wake up, only in full embedded designs
/**
 ****************************************************************************************
 * @brief Enable external wake up GPIO Interrupt.
 * @param[in] port The GPIO port of the external wake up signal
 * @param[in] pin The GPIO pin of the external wake up signal
 * @param[in] polarity The polarity of the external wake up interrupt. 0=active low. 1=active high
 * @return void
 ****************************************************************************************
 */
void ext_wakeup_enable(uint32_t port, uint32_t pin, uint8_t polarity);

/**
 ****************************************************************************************
 * @brief Disable external wake up GPIO Interrupt. Used on self wake up.
 * @return void
 ****************************************************************************************
 */
void ext_wakeup_disable(void);

/**
 ****************************************************************************************
 * @brief Callback function, called when external wakeup function is triggered.
 * @return void
 ****************************************************************************************
 */
void ext_wakeup_cb(void);
#endif

/**
 ****************************************************************************************
 * @brief Compute size of SW stack used. This function is compute the maximum size stack
 *        used by SW.
 * @return Size of stack used (in bytes)
 ****************************************************************************************
 */
uint16_t get_stack_usage(void);

/**
 ****************************************************************************************
 * @brief Re-boot FW. This function is used to re-boot the FW when error has been
 *        detected, it is the end of the current FW execution. After waiting transfers
 *        on UART to be finished, and storing the information that FW has re-booted by
 *        itself in a non-loaded area, the FW restart by branching at FW entry point.
 * @note When calling this function, the code after it will not be executed.
 * @param[in] error Error detected by FW
 * @return void
 ****************************************************************************************
 */
void platform_reset(uint32_t error);

/**
 ****************************************************************************************
 * @brief Wrapper of the platform reset. It will be invoked before a software reset
 *        is issued from the stack. Possible reasons will be included in the error field.
 * @param[in] error The reason for the reset. It will be one of:
 *            RESET_NO_ERROR,RESET_MEM_ALLOC_FAIL,RESET_TO_ROM,RESET_AND_LOAD_FW
 * @return void
 ****************************************************************************************
 */
void wrap_platform_reset(uint32_t error);

/**
 ****************************************************************************************
 * @brief Checks if system clocks (AHB & APB) can be lowered (weak definition).
 * @return true if system clocks can be lowered, otherwise false
 ****************************************************************************************
 */
bool app_use_lower_clocks_check(void);

/**
 ****************************************************************************************
 * @brief Sets the BLE wake-up delay.
 * @return void
 ****************************************************************************************
 */
void set_sleep_delay(void);

/**
 ****************************************************************************************
 * @brief Set and enable h/w Patch functions.
 * @return void
 ****************************************************************************************
 */
void patch_func(void);

/**
 ****************************************************************************************
 * @brief Starts RCX20 calibration.
 * @param[in] cal_time Calibration time in RCX20 cycles
 * @return void
 ****************************************************************************************
 */
void calibrate_rcx20(uint16_t cal_time);

/**
 ****************************************************************************************
 * @brief Calculates RCX20 frequency.
 * @param[in] cal_time Calibration time in RCX20 cycles
 * @return void
 ****************************************************************************************
 */
void read_rcx_freq(uint16_t cal_time);

/**
 ****************************************************************************************
 * @brief Selects convertion function (XTAL32 or RCX20) for low power cycles to us.
 * @param[in] lpcycles Low power cycles
 * @return uint32_t microseconds
 ****************************************************************************************
 */
uint32_t lld_sleep_lpcycles_2_us_sel_func(uint32_t lpcycles);

/**
 ****************************************************************************************
 * @brief Selects convertion function (XTAL32 or RCX20) for us to low power cycles.
 * @param[in] us microseconds
 * @return uint32_t Low power cycles
 ****************************************************************************************
 */
uint32_t lld_sleep_us_2_lpcycles_sel_func(uint32_t us);

/**
 ****************************************************************************************
 * @brief Runs conditionally (time + temperature) RF and coarse calibration.
 * @return void
 ****************************************************************************************
 */
void conditionally_run_radio_cals(void);

/**
 ****************************************************************************************
 * @brief Check gtl state.
 * @return If gtl is idle returns true. Otherwise returns false.
 ****************************************************************************************
 */
bool check_gtl_state(void);

/**
 ****************************************************************************************
 * @brief Initialise random number generator seed using random bits acquired from TRNG
 * @return void
 ****************************************************************************************
 */
void init_rand_seed_from_trng(void);

/**
 ****************************************************************************************
 * @brief Checks if system is in startup phase.
 * @return 1 if system is in startup phase, otherwise 0.
 ****************************************************************************************
 */
uint8_t check_sys_startup_period(void);

/**
 ****************************************************************************************
 * @brief Set the XTAL16M trim value.
 * @param[in] trim_value Trim value
 * @return void
 ****************************************************************************************
 */
void set_xtal16m_trim_value(uint16_t trim_value);

void dbg_prod_output(int mode, unsigned long *hardfault_args);

/// Assertions showing a critical error that could require a full system reset
#define ASSERT_ERR(cond)

/// Assertions showing a critical error that could require a full system reset
#define ASSERT_INFO(cond, param0, param1)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#define ASSERT_WARN(cond)

// required to define GLOBAL_INT_** macros as inline assembly. This file is included after
// definition of ASSERT macros as they are used inside ll.h
#include "ll.h"     // ll definitions

/// @} DRIVERS

#endif // _ARCH_H_
