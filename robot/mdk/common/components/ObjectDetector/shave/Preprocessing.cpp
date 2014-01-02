///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///

#include "Preprocessing.h"
#include <string.h>
#include <mv_types.h>
#include <mvcv.h>
#include <alloc.h>
#include <Defines.h>
#include "../include/ObjectDetection.h"
//#include <profile.h>

using namespace mvcv;

u32 dbgPre = 0;

void swapBuffs(u8* buffs[2])
{
	u8* tmp = buffs[0];
	buffs[0] = buffs[1];
	buffs[1] = tmp;
}

void rotateBuffers(u8* ptrs[], u32 num)
{
	 // Keep the first pointer because it will be overwritten
	 u8* tmp = ptrs[0];

	 // Shift all pointers to the left location
	 for (u32 i = 0; i < num - 1; i++)
	  ptrs[i] = ptrs[i + 1];

	 // Last pointer becomes the first
	 ptrs[num - 1] = tmp;
}

void Convert12bppTo8bpp(Mat* const in, Mat* const out)
{
	u8* inLine = (u8*)mv_malloc(in->step1());
	u8* outLine = (u8*)mv_malloc(out->step1());

	in->data = L2CACHE(in->data);

	for (u32 i = 0; i < in->rows; i++)
	{
		scDmaCopyBlocking(inLine, in->ptr(i), in->step1());

		convert12BppTo8Bpp(outLine, inLine, in->cols);

		scDmaCopyBlocking(out->ptr(i), outLine, out->step1());
	}

	in->data = L2BYPASS(in->data);

	mv_free(outLine);
	mv_free(inLine);
}

/// Compute integral image
void IntegralImageStream(Mat* const img, Mat* const sum, Mat* const sqsum)
{
    u8* sumLine[2];
	u8* sqsumLine[2];
	u32* sumBuff;
	fp32* sqsumBuff;
	u8* imgLine[2];
	u32 line = 0;
	u32 sumLineSize = sum->cols * sizeof(u32);
	u32 sqsumLineSize = sqsum->cols * sizeof(fp32);

	//profile& pIntImgStr = profileManager::getProfile(__FUNCTION__);
	//pIntImgStr.start();

	img->data = L2CACHE(img->data);

	// Allocate memory for lines
	sumBuff = (u32*)mv_malloc(sumLineSize + 1);
	sqsumBuff = (fp32*)mv_malloc(sqsumLineSize + 1);

	sumLine[0] = (u8*)mv_malloc(sum->step1());
	sumLine[1] = (u8*)mv_malloc(sum->step1());

	sqsumLine[0] = (u8*)mv_malloc(sqsum->step1());
	sqsumLine[1] = (u8*)mv_malloc(sqsum->step1());
	
	imgLine[0] = (u8*)mv_malloc(img->step1() + 1);
	imgLine[1] = (u8*)mv_malloc(img->step1() + 1);

	imgLine[0][0] = 0;
	imgLine[1][0] = 0;

	// Set sum buffers to 0 so we won't start with garbage values
	memset(sumBuff, 0x00, sumLineSize + 1);
	memset(sqsumBuff, 0x00, sqsumLineSize + 1);
	
	u8* imgLintPtr;

	// Fill the first line of integrals with 0
	scDmaSetup(DMA_TASK_0, (u8*)sumBuff, sum->ptr(0), sum->step1());
	scDmaSetup(DMA_TASK_1, (u8*)sqsumBuff, sqsum->ptr(0), sqsum->step1());
	// DMA in the first image line
	imgLintPtr = imgLine[0] + 1;
	scDmaSetup(DMA_TASK_2, (u8*)img->ptr(0), imgLintPtr, img->step1());
	imgLintPtr = imgLine[1] + 1;
	scDmaSetup(DMA_TASK_3, (u8*)img->ptr(1), imgLintPtr, img->step1());
	scDmaStart(DMA_TASK_3);
	scDmaWaitFinished();
	
	integralimage_sum_u32_asm(&imgLine[0], &sumLine[0], sumBuff, img->cols + 1);
	integralimage_sqsum_f32_asm(&imgLine[0], &sqsumLine[0], sqsumBuff, img->cols + 1);

	for (line = 2; line < img->rows; line++)
	{
		imgLintPtr = imgLine[0] + 1;

		// DMA in the next line
		scDmaSetup(DMA_TASK_0, (u8*)img->ptr(line), imgLintPtr, img->step1());
		// DMA out the results
		scDmaSetup(DMA_TASK_1, sumLine[0], sum->ptr(line - 1), sum->step1());
		scDmaSetup(DMA_TASK_2, sqsumLine[0], sqsum->ptr(line - 1), sqsum->step1());
		scDmaStart(DMA_TASK_2);

		integralimage_sum_u32_asm(&imgLine[1], &sumLine[1], sumBuff, img->cols + 1);
		integralimage_sqsum_f32_asm(&imgLine[1], &sqsumLine[1], sqsumBuff, img->cols + 1);

		swapBuffs(imgLine);
		swapBuffs(sumLine);
		swapBuffs(sqsumLine);

		scDmaWaitFinished();
	}

	// DMA out the last results
	scDmaSetup(DMA_TASK_0, sumLine[0], sum->ptr(line), sum->step1());
	scDmaSetup(DMA_TASK_1, sqsumLine[0], sqsum->ptr(line), sqsum->step1());
	scDmaStart(DMA_TASK_1);

	scDmaWaitFinished();

	mv_free(sumBuff);
	mv_free(sqsumBuff);
	mv_free(sumLine[0]);
	mv_free(sumLine[1]);
	mv_free(sqsumLine[0]);
	mv_free(sqsumLine[1]);
	mv_free(imgLine[0]);
	mv_free(imgLine[1]);

	img->data = L2BYPASS(img->data);

	//pIntImgStr.stop();

#ifdef DUMP_IMG_TO_FILE
    FILE* g;
    g = fopen("integralImageStreamedMvCv.bin", "wb");
    fwrite(sum->data, 1, sum->step * sum->rows, g);
    fclose(g);
    FILE* h;
    h = fopen("integralSquaredImageStreamedMvCv.bin", "wb");
    fwrite(sqsum->data, 1, sqsum->step * sqsum->rows, h);
    fclose(h);
#endif
}

