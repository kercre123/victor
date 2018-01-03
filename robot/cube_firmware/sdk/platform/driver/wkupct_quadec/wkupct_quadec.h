/**
 ****************************************************************************************
 *
 * @file wucpt_quadec.h
 *
 * @brief Wakeup IRQ & Quadrature Decoder driver header file.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


/* Important note: If, upon reception of interrupt from the wakeup timer or the quadrature
 *                 decoder, the system resumes from sleep mode and you wish to resume peripherals
 *                 functionality , it is necessary to include in your interrupt handler function(s)
 *                 - the ones you register using wkupct_register_callback() and/or
 *                                                quad_decoder_register_callback() -
 *                  the following lines:
 *
 *                    // Init System Power Domain blocks: GPIO, WD Timer, Sys Timer, etc.
 *                    // Power up and init Peripheral Power Domain blocks,
 *                    // and finally release the pad latches.
 *                    if(GetBits16(SYS_STAT_REG, PER_IS_DOWN))
 *                        periph_init();
 *
*/

#ifndef _WKUPCT_QUADEC_H_
#define _WKUPCT_QUADEC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>

/*
 * DEFINES
 ****************************************************************************************
 */

enum
{
    SRC_WKUP_IRQ = 0x01,
    SRC_QUAD_IRQ,
};

enum
{
    WKUPCT_PIN_POLARITY_HIGH = 0,  // the event counter is incremented if the respective input goes HIGH
    WKUPCT_PIN_POLARITY_LOW  = 1,  // the event counter is incremented if the respective input goes LOW
};

/**
 ****************************************************************************************
 * @brief Helper macro for the "pin selection" bitmask expected by function wkupct_enable_irq().
 * @param[in] port    GPIO port
 * @param[in] pin     GPIO pin
 * @return The bitmask value that selects the given GPIO
 * @note When the macro is used with constant arguments an optimizing compiler will collapse it
 *       to a constant value.
 *       If the macro arguments contain an illegal port/pin pair then a compilation error
 *       will be generated.
 ****************************************************************************************
 */
 #define WKUPCT_PIN_SELECT(port, pin) ( 1U << ( port == GPIO_PORT_0 && pin <= GPIO_PIN_7 ? pin :        \
                                                port == GPIO_PORT_1 && pin <= GPIO_PIN_5 ? 8 + pin :    \
                                                port == GPIO_PORT_2 && pin == GPIO_PIN_8 ? 14 :         \
                                                port == GPIO_PORT_2 && pin == GPIO_PIN_9 ? 15 :         \
                                                port == GPIO_PORT_2 && pin <= GPIO_PIN_7 ? 16 + pin :   \
                                                port == GPIO_PORT_3 && pin <= GPIO_PIN_7 ? 24 + pin :   \
                                               -1                                                       \
                                             )                                                          \
                                     )

/**
 ****************************************************************************************
 * @brief Helper macro for the "pin polarity" bitmask expected by function wkupct_enable_irq().
 * @param[in] port        GPIO port
 * @param[in] pin         GPIO pin
 * @param[in] polarity    The required polarity. Can be either WKUPCT_PIN_POLARITY_HIGH
 *                        or  WKUPCT_PIN_POLARITY_LOW.
 * @return The corresponding bitmask for the given GPIO's polarity.
 * @note When the macro is used with constant arguments an optimizing compiler will collapse it
 *       to a constant value.
 *       If the macro arguments contain an illegal port/pin pair then a compilation error
 *       will be generated.
 ****************************************************************************************
 */
#define WKUPCT_PIN_POLARITY(port, pin, polarity) ( (polarity) << ( port == GPIO_PORT_0 && pin <= GPIO_PIN_7 ? pin :          \
                                                                   port == GPIO_PORT_1 && pin <= GPIO_PIN_5 ? 8 + pin :      \
                                                                   port == GPIO_PORT_2 && pin == GPIO_PIN_8 ? 14 :           \
                                                                   port == GPIO_PORT_2 && pin == GPIO_PIN_9 ? 15 :           \
                                                                   port == GPIO_PORT_2 && pin <= GPIO_PIN_7 ? 16 + pin :     \
                                                                   port == GPIO_PORT_3 && pin <= GPIO_PIN_7 ? 24 + pin :     \
                                                                   -1                                                        \
                                                                 )                                                           \
                                                 )

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef enum
{
    WKUPCT_QUADEC_ERR_RESERVED = -1,
    WKUPCT_QUADEC_ERR_OK = 0,
} wkupct_quadec_error_t;

