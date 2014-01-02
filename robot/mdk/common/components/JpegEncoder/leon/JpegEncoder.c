///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     JPEG Encoding Functions.
///
/// This is the implementation of a simple JPEG library implementing JPEG Encoding operations.
///


// 1: Includes
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvSvu.h"
#include "DrvSvuDebug.h"
#include "DrvCpr.h"
#include "DrvTimer.h"
#include "swcShaveLoader.h"
#include "app_config.h"
#include "JpegEncoderApi.h"
#include "JpegEncoderDctTables.h"
#include "swcLeonUtils.h"
#include <stdio.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
extern u32 JPGSVE_sym(JPEGe_SHAVE_NO, mainjpeg);
extern u8 JPGSVE_sym(JPEGe_SHAVE_NO, TBL_QUANT_Y);
extern u8 JPGSVE_sym(JPEGe_SHAVE_NO, TBL_QUANT_C);

// 4: Static Local Data
// ----------------------------------------------------------------------------

bitstring fillbits; // filling bitstring for the bit alignment of the EOI marker
int outbytes;
unsigned char JPEG_JPEGGUARD_DATA_SECTION(jpeg_buff)[1024 * 1024];
unsigned char JPEG_BITSREAM_DATA_SECTION(Encoded_Bstr)[1280 * 720 / 4];

DQTinfotype DQTinfo =
{
    0xFFDB, 132,
    0, {
        5,  4,  4,  5,  4,  3,  5,  5,
        4,  5,  6,  6,  5,  6,  8, 14, //  0
        9,  8,  7,  7,  8, 17, 12, 13,
        10, 14, 20, 17, 21, 20, 19, 17, // 16
        19, 19, 22, 24, 31, 27, 22, 23,
        30, 23, 19, 19, 27, 37, 28, 30, // 32
        32, 33, 35, 35, 35, 21, 26, 38,
        41, 38, 34, 41, 31, 34, 35, 34  // 48
    }, 1, {
        6,  6,  6,  8,  7,  8, 16,  9,  9,  16, 34, 22, 19, 22, 34, 34, //  0
        34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, // 16
        34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, // 32
        34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34  // 48
  }
};

