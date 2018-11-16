/* Copyright (c) 2013, 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */


/* Sample program that maps an Android ION buffer into CL and runs a cl program on it 
 * Uses Qualcomm CL extensions mentioned here https://www.khronos.org/registry/cl/extensions/qcom/cl_qcom_ion_host_ptr.txt 
 * */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <clstlib.h>
#include "ion_mem.h"

#define PAGE_ALIGNMENT_MASK 0xfff

typedef struct _clst_ion_mem_map_test_state clst_ion_mem_map_test_state;

/* This is a single threaded application */
double elapsed_time_mapwr = 0.0;
double elapsed_time_maprd = 0.0;

struct _clst_ion_mem_map_test_state
{
    clst_basic_test_state basic;
    cl_mem                input_image;
    cl_mem                output_image;
    cl_kernel             kernel;
    struct mmap_info_ion  in_mapion;
    struct mmap_info_ion  out_mapion;
    cl_int                imgw;
    cl_int                imgh;
    size_t                image_row_pitch;
    size_t                element_size;
};


/* A simple OpenCL Kernel operating on 2 pointers*/ 
static const char *g_ion_mem_map_naive_program_source = "\n" \
"__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
"__kernel void copy2d( \n" \
"   __read_only image2d_t input_mem, \n" \
"   __write_only image2d_t output_mem, \n" \
"   const unsigned int width, \n" \
"   const unsigned int height) \n" \
"{ \n" \
"    int x = get_global_id(0); \n" \
"    int y = get_global_id(1); \n" \
"    if ((x >= width) || (y >= height)) \n" \
"        return; \n" \
"    int2 pos = (int2) (x,y); \n" \
"    float4 scaledPixel = read_imagef(input_mem,sampler,pos); \n" \
"    write_imagef(output_mem, pos, scaledPixel); \n" \
"} \n" \
"\n";


void ion_host_ptr_images2D_postamble(const clst_test_env env, void* user_data)
{
    clst_ion_mem_map_test_state *state = (clst_ion_mem_map_test_state*)user_data;

    if(!state)
    {
        LOG_ERROR_LOCATION(env);
        return;
    }

    if(!env->output_csv)
    {
        printf("Time taken to fill input: %f\n", elapsed_time_mapwr/10);
        printf("Time taken to read output: %f\n", elapsed_time_maprd/10);
        printf("Time taken for NDRange: ");
    }

    if(state->input_image)
    {
        clReleaseMemObject(state->input_image);
    }

    if(state->output_image)
    {
        clReleaseMemObject(state->output_image);
    }

    if(state->kernel)
    {
        clReleaseKernel(state->kernel);
    }

    mem_free_ion(state->in_mapion);
    mem_free_ion(state->out_mapion);

    clst_basic_test_release(env, &state->basic);
}

