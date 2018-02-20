#include <stdio.h> 
#include <sodium.h>

int main() {
  printf("nonce bytes [%d]\n", crypto_secretbox_NONCEBYTES);
}
