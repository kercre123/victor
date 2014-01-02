#include <sipp.h>

//WARNING: SIPP filters consider BASE starting from SLICE0, then START/FIRST offsets
//         get added to get the actual address; Therefore resolve the base addresses
//         starting from SLICE0 always.





#if defined(MYRIAD2)

#include <DrvSvuDefines.h>

// Program space with no data (bss)
#define MSHT_NOBITS         8


typedef struct
{
    unsigned char     id[16];           // ID of the MOF file
    unsigned short    type;             // Type of the object file
    unsigned short    target;           // Target processor of the .mof file
    unsigned int      version;          // ISA version file version 
    unsigned int      entry;            // Entry point virtual address
    unsigned int      phOffset;         // Program header table file offset
    unsigned int      shOffset;         // Section header table file offset
    unsigned int      flags;            // Processor-specific flags
    unsigned short    phSize;           // MOF header size in bytes
    unsigned short    phEntrySize;      // Program header table entry size
    unsigned short    phCount;          // Program header table entry count
    unsigned short    shEntrySize;      // Section header table entry size
    unsigned short    shCount;          // Section header table entry count
    unsigned short    shStringIndex;    // Section header string table index
} tMofFileHeader;

//MOF Section Header
typedef struct
{
    unsigned int    name;               // Section name (string tbl index)
    unsigned int    type;               // Section type
    unsigned int    flags;              // Section flags
    unsigned int    address;            // Section virtual addr at execution
    unsigned int    offset;             // Section file offset
    unsigned int    size;               // Section size in bytes
    unsigned int    link;               // Link to another section
    unsigned int    info;               // Additional section information
    unsigned int    addralign;          // Section alignment
    unsigned int    entsize;            // Entry size if section holds table
    unsigned int    core;               // core of section used by linker
} tMofSectionHeader;

//clean routines (mdk routines are broken; I already spent to much time with mdk issues)

//########################################################################################
//svuNo: 0..11
//winNo: 0..3 (A,B,C,D)
void sippSetShaveWindow(UInt32 svuNo, UInt32 winNo, UInt32 value)
{
  //Calculate address of the Slice config addr
    UInt32 sliceBase = SHAVE_0_BASE_ADR + SVU_SLICE_OFFSET * svuNo;
    
  //Set the proper WINDOW register
    SET_REG_WORD(sliceBase + SLC_TOP_OFFSET_WIN_A + 4 * winNo, value);
}

//########################################################################################
UInt32 sippGetShaveWindow(u32 svuNo, u32 winNo)
{
  //Calculate address of the Slice config addr
    UInt32 sliceBase = SHAVE_0_BASE_ADR + SVU_SLICE_OFFSET * svuNo;
    
  //Set the proper WINDOW register
    return GET_REG_WORD_VAL(sliceBase + SLC_TOP_OFFSET_WIN_A + 4 * winNo);
}

//########################################################################################
UInt32 sippSolveShaveAddr(u32 inAddr, u32 sliceNo)
{
    UInt32 resolved;
    switch (inAddr >> 24)
    {
        case 0x1C: resolved = (inAddr & 0x00FFFFFF) + sippGetShaveWindow(sliceNo, 0); break;
        case 0x1D: resolved = (inAddr & 0x00FFFFFF) + sippGetShaveWindow(sliceNo, 1); break;
        case 0x1E: resolved = (inAddr & 0x00FFFFFF) + sippGetShaveWindow(sliceNo, 2); break;
        case 0x1F: resolved = (inAddr & 0x00FFFFFF) + sippGetShaveWindow(sliceNo, 3); break;
        default  : resolved =  inAddr;  break; //absolute address, no translation is to be done
    }
    return resolved;
} 

//########################################################################################
static tMofFileHeader    swc_mbinH;
static tMofSectionHeader swc_secH;