static void
ion_host_ptr_preamble(const clst_test_env env,
                      const char*         kernel_name,
                      int                 cpu_cache_policy,
                      void**              user_data)
{
    clst_ion_mem_map_test_state    *state      = NULL;
    cl_int                   errcode     = CL_SUCCESS;
    *user_data = NULL;
    int ion_padding_in_bytes = 0;
    int device_page_size = 0;
    cl_mem_ion_host_ptr in_ionmem = {0};
    cl_mem_ion_host_ptr out_ionmem = {0};
    cl_image_format fmt;
    size_t origin[3]={0,0,0}, region[3]={0, 0, 1};
    size_t row_pitch         = 0;
    int    i                 = 0;
    int    img_buf_size      = 0;
    char extension_list[4096];

    clGetDeviceInfo(env->device[0], CL_DEVICE_EXTENSIONS, sizeof(extension_list), extension_list, NULL);
    if(strstr(extension_list,"cl_qcom_ion_host_ptr")==NULL)
    {
        printf("Error : 'cl_qcom_ion_host_ptr' extension not supported. \n");
        LOG_ERROR_LOCATION(env);
        return;
    }
    if(strstr(extension_list,"cl_qcom_ext_host_ptr")==NULL)
    {
        printf("Error : 'cl_qcom_ext_host_ptr' extension not supported. \n");
        LOG_ERROR_LOCATION(env);
        return;
    }

    fmt.image_channel_order = CL_RGBA;
    fmt.image_channel_data_type = CL_UNORM_INT8;

    state = clst_calloc(sizeof(clst_ion_mem_map_test_state));

    if(!state)
    {
        LOG_ERROR_LOCATION(env);
        return;
    }

    /* Hard-code the width, height for now */
    state->imgw = 256;
    state->imgh = 256;

    region[0] = state->imgw;
    region[1] = state->imgh;

    errcode = clst_basic_test_setup(env, 1, &g_ion_mem_map_naive_program_source, "-O3", &state->basic, 0);

    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    clGetDeviceInfo(env->device[0], CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM, sizeof(ion_padding_in_bytes), &ion_padding_in_bytes, NULL);
    clGetDeviceInfo(env->device[0], CL_DEVICE_PAGE_SIZE_QCOM, sizeof(device_page_size), &device_page_size, NULL);
    clGetDeviceImageInfoQCOM(env->device[0], state->imgw, state->imgh, &fmt, CL_IMAGE_ROW_PITCH, sizeof(state->image_row_pitch), &state->image_row_pitch, NULL);

    if(!env->output_csv)
    {
        printf("ION padding in bytes:\"%d\":\n", ion_padding_in_bytes);
        printf("Device Page Size:\"%d\":\n", device_page_size);
        printf("Image Row Pitch:\"%d\":\n", state->image_row_pitch);
    }

    img_buf_size = state->image_row_pitch * state->imgh;

/* ---------------- Input img ---------------- */
    state->in_mapion.bufsize = img_buf_size;
    state->in_mapion.bufsize += ion_padding_in_bytes;
    mem_alloc_ion(env, &state->in_mapion, device_page_size, cpu_cache_policy);

    if (state->in_mapion.pVirtualAddr == NULL)
    {
        printf("Error : Main ion allocation failed \n");
        exit(1);
    }

    if(((intptr_t)state->in_mapion.pVirtualAddr) & PAGE_ALIGNMENT_MASK)
    {
        printf("Error: Memory not aligned to %d bytes\n", device_page_size);
        exit(1);
    }
    in_ionmem.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
    in_ionmem.ext_host_ptr.host_cache_policy = cpu_cache_policy;
    in_ionmem.ion_filedesc = state->in_mapion.ion_info_fd.fd;
    in_ionmem.ion_hostptr = state->in_mapion.pVirtualAddr;

    state->input_image = clCreateImage2D(state->basic.clctx, CL_MEM_USE_HOST_PTR|CL_MEM_EXT_HOST_PTR_QCOM, &fmt, state->imgw, state->imgh, state->image_row_pitch, &in_ionmem, &errcode);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    errcode = clGetImageInfo(state->input_image, CL_IMAGE_ELEMENT_SIZE, sizeof(state->element_size), &state->element_size, NULL);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    if(!env->output_csv)
    {
        printf("Input Image width: %d height: %d element_size: %d\n", state->imgw, state->imgh, state->element_size);
    }

/* ---------------- Out img ---------------- */
    state->out_mapion.bufsize = img_buf_size;
    state->out_mapion.bufsize += ion_padding_in_bytes;
    mem_alloc_ion(env, &state->out_mapion, device_page_size, cpu_cache_policy);

    if (state->out_mapion.pVirtualAddr == NULL)
    {
        printf("Error : Main ion allocation failed \n");
        exit(1);
    }

    if(((intptr_t)state->out_mapion.pVirtualAddr) & PAGE_ALIGNMENT_MASK)
    {
        printf("Error: Memory not aligned to %d bytes\n", device_page_size);
        exit(1);
    }

    out_ionmem.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
    out_ionmem.ext_host_ptr.host_cache_policy = cpu_cache_policy;
    out_ionmem.ion_filedesc = state->out_mapion.ion_info_fd.fd;
    out_ionmem.ion_hostptr = state->out_mapion.pVirtualAddr;

    state->output_image = clCreateImage2D(state->basic.clctx, CL_MEM_USE_HOST_PTR|CL_MEM_EXT_HOST_PTR_QCOM, &fmt, state->imgw, state->imgh, state->image_row_pitch, &out_ionmem, &errcode);

    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    if(!env->output_csv)
    {
        printf("output: clCreateImage2D with ION_HOST_PTR_QCOM passed!\n");
        printf("Output Image width: %d height: %d element_size: %d\n", state->imgw, state->imgh, state->element_size);
    }

    state->kernel = clCreateKernel(state->basic.program[0], kernel_name, &errcode);

    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    // Input
    errcode = clSetKernelArg(state->kernel, 0, sizeof(cl_mem), &state->input_image);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    // Output
    errcode = clSetKernelArg(state->kernel, 1, sizeof(cl_mem), &state->output_image);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    errcode = clSetKernelArg(state->kernel, 2, sizeof(cl_int), &state->imgw);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }
    errcode = clSetKernelArg(state->kernel, 3, sizeof(cl_int), &state->imgh);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    /* Reset the timer values */
    elapsed_time_mapwr = 0.0;
    elapsed_time_maprd = 0.0;

    *user_data = state;

    return;

cleanup:
    ion_host_ptr_images2D_postamble(env, state);
    clst_free(state);
    *user_data = NULL;
}


