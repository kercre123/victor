#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char u8;

int JPEGStart(u8* out, int width, int height, int quality);
int JPEGCompress(u8* out, u8* in);
int JPEGEnd(u8* out);

void bye(const char* msg)
{
  printf("%s\n", msg);
  exit(1);
}

int main(int argc, char** argv)
{
  if (argc != 5)
    bye("Usage: enjpeg input.gray output.jpg widthxheight quality");
  
  int width = 0, height = 0, quality = 0;
  sscanf(argv[3], "%dx%d", &width, &height);
  if (!width || !height || width & 7 || height & 7)
    bye("widthxheight must be divisible by 8 - example: 320x240");
  
  sscanf(argv[4], "%d", &quality);
  if (quality < 1 || quality > 100)
    bye("quality must be between 1 and 100");
  
  int graysize = width*height;
  u8* in = (u8*)malloc(graysize*2+1);
  u8* out = (u8*)malloc(graysize);

	FILE* fp = fopen(argv[1], "rb");
  if (!fp)
    bye("Could not open input for reading");
	if (graysize != fread(in, 1, graysize+1, fp))
    bye("Input file should be exactly width*height bytes, in ImageMagick .gray format (8bpp grayscale)");
	fclose(fp);
  
  // Convert .gray file into YUYV by setting U/V values to zero
  for (int i = graysize-1; i >= 0; i--)
  {
    in[i*2] = in[i];
    in[i*2+1] = 0;
  }
  
  u8* outp = out;
  outp += JPEGStart(outp, width, height, quality);
  for (int row = 0; row < height; row += 8)
    outp += JPEGCompress(outp, in + width*2*row);
  outp += JPEGEnd(outp);
  
  fp = fopen(argv[2], "wb");
  if (!fp)
    bye("Could not open output for writing");
	fwrite(out, 1, outp-out, fp);
  fclose(fp);
}
