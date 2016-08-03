CRYPTO_OBJS = ../../crypto/aes.o ../../crypto/diffie.o ../../crypto/packet.o ../../crypto/sha1.o ../../crypto/bignum.o ../../crypto/hmac.o random.o ../../crypto/sha512.o ../../crypto/crc32.o ../../crypto/md5.o ../../crypto/rsa_pss.o
CRYPTO_LIB  = crypto.a

DIFFIE_OBJ 	= diffietest.o
DH_OBJ      = dhreverse.o

OUTPUT 	    = $(DIFFIE_TEST) $(SIGN_TEST) $(DH_TEST)

all: dhreverse #diffietest

clean:
	rm -Rf $(CRYPTO_OBJS) $(DIFFIE_OBJ) $(CRYPTO_LIB) $(OUTPUT)

crypto.a: $(CRYPTO_OBJS)
	ar rcs $@ $^

.cpp.o:
	gcc -o $@ -c $< -I ../../crypto -I ../generated -I ../include/anki/cozmo/robot/

diffietest: $(DIFFIE_OBJ) $(CRYPTO_LIB)
	gcc -o $@ $^
	./$@

dhreverse: $(DH_OBJ) $(CRYPTO_LIB)
	gcc -o $@ $^
	./$@
