/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic
 * Semiconductor ASA.Terms and conditions of usage are described in detail
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 * $LastChangedRevision: 230 $
 */

/** @file
 * @brief Implementation of hal_aes for nRF24LU1+
 *
 * Implementation of hardware abstraction layer (HAL) for the embedded
 * AES co-processor in nRF24LU1
 *
 */

#include "nrf24lu1p.h"
#include <stdint.h>
#include "hal_aes.h"


#define AES_BUF_SIZE                16

//-----------------------------------------------------------------------------
// AES register masks
//-----------------------------------------------------------------------------

#define AESCS_MODE_MASK             0x1C
#define AESCS_GO_MASK               0x01

#define AESCS_E_D_MASK              0x02
#define AESCS_E_D_BIT_POS           1
#define DECRYPT                     1
#define ENCRYPT                     0

#define AESIA1_KIN_MASK             0xf0
#define AESIA1_KIN_B0_POS           4

#define AESIA1_IV_MASK              0x0f
#define AESIA1_IV_B0_POS            0

#define AESIA2_DI_MASK              0xf0
#define AESIA2_DI_B0_POS            4

#define AESIA2_DO_MASK              0x0f
#define AESIA2_DO_B0_POS            0

//-----------------------------------------------------------------------------
// AES substitution table (used by aes_get_dec_key() only)
//-----------------------------------------------------------------------------

static code const uint8_t S[256] = {
 99, 124, 119, 123, 242, 107, 111, 197,  48,   1, 103,  43, 254, 215, 171, 118,
202, 130, 201, 125, 250,  89,  71, 240, 173, 212, 162, 175, 156, 164, 114, 192,
183, 253, 147,  38,  54,  63, 247, 204,  52, 165, 229, 241, 113, 216,  49,  21,
  4, 199,  35, 195,  24, 150,   5, 154,   7,  18, 128, 226, 235,  39, 178, 117,
  9, 131,  44,  26,  27, 110,  90, 160,  82,  59, 214, 179,  41, 227,  47, 132,
 83, 209,   0, 237,  32, 252, 177,  91, 106, 203, 190,  57,  74,  76,  88, 207,
208, 239, 170, 251,  67,  77,  51, 133,  69, 249,   2, 127,  80,  60, 159, 168,
 81, 163,  64, 143, 146, 157,  56, 245, 188, 182, 218,  33,  16, 255, 243, 210,
205,  12,  19, 236,  95, 151,  68,  23, 196, 167, 126,  61, 100,  93,  25, 115,
 96, 129,  79, 220,  34,  42, 144, 136,  70, 238, 184,  20, 222,  94,  11, 219,
224,  50,  58,  10,  73,   6,  36,  92, 194, 211, 172,  98, 145, 149, 228, 121,
231, 200,  55, 109, 141, 213,  78, 169, 108,  86, 244, 234, 101, 122, 174,   8,
186, 120,  37,  46,  28, 166, 180, 198, 232, 221, 116,  31,  75, 189, 139, 138,
112,  62, 181, 102,  72,   3, 246,  14,  97,  53,  87, 185, 134, 193,  29, 158,
225, 248, 152,  17, 105, 217, 142, 148, 155,  30, 135, 233, 206,  85,  40, 223,
140, 161, 137,  13, 191, 230,  66, 104,  65, 153,  45,  15, 176,  84, 187,  22,
};

//-----------------------------------------------------------------------------
// Internal function prototypes
//-----------------------------------------------------------------------------

void aes_set_mode(uint8_t mode);
void aes_select_e_d(uint8_t operation);
void aes_go();
uint8_t aes_busy();

void aes_data_write_buf(uint8_t *buf, uint8_t indirect_start_address, uint8_t length);
void aes_data_read_buf(uint8_t *buf, uint8_t indirect_start_address, uint8_t length);
void aes_keyin_write_buf(const uint8_t *buf, uint8_t indirect_address, uint8_t length);
void aes_initvect_write_buf(const uint8_t *buf, uint8_t indirect_start_address, uint8_t length);

//-----------------------------------------------------------------------------
// Function implementations
//-----------------------------------------------------------------------------

