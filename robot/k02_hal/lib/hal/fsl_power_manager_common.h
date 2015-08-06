/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __FSL_POWER_MANAGER_COMMON_H__
#define __FSL_POWER_MANAGER_COMMON_H__

#include "fsl_llwu_hal.h"
#include "fsl_gpio_driver.h"

/*!
 * @addtogroup power_manager
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define POWER_MAKE_WAKEUP_PIN(llwuPin,port,pin) ( llwuPin | ( GPIO_MAKE_PIN(port,pin)<<16U) )
#define POWER_GPIO_RESERVED 0xFFFFU
#define POWER_EXTRACT_LLWU_PIN(pin) (llwu_wakeup_pin_t)( ((uint32_t)pin)&0xFFU)
#define POWER_EXTRACT_GPIO_PINNAME(pin) ((((uint32_t)pin)>>16)&0xFFFFU)
/*
 * Include the cpu specific API.
 */
/*!
 * @brief Power manager internal wake up modules.
 */
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE
typedef enum _power_wakeup_module
{
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE0
    kPowerManagerWakeupLptmr      = kLlwuWakeupModule0,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE1
    kPowerManagerWakeupCmp0       = kLlwuWakeupModule1,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE2
    kPowerManagerWakeupCmp1       = kLlwuWakeupModule2,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE3
    kPowerManagerWakeupCmp2       = kLlwuWakeupModule3,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE4
    kPowerManagerWakeupTsi        = kLlwuWakeupModule4,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE5
    kPowerManagerWakeupRtcAlarm   = kLlwuWakeupModule5,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE6
    kPowerManagerWakeupDryIce     = kLlwuWakeupModule6,
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE7
    kPowerManagerWakeupRtcSeconds = kLlwuWakeupModule7,
#endif
    kPowerManagerWakeupMax        = FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE
} power_wakeup_module_t;

#endif

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
/*!
 * @brief Power manager external wake up pins.
 */
#if defined(KL03Z4_SERIES)

typedef enum _power_wakeup_pin
{
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    kPowerManagerWakeupPin4 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin4,HW_GPIOB,0),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    kPowerManagerWakeupPin7 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin7,HW_GPIOA,0),
#endif
} power_wakeup_pin_t;

#else
typedef enum _power_wakeup_pin
{
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0
    kPowerManagerWakeupPin0 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin0,HW_GPIOE,1),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1
    kPowerManagerWakeupPin1 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin1,HW_GPIOE,2),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2
    kPowerManagerWakeupPin2 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin2,HW_GPIOE,4),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3
    kPowerManagerWakeupPin3 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin3,HW_GPIOA,4),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    kPowerManagerWakeupPin4 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin4,HW_GPIOA,13),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5
    kPowerManagerWakeupPin5 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin5,HW_GPIOB,0),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6
    kPowerManagerWakeupPin6 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin6,HW_GPIOC,1),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    kPowerManagerWakeupPin7 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin7,HW_GPIOC,3),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8
    kPowerManagerWakeupPin8 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin8,HW_GPIOC,4),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9
    kPowerManagerWakeupPin9 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin9,HW_GPIOC,5),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10
    kPowerManagerWakeupPin10 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin10,HW_GPIOC,6),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11
    kPowerManagerWakeupPin11 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin11,HW_GPIOC,11),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12
    kPowerManagerWakeupPin12 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin12,HW_GPIOD,0),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13
    kPowerManagerWakeupPin13 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin13,HW_GPIOD,2),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14
    kPowerManagerWakeupPin14 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin14,HW_GPIOD,4),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15
    kPowerManagerWakeupPin15 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin15,HW_GPIOD,6),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16
    kPowerManagerWakeupPin16 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin16,HW_GPIOE,6),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17
    kPowerManagerWakeupPin17 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin17,HW_GPIOE,9),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18
    kPowerManagerWakeupPin18 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin18,HW_GPIOE,10),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19
    kPowerManagerWakeupPin19 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin19,HW_GPIOE,17),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20
    kPowerManagerWakeupPin20 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin20,HW_GPIOE,18),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21
    kPowerManagerWakeupPin21 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin21,HW_GPIOE,25),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22
    kPowerManagerWakeupPin22 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin22,HW_GPIOA,10),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23
    kPowerManagerWakeupPin23 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin23,HW_GPIOA,11),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24
    kPowerManagerWakeupPin24 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin24,HW_GPIOD,8),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25
    kPowerManagerWakeupPin25 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin25,HW_GPIOD,11),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26
    kPowerManagerWakeupPin26 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin26,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27
    kPowerManagerWakeupPin27 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin27,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28
    kPowerManagerWakeupPin28 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin28,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29
    kPowerManagerWakeupPin29 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin29,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30
    kPowerManagerWakeupPin30 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin30,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31
    kPowerManagerWakeupPin31 = POWER_MAKE_WAKEUP_PIN(kLlwuWakeupPin31,POWER_GPIO_RESERVED,POWER_GPIO_RESERVED),
#endif
}power_wakeup_pin_t;

#endif

#endif
/*! @}*/

#endif /* __FSL_POWER_MANAGER_COMMON_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

