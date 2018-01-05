#include <assert.h>
#include <string.h>
#include "board.h"
#include "crc16.h"
#include "nvReset.h"
#include "portable.h"
#include "timer.h"

#define DBG_ENABLE  0
#if DBG_ENABLE > 0
#include "console.h"
#endif

#define NV_RESET_MAGIC_NUM  0xDEADBEEF

//MCU Backup SRAM info
static const uint32_t nv_ram_addr = BKPSRAM_BASE; /*(AHB1PERIPH_BASE + 0x4000) -> 0x40024000*/
static const uint32_t nv_ram_size = 0x1000;       /*4k*/

//Data layout. Includes header/footer for internal use.
typedef struct {
  u32 magic_num;
  u8  dat[NV_RESET_MAX_LEN];
  u16 len;
  u16 crc;
} nv_reset_storage_t;

static void _enable_backup_sram(void)
{
  assert( NV_RESET_MAX_LEN % 4 == 0 ); //enforce word alignment
  assert( sizeof(nv_reset_storage_t) <= nv_ram_size );
  
  //Access to the backup SRAM:
  //1. Enable the power interface clock by setting the PWREN bits in the RCC APB1 peripheral clock enable register (RCC_APB1ENR)
  //2. Set the DBP bit in the PWR power control register (PWR_CR) to enable access to the backup domain
  //3. Enable the backup SRAM clock by setting BKPSRAMEN bit in the RCC AHB1 peripheral clock register (RCC_AHB1ENR)
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_BKPSRAMEN, ENABLE);
  
  //only need this to retain backup sram from VBAT
  //RCC_AHB1PeriphClockLPModeCmd(RCC_AHB1LPENR_BKPSRAMLPEN, ENABLE);
}

void nvReset(u8 *dat, u16 len)
{
  _enable_backup_sram();
  nv_reset_storage_t *pram = (nv_reset_storage_t*)nv_ram_addr;
  memset( pram, 0, sizeof(nv_reset_storage_t) ); //clear nv ram
  
  len = len > NV_RESET_MAX_LEN ? NV_RESET_MAX_LEN : len; //limit len to max data size
  if( dat != NULL && len > 0 ) {
    memcpy( pram->dat, dat, len );
    pram->len = len;
    pram->crc = crc16_compute( pram->dat, pram->len, NULL );  //calculate data CRC
    pram->magic_num = NV_RESET_MAGIC_NUM; //mark data as good
  }
  
  NVIC_SystemReset();
  while(1);
}

int nvResetGetLen(void)
{
  _enable_backup_sram();
  nv_reset_storage_t *pram = (nv_reset_storage_t*)nv_ram_addr;
  
  if( pram->magic_num == NV_RESET_MAGIC_NUM && pram->len <= NV_RESET_MAX_LEN )
    if( pram->crc == crc16_compute( pram->dat, pram->len, NULL ) )
      return pram->len;
  
  return -1;
}

int nvResetGet(u8 *out_dat, u16 max_out_len)
{
  nv_reset_storage_t *pram = (nv_reset_storage_t*)nv_ram_addr;
  int len = nvResetGetLen();
  if( len > 0 && out_dat != NULL )
    memcpy( out_dat, pram->dat, max_out_len < len ? max_out_len : len );
  return len;
}

#if DBG_ENABLE > 0
void nvResetDbgInspect(char* prefix, u8 *dat, u16 len)
{
  int dlen = len;
  if( dat == NULL ) {
    nv_reset_storage_t *pram = (nv_reset_storage_t*)nv_ram_addr;
    dat = pram->dat;
    dlen = nvResetGetLen();
  }
  
  //ConsolePrintf("nvResetInspect: ");
  if( prefix != NULL )
    ConsoleWrite( prefix );
  
  for( int i=0; i < dlen; i++) {
    if( i>0 && i%4==0 )
      ConsolePrintf("-");
    ConsolePrintf("%02x", dat[i]);
  }
  ConsolePrintf(" (%i)\n", dlen);
}
#endif

#if DBG_ENABLE > 0
void nvResetDbgInspectMemRegion(void)
{
  nv_reset_storage_t *pram = (nv_reset_storage_t*)nv_ram_addr;
  ConsolePrintf("nvRam address 0x%08x\n", nv_ram_addr);
  ConsolePrintf(".magic num: 0x%08x\n", pram->magic_num );
  ConsolePrintf(".len: 0x%04x (%i)\n", pram->len, pram->len );
  ConsolePrintf(".crc: 0x%04x\n", pram->crc );
  ConsolePrintf(".dat: ");
  for( int i=0; i < NV_RESET_MAX_LEN; i++) {
    if( i>0 && i%4==0 )
      ConsolePrintf("-");
    ConsolePrintf("%02x", pram->dat[i]);
  }
  ConsolePrintf("\n");
}
#endif

