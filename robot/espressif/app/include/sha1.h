/** Improvised header for undocumented SHA1 functions in Espressif ROM
 * It appears that these functions match the netbsd sha1 and wpa_supplicant hmac_sha1
 * Thanks to the NodeMCU community for some reverse engineering
 * https://github.com/nodemcu/nodemcu-firmware/blob/2128c42f0209e3da622eecb01bbc232d49a6aec1/app/include/rom.h
 * @author Daniel Casner <daniel@anki.com>
 */

#ifndef __sha1_h_
#define __sha1_h_

// SHA1 is assumed to match the netbsd sha1.h headers
#define SHA1_DIGEST_LENGTH		20
#define SHA1_DIGEST_STRING_LENGTH	41

typedef struct
{
	uint32_t state[5];
	uint32_t count[2];
	uint8_t buffer[64];
} SHA1_CTX;

extern void SHA1Init(SHA1_CTX * context);
extern void SHA1Final(uint8_t digest[SHA1_DIGEST_LENGTH], SHA1_CTX *context);
extern void SHA1Update(SHA1_CTX *context, const uint8_t * data, unsigned int len);


/** hmac_sha1 Implementation in ROM
 * HMAC-SHA1 over data buffer (RFC 2104)
 * @param[in] key Key for HMAC operations
 * @param[in] key_ken Length of the key in bytes
 * @param[in] data Pointer to the data area
 * @param[in] data_len Length of the data area
 * @param[out] mac Buffer for the hash (20 bytes) Returns: 0 on success, -1 of failure
 */
extern int hmac_sha1 (const u8* key, size_t key_len, const u8* data, size_t data_len, u8* mac);

#endif