void sippLoadMbin(u8 *mbinImg, u32 targetS)
{
    int sec;
    u32 srcA32;
    u32 dstA32;

   //get the program header
    swcU32memcpy((u32*)&swc_mbinH, (u32*)mbinImg, sizeof(tMofFileHeader));

   //parse sections:
    for (sec = 0; sec < swc_mbinH.shCount; sec++)
    {
      //get current section
        swcU32memcpy((u32*)&swc_secH, ((u32*)mbinImg) + ((swc_mbinH.shOffset + sec * swc_mbinH.shEntrySize) >> 2), sizeof(tMofSectionHeader));
 
      //if this is a BSS type of section, just continue
        if (swc_secH.type == MSHT_NOBITS)
            continue;
            
        srcA32 = (u32)(mbinImg + swc_secH.offset);
        dstA32 = sippSolveShaveAddr(swc_secH.address, targetS);

        swcU32memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
    }
}

//########################################################################################
void sippLoadMbinMulti(u8 *mbinImg, u32 firstSlc, u32 lastSlc)
{
    int sec, s;
    u32 srcA32;
    u32 dstA32;

   //get the program header
    swcU32memcpy((u32*)&swc_mbinH, (u32*)mbinImg, sizeof(tMofFileHeader));

   //parse sections:
    for (sec = 0; sec < swc_mbinH.shCount; sec++)
    {
      //get current section
        swcU32memcpy((u32*)&swc_secH, ((u32*)mbinImg) + ((swc_mbinH.shOffset + sec * swc_mbinH.shEntrySize) >> 2), sizeof(tMofSectionHeader));
 
      //if this is a BSS type of section, just continue
        if (swc_secH.type == MSHT_NOBITS)
            continue;
        
      //SRC from MBIN image is the same
        srcA32 = (u32)(mbinImg + swc_secH.offset);
        
      //Copy to all required destinations  
        for(s=firstSlc; s<=lastSlc; s++)
        {
          dstA32 = sippSolveShaveAddr(swc_secH.address, s);
          swcU32memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
        }
    }
}


// //########################################################################################
// //Optimized MBIN load for multiple shaves: if a section is to be loaded at same addr
// //for all shaves, then just load it once ! (e.g. code mapped in L2/DDR)
// void sippLoadMbinOpt(u8 *mbinImg, u32 firstSlc, u32 lastSlc)
// {
    // int sec;
    // u32 srcA32;
    // u32 dstA32;
    // u32 addr, common, slice;
    
   // //get the program header
    // swcU32memcpy((u32*)&swc_mbinH,  (u32*)mbinImg, sizeof(tMofFileHeader));

   // //parse sections:
    // for (sec = 0; sec < swc_mbinH.shCount; sec++)
    // {
      // //get current section
        // swcU32memcpy((u32*)&swc_secH, ((u32*)mbinImg) + ((swc_mbinH.shOffset + sec * swc_mbinH.shEntrySize) >> 2), sizeof(tMofSectionHeader));
 
      // //if this is a BSS type of section, just continue
        // if (swc_secH.type == MSHT_NOBITS)
            // continue;
            
        // srcA32 = (u32)(mbinImg + swc_secH.offset);
        
      // //the same MBIN is to be loaded on all shaves; check if destination addr is the same 
      // //(in which case, we can only load it once: e.g. CODE mapped in L2)
        // common = 1;//assume it is a common destination addr-space
        // dstA32 = sippSolveShaveAddr(swc_secH.address, firstSlc); //get first slice DST addr
        
        // for(slice=firstSlc+1; slice<=lastSlc; slice++)
        // {
          // addr = sippSolveShaveAddr(swc_secH.address, slice);
          // if(addr != dstA32)
          // {
             // common = 0;
             // break;
          // }          
        // }

      // //If section is common load it once:
        // if(common)
        // {
          // swcU32memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
        // }
        // else{ //else, load it per slice basis
          // for(slice=firstSlc; slice<=lastSlc; slice++)
          // {
           // dstA32 = sippSolveShaveAddr(swc_secH.address, slice);
           // swcU32memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
          // }
        // }
    // }
// }
#endif





