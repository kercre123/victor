/**
 ****************************************************************************************
 *
 * @file em_map_ble.h
 *
 * @brief Mapping of the different descriptors in the exchange memory
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef EM_MAP_BLE_H_
#define EM_MAP_BLE_H_

/**
 ****************************************************************************************
 * @addtogroup EM EM
 * @ingroup CONTROLLER
 * @brief Mapping of the different descriptors in the exchange memory
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"                // stack configuration
#include "em_map_ble_user.h"

#if (BT_DUAL_MODE)
#include "em_map_bt.h"
#else //BT_DUAL_MODE
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "co_bt.h"
#include "co_buf.h"
#include "_reg_ble_em_tx.h"
#include "_reg_ble_em_txe.h"
#include "_reg_ble_em_rx.h"
#include "_reg_ble_em_wpb.h"
#include "_reg_ble_em_wpv.h"
#include "_reg_ble_em_et.h"
#include "_reg_ble_em_cs.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/// number of entries for the exchange table
#define EM_BLE_EXCH_TABLE_LEN      16

/// number of frequencies / Depends on RF target
#if defined(CFG_RF_ATLAS)
  /// IcyTRx requires 40 x 32-bit words for Frequency table + 40 byte for VCO sub-band table
  #define EM_BLE_FREQ_TABLE_LEN  160
  #define EM_BLE_VCO_TABLE_LEN 40
#else
  /// Ripple/ExtRC requires 40 x 8-bit words for Frequency table / No VCO sub-band table
#define EM_BLE_FREQ_TABLE_LEN  40
  #define EM_BLE_VCO_TABLE_LEN 0
#endif

/// Size of the encryption area
#define EM_BLE_ENC_LEN         16

/// number of control structure entries for the exchange table
#define EM_BLE_CS_COUNT        (BLE_CONNECTION_MAX + 1)

/// number of control structure entries for the exchange table
#define EM_BLE_TXE_COUNT       (BLE_CONNECTION_MAX)

/// Status of the control structure in the exchange table
enum
{
    EM_BLE_ET_READY,
    EM_BLE_ET_PROCESSED,
    EM_BLE_ET_RESERVED,
    EM_BLE_ET_BYPASSED
};

/*
 * Mapping of the different elements in EM
 ****************************************************************************************
 */
/// Offset of the exchange table
#define EM_BLE_ET_OFFSET         0
/// Offset of the frequency table
#define EM_BLE_FT_OFFSET         (EM_BLE_ET_OFFSET + EM_BLE_EXCH_TABLE_LEN * REG_BLE_EM_ET_SIZE)
/// Offset of the plain data area (used for SW initiated encryption)
#define EM_BLE_ENC_PLAIN_OFFSET  (EM_BLE_FT_OFFSET + (EM_BLE_VCO_TABLE_LEN + EM_BLE_FREQ_TABLE_LEN) * sizeof(uint8_t))
/// Offset of the ciphered data area (used for SW initiated encryption)
#define EM_BLE_ENC_CIPHER_OFFSET (EM_BLE_ENC_PLAIN_OFFSET + EM_BLE_ENC_LEN * sizeof(uint8_t))
/// Offset of the control structure area
#ifndef __DA14581__
#if (BLE_CONNECTION_MAX_USER <= 4)
#define EM_BLE_CS_OFFSET         (EM_BLE_ENC_CIPHER_OFFSET + EM_BLE_ENC_LEN * sizeof(uint8_t))
#else
extern uint8_t cs_area$$Base[];
#define EM_BLE_CS_OFFSET         (cs_area$$Base - 0x80000)
#endif
#else
#define EM_BLE_CS_OFFSET         (EM_BLE_ENC_CIPHER_OFFSET + EM_BLE_ENC_LEN * sizeof(uint8_t))
#endif


/// Offset of the public white list area
#ifndef __DA14581__
#if (BLE_CONNECTION_MAX_USER <= 4)
#define EM_BLE_WPB_OFFSET        (EM_BLE_CS_OFFSET + EM_BLE_CS_COUNT_USER * REG_BLE_EM_CS_SIZE)
#else
#define EM_BLE_WPB_OFFSET        (EM_BLE_ENC_CIPHER_OFFSET + EM_BLE_ENC_LEN * sizeof(uint8_t))
#endif
/// Offset of the private white list area
#define EM_BLE_WPV_OFFSET        (EM_BLE_WPB_OFFSET + BLE_WHITELIST_MAX_USER * REG_BLE_EM_WPB_SIZE)  
/// Offset of the Connection Address
#define EM_BLE_CNXADD_OFFSET     (EM_BLE_WPV_OFFSET + BLE_WHITELIST_MAX_USER * REG_BLE_EM_WPV_SIZE)
/// Offset of the empty TX buffer area
#define EM_BLE_TXE_OFFSET        (EM_BLE_CNXADD_OFFSET + BD_ADDR_LEN) 
/// End of EM ret.part
#else	//__DA14581__
/// Offset of the public white list area
#define EM_BLE_WPB_OFFSET        (EM_BLE_CS_OFFSET + EM_BLE_CS_COUNT * REG_BLE_EM_CS_SIZE)
/// Offset of the private white list area
#define EM_BLE_WPV_OFFSET        (EM_BLE_WPB_OFFSET + BLE_CONNECTION_MAX + 2 * REG_BLE_EM_WPB_SIZE)  
/// Offset of the Connection Address
#define EM_BLE_CNXADD_OFFSET     (EM_BLE_WPV_OFFSET + BLE_CONNECTION_MAX + 2 * REG_BLE_EM_WPV_SIZE)
/// Offset of the empty TX buffer area
#define EM_BLE_TXE_OFFSET        (EM_BLE_CNXADD_OFFSET + BD_ADDR_LEN) 
/// End of EM ret.part
#endif	//__DA14581__
#define DUMMY_SIZE               (EM_BLE_TXE_OFFSET + EM_BLE_TXE_COUNT * REG_BLE_EM_TXE_SIZE) //(EM_BLE_RF_SPI_OFFSET) 