typedef enum
{
    RESERVATION_STATUS_FREE = 0,
    RESERVATION_STATUS_RESERVED_BY_UNIT_A,
    RESERVATION_STATUS_RESERVED_BY_UNIT_B,
    RESERVATION_STATUS_RESERVED_BY_BOTH_UNITS,
} reservation_status_t;

typedef union
{
    struct
    {
     bool reserved_by_wkupct:1;
     bool reserved_by_quad:1;
    } reservations;
    reservation_status_t reservation_status;
} wkupct_quad_IRQ_status_t;

typedef void (*wakeup_handler_function_t)(void);

typedef void (*quad_encoder_handler_function_t)(int16_t qdec_xcnt_reg, int16_t qdec_ycnt_reg, int16_t qdec_zcnt_reg);

typedef enum
{
    QUAD_DEC_CHXA_NONE_AND_CHXB_NONE = 0,
    QUAD_DEC_CHXA_P00_AND_CHXB_P01 = 1,
    QUAD_DEC_CHXA_P02_AND_CHXB_P03 = 2,
    QUAD_DEC_CHXA_P04_AND_CHXB_P05 = 3,
    QUAD_DEC_CHXA_P06_AND_CHXB_P07 = 4,
    QUAD_DEC_CHXA_P10_AND_CHXB_P11 = 5,
    QUAD_DEC_CHXA_P12_AND_CHXB_P13 = 6,
    QUAD_DEC_CHXA_P23_AND_CHXB_P24 = 7,
    QUAD_DEC_CHXA_P25_AND_CHXB_P26 = 8,
    QUAD_DEC_CHXA_P27_AND_CHXB_P28 = 9,
    QUAD_DEC_CHXA_P29_AND_CHXB_P20 = 10
} CHX_PORT_SEL_t;

typedef enum
{
    QUAD_DEC_CHYA_NONE_AND_CHYB_NONE = 0<<4,
    QUAD_DEC_CHYA_P00_AND_CHYB_P01 = 1<<4,
    QUAD_DEC_CHYA_P02_AND_CHYB_P03 = 2<<4,
    QUAD_DEC_CHYA_P04_AND_CHYB_P05 = 3<<4,
    QUAD_DEC_CHYA_P06_AND_CHYB_P07 = 4<<4,
    QUAD_DEC_CHYA_P10_AND_CHYB_P11 = 5<<4,
    QUAD_DEC_CHYA_P12_AND_CHYB_P13 = 6<<4,
    QUAD_DEC_CHYA_P23_AND_CHYB_P24 = 7<<4,
    QUAD_DEC_CHYA_P25_AND_CHYB_P26 = 8<<4,
    QUAD_DEC_CHYA_P27_AND_CHYB_P28 = 9<<4,
    QUAD_DEC_CHYA_P29_AND_CHYB_P20 = 10<<4
} CHY_PORT_SEL_t;

typedef enum
{
    QUAD_DEC_CHZA_NONE_AND_CHZB_NONE = 0<<8,
    QUAD_DEC_CHZA_P00_AND_CHZB_P01 = 1<<8,
    QUAD_DEC_CHZA_P02_AND_CHZB_P03 = 2<<8,
    QUAD_DEC_CHZA_P04_AND_CHZB_P05 = 3<<8,
    QUAD_DEC_CHZA_P06_AND_CHZB_P07 = 4<<8,
    QUAD_DEC_CHZA_P10_AND_CHZB_P11 = 5<<8,
    QUAD_DEC_CHZA_P12_AND_CHZB_P13 = 6<<8,
    QUAD_DEC_CHZA_P23_AND_CHZB_P24 = 7<<8,
    QUAD_DEC_CHZA_P25_AND_CHZB_P26 = 8<<8,
    QUAD_DEC_CHZA_P27_AND_CHZB_P28 = 9<<8,
    QUAD_DEC_CHZA_P29_AND_CHZB_P20 = 10<<8
} CHZ_PORT_SEL_t;

typedef struct
{
    CHX_PORT_SEL_t chx_port_sel;
    CHY_PORT_SEL_t chy_port_sel;
    CHZ_PORT_SEL_t chz_port_sel;
    uint16_t qdec_clockdiv;
    uint8_t qdec_events_count_to_trigger_interrupt;
} QUAD_DEC_INIT_PARAMS_t;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Internal function to disable WKUP_QUADEC_IRQn only when both Quadrature
 * Decoder and Wakeup Timer have unregistered from using it.
 * @return WKUPCT_QUADEC_ERR_RESERVED, WKUPCT_QUADEC_ERR_OK
 ****************************************************************************************
 */