void hal_aes_setup(bool decrypt, aes_modes_t mode, uint8_t *keyin, uint8_t *ivin)
{
   if(keyin != NULL)
   {
      aes_keyin_write_buf(keyin, 0, AES_BUF_SIZE);
   }

   if(ivin)
   {
      aes_initvect_write_buf(ivin, 0, AES_BUF_SIZE);
      aes_set_mode(ECB);            // Dummy change of mode in order to load init-vector
      aes_set_mode(CBC);
   }

   if(decrypt)
   {
     AESCS=(AESCS & ~AESCS_E_D_MASK) | DECRYPT<<AESCS_E_D_BIT_POS;
   }
   else
   {
     AESCS=(AESCS & ~AESCS_E_D_MASK) | ENCRYPT<<AESCS_E_D_BIT_POS;
   }

   aes_set_mode(mode);
}

void hal_aes_crypt(uint8_t *dest_buf, uint8_t *src_buf)
{
   aes_data_write_buf(src_buf, 0, AES_BUF_SIZE);
   aes_go();
   while(aes_busy());
   aes_data_read_buf(dest_buf, 0, AES_BUF_SIZE);
}

void hal_aes_get_dec_key(uint8_t *output_dec_key, uint8_t *input_enc_key)
{
   uint8_t index, loop, rcon_int; //lint -esym(644, rcon_int) "variable may not have been initialized"
   for(index=0; index<16; index++)
   {
      output_dec_key[index]=input_enc_key[index];
   }
   for(loop=0; loop<10; loop++)
   {
      if(loop==0)
      {
         rcon_int=1;
      }
      else
      {
         rcon_int=((rcon_int & 0x80) ? (rcon_int<<1) ^ 0x1b : rcon_int<<1); // xtime operation
      }

      output_dec_key[0]^=S[output_dec_key[13]];
      output_dec_key[1]^=S[output_dec_key[14]];
      output_dec_key[2]^=S[output_dec_key[15]];
      output_dec_key[3]^=S[output_dec_key[12]];
      output_dec_key[0] ^= rcon_int;

      for(index=0; index < 12; index++)
      {
         output_dec_key[index+4]^=output_dec_key[index];
      }
   }
}

void aes_set_mode(uint8_t mode)
{
   AESCS=(AESCS & ~AESCS_MODE_MASK) | mode<<2;
}

void aes_select_e_d(uint8_t operation)
{

   AESCS=(AESCS & ~AESCS_E_D_MASK) | operation<<1;
}

void aes_go()
{
   AESCS=AESCS | AESCS_GO_MASK;
}

uint8_t aes_busy()
{
   return AESCS & AESCS_GO_MASK;
}

void aes_data_write_buf(uint8_t *buf, uint8_t indirect_start_address, uint8_t length)
{
   int8_t index;
   AESIA2= (AESIA2 & ~AESIA2_DI_MASK) | (indirect_start_address << AESIA2_DI_B0_POS);
   for(index=length-1; index>=0; index--)
   {
      AESD=buf[index];
   }
}

void aes_data_read_buf(uint8_t *buf, uint8_t indirect_start_address, uint8_t length)
{
   int8_t index;
   AESIA2= (AESIA2 & ~AESIA2_DO_MASK) | (indirect_start_address << AESIA2_DO_B0_POS);
   for(index=length-1; index>=0; index--)
   {
      buf[index]=AESD;
   }
}

void aes_keyin_write_buf(const uint8_t *buf, uint8_t indirect_start_address, uint8_t length)
{
   int8_t index;
   AESIA1= (AESIA1 & ~AESIA1_KIN_MASK) | (indirect_start_address << AESIA1_KIN_B0_POS);
   for(index=length-1; index>=0; index--)
   {
      AESKIN=buf[index];
   }
}

void aes_initvect_write_buf(const uint8_t *buf, uint8_t indirect_start_address, uint8_t length)
{
   int8_t index;
   AESIA1= (AESIA1 & ~AESIA1_IV_MASK) | (indirect_start_address << AESIA1_IV_B0_POS);
   for(index=length-1; index>=0; index--)
   {
      AESIV=buf[index];
   }
}

