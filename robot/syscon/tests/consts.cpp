#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../crypto/bignum.h"

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

void dump(const int num) {
	printf("%i", num);
}

void dump(const big_num_cell_t num) {
	printf("0x%04x", num);
}

void dump(const bool value) {
	printf(value ? "true" : "false");
}

void dump(const big_num_t& num) {
	printf("{ ");
	dump(num.negative); 	
	printf(", ");
	dump(num.used); 
	printf(", {");
	for (int i = 0; i < num.used; i++) {
		if (i) printf(", ");
		dump(num.digits[i]); 
	}
	printf(" } }");
}

void dump(const big_mont_t& mont) {
	printf("{\n\r");
	dump(mont.shift); printf(",\n\r");
	dump(mont.modulo); printf(",\n\r");
	dump(mont.one); printf(",\n\r");
	dump(mont.r2); printf(",\n\r");
	dump(mont.rinv); printf(",\n\r");
	dump(mont.minv); printf("\n\r}\n\r");
}

int main(int argc, char** argv) {
	big_num_t g, p;
	big_mont_t mont;
	big_num_t p_mont;

	from_hex(p, "5");
	from_hex(g, "f4054f83bbbf11e4a838ee77a1027956213afe6c9e85c651505540f22e2c990fd64f02e68be29bd4012e2ec0354dd506a194aea02c9a70e21bf2ee3cca7fcb3d9269519d79126cbdcea7a191406289fa7f4ee08e3b9419bcdf42353c6782dbae06ecf6c3e585b7bfa78c73eee34b72b2afb8644463ff9d7af2d39b302ac2884b");
	mont_init(mont, g);
	mont_to(mont, p_mont, p);

	printf("---\r\n");

	printf("--- Montgomery settings used ---");
	dump(mont);
	dump(p_mont);
}