const DHTinfotype DHTinfo =
{
    0xFFC4, 0x01A2,
    0, {
        0,   1,   5,   1,   1,   1,   1,   1, //   0
        1,   0,   0,   0,   0,   0,   0,   0  //   8
    }, {
        0,   1,   2,   3,   4,   5,   6,   7, //   0
        8,   9,  10,  11                      //   8
    },
    16, {
        0,   2,   1,   3,   3,   2,   4,   3, //   0
        5,   5,   4,   4,   0,   0,   1, 125  //   8
    }, {
        1,   2,   3,   0,   4,  17,   5,  18, //   0
       33,  49,  65,   6,  19,  81,  97,   7, //   8
       34, 113,  20,  50, 129, 145, 161,   8, //  16
       35,  66, 177, 193,  21,  82, 209, 240, //  24
       36,  51,  98, 114, 130,   9,  10,  22, //  32
       23,  24,  25,  26,  37,  38,  39,  40, //  40
       41,  42,  52,  53,  54,  55,  56,  57, //  48
       58,  67,  68,  69,  70,  71,  72,  73, //  56
       74,  83,  84,  85,  86,  87,  88,  89, //  64
       90,  99, 100, 101, 102, 103, 104, 105, //  72
      106, 115, 116, 117, 118, 119, 120, 121, //  80
      122, 131, 132, 133, 134, 135, 136, 137, //  88
      138, 146, 147, 148, 149, 150, 151, 152, //  96
      153, 154, 162, 163, 164, 165, 166, 167, // 104
      168, 169, 170, 178, 179, 180, 181, 182, // 112
      183, 184, 185, 186, 194, 195, 196, 197, // 120
      198, 199, 200, 201, 202, 210, 211, 212, // 128
      213, 214, 215, 216, 217, 218, 225, 226, // 136
      227, 228, 229, 230, 231, 232, 233, 234, // 144
      241, 242, 243, 244, 245, 246, 247, 248, // 152
      249,   0                                // 160
    },
    1, {
        0,   3,   1,   1,   1,   1,   1,   1, //   0
        1,   1,   1,   0,   0,   0,   0,   0  //   8
    }, {
        0,   1,   2,   3,   4,   5,   6,   7, //   0
        8,   9,  10,  11                      //   8
    },
    17, {
        0,   2,   1,   2,   4,   4,   3,   4, //   0
        7,   5,   4,   4,   0,   1,   2, 119  //   8
    }, {
        0,   1,   2,   3,  17,   4,   5,  33, //   0
       49,   6,  18,  65,  81,   7,  97, 113, //   8
       19,  34,  50, 129,   8,  20,  66, 145, //  16
      161, 177, 193,   9,  35,  51,  82, 240, //  24
       21,  98, 114, 209,  10,  22,  36,  52, //  32
      225,  37, 241,  23,  24,  25,  26,  38, //  40
       39,  40,  41,  42,  53,  54,  55,  56, //  48
       57,  58,  67,  68,  69,  70,  71,  72, //  56
       73,  74,  83,  84,  85,  86,  87,  88, //  64
       89,  90,  99, 100, 101, 102, 103, 104, //  72
      105, 106, 115, 116, 117, 118, 119, 120, //  80
      121, 122, 130, 131, 132, 133, 134, 135, //  88
      136, 137, 138, 146, 147, 148, 149, 150, //  96
      151, 152, 153, 154, 162, 163, 164, 165, // 104
      166, 167, 168, 169, 170, 178, 179, 180, // 112
      181, 182, 183, 184, 185, 186, 194, 195, // 120
      196, 197, 198, 199, 200, 201, 202, 210, // 128
      211, 212, 213, 214, 215, 216, 217, 218, // 136
      226, 227, 228, 229, 230, 231, 232, 233, // 144
      234, 242, 243, 244, 245, 246, 247, 248, // 152
      249,  97                                // 160
  }
};

const SOSinfotype SOSinfo =
{
    0xFFDA,    //SOS marker: Start_of_Scan
    12,        //Scan header len (entire header len without maker field)
    3,         //num_of_components
    1, 0x00,   //ID, {DC entropy table sel, AC entropy table sel}
    2, 0x11,   //ID, {DC entropy table sel, AC entropy table sel}
    3, 0x11,   //ID, {DC entropy table sel, AC entropy table sel}
    0, 0x3F, 0 //Ss, Se, Bf={Ah,Al} these are ct for Baseline-DCT
};

// Start of Frame header
SOF0infotype SOF0info =
{
    0xFFC0,    //Baseline DCT=SOF0 (could have been SOF1/2/)
    17,        //Header len
    8,         //precision 8bit/sample
    0,         //height  =0 => num lines is defined by DNL marker at the end of first scan
    0,         //width   =0 => ???
    3,         //num_of_components = 3 : as we have Y, Cb, Cr
               //Id = component ID
                 //HV = Horizontal & Vertical sampling factor (1 here :0x11)
               //Qt = quantization table selector (1 out of 4), the decoder must have the dequantization table ...
    1, 0x22, 0,//Id_Y,  HV_Y,  QT_Y
    2, 0x11, 1,//Id_Cb, HV_Cb, QT_Cb
    3, 0x11, 1 //Id_Cr, HV_Cr, QT_Cr
};

// Restart interval header
RSTinfotype RSTinfo =
{
    0xFFDD, //DRI marker
    0x4,    //header len
    16     //Ri (num of Blocks in Restart interval
};

