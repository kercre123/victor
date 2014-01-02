///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Pattern Generator.
///

// 1: Includes
// ----------------------------------------------------------------------------

#include "PatternGeneratorApi.h"
#include "stdio.h"
#include "isaac_registers.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// must be even because YUV422. we write 2 pixel in the same time in memory
#define BORDER 4

// threshold for pattern checker 
#define THRESHOLD_Y     45
#define THRESHOLD_U     22
#define THRESHOLD_V     18

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
unsigned int patternWIDTH = 0;
unsigned int patternHEIGHT = 0;
char *patternYuvPlane;
PatternRGBColor defaultColor[] = { {255, 255, 255},
                                   {255, 255,   0},
                                   {  0, 200, 255},
                                   {  0, 128,   0},
                                   {255,   0, 255},
                                   {255,   0,   0},
                                   {  0,   0, 255},
                                   {  0,   0,   0}} ;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------

// RGB -> YUV convertor 
void converterRGBtoYUV(PatternRGBColor colorRGB, PatternYUVColor *colorYUV)
{
    (*colorYUV).Y = (( 66 * colorRGB.R + 129 * colorRGB.G +  25 * colorRGB.B + 128) >> 8) +  16;
    (*colorYUV).U = ((-38 * colorRGB.R -  74 * colorRGB.G + 112 * colorRGB.B + 128) >> 8) + 128;
    (*colorYUV).V = ((112 * colorRGB.R -  94 * colorRGB.G -  18 * colorRGB.B + 128) >> 8) + 128;
}


// Validate coordinates to avoid memory illegal accesses
void validateCoords(PatternPoint *point)
{
    if (point->x > patternWIDTH)
    {
        point->x = patternWIDTH - 1;
    }

    if (point->y > patternHEIGHT)
    {
        point->y = patternHEIGHT - 1;
    }

}


// This function draws a rectangle specifing topLeft and bottomRight positions in the frame
void drawColorRect(PatternPoint topLeftCorner, PatternPoint bottomRightCorner, PatternYUVColor color)
{
    unsigned int i,j;
    validateCoords(&topLeftCorner);
    validateCoords(&bottomRightCorner);

    for (i = topLeftCorner.y; i < bottomRightCorner.y; i++)
    {
        for (j = (topLeftCorner.x / 2); j < (bottomRightCorner.x / 2); j++)
        {
            patternYuvPlane[2 * i * patternWIDTH + 4 * j] = color.Y;
            patternYuvPlane[2 * i * patternWIDTH + 4 * j + 1] = color.U;
            patternYuvPlane[2 * i * patternWIDTH + 4 * j + 2] = color.Y;
            patternYuvPlane[2 * i * patternWIDTH + 4 * j + 3] = color.V;
        }
    }

}


// Generate the horizontal pattern
unsigned int CreateHorizontalColorBars (frameBuffer *frame, unsigned int interlaced)
{
    // declare
    PatternYUVColor myColorYUV;
    PatternPoint myTopLeft, myBottomRight;
    unsigned int k, w, lineWidth;

    // init
    patternWIDTH = frame->spec.width;
    patternHEIGHT = frame->spec.height;
    myTopLeft.x = 0;
    myTopLeft.y = 0;
    myBottomRight.x = frame->spec.width;
    myBottomRight.y = frame->spec.height;
    patternYuvPlane = (char *)frame->p1;

    // all image white
    converterRGBtoYUV(defaultColor[0], &myColorYUV);
    drawColorRect(myTopLeft, myBottomRight, myColorYUV);

    // add color line
    lineWidth = (patternHEIGHT - 2 * BORDER) / 8;
    myTopLeft.x = BORDER;
    myTopLeft.y = BORDER;
    myBottomRight.x = patternWIDTH - BORDER;
    myBottomRight.y = BORDER + lineWidth;

    for (k = 0; k < 7; k++)
    {
        converterRGBtoYUV(defaultColor[k], &myColorYUV);
        drawColorRect(myTopLeft, myBottomRight, myColorYUV);
        myTopLeft.y += lineWidth;
        myBottomRight.y += lineWidth;
    }
    converterRGBtoYUV(defaultColor[7], &myColorYUV);
    myBottomRight.y = patternHEIGHT - BORDER;
    drawColorRect(myTopLeft, myBottomRight, myColorYUV);

    // check if the frame is interlaced
    if (interlaced == 1)
    {
        converterRGBtoYUV(defaultColor[7], &myColorYUV);
        for (k = BORDER; k < (patternHEIGHT - BORDER); k++)
            for (w = 0; w < (BORDER / 2); w++)
                if ((k % 2) == 0)
                {
                    // Left Border
                    patternYuvPlane[2* k * patternWIDTH + 4 * w] = myColorYUV.Y;
                    patternYuvPlane[2* k * patternWIDTH + 4 * w + 1] = myColorYUV.U;
                    patternYuvPlane[2* k * patternWIDTH + 4 * w + 2] = myColorYUV.Y;
                    patternYuvPlane[2* k * patternWIDTH + 4 * w + 3] = myColorYUV.V;
                    // Right Border
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 4] = myColorYUV.Y;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 3] = myColorYUV.U;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 2] = myColorYUV.Y;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 1] = myColorYUV.V;
                }
    }

    return 1;
}


