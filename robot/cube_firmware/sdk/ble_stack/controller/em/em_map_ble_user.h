#ifndef EM_MAP_BLE_USER_H_
#define EM_MAP_BLE_USER_H_

#include "rwip_config.h"                // stack configuration

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "co_bt.h"
#include "co_buf.h"

#include "rwble_hl_config.h"
#include "da1458x_scatter_config.h"

/// number of control structure entries for the exchange table
//#define BLE_CONNECTION_MAX_USER		 4

///EXCHANGE MEMORY
#define EM_BLE_CS_COUNT_USER            (BLE_CONNECTION_MAX_USER + 1)
#define BLE_WHITELIST_MAX_USER          (BLE_CONNECTION_MAX_USER + 2)
#define EM_BLE_TXE_COUNT_USER           (BLE_CONNECTION_MAX_USER)


#if (BLE_CONNECTION_MAX_USER == 1)
#define BLE_TX_BUFFER_DATA_USER         (5)
#else
#define BLE_TX_BUFFER_DATA_USER         (BLE_CONNECTION_MAX_USER * 3)
#endif 

#define BLE_TX_BUFFER_ADV_USER          (3)

#define BLE_TX_BUFFER_CNTL_USER         (BLE_CONNECTION_MAX_USER)

/// Total number of elements in the TX buffer pool
#define BLE_TX_BUFFER_CNT_USER          (BLE_TX_BUFFER_DATA_USER + BLE_TX_BUFFER_CNTL_USER + BLE_TX_BUFFER_ADV_USER)


/// Number of receive buffers in the RX ring. This number defines the interrupt
/// rate during a connection event. An interrupt is asserted every BLE_RX_BUFFER_CNT/2
/// reception. This number has an impact on the size of the exchange memory. This number
/// may have to be increased when CPU is very slow to free the received data, in order not
/// to overflow the RX ring of buffers.
#define BLE_RX_BUFFER_CNT_USER          (8)


///HEAP
#if (BLE_CONNECTION_MAX_USER == 1)
#define RWIP_HEAP_NON_RET_SIZE_USER     1024
#else
#define RWIP_HEAP_NON_RET_SIZE_USER     2048 //(1024 * BLE_CONNECTION_MAX_USER)
#endif

#define RWIP_HEAP_ENV_SIZE_USER         ((BLE_HEAP_ENV_SIZE + BLEHL_HEAP_ENV_SIZE) * BLE_CONNECTION_MAX_USER)

#define BLE_HEAP_MSG_SIZE_USER 			(256 * (BLE_CONNECTION_MAX_USER+1) + 96 * (2*BLE_CONNECTION_MAX_USER+1))
#define BLEHL_HEAP_MSG_SIZE_USER		(256 + 256 * BLE_CONNECTION_MAX_USER)
#define RWIP_HEAP_MSG_SIZE_USER			(BLE_HEAP_MSG_SIZE_USER  +  BLEHL_HEAP_MSG_SIZE_USER)

#endif // EM_MAP_BLE_USER_H_
