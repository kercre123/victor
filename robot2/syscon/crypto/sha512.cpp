// SHA-512. Adapted from LibTomCrypt. This code is Public Domain
#include "sha512.h"

#include <string.h>
#include <stdint.h>

#define MIN( x, y ) ( ((x)<(y))?(x):(y) )

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CONSTANTS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The K array
static const uint64_t K[80] = {
  0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
  0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
  0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
  0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
  0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
  0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
  0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
  0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
  0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
  0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
  0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
  0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
  0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
  0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
  0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
  0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
  0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
  0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
  0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
  0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static const uint64_t INIT_STATE[] = {
  0x6a09e667f3bcc908ULL,
  0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL,
  0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL,
  0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL,
  0x5be0cd19137e2179ULL
};

#define BLOCK_SIZE          SHA512_BLOCK_SIZE

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  INTERNAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void BIGSWAP (void* out, const void* in) {
  uint8_t* target = (uint8_t*)out;
  uint8_t* source = (uint8_t*)in;

  for (unsigned int i = 0; i < sizeof(uint64_t); i++) {
    target[i^7] = source[i];
  }
}

static uint64_t Ch(uint64_t x, uint64_t y, uint64_t z) {
  return z ^ (x & (y ^ z));
}

static uint64_t Maj(uint64_t x, uint64_t y, uint64_t z) {
  return ((x | y) & z) | (x & y);
}

static uint64_t Rt(uint64_t x, uint64_t n) {
  return (x >> n) | (x << (64 - (n)));
}

static uint64_t Rs(uint64_t x, uint64_t n) {
  return ( x & 0xFFFFFFFFFFFFFFFFULL) >> (uint64_t) n;
}

static uint64_t Sigma0(uint64_t x) {
  return Rt(x, 28) ^ Rt(x, 34) ^ Rt(x, 39);
}

static uint64_t Sigma1(uint64_t x) {
  return Rt(x, 14) ^ Rt(x, 18) ^ Rt(x, 41);
}

static uint64_t Gamma0(uint64_t x) {
  return Rt(x, 1)  ^ Rt(x, 8)  ^ Rs(x, 7);
}

static uint64_t Gamma1(uint64_t x) {
  return Rt(x, 19) ^ Rt(x, 61) ^ Rs(x, 6);
}

static void Sha512Round(
  uint64_t* W, uint64_t  i,
  uint64_t  a, uint64_t  b, uint64_t  c, uint64_t& d,
  uint64_t  e, uint64_t  f, uint64_t  g, uint64_t& h )
{

  uint64_t    t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
  uint64_t    t1 = Sigma0(a) + Maj(a, b, c);

   d += t0;                                          \
   h  = t0 + t1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TransformFunction
//
//  Compress 1024-bits
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void TransformFunction( sha512_state& md, const uint8_t* Buffer ) {
  uint64_t    S[8];
  uint64_t    W[80];
  int         i;

  // Copy state into S
  for( i=0; i<8; i++ )
  {
    S[i] = md.state[i];
  }

  // Copy the state into 1024-bits into W[0..15]
  for( i=0; i<16; i++ )
  {
    BIGSWAP(&W[i], &Buffer[8*i]);
  }

  // Fill W[16..79]
  for( i=16; i<80; i++ )
  {
    W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
  }

  // Compress
  for( i=0; i<80; i+=8 )
  {
    Sha512Round(W,i+0,S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7]);
    Sha512Round(W,i+1,S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6]);
    Sha512Round(W,i+2,S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5]);
    Sha512Round(W,i+3,S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4]);
    Sha512Round(W,i+4,S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3]);
    Sha512Round(W,i+5,S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2]);
    Sha512Round(W,i+6,S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1]);
    Sha512Round(W,i+7,S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0]);
  }

  // Feedback
  for( i=0; i<8; i++ )
  {
    md.state[i] = md.state[i] + S[i];
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sha512_init(sha512_state& context) {
  context.curlen = 0;
  context.length = 0;
  memcpy(context.state, INIT_STATE, sizeof(INIT_STATE));
}

void sha512_process(sha512_state& md, const void* in, uint32_t inlen) {
  uint32_t    n;

  if( md.curlen > sizeof(md.buf) )
  {
     return;
  }

  while( inlen > 0 )
  {
    if( md.curlen == 0 && inlen >= BLOCK_SIZE )
    {
       TransformFunction( md, (uint8_t *)in );
       md.length += BLOCK_SIZE * 8;
       in = (const uint8_t*)in + BLOCK_SIZE;
       inlen -= BLOCK_SIZE;
    }
    else
    {
       n = MIN( inlen, (BLOCK_SIZE - md.curlen) );
       memcpy( md.buf + md.curlen, in, (size_t)n );
       md.curlen += n;
       in = (uint8_t*)in + n;
       inlen -= n;
       if( md.curlen == BLOCK_SIZE )
       {
        TransformFunction( md, md.buf );
        md.length += 8*BLOCK_SIZE;
        md.curlen = 0;
       }
     }
  }
}

void sha512_done(sha512_state& md, void* out) {
  uint8_t* digest = (uint8_t*) out;
  int i;

  if( md.curlen >= sizeof(md.buf) )
  {
     return;
  }

  // Increase the length of the message
  md.length += md.curlen * 8ULL;

  // Append the '1' bit
  md.buf[md.curlen++] = (uint8_t)0x80;

  // If the length is currently above 112 bytes we append zeros
  // then compress.  Then we can fall back to padding zeros and length
  // encoding like normal.
  if( md.curlen > 112 )
  {
    while( md.curlen < 128 )
    {
      md.buf[md.curlen++] = (uint8_t)0;
    }
    TransformFunction( md, md.buf );
    md.curlen = 0;
  }

  // Pad up to 120 bytes of zeroes
  // note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
  // > 2^64 bits of data... :-)
  while( md.curlen < 120 )
  {
    md.buf[md.curlen++] = (uint8_t)0;
  }

  // Store length
  BIGSWAP( md.buf+120, &md.length );
  TransformFunction( md, md.buf );

  // Copy output
  for( i=0; i<8; i++ )
  {
    BIGSWAP( digest+(8*i), &md.state[i] );
  }
}