const APP0infotype APP0info =
{
    0xFFE0, 16,
    'J', 'F', 'I', 'F', 0,
    1, 1,
    0,
    1, 1,
    0, 0
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

//andreil: some wrappers:
void JPEG_CODE_SECTION(writebyte)(unsigned char b)
{
    swcLeonSetByte(&jpeg_buff[outbytes++], le_pointer, b);
    asm volatile ("nop; nop; nop; nop; nop; nop; nop; nop;");
}

void JPEG_CODE_SECTION(writehword)(unsigned short w)
{
    // ( writebyte( (w)/256 ), writebyte( (w)%256 ) )
    writebyte(w >> 8);
    writebyte(w & 0xFF);
}

void JPEG_CODE_SECTION(write_APP0info())
{
    writehword(APP0info.marker);
    writehword(APP0info.length);
    writebyte('J');
    writebyte('F');
    writebyte('I');
    writebyte('F');
    writebyte(0);
    writebyte(APP0info.versionhi);
    writebyte(APP0info.versionlo);
    writebyte(APP0info.xyunits);
    writehword(APP0info.xdensity);
    writehword(APP0info.ydensity);
    writebyte(APP0info.thumbnwidth);
    writebyte(APP0info.thumbnheight);
}


// We should overwrite width and height
void JPEG_CODE_SECTION(write_SOF0info)()
{
    writehword(SOF0info.marker);
    writehword(SOF0info.length);
    writebyte(SOF0info.precision);
    writehword(SOF0info.height);
    writehword(SOF0info.width);
    writebyte(SOF0info.nrofcomponents);
    writebyte(SOF0info.IdY);
    writebyte(SOF0info.HVY);
    writebyte(SOF0info.QTY);
    writebyte(SOF0info.IdCb);
    writebyte(SOF0info.HVCb);
    writebyte(SOF0info.QTCb);
    writebyte(SOF0info.IdCr);
    writebyte(SOF0info.HVCr);
    writebyte(SOF0info.QTCr);
}


void JPEG_CODE_SECTION(write_DQTinfo)()
{
    unsigned char i;
    writehword(DQTinfo.marker);
    writehword(DQTinfo.length);
    writebyte( DQTinfo.QTYinfo);
    for (i = 0; i < 64; ++i)
    {
        writebyte(DQTinfo.Ytable[i]);
    }
    writebyte(DQTinfo.QTCbinfo);
    for (i = 0; i < 64; ++i)
    {
        writebyte(DQTinfo.Cbtable[i]);
    }
}


void JPEG_CODE_SECTION(write_DHTinfo)()
{
    unsigned char i;
    writehword(DHTinfo.marker);
    writehword(DHTinfo.length);
    writebyte(DHTinfo.HTYDCinfo);

    for (i = 0; i < 16; ++i)
    {
        writebyte(DHTinfo.YDC_nrcodes[i]);
    }
    for (i = 0; i <= 11; ++i)
    {
        writebyte(DHTinfo.YDC_values[i]);
    }
    writebyte(DHTinfo.HTYACinfo);
    for (i = 0; i < 16; ++i)
    {
        writebyte( DHTinfo.YAC_nrcodes[i] );
    }
    for (i = 0; i <= 161; ++i)
    {
        writebyte(DHTinfo.YAC_values[i]);
    }
    writebyte(DHTinfo.HTCbDCinfo);
    for (i = 0; i < 16; ++i)
    {
        writebyte(DHTinfo.CbDC_nrcodes[i]);
    }
    for (i = 0; i <= 11; ++i)
    {
        writebyte(DHTinfo.CbDC_values[i]);
    }
    writebyte(DHTinfo.HTCbACinfo);
    for (i = 0; i < 16; ++i)
    {
        writebyte(DHTinfo.CbAC_nrcodes[i]);
    }
    for (i = 0; i <= 161; ++i)
    {
        writebyte(DHTinfo.CbAC_values[i]);
    }
}


// Nothing to overwrite for SOSinfo
void JPEG_CODE_SECTION(write_SOSinfo())
{
    writehword(SOSinfo.marker);
    writehword(SOSinfo.length);
    writebyte(SOSinfo.nrofcomponents);
    writebyte(SOSinfo.IdY);
    writebyte(SOSinfo.HTY);
    writebyte(SOSinfo.IdCb);
    writebyte(SOSinfo.HTCb);
    writebyte(SOSinfo.IdCr);
    writebyte(SOSinfo.HTCr);
    writebyte(SOSinfo.Ss);
    writebyte(SOSinfo.Se);
    writebyte(SOSinfo.Bf);
}


//andreil: write restart interval....
void JPEG_CODE_SECTION(write_DRIinfo)()
{
    writehword(RSTinfo.DRI);
    writehword(RSTinfo.Lr);
    writehword(RSTinfo.Ri);

}
// Set Quantization matrices(Q_L, Q_C) based on [Quantization level]
void JPEG_CODE_SECTION(setQmatrices)(int qlevel)
{
    int i, j;

    Q_level = qlevel;
    memcpy(Q_L, Q50_luma,   8 * 8 * 4);//default Q LUMA
    memcpy(Q_C, Q50_chroma, 8 * 8 * 4);//default Q CHROMA

    if (qlevel < 50)
    {
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                Q_L[i][j] *= (50.0f / qlevel); //adjust LUMA
                Q_C[i][j] *= (50.0f / qlevel); //adjust CHROMA
             }
        }
    }
    else
        if (qlevel > 50)
        {
            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    Q_L[i][j] *= (100.0f - qlevel) / 50.0f; //adjust LUMA
                    Q_C[i][j] *= (100.0f - qlevel) / 50.0f; //adjust CHROMA
                }
            }
         }
  //else qlevel = 50, nothing to do (defaults)
}