// Generate the vertical pattern
unsigned int CreateVerticalColorBars (frameBuffer *frame, unsigned int interlaced)
{
    // declare
    PatternYUVColor myColorYUV;
    PatternPoint myTopLeft, myBottomRight;
    unsigned int k, w, lineWidth;

    // init
    patternWIDTH = frame->spec.width;
    patternHEIGHT = frame->spec.height;
    myTopLeft.x = 0;
    myTopLeft.y = 0;
    myBottomRight.x = frame->spec.width;
    myBottomRight.y = frame->spec.height;
    patternYuvPlane = (char *)frame->p1;

    // all image white
    converterRGBtoYUV(defaultColor[0], &myColorYUV);
    drawColorRect(myTopLeft, myBottomRight, myColorYUV);

    // add color line
    lineWidth = (patternWIDTH - 2 * BORDER) / 8;
    myTopLeft.x = BORDER;
    myTopLeft.y = BORDER;
    myBottomRight.x = BORDER + lineWidth;
    myBottomRight.y = patternHEIGHT - BORDER;

    for (k = 0; k < 7; k++)
    {
        converterRGBtoYUV(defaultColor[k], &myColorYUV);
        drawColorRect(myTopLeft, myBottomRight, myColorYUV);
        myTopLeft.x += lineWidth;
        myBottomRight.x += lineWidth;
    }
    converterRGBtoYUV(defaultColor[7], &myColorYUV);
    myBottomRight.x = frame->spec.width - BORDER;
    drawColorRect(myTopLeft, myBottomRight, myColorYUV);

    // check if the frame is interlaced
    if (interlaced == 1)
    {
        converterRGBtoYUV(defaultColor[7], &myColorYUV);
        for (k = BORDER; k < (frame->spec.height - BORDER); k++)
            for (w = 0; w < (BORDER / 2); w++)
                if ((k % 2) == 0)
                {
                    // Left Border
                    patternYuvPlane[2 * k * patternWIDTH + 4 * w] = myColorYUV.Y;
                    patternYuvPlane[2 * k * patternWIDTH + 4 * w + 1] = myColorYUV.U;
                    patternYuvPlane[2 * k * patternWIDTH + 4 * w + 2] = myColorYUV.Y;
                    patternYuvPlane[2 * k * patternWIDTH + 4 * w + 3] = myColorYUV.V;
                    // Right Border
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 4] = myColorYUV.Y;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 3] = myColorYUV.U;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 2] = myColorYUV.Y;
                    patternYuvPlane[2 * (k + 1) * patternWIDTH - 4 * w - 1] = myColorYUV.V;
                }
    }

    return 1;
}


// Custom color stripes pattern
unsigned int CreateColorStripesPattern (frameBuffer *frame)
{
    // declare
    PatternRGBColor leftColor, rightColor;
    PatternYUVColor myLeftColorYUV, myRightColorYUV;
    PatternPoint myTopLeft, myBottomRight;
    unsigned int y, stripeWidth;

    // init
    patternWIDTH = frame->spec.width;
    patternHEIGHT = frame->spec.height;
    myTopLeft.x = 0;
    myTopLeft.y = 0;
    myBottomRight.x = frame->spec.width;
    myBottomRight.y = frame->spec.height;
    patternYuvPlane = (char *)frame->p1;

    // all image white
    stripeWidth = frame->spec.height / 64;
    converterRGBtoYUV(defaultColor[0], &myLeftColorYUV);
    drawColorRect(myTopLeft, myBottomRight, myLeftColorYUV);

    // add color line
    myBottomRight.x = patternWIDTH / 2;
    myBottomRight.y = stripeWidth;

    for (y = 0;  y < 64; y++)
    {
        leftColor.R = ((y >> 0) & 0x1) * 255;
        leftColor.G = ((y >> 1) & 0x1) * 255;
        leftColor.B = ((y >> 2) & 0x1) * 255;
        converterRGBtoYUV(leftColor, &myLeftColorYUV);
        drawColorRect(myTopLeft, myBottomRight, myLeftColorYUV);

        myTopLeft.x = frame->spec.width / 2;
        myBottomRight.x = frame->spec.width;
        rightColor.R = ((y >> 3) & 0x1) * 255;
        rightColor.G = ((y >> 4) & 0x1) * 255;
        rightColor.B = ((y >> 5) & 0x1) * 255;
        converterRGBtoYUV(rightColor, &myRightColorYUV);
        drawColorRect(myTopLeft, myBottomRight, myRightColorYUV);

        myTopLeft.x = 0;
        myTopLeft.y += stripeWidth;
        myBottomRight.y += stripeWidth;
    }

    return 1;
}


