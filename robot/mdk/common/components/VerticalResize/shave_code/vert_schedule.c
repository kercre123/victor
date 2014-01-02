///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Scheduler for getting vertical lines for Lanczos resize
///

#include <svuCommonShave.h>
#include <stdlib.h>

#define FILTER_LENGTH 6
#define MAX_WIDTH (1920*2)
#define SHAVE_HALT __asm("BRU.swih 0x001F \n\t nop 5");

//Header for the function to use in the inline assembly
void OutLineCompute(unsigned char* input_lines[FILTER_LENGTH],
        unsigned char* out_line,unsigned int width,unsigned int filter_pos);

//Local data
float resize_ratio,current_point;

unsigned char lines_buffers[FILTER_LENGTH*2*MAX_WIDTH];
//Double buffering on the output
unsigned char out_lines[MAX_WIDTH*2];

unsigned int current_out=0;

unsigned char* lines[FILTER_LENGTH]={
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0,
        (unsigned char*)0x0
};

unsigned int lines_in[FILTER_LENGTH]={0,0,0,0,0,0};
//These refer to the first and last line brought in
unsigned int first_line,last_line;
int nextfreebuf=0,firstusedbuf=0;
unsigned int line_counter;
unsigned int lines_stored=0;
volatile unsigned int gracefull_halt=0;
unsigned int address;
//
// Input: i18 - output address, i17 - input address, i16 - in_out width, i15 - in height, i14 - out height
void __attribute__((section(".text.__ENTRY__sect")))
    VertResize422i(unsigned char* out_address, unsigned char* in_address,
        unsigned int in_out_width, unsigned int in_height, unsigned int out_height)
{
    int i,k,l;
    float old_pos;
    //on start fill in all the indexes with the lines taken in
    for (i=0;i<FILTER_LENGTH;i++){
        lines_in[i]=i;
    }
    //Bring in line 0 on first 3 lines because we are using mirroring
    scDmaSetupFull(DMA_TASK_0,DMA_ENABLE,in_address,&lines_buffers[0*in_out_width*2],in_out_width*2,0,0);
    scDmaSetupFull(DMA_TASK_1,DMA_ENABLE,in_address,&lines_buffers[1*in_out_width*2],in_out_width*2,0,0);
    scDmaSetupFull(DMA_TASK_2,DMA_ENABLE,in_address,&lines_buffers[2*in_out_width*2],in_out_width*2,0,0);
    scDmaSetupFull(DMA_TASK_3,DMA_ENABLE,in_address+1*in_out_width*2,&lines_buffers[3*in_out_width*2],in_out_width*2,0,0);
    scDmaStart(START_DMA0123);
    scDmaWaitFinished();
    //And bring in next 3 lines to start computing lanczos resize
    scDmaSetupFull(DMA_TASK_0,DMA_ENABLE,in_address+2*in_out_width*2,&lines_buffers[4*in_out_width*2],in_out_width*2,0,0);
    scDmaSetupFull(DMA_TASK_1,DMA_ENABLE,in_address+3*in_out_width*2,&lines_buffers[5*in_out_width*2],in_out_width*2,0,0);
    scDmaStart(START_DMA01);

    //Fill lines addresses
    for (i=0;i<FILTER_LENGTH;i++){
        lines[i]=&lines_buffers[lines_in[i]*in_out_width*2];
    }

    first_line=0;
    last_line=5;
    firstusedbuf=0;
    nextfreebuf=6;

    //Set up the initial starting point
    resize_ratio=(float)(in_height-1)/(float)(out_height-1);
    current_point=0.0f;
    for (line_counter=0;line_counter<out_height;line_counter++){
        int no_lines_to_bring,input_line,oldfreebuf;
        int integer_pos;
        scDmaWaitFinished();
        //No DMA pending so this is a suitable place for gracefully halting the shave
        if (gracefull_halt==1){
            gracefull_halt=0;
            SHAVE_HALT;
        }

        old_pos=current_point;
        oldfreebuf=nextfreebuf;

        //See which are the next lines that need bringing in
        current_point+=resize_ratio;
        //Force truncating
        integer_pos=(unsigned int)(current_point);
        no_lines_to_bring=(integer_pos+5)-last_line;

        //Form the lines to send
        for (k=0;k<FILTER_LENGTH;k++){
            lines[k]=&lines_buffers[((firstusedbuf+k)%(FILTER_LENGTH*2))*in_out_width*2];
        }

        //We could  bring in as much as 4 lines while processing the previous ones
        if (no_lines_to_bring>0){
            //Hmm.. why (integer_pos+3-no_lines_to_bring+1) ?!?
            //Took a while to get this. Basically, what we want is to always get the
            //number of needed lines BUT, if it's one needed line, we need to get
            //that here. If it's 2 lines we need, we need to get the first before last
            //here and the last in the next if. If it's 3 lines we need we need to get the second before last here,
            //the first before last in the next if, and the last one two if's from here.
            //This expression manages to get that formula in:
            //integer_pos+3 -> that's the last position
            //-no_lines_to_bring+1 -> selects the required line by going to the first
            //solicited line. Next if will have no_lines_to_bring+2 which gets the
            //second solicited line and so on.
            input_line=(integer_pos+3-no_lines_to_bring+1)<0 ? 0 : (integer_pos+3-no_lines_to_bring+1)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+1);
            scDmaSetupFull(DMA_TASK_0,DMA_ENABLE,
                    in_address+input_line*in_out_width*2,
                    &lines_buffers[nextfreebuf*in_out_width*2],in_out_width*2,0,0);
//            __asm(
//                    "vau.xor v0, v0, v0\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                    :"v0","i0","i1"
//                    );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }
        if (no_lines_to_bring>1){
            input_line=(integer_pos+3-no_lines_to_bring+2)<0 ? 0 : (integer_pos+3-no_lines_to_bring+2)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+2);
            scDmaSetupFull(DMA_TASK_1,DMA_ENABLE,
                    in_address+input_line*in_out_width*2,
                    &lines_buffers[nextfreebuf*in_out_width*2],in_out_width*2,0,0);
//            __asm(
//                    "vau.xor v1, v1, v1\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                     :"v1","i0","i1"
//            );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }
        if (no_lines_to_bring>2){
            input_line=(integer_pos+3-no_lines_to_bring+3)<0 ? 0 : (integer_pos+3-no_lines_to_bring+3)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+3);
            scDmaSetupFull(DMA_TASK_2,DMA_ENABLE,
                    in_address+input_line*in_out_width*2,
                    &lines_buffers[nextfreebuf*in_out_width*2],in_out_width*2,0,0);
//            __asm(
//                    "vau.xor v2, v2, v2\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                     :"v2","i0","i1"
//            );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }
        if (no_lines_to_bring>3){
            input_line=(integer_pos+3-no_lines_to_bring+4)<0 ? 0 : (integer_pos+1)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+4);
            scDmaSetupFull(DMA_TASK_3,DMA_ENABLE,
                    in_address+input_line*in_out_width*2,
                    &lines_buffers[nextfreebuf*in_out_width*2],in_out_width*2,0,0);
//            __asm(
//                    "vau.xor v3, v3, v3\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                     :"v3","i0","i1"
//            );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }

        //And start the dma task. Again, on a per case basis
        if (no_lines_to_bring>3){
            scDmaStart(START_DMA0123);
        }else{
            if (no_lines_to_bring==3){
                scDmaStart(START_DMA012);
            }
            if (no_lines_to_bring==2){
                scDmaStart(START_DMA01);
            }
            if (no_lines_to_bring==1){
                scDmaStart(START_DMA0);
            }
        }

        //scDmaWaitFinished();
        //Launch Lanczos on the currently brought in lines sending the index
        //to the filter to choose
        //Multiplying width with 2 since it's YUYV422 not some planar input
        OutLineCompute(lines,&out_lines[0],in_out_width*2,
                (unsigned int)((old_pos-(float)((unsigned int)old_pos))*127.0f));

        //wait for any remaining transfers
        scDmaWaitFinished();

        //No DMA pending so this is a suitable place for gracefully halting the shave
        if (gracefull_halt==1){
            gracefull_halt=0;
            SHAVE_HALT;
        }

        //DEBUG ONLY: Inject slowness into the algorithm
/*        for (l=0;l<200;l++){
            __asm(
            "nop 100\n"
            );
        }*/

        //Copy out to DDR the requested line by also doubling it
        scDmaSetupFull(DMA_TASK_0,DMA_ENABLE,
                &out_lines[0],
                out_address + line_counter*in_out_width*4, in_out_width * 2, 0, 0);
        address=out_address + line_counter*in_out_width*4;
        //Check if there are any other lines left to bring in too
        if (no_lines_to_bring > 4){
            input_line=(integer_pos+3-no_lines_to_bring+5)<0 ? 0 : (integer_pos+3-no_lines_to_bring+5)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+5);
            scDmaSetupFull(DMA_TASK_1,DMA_ENABLE,
                    in_address + input_line * in_out_width * 2,
                    &lines_buffers[nextfreebuf*in_out_width * 2],in_out_width * 2,0,0);
//            __asm(
//                    "vau.xor v4, v4, v4\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                     :"v4","i0","i1"
//            );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }
        if (no_lines_to_bring>5){
            input_line=(integer_pos+3-no_lines_to_bring+6)<0 ? 0 : (integer_pos+3-no_lines_to_bring+6)>(in_height-1) ? (in_height-1) : (integer_pos+3-no_lines_to_bring+6);
            scDmaSetupFull(DMA_TASK_2,DMA_ENABLE,
                    in_address+input_line*in_out_width * 2,
                    &lines_buffers[nextfreebuf*in_out_width * 2],in_out_width * 2,0,0);
//            __asm(
//                    "vau.xor v5, v5, v5\n"
//                    "cmu.cpii i0, %0\n"
//                    "cmu.cpii i1, %1\n"
//                    :
//                    :"r"(input_line),"r"(&lines_buffers[nextfreebuf*in_out_width*2])
//                     :"v5","i0","i1"
//            );
            nextfreebuf=(nextfreebuf+1)%(FILTER_LENGTH*2);
            firstusedbuf=(firstusedbuf+1)%(FILTER_LENGTH*2);
        }
        //And start the dma transfers again
        if (no_lines_to_bring>5){
            scDmaStart(START_DMA012);
            lines_stored++;
        }else{
            if (no_lines_to_bring>4){
                scDmaStart(START_DMA01);
                lines_stored++;
            }else{
                scDmaStart(START_DMA0);
                lines_stored++;
            }
        }

        //Upgrade last and first line
        first_line=integer_pos;
        last_line=integer_pos+5;

    }

    //Wait for last transfers
    scDmaWaitFinished();

    //Exit point. We're actually stopping the shave, not returning anywhere
    SHAVE_HALT;
    return;
}
