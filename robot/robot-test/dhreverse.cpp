#include <string.h>
#include <stdio.h>

#include "aes.h"
#include "diffie.h"

int main(int argc, char** argv) {
  const uint8_t local_secret[16] = { 84,28,115,238,0,95,69,53,184,61,188,161,48,9,102,183 };
  const uint8_t remote_secret[16] = { 90,247,252,236,142,200,246,47,86,231,2,5,94,55,164,188 };
  const uint8_t encoded_key[16] = { 26,110,131,232,127,199,91,113,152,160,212,165,26,251,22,170 };
  const uint8_t expected_key[16] = { 0x38, 0x51, 0x4b, 0x48, 0xfc, 0x9c, 0xcc, 0x04, 0x75, 0xc5, 0x0e, 0x1e, 0x39, 0xae, 0xec, 0x5e };

  uint32_t pinCode = 0x24017305;

  uint8_t key[16];

  memcpy(key, encoded_key, sizeof(key));

  dh_reverse(local_secret, remote_secret, pinCode, key);

  printf ("\ngot:  ");
  for (int i = 0; i < 16; i++) printf(" %02x", key[i]);
  printf ("\nwant: ");
  for (int i = 0; i < 16; i++) printf(" %02x", expected_key[i]);
  printf ("\n");

  return memcmp(key, expected_key, sizeof(key));
}
