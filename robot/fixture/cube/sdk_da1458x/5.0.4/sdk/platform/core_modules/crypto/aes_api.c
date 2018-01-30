#include "aes_api.h"

#if (USE_AES)

#include "sw_aes.h"
#include "smpm.h"
#include "gapm_task.h"
#include "aes.h"

/* using a dummy IV vector filled with zeroes for the software decryption since the chip does not use IV for encryption */
extern unsigned char IV[];

/**
 ****************************************************************************************
 * @brief AES set encrypt/decrypt key.
 * @param[in] userKey   The key data.
 * @param[in] bits      Key number of bits. Should be 128
 * @param[in] key       AES_KEY structure pointer.
 * @param[in] enc_dec   0 set decrypt key, 1 set encrypt key.
 * @return 0 if successfull, -1 if userKey or key are NULL, -2 if bits is not 128.
 ****************************************************************************************
 */
int aes_set_key(const unsigned char *userKey, const int bits, AES_KEY *key, int enc_dec)
{
    if(enc_dec == AES_ENCRYPT)
    {
        u32 *rk;
        rk = key->ks;
        rk[0] = GETU32(userKey     );
        rk[1] = GETU32(userKey +  4);
        rk[2] = GETU32(userKey +  8);
        rk[3] = GETU32(userKey + 12);
        
        return 0;
    }
    if (enc_dec == AES_DECRYPT)
    { 
        if (bits == 128)
           AES_set_key(key,userKey,IV,AES_MODE_128);
      else if (bits == 256)
           AES_set_key(key,userKey,IV,AES_MODE_256);
            
      AES_convert_key(key);
    }
    return 0; /* now we always return 0 since the handling functions changed to void */
}

/**
 ****************************************************************************************
 * @brief AES encrypt/decrypt block.
 * @param[in] in        The data block (16bytes).
 * @param[in] out       The encrypted/decrypted output of the operation (16bytes)
 * @param[in] key       AES_KEY structure pointer.
 * @param[in] enc_dec   0 decrypt, 1 encrypt.
 * @param[in] ble_flags used to specify whether the encryption/decryption 
 * will be performed syncronously or asynchronously (message based)
 * also if ble_safe is specified in ble_flags rwip_schedule() will be called
 * to avoid loosing any ble events
 * @return 0 if successfull, -1 if SMPM uses the HW block.
 ****************************************************************************************
 */
int aes_enc_dec(unsigned char *in, unsigned char *out, AES_KEY *key, int enc_dec, unsigned char ble_flags)
{
    int j;
    
    if(ble_flags & BLE_SAFE_MASK)
        rwip_schedule();        
    
    if(enc_dec == AES_ENCRYPT)
    {
        //check to avoid conflict with SMPM/LLM
        if(smpm_env.operation && ((struct smp_cmd *)(smpm_env.operation))->operation == GAPM_USE_ENC_BLOCK)
            return -1;
        
        SetWord32(BLE_AESKEY31_0_REG,key->ks[3]);
        SetWord32(BLE_AESKEY63_32_REG,key->ks[2]);
        SetWord32(BLE_AESKEY95_64_REG,key->ks[1]);
        SetWord32(BLE_AESKEY127_96_REG,key->ks[0]);
        
        for(j=0;j<16;j++) {
            *(volatile uint8*)(uint32)(0x80000+jump_table_struct[offset_em_enc_plain]+j)=in[j];
        }

        // set the pointer on the data to encrypt.
        SetWord32(BLE_AESPTR_REG,jump_table_struct[offset_em_enc_plain]);    
        
        //start the hw block
        SetWord32(BLE_AESCNTL_REG,1);
        
        while(GetWord32(BLE_AESCNTL_REG)==1)
            if(ble_flags & BLE_SAFE_MASK)
                rwip_schedule();        
    
        for(j=0;j<16;j++) {
            out[j] = *(volatile uint8*)(uint32)(0x80000+jump_table_struct[offset_em_enc_cipher]+j);
        }
    }
    else
    {
#ifdef USE_AES_DECRYPT
        for(j=0;j<16;j+=4) {
            *(uint32_t *)&in[j] = GETU32(&in[j]);
        }
        AES_decrypt(key, (uint32_t *)in);
        //decrypt is performed in place, so copy to out afterwards
        for(j=0;j<16;j+=4) {
            for (int k=3;k>=0;k--)
                out[j+k] = in[j+3-k];
        }
#else
        return -6;
#endif
    }
    
    return 0;
}

#endif // (USE_AES)
