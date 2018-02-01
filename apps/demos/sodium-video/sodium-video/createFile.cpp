//
//  createFile.cpp
//  sodium-video
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "createFile.h"
#include <string.h>
#include <unistd.h>
#include <chrono>

SodiumVideo::SodiumVideo() {
  FILE *rawFile = fopen("bigFile.txt", "w+");
  const int FILESIZE = 1000000;
  char* data = (char*)malloc(FILESIZE);
  
  memset(data, 1, FILESIZE);

  GenerateKeys();
  
  unsigned char cipher[FILESIZE + crypto_aead_xchacha20poly1305_ietf_ABYTES];
  unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
  uint64_t cipherLength;
  
  crypto_aead_xchacha20poly1305_ietf_encrypt(cipher, &cipherLength, (const unsigned char*)data, (uint64_t)FILESIZE, nullptr, 0, nullptr, nonce, _ServerTx);
  
  printf("writing encrypted with size: %d -- >\n", cipherLength);
  for(int i = 0; i < 20; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
  for(int i = 0; i < 20; i++) {
    printf("%02X ", cipher[i]);
  }
  
  printf("\n\n");
  fseek(rawFile, 0, SEEK_SET);
  int w = write(fileno(rawFile), cipher, cipherLength);
  
  fseek(rawFile, 0, SEEK_SET);
  printf("reading encrypted: %d\n", w);
  for(int i = 0; i < 20; i++) {
    int value;
    read(fileno(rawFile), &value, 1);
    printf("%02X ", value);
  }
  
  unsigned char msg[cipherLength];
  uint64_t msgLength;
  
  auto start = std::chrono::high_resolution_clock::now();
  int de = crypto_aead_xchacha20poly1305_ietf_decrypt(msg, &msgLength, nullptr, cipher, cipherLength, nullptr, 0, nonce, _ServerTx);
  auto finish = std::chrono::high_resolution_clock::now();
  
  uint64_t nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
  printf("\n\nreading decrypted: %d time: %f\n", de, (float)nanoseconds / 1000000);
  for(int i = 0; i < 20; i++) {
    printf("%02X ", msg[i]);
  }
  printf("\n");
  
  fclose(rawFile);
  free(data);
}

void SodiumVideo::GenerateKeys() {
  unsigned char serverPk[crypto_kx_PUBLICKEYBYTES];
  unsigned char serverSk[crypto_kx_SECRETKEYBYTES];
  
  unsigned char clientPk[crypto_kx_PUBLICKEYBYTES];
  unsigned char clientSk[crypto_kx_SECRETKEYBYTES];
  
  crypto_kx_keypair(serverPk, serverSk);
  crypto_kx_keypair(clientPk, clientSk);
  
  int r = crypto_kx_server_session_keys(_ServerRx, _ServerTx, serverPk, serverSk, clientPk);
}
