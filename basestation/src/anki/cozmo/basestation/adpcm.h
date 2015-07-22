// First block should starts with index + predictor = 0
// Resulting values are the state for the next block
// Length is in input samples (800)

#include <stdint.h>

void encodeADPCM(int &index, int &predictor, int16_t *in, uint8_t *out, int length);