void JPEG_CODE_SECTION(calc_idct)(float a[N][N],float b[N][N],float c[N][N])
{
    int i,j;
    for (i=0; i<N; i++)
    {
        for (j=0; j<N; j++)
        {
            c[i][j]= (b[i][j] / 16384) *  a[i][j];
        }
    }
}


void JPEG_CODE_SECTION(calc_dct)(float a[N][N], float b[N][N], float c[N][N])
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            c[i][j] = 1 / ((b[i][j] / 2048) *  a[i][j]);
        }
    }
}


// Sets quantization matrices and computes DCT/IDCT coefficients
// based on input "level"
void JPEG_CODE_SECTION(compute_DCT_coefs)(int qlevel)
{
    float qcoefs_L[N][N]; //DCT coefs
    float qcoefs_C[N][N];
    float iqcoefs_L[N][N]; //IDCT coefs
    float iqcoefs_C[N][N];

    setQmatrices(qlevel);

    calc_dct (Q_L, CT_COEFS, qcoefs_L);  memcpy(&fdtbl_Y[0],  &qcoefs_L[0][0], sizeof(fdtbl_Y));
    calc_dct (Q_C, CT_COEFS, qcoefs_C);  memcpy(&fdtbl_Cb[0], &qcoefs_C[0][0], sizeof(fdtbl_Cb));

    calc_idct(Q_L, CT_COEFS, iqcoefs_L); memcpy(&IDCT_Yfactors[0], &iqcoefs_L[0][0], sizeof(IDCT_Yfactors));
    calc_idct(Q_C, CT_COEFS, iqcoefs_C); memcpy(&IDCT_Cfactors[0], &iqcoefs_C[0][0], sizeof(IDCT_Cfactors));
}

void JPEG_CODE_SECTION(SvuSetParams)(jpegFrameSpec *jFmSpec, u32 shaveNumber)
{
    DrvSvutSrfWrite(shaveNumber, 31, (u32)jFmSpec->Y);
    DrvSvutSrfWrite(shaveNumber, 30, (u32)jFmSpec->U);
    DrvSvutSrfWrite(shaveNumber, 29, (u32)jFmSpec->V);
    DrvSvutIrfWrite(shaveNumber, 26, (u32)Encoded_Bstr);
    DrvSvutIrfWrite(shaveNumber, 31, jFmSpec->width);
    DrvSvutIrfWrite(shaveNumber, 30, jFmSpec->height);
}

