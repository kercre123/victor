#include <stdio.h>
#include <string.h>

#include "bignum.h"
#include "aes.h"
#include "sha512.h"
#include "crc32.h"
#include "rsa_pss.h"

#include "publickey.h"

#include "rec_protocol.h"

static uint8_t aes_iv[AES_KEY_LENGTH];
static uint8_t aes_key[AES_KEY_LENGTH];
static sha512_state hash;

void gen_random(void* ptr, int length) {}

bool crcValid(FirmwareBlock& block) {
	uint32_t checksum = calc_crc32((uint8_t*)block.flashBlock, sizeof(block.flashBlock));
	return checksum == block.checkSum;
}

bool comparePlain(FILE* plain, int address, uint8_t* data) {
	FirmwareBlock p;

	do {
		if (!fread(&p, sizeof(p), 1, plain)) {
			printf("\n\r%08x: Block missing\n\r", address);
			return false;
		}
	} while (address != p.blockAddress);
	
	if (memcmp(p.flashBlock, data, sizeof(p.flashBlock)) != 0) {
		printf("\n\r%08x: AES decode failure\n\r", address);
		return false;
	}

	return true;
}

int main (int argc, char** argv) {
	FILE* plain;
	FILE* encoded;
	bool failure = false;
	uint8_t digest[SHA512_DIGEST_SIZE];

	union {
		FirmwareBlock block;
		FirmwareHeaderBlock header;
		CertificateData cert;
	};

	sha512_init(hash);

	if (argc < 3) { return -1; }

	plain = fopen(argv[1], "rb");
	encoded = fopen(argv[2], "rb");

	while (fread(&block, sizeof(block), 1, encoded)) {
		if (!crcValid(block)) {
			failure = true;
			printf("\n\r%08x: CRC Failed\n\r", block.blockAddress);
			continue ;
		}

		if (block.blockAddress != CERTIFICATE_BLOCK) {
			sha512_process(hash, &block, sizeof(block));
		}

		switch (block.blockAddress) {
		case COMMENT_BLOCK:
			break ;
		case HEADER_BLOCK:
			memcpy(aes_iv, header.aes_iv, AES_KEY_LENGTH);
			break ;
		case CERTIFICATE_BLOCK:
			sha512_done(hash, digest);
			sha512_init(hash);

			if (!verify_cert(RSA_CERT_MONT, CERT_RSA, digest, cert.data, cert.length)) {
				failure = true;
				printf("\n\r%08x: CERT FAIL\n\r", block.blockAddress);
				continue ;
			}

			break ;
		default: // FLASH BLOCK
			aes_cfb_decode(AES_KEY, aes_iv, 
				(uint8_t*)block.flashBlock,
				(uint8_t*)block.flashBlock, 
				sizeof(block.flashBlock), aes_iv);

			if (!comparePlain(plain, block.blockAddress, (uint8_t*)block.flashBlock)) {
				failure = true;
				continue ;
			}

			break ;
		}

		printf(".");
	}
	printf("\n\r");

	printf(failure ? "\n\rFAILED\n\r" : "\n\rPASSED\n\r");

	return failure ? -1 : 0;
}