wkupct_quadec_error_t wkupct_quad_disable_IRQ(void);

/**
 ****************************************************************************************
 * @brief WKUPCT IRQ Handler.
 * @return void
 ****************************************************************************************
 */
void WKUP_QUADEC_Handler(void);

/**
 ****************************************************************************************
 * @brief Enable Wakeup IRQ.
 * @param[in] sel_pins      Select enabled inputs (0-disabled, 1-enabled)
 *                          - Bits 0-7   -> port 0(P00..P07)
 *                          - Bits 8-13  -> port 1(P10..P15)
 *                          - Bits 14-15 -> port 2(P28,P29)
 *                          - Bits 16-23 -> port 2(P00..P07)
 *                          - Bits 24-31 -> port 3(P30..P37)
 * @param[in] pol_pins      Inputs' polarity (0-high, 1-low)
 *                          - Bits 0-7   -> port 0(P00..P07)
 *                          - Bits 8-13  -> port 1(P10..P15)
 *                          - Bits 14-15 -> port 2(P28,P29)
 *                          - Bits 16-23 -> port 2(P00..P07
 *                          - Bits 24-31 -> port 3(P30..P37)
 * @param[in] events_num    Number of events before wakeup interrupt. Max 255.
 * @param[in] deb_time      Debouncing time. Max 0x3F (63 msec)
 * @return void
 ****************************************************************************************
 */
void wkupct_enable_irq(uint32_t sel_pins, uint32_t pol_pins, uint16_t events_num, uint16_t deb_time);

/**
 ****************************************************************************************
 * @brief Unregisters WKUP_QUADEC_IRQn use by Wakeup Timer and calls
 * wkupct_quad_disable_IRQ.
 * @note WKUP_QUADEC_IRQn interrupt will be disabled, only if also the Quadrature Decoder
 * has unregistered from usin the IRQ.
 * @return WKUPCT_QUADEC_ERR_OK, WKUPCT_QUADEC_ERR_RESERVED
 ****************************************************************************************
 */
wkupct_quadec_error_t wkupct_disable_irq(void);

/**
 ****************************************************************************************
 * @brief Register Callback function for Wakeup IRQ.
 * @param[in] callback Callback function's reference
 * @return void
 ****************************************************************************************
 */
void wkupct_register_callback(wakeup_handler_function_t callback);

/**
 ****************************************************************************************
 * @brief Register Callback function for Quadrature Decoder IRQ.
 * @param[in] callback Callback function's reference.
 * @return void
 ****************************************************************************************
 */
void quad_decoder_register_callback(uint32_t* callback);

/**
 ****************************************************************************************
 * @brief Initialize Quadrature Decoder.
 * @param[in] quad_dec_init_params Pointer to QUAD_DEC_INIT_PARAMS_t structure
 * @return void
 ****************************************************************************************
 */
void quad_decoder_init(QUAD_DEC_INIT_PARAMS_t *quad_dec_init_params);

/**
 ****************************************************************************************
 * @brief Release Quadrature Decoder.
 * @return void
 ****************************************************************************************
 */
void quad_decoder_release(void);

/**
 ****************************************************************************************
 * @brief Get Quadrature Decoder X counter.
 * @return counter value
 ****************************************************************************************
 */
int16_t quad_decoder_get_x_counter(void);

/**
 ****************************************************************************************
 * @brief Get Quadrature Decoder Y counter.
 * @return counter value
 ****************************************************************************************
 */
int16_t quad_decoder_get_y_counter(void);

/**
 ****************************************************************************************
 * @brief Get Quadrature Decoder Z counter.
 * @return counter value
 ****************************************************************************************
 */
int16_t quad_decoder_get_z_counter(void);

/**
 ****************************************************************************************
 * @brief Enable Quadrature Decoder interrupt.
 * @param[in] event_count event counter
 * @return void
 ****************************************************************************************
 */
void quad_decoder_enable_irq(uint8_t event_count);

/**
 ****************************************************************************************
 * @brief Unregisters WKUP_QUADEC_IRQn use by Quadrature Decoder and calls
 * wkupct_quad_disable_IRQ.
 * @note WKUP_QUADEC_IRQn interrupt will be disabled, only if also the Wakeup Timer has
 * unregisterd from usin the IRQ.
 * @return WKUPCT_QUADEC_ERR_OK, WKUPCT_QUADEC_ERR_RESERVED
 ****************************************************************************************
 */
wkupct_quadec_error_t quad_decoder_disable_irq(void);

#endif // _WKUPCT_QUADEC_H_
