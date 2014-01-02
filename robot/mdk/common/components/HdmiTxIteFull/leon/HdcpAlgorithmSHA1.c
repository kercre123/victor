
/*************************************************************

   @file                <sha1.c>
   @author              Max.Kao@ite.com.tw
   @date                2011/11/18
   @fileversion:        ITE_HDMI1.4_AVR_SAMPLECODE_2.04

 *************************************************************/
/* Alex: Useful Link : http://en.wikipedia.org/wiki/SHA-1
 ************************************************************/
#include "TxHdcp.h"

#define rol(x,y) (((x) << (y)) | (((ULONG)x) >> (32-y)))

ULONG w[80];

void HDMITXITEFULL_CODE_SECTION(SHA_Simple) (void *p, WORD len, BYTE *output)
{
    WORD i,t;
    ULONG c;
    BYTE *pBuff = (BYTE *)p;

    #ifdef DEBUG_TIMER
    AuthCalVRStart=ucTickCount;
    #endif

    for (i = 0; i < len; i++)
    {
        t = i / 4;

        if ((i % 4) == 0)
        {
            w[t]=0;
        }

        c = pBuff[i];
        c <<= (3 - (i % 4)) * 8;
        w[t] |= c;

        #ifdef _PRINT_HDMI_TX_HDCP_
        // //TXHDCP_DEBUG_PRINTF(("pBuff[%d]=%X,c=%08lX,w[%d]=%08lX\n",i,pBuff[i],c,t,w[t]));
        #endif
    }

    t = i / 4;

    if (i % 4 == 0)
    {
        w[t]=0;
    }

    //c=0x80 << ((3-i%4)*24);
    c = 0x80;
    c <<= ((3 - i % 4) * 8);
    w[t] |= c;
    t++;
    
    for(; t < 15; t++)
    {
        w[t] = 0;
    }

    w[15] = len * 8;

    SHATransform(sha);

    #ifdef DEBUG_TIMER
    AuthCalVREnd=ucTickCount;
    #endif

    for (i = 0; i < 5; i++)
    {
        output[i * 4 + 3] = (BYTE)((sha[i] >> 24) & 0xFF);
        output[i * 4 + 2] = (BYTE)((sha[i] >> 16) & 0xFF);
        output[i * 4 + 1] = (BYTE)((sha[i] >>  8) & 0xFF);
        output[i * 4 + 0] = (BYTE)(sha[i] & 0xFF);
    }
}

void HDMITXITEFULL_CODE_SECTION(SHATransform) (ULONG * h)
{
    ULONG    t;
    ULONG tmp;

    for (t = 16; t < 80; t++)
    {
        tmp = w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16];
        w[t] = rol(tmp, 1);

        #ifdef _PRINT_HDMI_TX_HDCP_
        ////TXHDCP_DEBUG_PRINTF(("w[%2d]=%08lX\n",t,w[t]));
        #endif
    }

    h[0] = 0x67452301;
    h[1] = 0xefcdab89;
    h[2] = 0x98badcfe;
    h[3] = 0x10325476;
    h[4] = 0xc3d2e1f0;

    for (t = 0; t < 20; t++)
    {
        tmp = rol(h[0],5) + ((h[1] & h[2]) | (h[3] & ~h[1])) + h[4] + w[t] + 0x5a827999;
        #ifdef _PRINT_HDMI_TX_HDCP_
        // //TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
        #endif
        h[4] = h[3];
        h[3] = h[2];
        h[2] = rol(h[1],30);
        h[1] = h[0];
        h[0] = tmp;
    }

    for (t = 20; t < 40; t++)
    {
        tmp = rol(h[0],5) + (h[1] ^ h[2] ^ h[3]) + h[4] + w[t] + 0x6ed9eba1;
        #ifdef _PRINT_HDMI_TX_HDCP_
        // //TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
        #endif
        h[4] = h[3];
        h[3] = h[2];
        h[2] = rol(h[1],30);
        h[1] = h[0];
        h[0] = tmp;
    }

    for (t = 40; t < 60; t++)
    {
        tmp = rol(h[0],5) + ((h[1] & h[2]) | (h[1] & h[3]) | (h[2] & h[3])) + h[4] + w[t] + 0x8f1bbcdc;
        #ifdef _PRINT_HDMI_TX_HDCP_
        ////TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
        #endif
        h[4] = h[3];
        h[3] = h[2];
        h[2] = rol(h[1],30);
        h[1] = h[0];
        h[0] = tmp;
    }

    for (t = 60; t < 80; t++)
    {
        tmp = rol(h[0],5) + (h[1] ^ h[2] ^ h[3]) + h[4] + w[t] + 0xca62c1d6;
        #ifdef _PRINT_HDMI_TX_HDCP_
         ////TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
        #endif
        h[4] = h[3];
        h[3] = h[2];
        h[2] = rol(h[1],30);
        h[1] = h[0];
        h[0] = tmp;
    }

    #ifdef _PRINT_HDMI_TX_HDCP_
    ////TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
    #endif
    h[0] += 0x67452301;
    h[1] += 0xefcdab89;
    h[2] += 0x98badcfe;
    h[3] += 0x10325476;
    h[4] += 0xc3d2e1f0;

    #ifdef _PRINT_HDMI_TX_HDCP_
    ////TXHDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",h[0],h[1],h[2],h[3],h[4]));
    #endif
}




