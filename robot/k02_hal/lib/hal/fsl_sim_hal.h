/*
* Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
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

#if !defined(__FSL_SIM_HAL_H__)
#define __FSL_SIM_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*! @addtogroup sim_hal*/
/*! @{*/

/*! @file*/

/*******************************************************************************
* Definitions
******************************************************************************/

/*! @brief SIM HAL API return status*/
typedef enum _sim_hal_status {
    kSimHalSuccess,  /*!< Success.      */
    kSimHalFail,     /*!< Error occurs. */
} sim_hal_status_t;

/*******************************************************************************
* API
******************************************************************************/

/*
* Include the CPU-specific clock API header files.
*/
#if (defined(K02F12810_SERIES))
    /* Clock System Level API header file */
    #include "fsl_sim_hal_MK02F12810.h"

#elif (defined(K20D5_SERIES))

#elif (defined(K22F12810_SERIES))

/* Clock System Level API header file */
#include "fsl_sim_hal_MK22F12810.h"


#elif (defined(K22F25612_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK22F25612/fsl_sim_hal_MK22F25612.h"


#elif (defined(K22F51212_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK22F51212/fsl_sim_hal_MK22F51212.h"


#elif (defined(K24F12_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK24F12/fsl_sim_hal_MK24F12.h"

#elif (defined(K24F25612_SERIES))

#include "../src/sim/MK24F25612/fsl_sim_hal_MK24F25612.h"

#elif (defined(K60D10_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK60D10/fsl_sim_hal_MK60D10.h"

#elif (defined(K63F12_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK63F12/fsl_sim_hal_MK63F12.h"

#elif (defined(K64F12_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MK64F12/fsl_sim_hal_MK64F12.h"

#elif (defined(K65F18_SERIES))


#elif (defined(K66F18_SERIES))


#elif (defined(K70F12_SERIES))


#elif (defined(K70F15_SERIES))


#elif (defined(KL02Z4_SERIES))


#elif (defined(KL03Z4_SERIES))
/* Clock System Level API header file */
#include "../src/sim/MKL03Z4/fsl_sim_hal_MKL03Z4.h"



#elif (defined(KL05Z4_SERIES))


#elif (defined(KL13Z4_SERIES))


#elif (defined(KL23Z4_SERIES))


#elif (defined(KL25Z4_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MKL25Z4/fsl_sim_hal_MKL25Z4.h"

#elif (defined(KL26Z4_SERIES))


#elif (defined(KL33Z4_SERIES))


#elif (defined(KL43Z4_SERIES))


#elif (defined(KL46Z4_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MKL46Z4/fsl_sim_hal_MKL46Z4.h"

#elif (defined(KV30F12810_SERIES))
/* Clock System Level API header file */
#include "../src/sim/MKV30F12810/fsl_sim_hal_MKV30F12810.h"

#elif (defined(KV31F12810_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MKV31F12810/fsl_sim_hal_MKV31F12810.h"

#elif (defined(KV31F25612_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MKV31F25612/fsl_sim_hal_MKV31F25612.h"


#elif (defined(KV31F51212_SERIES))

/* Clock System Level API header file */
#include "../src/sim/MKV31F51212/fsl_sim_hal_MKV31F51212.h"

#elif (defined(KV40F15_SERIES))


#elif (defined(KV43F15_SERIES))


#elif (defined(KV44F15_SERIES))


#elif (defined(KV45F15_SERIES))


#elif (defined(KV46F15_SERIES))

#elif (defined(KV10Z7_SERIES))

#include "../src/sim/MKV10Z7/fsl_sim_hal_MKV10Z7.h"

#else
#error "No valid CPU defined!"
#endif

/*! @}*/

#endif /* __FSL_SIM_HAL_H__*/
/*******************************************************************************
* EOF
******************************************************************************/