cl_int ion_mem_map(const clst_test_env env, void* user_data, double *elapsed_time_ndr)
{
    cl_int                              errcode = 0;
    clst_ion_mem_map_test_state        *state   = (clst_ion_mem_map_test_state*)user_data;
    cl_uint                            *reference = NULL;
    cl_uint                             i;
    size_t                              global_size[2] = {0, 0};
    int                                 err;
    size_t                              origin[3]         = {0,0,0}, region[3] = {0, 0, 1};
    size_t                              row_pitch         = 0;
    long64                              start             = 0;
    cl_uchar*                           pinput = NULL;
    cl_uchar*                           poutput = NULL;
    cl_uchar*                           inp = NULL;
    cl_uchar*                           outp = NULL;
    int                                 img_buf_size      = 0;

    if(!state)
    {
        LOG_ERROR_LOCATION(env);
        return CL_OUT_OF_HOST_MEMORY;
    }

    img_buf_size = state->image_row_pitch * state->imgh;

    global_size[0] = state->imgw;
    global_size[1] = state->imgh;

    region[0] = state->imgw;
    region[1] = state->imgh;

    /* Initialize input image */
    start = clst_clock_start();
    pinput = clEnqueueMapImage(state->basic.queue[0], state->input_image, CL_TRUE, CL_MAP_WRITE, origin, region, &row_pitch, NULL,
                                        0, NULL, NULL, &errcode);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    /* pinput is the Buffer that is now accessible by CPU and can be updated . IN this example the buffer is updated with 0x0 and 0xff */
    /* OEMs can read a YUV buffer into this mapped buffer */

    inp = pinput;
    memset(inp, 0x0, img_buf_size);

    for(i = 0; i < img_buf_size; i+=row_pitch)
    {
        memset(inp+i, 0xff, state->imgw * state->element_size);
    }
    elapsed_time_mapwr += (clst_clock_diff(start) * 1000000);

    /* pinput is now unmapped. It can be accessed by GPU */ 

    errcode =  clEnqueueUnmapMemObject(state->basic.queue[0], state->input_image, pinput, 0, NULL, NULL);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }
     

    /* Initialize output image */
    poutput = clEnqueueMapImage(state->basic.queue[0], state->output_image, CL_TRUE, CL_MAP_WRITE, origin, region, &row_pitch, NULL,
                                        0, NULL, NULL, &errcode);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    memset(poutput, 0x0, img_buf_size);

   errcode = clEnqueueUnmapMemObject(state->basic.queue[0], state->output_image, poutput, 0, NULL, NULL);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    start = clst_clock_start();
    errcode = clEnqueueNDRangeKernel(state->basic.queue[0], state->kernel, 2 /* work_dim */,
                    NULL /* global_work_offset */, &global_size[0], NULL /* local_work_size */,
                    0, NULL, NULL);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    clFinish(state->basic.queue[0]);
    // calculate time taken in microseconds
    *elapsed_time_ndr = clst_clock_diff(start) * 1000000;

    //
    // Verify the output
    //
    start = clst_clock_start();
    poutput = clEnqueueMapImage(state->basic.queue[0], state->output_image, CL_TRUE, CL_MAP_READ, origin, region, &row_pitch, NULL,
                                        0, NULL, NULL, &errcode);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    inp = (cl_uchar*)state->in_mapion.pVirtualAddr;
    outp = poutput;
    for(i = 0; i < (row_pitch * state->imgh); i+=row_pitch)
    {

#if DEBUG
        if(!env->output_csv)
        {
            printf("comparing from %d through %d\n", i, i+(state->imgw * state->element_size)-1);
        }
#endif
        if(memcmp(inp+i, outp+i, state->imgw))
        {
            if(!env->output_csv)
            {
                printf("Output image verification failed\n");
            }
            return -1;
        }
    }
    elapsed_time_maprd += (clst_clock_diff(start) * 1000000);

#if DEBUG
for(i=0;i<(row_pitch * state->imgh); i++)
printf("inp[%d]=%x  outp[%d]=%x\n", i, inp[i], i, outp[i]);
#endif

    errcode = clEnqueueUnmapMemObject(state->basic.queue[0], state->output_image, poutput, 0, NULL, NULL);
    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    errcode = clFinish(state->basic.queue[0]);

    if(errcode)
    {
        LOG_ERROR_LOCATION(env);
        return errcode;
    }

    return CL_SUCCESS;
}

void ion_host_ptr_writecombine_images2D_preamble(const clst_test_env env, void** user_data)
{
    ion_host_ptr_preamble(env, "copy2d", CL_MEM_HOST_UNCACHED_QCOM, user_data);
}

void ion_host_ptr_writeback_images2D_preamble(const clst_test_env env, void** user_data)
{
    ion_host_ptr_preamble(env, "copy2d", CL_MEM_HOST_WRITEBACK_QCOM, user_data);
}

int main(int argc, char* argv[])
{
    static clst_test tests[] =
    {
        {"host_ptr_images2D_writecombine", ion_host_ptr_writecombine_images2D_preamble, ion_mem_map, ion_host_ptr_images2D_postamble,
            "Map ION host ptr to gpu address space (CL Image)", "us"},
        {"host_ptr_images2D_writeback", ion_host_ptr_writeback_images2D_preamble, ion_mem_map, ion_host_ptr_images2D_postamble,
            "Map ION host ptr to gpu address space (CL Image)", "us"},
    };

    return clst_main(argc, argv,
                     CLST_PERFORMANCE_TESTS,
                     "ion_mem_map_image",
                     "Map ION host ptr to gpu address space (CL Image)",
                     sizeof(tests)/sizeof(tests[0]), tests);
}
