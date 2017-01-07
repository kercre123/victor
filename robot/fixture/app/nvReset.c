#include "crc16.h"
#include "nvReset.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "hal/board.h"
#include <string.h>

#define NV_RESET_MAGIC_NUM  0xDEADBEEF

//Data layout. Includes header/footer for internal use.
typedef struct {
  u32 magic_num;
  u8  dat[NV_RESET_MAX_LEN];
  u16 len;
  u16 crc;
} nv_reset_storage_t;

//allocate storage in no-init ram section
static nv_reset_storage_t m_nv_region;// __attribute__( ( section( "NoInit")) );

void nvReset(u8 *dat, u16 len)
{
  nv_reset_storage_t *pram = &m_nv_region;
  memset( pram, 0, sizeof(m_nv_region) );
  
  len = len > NV_RESET_MAX_LEN ? NV_RESET_MAX_LEN : len; //limit len to max data size
  if( dat != NULL && len > 0 )
  {
    memcpy( pram->dat, dat, len );
    pram->len = len;
    pram->crc = crc16_compute( pram->dat, pram->len+2, NULL );  //calculate data+len CRC
    pram->magic_num = NV_RESET_MAGIC_NUM; //mark data as good
  }
  
  NVIC_SystemReset();
  while(1);
}

int nvResetGetLen(void)
{
  nv_reset_storage_t *pram = &m_nv_region;
  
  if( pram->magic_num == NV_RESET_MAGIC_NUM && pram->len <= NV_RESET_MAX_LEN )
    if( pram->crc == crc16_compute( pram->dat, pram->len+2, NULL ) )
      return pram->len;
  
  return -1;
}

int nvResetGet(u8 *out_dat, u16 max_out_len)
{
  nv_reset_storage_t *pram = &m_nv_region;
  int len = nvResetGetLen();
  if( len > 0 && out_dat != NULL )
    memcpy( out_dat, pram->dat, max_out_len < len ? max_out_len : len );
  return len;
}