#if defined(SIPP_PC) || (defined(__sparc) && defined(MYRIAD2))
//########################################################################################
// Compact and initialize buffer definitions
// We need to do this as on context switch we'll just copy the params
// parNo can be 0 or 1. Some HW filters have a single parent, others have 2
//########################################################################################
void sippIbufSetup(SippFilter *fptr, UInt32 parNo)
{
    SippHwBuf  *iBuf;
    SippFilter *par = fptr->parents[parNo];

  //Allocate some mem for the input buffer description
    fptr->iBuf[parNo] = (SippHwBuf*)sippMemAlloc(mempool_sipp, sizeof(SippHwBuf));

  //Config the Ibuf
    iBuf = fptr->iBuf[parNo];

    iBuf->base = (UInt32)WRESOLVE(par->outputBuffer, 0) 
                                  + par->hPadding * par->bpp;//add left padding !

    iBuf->cfg  =  par->nLines                            //number of lines
                  |(1                << SIPP_SC_OFFSET)  //enable synchronous ctrl
                  |((par->nPlanes-1) << SIPP_NP_OFFSET)  //number of planes
                  |( par->bpp        << SIPP_FO_OFFSET) ;//format

    iBuf->ls   =  par->lineStride  * par->bpp;
    iBuf->ps   =  par->planeStride * par->bpp;
    iBuf->ctx  =  0;

  //Enable interrupt after each output line (0) and program line chunk
    iBuf->irqRate =   (par->sliceWidth*par->bpp << 16)
                    | (par->firstOutSlc         <<  8);
}

//########################################################################################
// Typical OUTPUT buffer setup for a HW module
//########################################################################################
void sippObufSetup(SippFilter *fptr)
{
   SippHwBuf *oBuf; 

 //Allocate some mem for the input buffer description
   fptr->oBuf    = (SippHwBuf*)sippMemAlloc(mempool_sipp, sizeof(SippHwBuf));

 //Config the output buffer
   oBuf = fptr->oBuf;
   oBuf->base = (UInt32)WRESOLVE(fptr->outputBuffer, 0)
                                 +fptr->hPadding * fptr->bpp;

   oBuf->cfg  =    fptr->nLines                         //number of lines
                |((fptr->nPlanes-1) << SIPP_NP_OFFSET)  //number of planes
                |( fptr->bpp        << SIPP_FO_OFFSET); //format

   oBuf->ls  =  fptr->lineStride  * fptr->bpp;
   oBuf->ps  =  fptr->planeStride * fptr->bpp;
   oBuf->ctx =  0;

 //Enable interrupt after each output line (0) and program line chunk, first output slice
   oBuf->irqRate =  (fptr->sliceWidth*fptr->bpp << 16)
                  | (fptr->firstOutSlc          <<  8);
}

//########################################################################################
//Typical Context switch for filters with 1 parent and full parameter set double buffered
//Most filters fit this criteria: Raw, Debayer, Debayer Post Proc, Sharpen, Median, Conv
//Edge_op, Harris
void ctxSwitchOnePar(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
  //Save old filter context
    oldF->iBuf[0]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][0]);
    oldF->oBuf->ctx       = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

  //Load new filter context
    SET_REG_WORD(pl->ictxAddr[unitID][0], (1<<31) | newF->iBuf[0]->ctx);
    SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}

//########################################################################################
//Typical setup for Two parent buffers: Luma Denoise, Lsc
void ctxSwitchTwoPar(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
  //Save old filter context
    oldF->iBuf[0]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][0]);
    oldF->iBuf[1]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][1]);
    oldF->oBuf->ctx       = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

  //Load new filter context
    SET_REG_WORD(pl->ictxAddr[unitID][0], (1<<31) | newF->iBuf[0]->ctx);
    SET_REG_WORD(pl->ictxAddr[unitID][1], (1<<31) | newF->iBuf[1]->ctx);
    SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}


