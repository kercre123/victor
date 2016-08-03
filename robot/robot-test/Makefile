CRYPTO_OBJS = ../../crypto/aes.o ../../crypto/diffie.o ../../crypto/packet.o ../../crypto/sha1.o ../../crypto/bignum.o ../../crypto/hmac.o random.o ../../crypto/sha512.o ../../crypto/crc32.o ../../crypto/md5.o ../../crypto/rsa_pss.o
CRYPTO_LIB  = crypto.a

DIFFIE_OBJ 	= diffietest.o
DH_OBJ      = dhreverse.o
SIGN_OBJ    = signtest.o

OUTPUT 	    = $(DIFFIE_TEST) $(SIGN_TEST) $(DH_TEST)

TESTS 		= dhreverse diffietest signtest

DEVELOPMENT_KEY	= certs/development.pem
AES_KEY			= FA67CE138A376CD21A26118615326AA5
SIGN_FLAGS		= --sign $(AES_KEY) $(DEVELOPMENT_KEY)

BUILD_DIR	= ./build
WIFI_BINARY	= $(BUILD_DIR)/esp.user.bin
RTIP_BINARY	= $(BUILD_DIR)/robot.axf
BODY_BINARY	= $(BUILD_DIR)/syscon.axf

UNSIGNED    = plain.safe
SIGNED   	= signed.safe

all: $(TESTS)

clean:
	rm -Rf $(TESTS) $(CRYPTO_OBJS) $(SIGN_OBJ) $(DIFFIE_OBJ) $(DH_OBJ) $(CRYPTO_LIB) $(OUTPUT) $(UNSIGNED) $(SIGNED)

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

signtest: $(SIGN_OBJ) $(CRYPTO_LIB) $(UNSIGNED) $(SIGNED)
	gcc -o $@ $(SIGN_OBJ) $(CRYPTO_LIB)
	./$@ $(UNSIGNED) $(SIGNED)

signed.safe:
	cd ..; python3 ./tools/sign.py -c 0 --wifi $(WIFI_BINARY) --rtip $(RTIP_BINARY) --body $(BODY_BINARY) $(SIGN_FLAGS) ./robot-test/$@

plain.safe:
	cd ..; python3 ./tools/sign.py -c 0 --wifi $(WIFI_BINARY) --rtip $(RTIP_BINARY) --body $(BODY_BINARY) ./robot-test/$@
