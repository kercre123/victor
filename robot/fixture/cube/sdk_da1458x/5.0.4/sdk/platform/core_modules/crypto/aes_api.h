#ifndef __AES_API_H__
#define __AES_API_H__

#include "rwip.h"

#if (USE_AES)

#include "sw_aes.h"
#include "aes_locl.h"
#include "ke_task.h"

typedef AES_CTX AES_KEY;

#define BLE_SAFE_MASK   0x0F
#define BLE_SYNC_MASK   0xF0

int aes_set_key(const unsigned char *userKey, const int bits, AES_KEY *key, int enc_dec);
int aes_enc_dec(unsigned char *in, unsigned char *out, AES_KEY *key, int enc_dec, unsigned char ble_sync_safe);

#endif // (USE_AES)

#endif  //__AES_API_H__