//########################################################################################
//Second parent optional : chroma denoise
//WARNING: this is a UNIT CTX switch, might have multiple instances of Chroma denoise
//         some that have 1 parent, some that have 2 parents.
//         Old filter might have 2 Inputs and new one might have 1 Input.
//         So at restore/load, we check the prope filter and see how many parents it has
// Note: this routine could replace ctxSwitchTwoPar and ctxSwitchOnePar
void ctxSwitchChromaDns(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
  //Save old filter context
    oldF->iBuf[0]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][0]);

    if(oldF->iBuf[1])
       oldF->iBuf[1]->ctx = GET_REG_WORD_VAL(pl->ictxAddr[unitID][1]);

    oldF->oBuf->ctx       = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

  //Load new filter context
    SET_REG_WORD(pl->ictxAddr[unitID][0], (1<<31) | newF->iBuf[0]->ctx);

    if(newF->iBuf[1])
       SET_REG_WORD(pl->ictxAddr[unitID][1], (1<<31) | newF->iBuf[1]->ctx);

    SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}

//########################################################################################
// The Line init of poly-phase scaler computex ICTX before each run, so no point saving it
// and definitely NOT reload old context, as the line-init will compute the proper value
// So what's left to do: save current OCTX, load new OCTX
void ctxSwitchPoly(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
  //Save old filter context
    oldF->oBuf->ctx       = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

  //Load new filter context
    SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}
#endif


//#######################################################################################
//Need to use centralized SET_REG_WORD/GET_REG_WORD, as they will refer to different
//units; which get decoded based on ADDR range. (e.g. sipp, bicubic)

//Note: on SPARC, SET_REG_WORD is already defined ...
//      on Shave, it makes no sense, so only let definition for Windows-Leon 

#if defined(SIPP_PC)
 #include "CmxDma.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 void initHwModels()
 {
     extern UInt8 cmx[];

     //Tell CMXDMA model where base of CMX is:
     cmxDma.SetCMXBaseAddress((UInt32)cmx);

     //Tell SIPP models where base of CMX is:
          sippHwAcc.cc.SetCmxBasePointer((void*)cmx);
         sippHwAcc.cdn.SetCmxBasePointer((void*)cmx);
        sippHwAcc.conv.SetCmxBasePointer((void*)cmx);
        sippHwAcc.dbyr.SetCmxBasePointer((void*)cmx);
     sippHwAcc.dbyrppm.SetCmxBasePointer((void*)cmx);
        sippHwAcc.edge.SetCmxBasePointer((void*)cmx);
        sippHwAcc.harr.SetCmxBasePointer((void*)cmx);
         sippHwAcc.lsc.SetCmxBasePointer((void*)cmx);
        sippHwAcc.luma.SetCmxBasePointer((void*)cmx);
         sippHwAcc.lut.SetCmxBasePointer((void*)cmx);
         sippHwAcc.med.SetCmxBasePointer((void*)cmx);
         sippHwAcc.raw.SetCmxBasePointer((void*)cmx);
     sippHwAcc.upfirdn.SetCmxBasePointer((void*)cmx);
         sippHwAcc.usm.SetCmxBasePointer((void*)cmx);
 }

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 void SET_REG_WORD(UInt32 addr, UInt32 value)
 {
     //SIPP addr space?
     if((addr >= SIPP_BASE0_ADR                        ) && 
        (addr <= SIPP_UPFIRDN_HCOEFF_P15_7_6_SHADOW_ADR) )
     {
       sippHwAcc.ApbWrite(addr, value);
     }
 }

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 UInt32 GET_REG_WORD_VAL(UInt32 addr)
 {
     //SIPP addr space?
     if((addr >= SIPP_BASE0_ADR                        ) && 
        (addr <= SIPP_UPFIRDN_HCOEFF_P15_7_6_SHADOW_ADR) )
     return (UInt32)sippHwAcc.ApbRead(addr);

     return 0xDEADDEAD;
 }

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 void SET_REG_DWORD(UInt32 addr, UInt64 value)
 {
    //TBD: check addr range and invoke proper model WR
     cmxDma.AhbWrite(addr, value);
 }
#endif
