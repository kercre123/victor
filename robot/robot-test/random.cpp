#include <stdlib.h>

void gen_random(void* out, int length) {
	uint8_t* data = (uint8_t*) out;
	while(length-- > 0) *(data++) = rand();
}