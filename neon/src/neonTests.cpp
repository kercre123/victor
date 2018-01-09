#include "neonTests.h"

#include "anki/vision/basestation/image.h"

#include "anki/common/basestation/array2d_impl.h"

#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <future>
#include <thread>

#include <arm_neon.h>

namespace {

}

namespace Neon {

void TimeThisFunc(void (*func)(Anki::Vision::ImageRGB*), u32 numRuns);
void ManualSwapChannels(Anki::Vision::ImageRGB* img);
void OpenCVMixSwapChannels(Anki::Vision::ImageRGB* img);
void OpenCVCvtSwapChannels(Anki::Vision::ImageRGB* img);
void NeonIntrSwapChannels(Anki::Vision::ImageRGB* img);
void NeonSwapChannels(Anki::Vision::ImageRGB* img);
void NeonUnrollSwapChannels(Anki::Vision::ImageRGB* img);
void NeonParallelSwapChannels(Anki::Vision::ImageRGB* img);

void downsample_frame(const uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp);
void NeonDownsample(u8* ptr, u32 width, u32 height, u8 bpp, u8* out);
void NeonDownsample2(u8* ptr, u32 width, u32 height, u8 bpp, u8* out);

void RunTest(char* arg)
{
  const std::string which = std::string(arg);

  if(which == "opencvmix")
  {
    printf("Running opencv mix swap\n");

    TimeThisFunc(OpenCVMixSwapChannels, 10000);
  }
  else if(which == "opencvcvt")
  {
    printf("Running opencv cvt swap\n");

    TimeThisFunc(OpenCVCvtSwapChannels, 10000);
  }
  else if(which == "manual")
  {
    printf("Running manual swap\n");

    TimeThisFunc(ManualSwapChannels, 10000);
  }
  else if(which == "neonint")
  {
    printf("Running neonint swap\n");

    TimeThisFunc(NeonIntrSwapChannels, 10000);
  }
  else if(which == "neon")
  {
    printf("Running neon swap\n");

    TimeThisFunc(NeonSwapChannels, 10000);
  }
  else if(which == "neonunroll")
  {
    printf("Running neonunroll swap\n");

    TimeThisFunc(NeonUnrollSwapChannels, 10000);
  }
  else if(which == "neonparallel")
  {
    printf("Running neonparallel swap\n");

    TimeThisFunc(NeonParallelSwapChannels, 10000);
  }
  else if(which == "downorig")
  {
    printf("Running orig downsample\n");

    FILE* f;
    f = fopen("/data/raw", "r");
    u8 arr[720][1600];
    fread(arr, sizeof(u8), 720*1600, f);
    fclose(f);

    int width = 1600;
    int height = 720;
    int bpp = 10;
    u8* out = (u8*)(malloc((width * 8/bpp/2) * (height/2) * sizeof(u8) * 3));


    const int numRuns = 1000;

    double avg = 0;

    auto tstart = std::chrono::high_resolution_clock::now();
    for(u32 i = 0; i < numRuns; ++i)
    {
      auto start = std::chrono::high_resolution_clock::now();

      downsample_frame((const u8*)arr, out, width, height, bpp);
      
      auto end = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> elapsed = end - start;
      avg += elapsed.count();
    }
    auto tend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> telapsed = tend - tstart;

    printf("Avg %f\n", avg/numRuns);
    printf("Total %f\n", telapsed.count());
  } 
  else if(which == "down")
  {
    printf("Running neon downsample\n");

    // u8 arr[2][80] = { {1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9},
    //                   {1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9, 1, 2, 3, 4, 9, 5, 6, 7, 8, 9} };

    // NeonDownsample((u8*)arr, 80, 2, 10);

    FILE* f;
    f = fopen("/data/raw", "r");
    alignas(64) static u8 arr [720][1600];
    fread(arr, sizeof(u8), 720*1600, f);
    fclose(f);

    // int width = 1600;
    // int height = 720;
    // int bpp = 10;
    // u8* out = (u8*)(malloc((width * 8/bpp/2) * (height/2) * sizeof(u8) * 3));
    
    alignas(64) static u8 out [360][640][3];


    const int numRuns = 1;

    double avg = 0;

    auto tstart = std::chrono::high_resolution_clock::now();
    for(u32 i = 0; i < numRuns; ++i)
    {
      auto start = std::chrono::high_resolution_clock::now();

      NeonDownsample((u8*)arr, 1600, 720, 10, (u8*)out);
      
      auto end = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> elapsed = end - start;
      avg += elapsed.count();
    }
    auto tend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> telapsed = tend - tstart;

    printf("Avg %f\n", avg/numRuns);
    printf("Total %f\n", telapsed.count());

    Anki::Vision::ImageRGB img(360,640,(u8*)out);
    img.Save("./output1.png");

  }
  else if(which == "down2")
  {
    printf("Running neon downsample2\n");

    FILE* f;
    f = fopen("/data/raw", "r");
    alignas(64) static u8 arr [720][1600];
    fread(arr, sizeof(u8), 720*1600, f);
    fclose(f);
    
    alignas(64) static u8 out [360][640][3];

    const int numRuns = 10000;

    double avg = 0;

    auto tstart = std::chrono::high_resolution_clock::now();
    for(u32 i = 0; i < numRuns; ++i)
    {
      auto start = std::chrono::high_resolution_clock::now();

      NeonDownsample2((u8*)arr, 1600, 720, 10, (u8*)out);
      
      auto end = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> elapsed = end - start;
      avg += elapsed.count();
    }
    auto tend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> telapsed = tend - tstart;

    printf("Avg %f\n", avg/numRuns);
    printf("Total %f\n", telapsed.count());

    Anki::Vision::ImageRGB img(360,640,(u8*)out);
    img.Save("./output2.png");

  }
  else
  {
    printf("Unsupported Argument %s\n", which.c_str());
  }
}

alignas(64) static Anki::Vision::ImageRGB out(256, 550);

void TimeThisFunc(void (*func)(Anki::Vision::ImageRGB*), u32 numRuns)
{
  alignas(64) static Anki::Vision::ImageRGB img;
  img.Load("/data/local/tmp/neon/test.png");

  // (img.GetNumRows(), img.GetNumCols());

  img.CopyTo(out);

  double avg = 0;

  auto tstart = std::chrono::high_resolution_clock::now();
  for(u32 i = 0; i < numRuns; ++i)
  {
    auto start = std::chrono::high_resolution_clock::now();

    // printf("ere\n");
    func(&out);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    avg += elapsed.count();
  }
  auto tend = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> telapsed = tend - tstart;

  printf("Avg %f\n", avg/numRuns);
  printf("Total %f\n", telapsed.count());

  out.Save("./out1.png");
}





void OpenCVMixSwapChannels(Anki::Vision::ImageRGB* img)
{
  const int fromTo[] = { 0,0, 1,1, 2,2 };
  cv::mixChannels(&img->get_CvMat_(), 1, &img->get_CvMat_(), 1, fromTo, 3);
}

void OpenCVCvtSwapChannels(Anki::Vision::ImageRGB* img)
{
  cv::cvtColor(img->get_CvMat_(), img->get_CvMat_(), cv::COLOR_RGB2BGR);
}

void ManualSwapChannels(Anki::Vision::ImageRGB* img)
{
  s32 nrows = img->GetNumRows();
  s32 ncols = img->GetNumCols();
  
  if (img->IsContinuous()) 
  {
    ncols *= nrows;
    nrows = 1;
  }

  for(s32 i=0; i<nrows; ++i)
  {
    Anki::Vision::PixelRGB* data_i = img->GetRow(i);

    for (s32 j=0; j<ncols; ++j)
    {
      // std::swap(data_i[j][0], data_i[j][2]);
      u8 r = data_i[j][0];
      data_i[j][0] = data_i[j][1];
      data_i[j][1] = r; 
    }
  }
}

void NeonIntrSwapChannels(Anki::Vision::ImageRGB* img)
{
  const u32 numPixels = (img->GetNumRows() * img->GetNumCols());
  const u32 range = numPixels / 8;
  u8* mat = reinterpret_cast<u8*>(&img->get_CvMat_());

  for(int i = 0; i < range; ++i)
  {
    uint8x8x3_t rgb = vld3_u8(mat);
    uint8x8_t tmp = rgb.val[0];
    rgb.val[0] = rgb.val[2];
    rgb.val[2] = tmp;
    vst3_u8(mat, rgb);
  }
}

void NeonSwapChannels(Anki::Vision::ImageRGB* img)
{
  const u32 numPixels = (img->GetNumRows() * img->GetNumCols());
  const u32 range = numPixels / 8;
  u8* mat = reinterpret_cast<u8*>(out.get_CvMat_()[0]);
  // printf("%p\n", mat);
  for(int i = 0; i < range; ++i)
  {
    __asm__ volatile
    (
      "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
      "VSWP.8 d0, d2\n\t"
      "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"
      : [mat] "+r" (mat)
      : [mat] "r" (mat)
      : "d0", "d1", "d2", "memory"
    );
  }
}

void NeonUnrollSwapChannels(Anki::Vision::ImageRGB* img)
{
  const u32 numPixels = (img->GetNumRows() * img->GetNumCols());
  const u32 range = numPixels / (8*4);
  u8* mat = reinterpret_cast<u8*>(out.get_CvMat_()[0]);

  for(int i = 0; i < range; ++i)
  {
    __asm__ volatile
    (
      "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
      "VSWP d0, d2\n\t"
      "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"

      "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
      "VSWP d0, d2\n\t"
      "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"
      
      "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
      "VSWP d0, d2\n\t"
      "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"

      "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
      "VSWP d0, d2\n\t"
      "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"
      : [mat] "+r" (mat)
      : [mat] "r" (mat)
      : "d0", "d1", "d2", "memory"
    );
  }
}

void NeonParallelSwapChannels(Anki::Vision::ImageRGB* img)
{
  const u32 numBytes = (img->GetNumRows() * img->GetNumCols()) * 3;
  // printf("num %u\n", numBytes);
  u8* mat = reinterpret_cast<u8*>(img->get_CvMat_()[0]);

  int num_cpus = std::thread::hardware_concurrency();
  num_cpus = 4;
  const u32 numNeonOps = numBytes / (24*num_cpus);
  const u32 numBytesPerThread = numBytes/num_cpus;
  const u32 numArmOps  = numBytesPerThread - (numNeonOps*24);
  // printf("%d %d\n", numNeonOps, numArmOps);

  std::vector<std::future<void>> futures(num_cpus);
  for (int cpu = 0; cpu != num_cpus; ++cpu) 
  {
    u8* ptr = mat + (cpu * numBytesPerThread);

    futures[cpu] = std::async(std::launch::async, [numNeonOps, numArmOps, ptr]() {
      for(int i = 0; i < numNeonOps; ++i)
      {
        // printf("%d %d\n", cpu, i);
        __asm__ volatile
        (
          "VLD3.8 {d0, d1, d2}, [%[mat]]\n\t"
          "VSWP d0, d2\n\t"
          "VST3.8 {d0, d1, d2}, [%[mat]]!\n\t"
          :
          : [mat] "r" (ptr)
        );
        // uint8x8x3_t rgb = vld3_u8(ptr);
        // uint8x8_t tmp = rgb.val[0];
        // rgb.val[0] = rgb.val[2];
        // rgb.val[2] = tmp;
        // vst3_u8(ptr, rgb);
      }

      u8* ptr2 = ptr;
      for(int i = 0; i < numArmOps; ++i)
      {
        u8 tmp = ptr2[0];
        ptr2[0] = ptr2[2];
        ptr2[2] = tmp;
        ptr2 += 3;
      }
    });
  }

  for (int cpu = 0; cpu != num_cpus; ++cpu)
  {
    futures[cpu].get();
  }
}

#undef CLIP
#define CLIP(in__, out__) out__ = ( ((in__) < 0) ? 0 : ((in__) > 255 ? 255 : (in__)) )

#define X 640
#define Y 360
void downsample_frame(const uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  uint8_t *outR, *outG, *outB;
  int i, j;
  int tmp;

  outB = &rgb[0];
  outG = &rgb[1];
  outR = &rgb[2];

  // Raw image are reported as 1280x720, 10bpp BGGR MIPI Bayer format
  // Based on frame metadata, the raw image dimensions are actually 1600x720 10bpp pixels.
  // Simple conversion + downsample to RGB yields: 640x360 images
  const int dim = (bayer_sx*bayer_sy);

  // output rows have 8-bit pixels
  const int out_sx = bayer_sx * 8/bpp;

  const int dim_step = (bayer_sx << 1);
  const int out_dim_step = (out_sx << 1);

  int out_i, out_j;
  
  for (i = 0, out_i = 0; i < dim; i += dim_step, out_i += out_dim_step) {
    // process 2 rows at a time
    for (j = 0, out_j = 0; j < bayer_sx; j += 5, out_j += 4) {
      // process 4 col at a time
      // A B A_ B_ -> B G B G
      // C D C_ D_    G R G R

      // Read 4 10-bit px from row0
      const int r0_idx = i + j;
      uint16_t px_A  = (bayer[r0_idx+0] << 2) | ((bayer[r0_idx+4] & 0xc0) >> 6);
      uint16_t px_B  = (bayer[r0_idx+1] << 2) | ((bayer[r0_idx+4] & 0x30) >> 4);
      uint16_t px_A_ = (bayer[r0_idx+2] << 2) | ((bayer[r0_idx+4] & 0x0c) >> 2);
      uint16_t px_B_ = (bayer[r0_idx+3] << 2) |  (bayer[r0_idx+4] & 0x03);

      // Read 4 10-bit px from row1
      const int r1_idx = (i + bayer_sx) + j;
      uint16_t px_C  = (bayer[r1_idx+0] << 2) | ((bayer[r1_idx+4] & 0xc0) >> 6);
      uint16_t px_D  = (bayer[r1_idx+1] << 2) | ((bayer[r1_idx+4] & 0x30) >> 4);
      uint16_t px_C_ = (bayer[r1_idx+2] << 2) | ((bayer[r1_idx+4] & 0x0c) >> 2);
      uint16_t px_D_ = (bayer[r1_idx+3] << 2) |  (bayer[r1_idx+4] & 0x03);

      // output index:
      // out_i represents 2 rows -> divide by 4
      // out_j represents 1 col  -> divide by 2
      const int rgb_0_idx = ((out_i >> 2) + (out_j >> 1)) * 3;
      tmp = (px_C + px_B) >> 1;
      CLIP(tmp, outG[rgb_0_idx]);
      tmp = px_D;
      CLIP(tmp, outR[rgb_0_idx]);
      tmp = px_A;
      CLIP(tmp, outB[rgb_0_idx]);

      const int rgb_1_idx = ((out_i >> 2) + ((out_j+2) >> 1)) * 3;
      tmp = (px_C_ + px_B_) >> 1;
      CLIP(tmp, outG[rgb_1_idx]);
      tmp = px_D_;
      CLIP(tmp, outR[rgb_1_idx]);
      tmp = px_A_;
      CLIP(tmp, outB[rgb_1_idx]);
    }
  }
}


__attribute__((noinline)) void NeonDownsample(u8* ptr, u32 width, u32 height, u8 bpp, u8* out)
{
  register u8* r0 asm ("r0") = ptr;
  register u8* r1 asm ("r1") = out;
  register u8* r2 asm ("r2") = ptr;
  register uint r3 asm ("r3") = height >> 1;
  register uint r6 asm ("r6") = 19;

  __asm__ volatile
  (
    // Save these registers
    "VPUSH {d8-d15} \n\t"
    "PUSH {r4-r6} \n\t"

    // Clear r4 (outer loop counter)
    "MOV r4, 0 \n\t"

    // Move ptr2 to the next row in the image
    "ADD %[ptr2], %[ptr], #1600 \n\t"

    // Outer loop
    "1:\n\t"
    // If outer loop count has reached numOuter branch to done (4)
    "CMP r4, %[numOuter] \n\t"
    "BEQ 4f \n\t"
    // Otherwise set r5 (inner loop counter) to 0
    "MOV r5, 0 \n\t"

    // Inner loop
    "2:\n\t"
    // The inner loop is a lot of very similar operations
    // Perform a deinterleaving load of the elements in the first row
    // Delinterleave as n 2 element u8 structures (will load 2*8 bytes of data)
    // Orig data in bytes r,g,r,g,l,... format (r = upper 8 bits of red, g = upper bits of green,
    //   l = the lower 2 bits of each of the previous 4 bytes in order
    // Deinterleaved should look something like lrrgglrr in d0, rgglrrgg in d1
    "VLD2.8 {d0, d1}, [%[ptr]]! \n\t"

    // Begin replacement of low byte sections, shift sections after them
    // Don't care about the byte (l) contain the packed low bits so use bit selection to 
    // replace it with the bytes following it
    "VMOV.64 d10, d0 \n\t" // Duplicate d0 into d10
    "VSHR.U64 d10, d10, #8 \n\t" // Shift d10 right by 8 bits
    "VMOV.I64 d20, #0x000000000000FFFF \n\t" // Bits to select
    "VBSL.64 d20, d0, d10 \n\t" // Should select first 2 bytes from d0 and the last 6 bytes from d10 to go into d20
    // d20 should be the same as d0 but with the low bytes (l) removed and everything right shifted into the removed spots
    // __rrggrr

    // Repeat for d1
    "VMOV.64 d10, d1 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d21, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d21, d1, d10 \n\t" // _rggrrgg

    // Repeat delinterleave and byte removal for following 4 16byte chunks of data
    "VLD2.8 {d2, d3}, [%[ptr]]! \n\t"

    "VMOV.64 d10, d2 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d22, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d22, d2, d10 \n\t"

    // d3 has two interior low bytes so need to do two byte removals
    "VMOV.64 d10, d3 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d30, #0x00000000000000FF \n\t"
    "VBSL.64 d30, d3, d10 \n\t"
    "VMOV.64 d10, d30 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d23, #0x000000FFFFFFFFFF \n\t"
    "VBSL.64 d23, d30, d10 \n\t"


    "VLD2.8 {d4, d5}, [%[ptr]]! \n\t"

    "VMOV.64 d10, d4 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d30, #0x00000000000000FF \n\t"
    "VBSL.64 d30, d4, d10 \n\t"
    "VMOV.64 d10, d30 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d24, #0x000000FFFFFFFFFF \n\t"
    "VBSL.64 d24, d30, d10 \n\t"

    "VMOV.64 d10, d5 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d25, #0x0000000000FFFFFF \n\t"
    "VBSL.64 d25, d5, d10 \n\t"


    "VLD2.8 {d6, d7}, [%[ptr]]! \n\t"

    "VMOV.64 d10, d6 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d26, #0x0000000000FFFFFF \n\t"
    "VBSL.64 d26, d6, d10 \n\t"

    "VMOV.64 d10, d7 \n\t"
    "VSHR.U64 d7, d7, #8 \n\t"
    "VSHR.U64 d10, d10, #16 \n\t"
    "VMOV.I64 d27, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d27, d7, d10 \n\t"


    "VLD2.8 {d8, d9}, [%[ptr]]! \n\t"

    "VMOV.64 d10, d8 \n\t"
    "VSHR.U64 d8, d8, #8 \n\t"
    "VSHR.U64 d10, d10, #16 \n\t"
    "VMOV.I64 d28, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d28, d8, d10 \n\t"        

    "VMOV.64 d10, d9 \n\t"
    "VSHR.U64 d10, d10, #8 \n\t"
    "VMOV.I64 d29, #0x000000000000FFFF \n\t"
    "VBSL.64 d29, d9, d10 \n\t"  

    // At this point d20 - d29 contain various amounts of alternating chunks of data in the rough format of ggrrggrr
    // The last or last 2 bytes of each are garbage and need to be replaced by the follow chunk's bytes

    // Shift left to make all important bytes left aligned
    "VSHL.I64 d20, d20, #16 \n\t"
    // Concatinate the 6 valid bytes from d20 with 2 bytes from d22
    "VEXT.8 d20, d20, d22, #2 \n\t" // d20 is complete as ggrrggrr

    "VSHL.I64 d22, d22, #8 \n\t"
    "VEXT.8 d22, d22, d24, #3 \n\t"

    "VSHL.I64 d24, d24, #16 \n\t"
    "VEXT.8 d24, d24, d26, #5 \n\t"

    "VSHL.I64 d26, d26, #8 \n\t"
    "VEXT.8 d26, d26, d28, #6 \n\t"


    "VSHL.I64 d21, d21, #8 \n\t"
    "VEXT.8 d21, d21, d23, #1 \n\t" // d21 is complete

    "VSHL.I64 d23, d23, #16 \n\t"
    "VEXT.8 d23, d23, d25, #3 \n\t" // d21 is complete

    "VSHL.I64 d25, d25, #8 \n\t"
    "VEXT.8 d25, d25, d27, #4 \n\t" // d21 is complete

    "VSHL.I64 d27, d27, #16 \n\t"
    "VEXT.8 d27, d27, d29, #6 \n\t" // d21 is complete

    // Begin swapping chunks via bit select to get all red together and all green together

    "VMOV.I64 d1, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d1, d20, d21 \n\t" // green

    "VMOV.I64 d0, #0x0000FFFF0000FFFF \n\t"
    "VBSL.64 d0, d20, d21 \n\t" // red

    "VMOV.I64 d3, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d3, d22, d23 \n\t" // green

    "VMOV.I64 d2, #0x0000FFFF0000FFFF \n\t"
    "VBSL.64 d2, d22, d23 \n\t" // red

    "VMOV.I64 d5, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d5, d24, d25 \n\t" // green

    "VMOV.I64 d4, #0x0000FFFF0000FFFF \n\t"
    "VBSL.64 d4, d24, d25 \n\t" // red

    "VMOV.I64 d7, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d7, d26, d27 \n\t" // green

    "VMOV.I64 d6, #0x0000FFFF0000FFFF \n\t"
    "VBSL.64 d6, d26, d27 \n\t" // red


    // Repeat all operations done on row1 (ptr) on row2 (ptr2)

    "VLD2.8 {d10, d11}, [%[ptr2]]! \n\t"

    "VMOV.64 d30, d10 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d20, #0x000000000000FFFF \n\t"
    "VBSL.64 d20, d10, d30 \n\t"

    "VMOV.64 d30, d11 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d21, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d21, d11, d30 \n\t"


    "VLD2.8 {d12, d13}, [%[ptr2]]! \n\t"

    "VMOV.64 d30, d12 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d22, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d22, d12, d30 \n\t"

    "VMOV.64 d30, d13 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d31, #0x00000000000000FF \n\t"
    "VBSL.64 d31, d13, d30 \n\t"
    "VMOV.64 d30, d31 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d23, #0x000000FFFFFFFFFF \n\t"
    "VBSL.64 d23, d31, d30 \n\t"  


    "VLD2.8 {d14, d15}, [%[ptr2]]! \n\t"

    "VMOV.64 d30, d14 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d31, #0x00000000000000FF \n\t"
    "VBSL.64 d31, d14, d30 \n\t"
    "VMOV.64 d30, d31 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d24, #0x000000FFFFFFFFFF \n\t"
    "VBSL.64 d24, d31, d30 \n\t" 

    "VMOV.64 d30, d15 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d25, #0x0000000000FFFFFF \n\t"
    "VBSL.64 d25, d15, d30 \n\t"


    "VLD2.8 {d16, d17}, [%[ptr2]]! \n\t"

    "VMOV.64 d30, d16 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d26, #0x0000000000FFFFFF \n\t"
    "VBSL.64 d26, d16, d30 \n\t"

    "VMOV.64 d30, d17 \n\t"
    "VSHR.U64 d17, d17, #8 \n\t"
    "VSHR.U64 d30, d30, #16 \n\t"
    "VMOV.I64 d27, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d27, d17, d30 \n\t"


    "VLD2.8 {d18, d19}, [%[ptr2]]! \n\t"

    "VMOV.64 d30, d18 \n\t"
    "VSHR.U64 d18, d18, #8 \n\t"
    "VSHR.U64 d30, d30, #16 \n\t"
    "VMOV.I64 d28, #0x00000000FFFFFFFF \n\t"
    "VBSL.64 d28, d18, d30 \n\t"        

    "VMOV.64 d30, d19 \n\t"
    "VSHR.U64 d30, d30, #8 \n\t"
    "VMOV.I64 d29, #0x000000000000FFFF \n\t"
    "VBSL.64 d29, d19, d30 \n\t"        
    

    "VSHL.I64 d20, d20, #16 \n\t"
    "VEXT.8 d20, d20, d22, #2 \n\t"

    "VSHL.I64 d22, d22, #8 \n\t"
    "VEXT.8 d22, d22, d24, #3 \n\t"

    "VSHL.I64 d24, d24, #16 \n\t"
    "VEXT.8 d24, d24, d26, #5 \n\t"

    "VSHL.I64 d26, d26, #8 \n\t"
    "VEXT.8 d26, d26, d28, #6 \n\t"


    "VSHL.I64 d21, d21, #8 \n\t"
    "VEXT.8 d21, d21, d23, #1 \n\t"

    "VSHL.I64 d23, d23, #16 \n\t"
    "VEXT.8 d23, d23, d25, #3 \n\t"

    "VSHL.I64 d25, d25, #8 \n\t"
    "VEXT.8 d25, d25, d27, #4 \n\t"

    "VSHL.I64 d27, d27, #16 \n\t"
    "VEXT.8 d27, d27, d29, #6 \n\t"


    "VMOV.I64 d11, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d11, d20, d21 \n\t" // blue

    // Ignoring green pixels from row2, could average them with the green from row1...
    // "VMOV.I64 d10, #0x0000FFFF0000FFFF \n\t"
    // "VBSL.64 d10, d20, d21 \n\t" // green

    "VMOV.I64 d13, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d13, d22, d23 \n\t" // blue

    // "VMOV.I64 d12, #0x0000FFFF0000FFFF \n\t"
    // "VBSL.64 d12, d22, d23 \n\t" // green

    "VMOV.I64 d15, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d15, d24, d25 \n\t" // blue

    // "VMOV.I64 d14, #0x0000FFFF0000FFFF \n\t"
    // "VBSL.64 d14, d24, d25 \n\t" // green

    "VMOV.I64 d17, #0xFFFF0000FFFF0000 \n\t"
    "VBSL.64 d17, d26, d27 \n\t" // blue

    // "VMOV.I64 d16, #0x0000FFFF0000FFFF \n\t"
    // "VBSL.64 d16, d26, d27 \n\t" // green


    "VSWP.8 d2, d11 \n\t"
    "VQSHL.U8 d0, d0, #2 \n\t" // Saturating unsigned left shift of each element to fake adding its low 2 bits
    "VQSHL.U8 d1, d1, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d2, d2, #2 \n\t" // Saturating unsigned left shift
    "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

    "VSWP.8 d2, d11 \n\t"
    "VSWP.8 d4, d13 \n\t"
    "VQSHL.U8 d2, d2, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d3, d3, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d4, d4, #2 \n\t" // Saturating unsigned left shift
    "VST3.8 {d2, d3, d4}, [%[out]]! \n\t"

    "VSWP.8 d4, d13 \n\t"
    "VSWP.8 d6, d15 \n\t"
    "VQSHL.U8 d4, d4, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d5, d5, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d6, d6, #2 \n\t" // Saturating unsigned left shift
    "VST3.8 {d4, d5, d6}, [%[out]]! \n\t"

    "VSWP.8 d6, d15 \n\t"
    "VSWP.8 d8, d17 \n\t"
    "VQSHL.U8 d6, d6, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d7, d7, #2 \n\t" // Saturating unsigned left shift
    "VQSHL.U8 d8, d8, #2 \n\t" // Saturating unsigned left shift
    "VST3.8 {d6, d7, d8}, [%[out]]! \n\t"

    "CMP r5, %[numInner] \n\t"
    "BEQ 3f \n\t"
    "ADD r5, r5, #1 \n\t"
    "B 2b \n\t"

    "3: \n\t" // Inner loop done
    "ADD r4, r4, #1 \n\t"
    "ADD %[ptr], %[ptr], #1600 \n\t"
    "ADD %[ptr2], %[ptr2], #1600 \n\t"
    "B 1b \n\t"

    "4: \n\t" // Done
    "POP {r4-r6} \n\t"
    "VPOP {d8-d15} \n\t"

    :
    : [ptr] "r" (r0), 
      [out] "r" (r1), 
      [ptr2] "r" (r2), 
      [numOuter] "r" (r3), 
      [numInner] "r" (r6)
    : "r4","r5","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9",
      "d10","d11","d12","d13","d14","d15","d16","d17","d18","d19",
      "d20","d21","d22","d23","d24","d25","d26","d27","d28","d29",
      "d30","d31", "memory"
  );
}


__attribute__((noinline)) void NeonDownsample2(u8* ptr, u32 width, u32 height, u8 bpp, u8* out)
{
  const u32 kNumBytesProcessedPerLoop = 20;
  const u32 kNumInnerLoops = width / kNumBytesProcessedPerLoop;
  const u32 kNumOuterLoops = height >> 1;

  u8* ptr2 = ptr + width;

  for(int i = 0; i < kNumOuterLoops; ++i)
  {
    for(int j = 0; j < kNumInnerLoops; ++j)
    {
      __asm__ volatile
      (
        "VLD1.8 {d0}, [%[ptr]] \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VLD1.8 {d1}, [%[ptr]] \n\t"
        "VSHL.I64 d0, d0, #32 \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t" // After ld and shl so they can be dual issued
        "VEXT.8 d0, d0, d1, #4 \n\t" // d0 is alternating red and green

        "VLD1.8 {d1}, [%[ptr]] \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VLD1.8 {d2}, [%[ptr]] \n\t"
        "VSHL.I64 d1, d1, #32 \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t" // After ld and shl so they can be dual issued
        "VEXT.8 d1, d1, d2, #4 \n\t" // d1 is alternating red and green

        "VUZP.8 d0, d1 \n\t" // d0 is red, d1 is green

        "VLD1.8 {d2}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "VSHL.I64 d2, d2, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t" // After ld and rev so they can be dual issued
        "VEXT.8 d2, d2, d3, #4 \n\t" // d2 is alternating green and blue

        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d4}, [%[ptr2]] \n\t"
        "VSHL.I64 d3, d3, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t" // After ld and rev so they can be dual issued
        "VEXT.8 d3, d3, d4, #4 \n\t" // d3 is alternating green and blue

        "VUZP.8 d3, d2 \n\t" // d2 is blue, d3 is green

        "VQSHL.U8 d0, d0, #2 \n\t" // Saturating unsigned left shift
        "VQSHL.U8 d1, d1, #2 \n\t" // Saturating unsigned left shift
        "VQSHL.U8 d2, d2, #2 \n\t" // Saturating unsigned left shift

        "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

        : [ptr] "+r" (ptr), 
          [out] "+r" (out), 
          [ptr2] "+r" (ptr2)
        : [ptr] "r" (ptr), 
          [out] "r" (out), 
          [ptr2] "r" (ptr2)
        : "d0","d1","d2","d3","d4", "memory"
      );
    }
    ptr += width;
    ptr2 += width;
  }
  
}
}