// The JPEG encoding algorithm
void JPEG_CODE_SECTION(JPEG_encode)(jpegFrameSpec *jFmSpec, unsigned int quality_coef)
{
    int i, len, bitfill;
    unsigned char *src;
    unsigned int  bitbuff;
    u32 *pSrc32, *pDst32;
    static int first_time = 1;
    tyTimeStamp executionTimer;

    //only compute tables the first time...
    if (first_time)
    {
        first_time = 0;

        //Based on Quantization Level (Q_LEVEL), compute Quantization Matrices:
        compute_DCT_coefs(quality_coef);

        //Based on new data: update tables that are to be stored in JPEG file:
        for (i = 0; i < 64; i++) DQTinfo.Ytable [zigzag[i]] = Q_L[i >> 3][i & 0x7];
        for (i = 0; i < 64; i++) DQTinfo.Cbtable[zigzag[i]] = Q_C[i >> 3][i & 0x7];

        //Update quantization tables:
        pSrc32 = (u32*)&fdtbl_Y[0];
        pDst32 = (u32*)((u32)&JPGSVE_sym(JPEGe_SHAVE_NO, TBL_QUANT_Y));
        for (i = 0; i < 64; i++) pDst32[i] = pSrc32[i];

        pSrc32 = (u32*)&fdtbl_Cb[0];
        pDst32 = (u32*)((u32)&JPGSVE_sym(JPEGe_SHAVE_NO, TBL_QUANT_C));
        for (i = 0; i < 64; i++) pDst32[i] = pSrc32[i];
    }

    SOF0info.width  = jFmSpec->width;
    SOF0info.height = jFmSpec->height;

    //Write headers:
    outbytes = 0;
    writehword(0xFFD8); //WR: SOI marker
    write_APP0info();   //WR: App header
    write_DQTinfo();    //WR: Dequantize table
    write_SOF0info();   //WR: StartOfFrame header
    write_DHTinfo();    //
    write_SOSinfo();    //WR: StartOfScan header
    //Encode Stream:

    // Reset Shaves
    swcResetShave(JPEGe_SHAVE_NO);
    SvuSetParams(jFmSpec, JPEGe_SHAVE_NO);
    swcStartShave(JPEGe_SHAVE_NO, (u32)&JPGSVE_sym(JPEGe_SHAVE_NO, mainjpeg));
    swcWaitShave(JPEGe_SHAVE_NO);

    //Get the number of encoded bytes:
    len  =  DrvSvutIrfRead(SVU(JPEGe_SHAVE_NO), IRF(26));
    len  -= (u32)Encoded_Bstr;

    bitfill = DrvSvutIrfRead(SVU(JPEGe_SHAVE_NO), IRF(21));

    //printf("\nWARNING : bitfill = %d\n", bitfill);
    //Read whatever is left of bitbuff and append it to stream
    bitbuff = DrvSvutIrfRead(SVU(JPEGe_SHAVE_NO), IRF(22));

    // Copy encoded bytes from slice to JPEG-buffer
    src = (unsigned char*)Encoded_Bstr;

    swcLeonMemCpy(&jpeg_buff[outbytes], le_pointer, src, le_pointer, len);

    outbytes += len;

    //if (bitfill != 0)
    {
        writebyte((bitbuff >> 24) & 0xFF);
        writebyte((bitbuff >> 16) & 0xFF);
    }

    // Append terminator... handle bit alignment...
    writehword(0xFFD9);//WR : EOI marker

    //Add 3 bytes to compensate for 4 bytes SPARC alignment. Just to make sure
    //all of the useful information is also brought in
    outbytes += 3;

}
