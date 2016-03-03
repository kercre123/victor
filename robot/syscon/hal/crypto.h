#ifndef __RNG_H
#define __RNG_H

enum CryptoOperation {
	CRYPTO_GENERATE_RANDOM,
	CRYPTO_AES_ENCRYPT,
	CRYPTO_AES_DECRYPT,
	CRYPTO_DIFFIE_HELLMAN
};

typedef void (*crypto_callback)(struct CryptoTask*);

struct CryptoTask {
	crypto_callback callback;
	CryptoOperation op;
	void *input;
	void *output;
	int	 length;
};

namespace Crypto {
  void init();
	void manage();
  void random(void* data, int length);
	void execute(CryptoTask* task);
}

#endif
