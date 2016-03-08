#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../crypto/bignum.h"

// This is not crypto secure
void random_big(big_num_t& num, int size) {
	num.used = size;
	num.negative = false;

	for (int i = 0; i < size; i++) {
		num.digits[i] = (big_num_cell_t) rand();
	}
}

void from_hex(big_num_t& num, const char* hex) {
	int len = (int)strlen(hex);
	int bit = 0;
	int index = 0;

	memset(&num, 0, sizeof(big_num_t));
	hex += len - 1;

	num.used = (len * 4 + CELL_BITS - 1) / CELL_BITS;

	if (num.used > CELL_SIZE) {
		printf("OVERFLOW");
		return ;
	}

	while (len-- > 0) {
        int ch = *(hex--);
		int digit;

		switch(ch) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				digit = ch - '0';
				break ;
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				digit = 10 + ch - 'a';
				break ;
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				digit = 10 + ch - 'A';
				break ;
            default:
                digit = 0;
		}

		num.digits[index] |= digit << bit;
		bit += 4;

		if (bit >= CELL_BITS) {
			index++; 
			bit = 0;
		}
	}
}

void print_hex(const big_num_t& num) {
	if (num.used == 0) {
		printf("0\r\n");
		return ;
	}

	if (num.negative) {
		printf("-");
	}

	int i = num.used - 1;
	printf("%x", num.digits[i--]);
	while(i >= 0) {
		printf("%04x", num.digits[i--]);
	}
	printf("\r\n");
}


int main(int argc, char** argv) {
	big_num_t x, y;

	big_num_t t1, t2;

	srand(time(NULL));
	random_big(x, 128 / CELL_BITS);
	random_big(y, 128 / CELL_BITS);
	print_hex(x);
	print_hex(y);

	// Attempt to do a montgomery power (p ** x ** y)
	printf("---\r\n");
	mont_power(DEFAULT_DIFFIE_GROUP, t1, DEFAULT_DIFFIE_GENERATOR, x);
	mont_power(DEFAULT_DIFFIE_GROUP, t1, t1, y);
	mont_from(DEFAULT_DIFFIE_GROUP, t2, t1);
	print_hex(t2);

	// Attempt to do a montgomery power (p ** y ** x)
	printf("---\r\n");
	mont_power(DEFAULT_DIFFIE_GROUP, t1, DEFAULT_DIFFIE_GENERATOR, y);
	mont_power(DEFAULT_DIFFIE_GROUP, t1, t1, x);
	mont_from(DEFAULT_DIFFIE_GROUP, t2, t1);
	print_hex(t2);

	return 0;
}

