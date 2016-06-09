#include <stdio.h>

#include "bignum.h"
#include "publickeys.h"

void print(uint32_t num) {
	printf("0x%x", num);
}

void print(uint8_t* numbers, int count) {
	printf("{ ");
	
	for (int i = 1; i < count; i++) {
		print(*(numbers++)); printf(", ");
	}

	if (count) print(*numbers);

	printf(" }");
}

void print(uint16_t* numbers, int count) {
	printf("{ ");
	
	for (int i = 1; i < count; i++) {
		print(*(numbers++)); printf(", ");
	}

	if (count) print(*numbers);

	printf(" }");
}

void print(uint32_t* numbers, int count) {
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
	print(num.digits, num.used);
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
	big_mont_t mont;
	big_num_t exp;

	mont_init(mont, CERT_RSA.modulo);

	printf("\nstatic const big_mont_t FLASH_STORE RSA_CERT_MONT =\n");
	print(mont);
	printf(";\n");

	mont_init(mont, DIFFIE_RSA.modulo);

	printf("\nstatic const big_mont_t FLASH_STORE RSA_DIFFIE_MONT =\n");
	print(mont);
	printf(";\n");

	mont_to(mont, exp, DIFFIE_RSA.exp);
	printf("\nstatic const big_num_t FLASH_STORE RSA_DIFFIE_EXP_MONT =\n");
	print(exp);
	printf(";\n");
}