#ifdef __DA14581__
#if (BLE_CONNECTION_MAX_USER > 1)
extern uint8_t cs_area$$Base[];
#undef EM_BLE_WPB_OFFSET
#undef EM_BLE_WPV_OFFSET
#define EM_BLE_WPB_OFFSET        (cs_area$$Base - 0x80000)
#define EM_BLE_WPV_OFFSET        (EM_BLE_WPB_OFFSET + (BLE_CONNECTION_MAX + 2) * REG_BLE_EM_WPB_SIZE)  
#endif
#endif

#define EM_580_TX_BUFFER_SZ     (BLE_TX_BUFFER_CNT * REG_BLE_EM_TX_SIZE)
#define EM_580_RX_BUFFER_SZ     (BLE_RX_BUFFER_CNT * REG_BLE_EM_RX_SIZE)
#define EM_580_TOTAL_BUFFER_SZ  (EM_580_TX_BUFFER_SZ + EM_580_RX_BUFFER_SZ)

#ifndef __DA14581__
#if (EM_580_TOTAL_BUFFER_SZ > (0x2000 - 0x768) )
#error "Not enough memory!"
#endif
#else
#if (EM_580_TOTAL_BUFFER_SZ > (0x2000 - 0x908) )
#error "Not enough memory!"
#endif
#endif

#if ( (REINIT_DESCRIPT_BUF == 0) && (USE_MEMORY_MAP != EXT_SLEEP_SETUP) ) 
                                //---- Setup DS_x without re-init of descript ----
#define OFFSET_BUFFERS          ((0x2000 - EM_580_TOTAL_BUFFER_SZ) & ~3)//BEGINNING OF BUFFERS IS SET AT THE END OF RetRAM

#else                           //---- Setup ES, DS_x with re-init of descript ----
#define OFFSET_BUFFERS          ((0x3000 - EM_580_TOTAL_BUFFER_SZ) & ~3)//BEGINNING OF BUFFERS IS SET AT THE END OF "Extended RetRAM"
#endif


/// Offset of the TX buffer area
#define EM_BLE_TX_OFFSET        (OFFSET_BUFFERS)

/// Offset of the RX buffer area
#define EM_BLE_RX_OFFSET        (EM_BLE_TX_OFFSET + BLE_TX_BUFFER_CNT * REG_BLE_EM_TX_SIZE)

#define EM_SYSMEM_START			(0x80000 + EM_BLE_TX_OFFSET)                                //ABSOLUTE ADDRESS OF EM IN RAM

#define EM_SYSMEM_END			(EM_BLE_RX_OFFSET + BLE_RX_BUFFER_CNT * REG_BLE_EM_RX_SIZE) //RELATIVE END ADDRESS OF EM IN RAM

#define EM_SYSMEM_SIZE          (EM_SYSMEM_END - OFFSET_BUFFERS)                            //SIZE OF EM IN SYSRAM

/*
 * HEAPS
 ******************************/
///Not retained HEAP
#ifndef NON_RET_HEAP_SZ
#define RWIP_HEAP_NON_RET_SIZE_JT	   	RWIP_HEAP_NON_RET_SIZE_USER //IN BYTES
#else
#define RWIP_HEAP_NON_RET_SIZE_JT	   	NON_RET_HEAP_SZ             //IN BYTES
#endif

///Env, DB and Msg HEAPs
#ifndef ENV_HEAP_SZ
#define RWIP_HEAP_ENV_SIZE_JT	       	RWIP_HEAP_ENV_SIZE_USER 	//IN BYTES	
#else
#define RWIP_HEAP_ENV_SIZE_JT	       	ENV_HEAP_SZ 	            //IN BYTES	
#endif

#ifndef DB_HEAP_SZ
#define RWIP_HEAP_DB_SIZE_JT			1024                        //IN BYTES
#else
#define RWIP_HEAP_DB_SIZE_JT			DB_HEAP_SZ                  //IN BYTES
#endif

#ifndef MSG_HEAP_SZ
#define RWIP_HEAP_MSG_SIZE_JT			RWIP_HEAP_MSG_SIZE_USER     //IN BYTES	
#else
#define RWIP_HEAP_MSG_SIZE_JT			MSG_HEAP_SZ                 //IN BYTES	
#endif

#endif //BT_DUAL_MODE
/// @} LLDEXMEM

#endif // EM_MAP_BLE_H_
