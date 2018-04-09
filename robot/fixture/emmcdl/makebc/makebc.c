// Create a birth certificate with the given ESN, HW version, and model number
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void bye(const char* msg)
{
  printf("%s\n", msg);
  exit(1);
}
int main(int argc, char** argv)
{
  FILE* fp;
  int bc[256];
  unsigned int esn = 0, hwver = 0, model = 0;
  memset(bc, 0, 1024);
  if (argc != 5
      || 1 != sscanf(argv[2], "%x", &esn) || 8 != strlen(argv[2])
      || 1 != sscanf(argv[3], "%x", &hwver) || 4 != strlen(argv[3])
      || 1 != sscanf(argv[4], "%x", &model) || 4 != strlen(argv[4]))
    bye("usage: makebc outfile esn hw_ver model\nexample: makebc /path/birthcertificate 123abcde 0003 0001");
  if ((esn>>16) < 0x10 || (esn>>16) > 0x8000)
    bye("invalid esn - must be in range 0x001xxxxx..0x7ffxxxxx");
  if (!hwver || !model)
    bye("invalid hwver or model - must be >0");
  bc[0] = esn;
  bc[255] = ~esn;
  bc[1] = hwver;
  bc[2] = model;
  if (!(fp = fopen(argv[1], "wb")) || sizeof(bc) != fwrite(bc, 1, sizeof(bc), fp) || fclose(fp))
    bye("couldn't write file");
  printf("wrote %s with ESN=0x%08x, hwver=0x%04x, model=0x%04x\n", argv[1], esn, hwver, model);
}