void Downsample(Mat* const img, Mat* const small)
{
    //profile& pDownsampleImg = profileManager::getProfile(__FUNCTION__);
	//pDownsampleImg.start();
    u8* imgLine[5];
    for (int i = 0; i < 5; i++)
    {
        imgLine[i] = (u8*)mv_malloc(img->step1());
    }
    int cnt =0;
    for (int i = 0; i < 5; cnt++, i+=2)
    {
        scDmaCopyBlocking(imgLine[cnt], img->ptr(i), img->step1());
    }

    u8* outLine;
    outLine = (u8*)mv_malloc(small->step1());
    for (int i =0; i < img->rows; i+=2, cnt++)
    {
        pyrdown_asm(imgLine, &outLine, img->cols);
        rotateBuffers(imgLine, 5);
#if 1 /*NOTE: To check */
		if(i < img->rows - 6)
		{
			scDmaCopyBlocking(imgLine[4], img->ptr(i), img->step1());
			scDmaCopyBlocking(small->ptr(cnt), outLine, small->step1());
		}
#else
        scDmaCopyBlocking(imgLine[4], img->ptr(i), img->step1());
        scDmaCopyBlocking(small->ptr(cnt), outLine, small->step1());
#endif
    }
    
    for (int i = 0; i<5; i++)
    {
        mv_free(imgLine[i]);
    }
    mv_free(outLine);
    //pDownsampleImg.stop();
}

extern "C" void Preprocessing(frameBuffer* const sensorImg, ObjectDetectionConfig* const cfg)
{
    SHAVE_START;
	
	dbgPre++;

	// Create image objects
	Mat sens(sensorImg->spec.height, sensorImg->spec.width, (sensorImg->spec.bytesPP == 2) ? CV_16U : CV_8U, sensorImg->p1);
	Mat img(cfg->input_image.spec.height, cfg->input_image.spec.width, CV_8UC1, cfg->input_image.p1);
    Mat small(cfg->input_image.spec.height / DOWNSAMPLE_FACTOR, cfg->input_image.spec.width / DOWNSAMPLE_FACTOR, CV_8UC1, cfg->small_image.p1); 
    Mat sum(small.rows + 1, small.cols + 1, CV_32SC1, cfg->sum_image.p1);
	Mat sqsum(small.rows + 1, small.cols + 1, CV_32FC1, cfg->sqsum_image.p1); 

	// Convert to 8 bits/pixel
	if (sensorImg->spec.bytesPP == 2)
		Convert12bppTo8bpp(&sens, &img);
	else
		img = sens;

	// Downsample the image by 2
	if (DOWNSAMPLE_FACTOR == 2)
		Downsample(&img, &small);
	else
		small = img;

	// Compute integral images
    IntegralImageStream(&small, &sum, &sqsum);

	dbgPre++;

#if 0
	for (u32 i = 0; i < profileManager::getProfileNumber(); i++)
	{
		profile& prof = profileManager::getProfile(i);
		printf("%s: %.2f ms\n", prof.name(), prof.mtime());
	}
#endif

	SHAVE_STOP;
}