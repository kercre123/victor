#include <stdio.h>

#include "bignum.h"
#include "publickey.h"

big_mont_t mont;

void print(uint16_t num) {
	printf("0x%04x", num);
}

void print(uint16_t* numbers, int count) {
	printf("{ ");
	
	for (int i = 1; i < count; i++) {
		print(*(numbers++)); printf(", ");
	}

	if (count) print(*numbers);

	printf(" }");
}

void print(big_num_t& num) {
	printf("{ ");
	printf(num.negative ? "true, " : "false, ");
	print(num.used);
	printf(", ");
	print(num.digits, CELL_SIZE);
	printf(" }");
}

void print(big_mont_t& mont) {
  printf("{\n  ");
  print(mont.shift); printf(",\n  ");
  print(mont.one); printf(",\n  ");
  print(mont.modulo); printf(",\n  ");
  print(mont.r2); printf(",\n  ");
  print(mont.rinv); printf(",\n  ");
  print(mont.minv);
  printf("\n}");
}

int main(int argc, char** argv) {
	mont_init(mont, CERT_RSA.modulo);

	printf("static const big_mont_t FLASH_STORE RSA_CERT_MONT =\n");
	print(mont);
	printf(";\n");
}