// Grey gradient pattern
unsigned int CreateLinearGreyPattern (frameBuffer *frame)
{
     // declare
    PatternYUVColor myColorYUV;
    PatternRGBColor myColorRGB;
    PatternPoint myTopLeft, myBottomRight;
    unsigned int k, lineWidth;

    // init
    patternWIDTH = frame->spec.width;
    patternHEIGHT = frame->spec.height;
    myTopLeft.x = 0;
    myTopLeft.y = 0;
    myBottomRight.x = frame->spec.width;
    myBottomRight.y = frame->spec.height;
    patternYuvPlane = (char *)frame->p1;

    // all image white
    converterRGBtoYUV(defaultColor[0], &myColorYUV);
    drawColorRect(myTopLeft, myBottomRight, myColorYUV);

    // add color line
    lineWidth = patternWIDTH / 17;
    myBottomRight.x = lineWidth;
    myBottomRight.y = patternHEIGHT;

    for (k = 0; k < 17; k++)
    {
        myColorRGB.R = k * 15;
        myColorRGB.G = k * 15;
        myColorRGB.B = k * 15;
        converterRGBtoYUV(myColorRGB, &myColorYUV);
        drawColorRect(myTopLeft, myBottomRight, myColorYUV);
        myTopLeft.x += lineWidth;
        if (k == 15)
            myBottomRight.x = frame->spec.width;
        else
            myBottomRight.x += lineWidth;
    }

    return 1;
}


// Check the pattern
unsigned int PatternCheck (frameBuffer *inputFrame, frameBuffer *outputFrame, unsigned int interlaced)
{
    printf("Start check pattern !\n");
     
    // declare
    unsigned int resultOfCheck = 1;
    unsigned int i, j;
    char *inputPattern, *outputPattern;
    char message[255] = ""; 
    unsigned int testLineOffset = 0;

    // set offset for interlaced resolution
    if (interlaced == 1)
    {
        testLineOffset = 2;
    }
    
    // test Vic
    inputPattern  = (char *)inputFrame->p1;
    outputPattern = (char *)outputFrame->p1;


    // test pixel by pixel
    for (j = testLineOffset; j < (outputFrame->spec.height - testLineOffset); j++)
    {
    	for (i = 0; i < (2 * outputFrame->spec.width); i += 8) /* one line contain 2*width char */
    	{
    		//CheckPatternPrintf ("%c - %c\n", inputPattern[j*2*outputFrame->spec.width + i], outputPattern[j*2*outputFrame->spec.width + i]);
    		//CheckPatternPrintf ("%c - %c\n", inputPattern[j*2*outputFrame->spec.width + i+1], outputPattern[j*2*outputFrame->spec.width + i+1]);

    		if ((inputPattern[j * 2 * outputFrame->spec.width + i] > (outputPattern[j * 2 * outputFrame->spec.width + i] + THRESHOLD_Y)) ||
    				(inputPattern[j * 2 * outputFrame->spec.width + i] < (outputPattern[j * 2 * outputFrame->spec.width + i] - THRESHOLD_Y)))
    		{
    			sprintf(message, "Diffrent pixel (%d , %d)", i, j);
    			printf("%s \n", message);
    			resultOfCheck = 0;
    			break;
    		}
    		if ((inputPattern[j * 2 * outputFrame->spec.width + i + 1] > (outputPattern[j * 2 * outputFrame->spec.width + i + 1] + THRESHOLD_U)) ||
    				(inputPattern[j * 2 * outputFrame->spec.width + i + 1] < (outputPattern[j * 2 * outputFrame->spec.width + i + 1] - THRESHOLD_U)))
    		{
    			sprintf(message, "Diffrent pixel (%d , %d)", i, j);
    			printf("%s \n", message);
    			resultOfCheck = 0;
    			break;
    		}
    		if ((inputPattern[j * 2 * outputFrame->spec.width + i + 3] > (outputPattern[j * 2 * outputFrame->spec.width + i + 3] + THRESHOLD_V)) ||
    				(inputPattern[j * 2 * outputFrame->spec.width + i + 3] < (outputPattern[j * 2 * outputFrame->spec.width + i + 3] - THRESHOLD_V)))
    		{
    			sprintf(message, "Diffrent pixel (%d , %d)", i, j);
    			printf("%s \n", message);
    			resultOfCheck = 0;
    			break;
    		}
    	}

    }

    if (resultOfCheck == 0)
    {
    	printf("Pattern Check Mismatch !\n");
    }
    else 
    {
    	printf("Pattern Checked !\n");
    }

    printf("Finish check pattern !\n\n");
    return resultOfCheck;
}

