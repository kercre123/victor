/* Copyright (c) 2014-2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "clstlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#define CSV_SIZE 1024

typedef struct _clst_context* clst_context;

/** Pseudorandom number generator state */
cl_ulong g_rand;


CLST_API void clst_srand(cl_ulong seed)
{
    seed += ~(seed <<  9);
    seed ^=  (seed >> 14);
    seed +=  (seed <<  4);
    seed ^=  (seed >> 10);

    g_rand = seed;
}

CLST_API void clst_set_rand_state(cl_ulong new_state)
{
    g_rand = new_state;
}

CLST_API cl_ulong clst_get_rand_state(void)
{
    return g_rand;
}


struct _clst_context
{
    /** Name of this suite */
    const char*         suite_name;

    /** Brief description of this suite */
    const char*         suite_description;

    /** Number of tests */
    int                 num_tests;

    /** Tests in this suite */
    const clst_test*    test;

    /** Devices that must be used to create the CL context for this test */
    /*@{*/
    int                 num_devices;
    cl_device_id        device[CLST_MAX_DEVICES];
    /*@}*/

    /** What type of data will be displayed */
    enum
    {
        /** Display the time it took to run the test */
        CLST_DISPLAY_PERFORMANCE = 1,
        /** Display whether the test passed or failed */
        CLST_DISPLAY_RESULT      = 2

    }                   display;

    /** How many times to run the tests */
    int                 reps;

    /** Whether or not these tests will be run with fast relaxed math */
    int                 fast_math;
};

static clst_context
clst_create_context(clst_suite_type  suite_type,
                    const char*      suite_name,
                    const char*      suite_description)
{
    clst_context ctx = clst_calloc(sizeof(struct _clst_context));

    if(!ctx)
    {
        return NULL;
    }

    switch(suite_type)
    {
    case CLST_PERFORMANCE_TESTS:
        ctx->display = CLST_DISPLAY_PERFORMANCE;
        break;
    default:
    case CLST_REGRESSION_TESTS:
        ctx->display = CLST_DISPLAY_RESULT;
        break;
    }

    ctx->suite_name        = suite_name;
    ctx->suite_description = suite_description;
    ctx->reps              = -1;

    return ctx;
}

static void
clst_list_tests(clst_context ctx, int num_tests, const clst_test* test)
{
    int i = 0;

    printf("%s\n\n", ctx->suite_description);
    printf("List of tests in this suite:\n\n");

    for(i = 0; i < num_tests; ++i)
    {
        printf("  %-16s : %s\n", test[i].test_name, test[i].test_description);

        if(i + 1 < num_tests)
        {
            printf("\n");
        }
    }
}

static void
clst_list_platforms(clst_context ctx)
{
    cl_uint         i             = 0;
    cl_uint         num_platforms = 0;
    cl_platform_id  platform[CLST_MAX_PLATFORMS];
    cl_char         str[CLST_MAX_STR_LEN];

    (void)ctx;

    clGetPlatformIDs(sizeof(platform)/sizeof(platform[0]), platform, &num_platforms);

    if(num_platforms)
    {
        printf("List of available platforms:\n\n");

        for(i = 0; i < num_platforms; ++i)
        {
            clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, sizeof(str), str, NULL);
            printf("Platform \"%s\":\n", str);

            clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR, sizeof(str), str, NULL);
            printf("  Vendor : %s\n", str);

            clGetPlatformInfo(platform[i], CL_PLATFORM_VERSION, sizeof(str), str, NULL);
            printf("  Version: %s\n", str);

            clGetPlatformInfo(platform[i], CL_PLATFORM_PROFILE, sizeof(str), str, NULL);
            printf("  Profile: %s\n", str);
            printf("\n");
        }
    }
    else
    {
        printf("No OpenCL platforms available on the system.\n");
    }
}

static cl_platform_id
clst_find_platform_by_name(const char* platform_name)
{
    cl_uint         i             = 0;
    cl_uint         num_platforms = 0;
    cl_platform_id  platform[CLST_MAX_PLATFORMS];
    cl_char         str[CLST_MAX_STR_LEN];

    clGetPlatformIDs(sizeof(platform)/sizeof(platform[0]), platform, &num_platforms);

    for(i = 0; i < num_platforms; ++i)
    {
        clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, sizeof(str), str, NULL);

        if(!strcmp((char*)str, platform_name))
        {
            return platform[i];
        }
    }

    return NULL;
}

static void
clst_list_devices(clst_context ctx, cl_platform_id platform)
{
    cl_uint         i               = 0;
    cl_uint         num_devices     = 0;
    cl_device_id    device[CLST_MAX_DEVICES];
    cl_char         str[CLST_MAX_STR_LEN];
    cl_bool         image_support   = CL_FALSE;
    cl_device_type  device_type     = 0;

    (void)ctx;

    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, sizeof(device)/sizeof(device[0]), device, &num_devices);

    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(str), str, NULL);
    printf("List of available devices on platform \"%s\":\n\n", str);

    for(i = 0; i < num_devices; ++i)
    {
        clGetDeviceInfo(device[i], CL_DEVICE_NAME, sizeof(str), str, NULL);
        printf("Device \"%s\":\n", str);

        clGetDeviceInfo(device[i], CL_DEVICE_VENDOR, sizeof(str), str, NULL);
        printf("  Vendor : %s\n", str);

        clGetDeviceInfo(device[i], CL_DEVICE_VERSION, sizeof(str), str, NULL);
        printf("  Version: %s\n", str);

        clGetDeviceInfo(device[i], CL_DEVICE_PROFILE, sizeof(str), str, NULL);
        printf("  Profile: %s\n", str);

        clGetDeviceInfo(device[i], CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
        printf("  Device type: ");

        if(device_type & CL_DEVICE_TYPE_CPU)
        {
            printf("CPU ");
        }

        if(device_type & CL_DEVICE_TYPE_GPU)
        {
            printf("GPU ");
        }

        if(device_type & CL_DEVICE_TYPE_ACCELERATOR)
        {
            printf("ACCELERATOR ");
        }

        if(device_type & CL_DEVICE_TYPE_DEFAULT)
        {
            printf("DEFAULT");
        }
        printf("\n");

        clGetDeviceInfo(device[i], CL_DEVICE_IMAGE_SUPPORT, sizeof(image_support), &image_support, NULL);
        printf("  Supports images: %s\n", image_support? "YES" : "NO");
        printf("\n");
    }
}


static void
clst_show_usage(clst_context ctx)
{
    printf("Usage: %s <options>\n", ctx->suite_name);
    printf("\n");

    printf("Query options:\n");
    printf("  -tlist         List all the tests in this suite.\n");
    printf("  -plist         Lists all the available platforms.\n");
    printf("  -dlist [plat]  Lists all the devices available in platform [plat].\n");
    printf("                 If [plat] is omitted, lists all devices on the first\n");
    printf("                 platform found.\n");
    printf("\n");

    printf("Output options:\n");
    printf("  -perf          Enable performance mode: display the time it takes to run\n");
    printf("                 each test.\n");
    printf("  -regress       Enable regression mode: display whether each test passed.\n");
    printf("  -csv           Output results in CSV format. This option is orthogonal\n");
    printf("                 with performance and regression modes.\n");
    printf("  -noverify      Skips Output verification. Allows tests to complete faster\n");
    printf("\n");

    printf("Test options:\n");
    printf("  -t <name>      Run a single test <name>. If this option is not specified,\n");
    printf("                 all tests in this suite will be run\n");
    printf("  -p <platform>  Selects the platform on which to run the tests. If none is\n");
    printf("                 selected, the tests will run on all the available platforms.\n");
    printf("  -device <type> Selects the devices on which to run the tests. If none\n");
    printf("                 are selected, the tests will run on all devices in the\n");
    printf("                 selected platforms.\n");
    printf("                 The device types are: CPU, GPU, ACCELERATOR, DEFAULT, ALL.\n\n");
    printf("  -wimpy         Run tests in wimpy mode, which is faster but less thorough.\n");
    printf("  -rep <reps>    Run each test <reps> times. If the suite is running in \n");
    printf("                 performance mode, the reported time is the trimmed mean of\n");
    printf("                 the runs. If the suite is running in regression mode, then a\n");
    printf("                 test is said to pass if and only if it passed in all of them.\n");
    printf("                 The default is value is 1 for regression mode and %d for\n", CLST_DEFAULT_REPS_PERFORMANCE);
    printf("                 performance mode.\n");
    printf("  -b <size>      Specify the buffer size in bytes for memory tests which have implemented it.\n");
    printf("                 This size overrides -w, -h, and -d for tests that support it.\n");
    printf("  -w <size>      Specifies the global work size 0 for the test. If all 3 work\n");
    printf("                 sizes are not specified, the test will use its own defaults.\n");
    printf("  -h <size >     Specifies the global work size 1 for the test.\n");
    printf("  -d <size >     Specifies the global work size 2 for the test.\n");
    printf("  -lw <size>     Specifies the local work size 0 for the test. If all 3 work\n");
    printf("                 sizes are not specified, the test will use its own defaults.\n");
    printf("  -lh <size >    Specifies the local work size 1 for the test.\n");
    printf("  -ld <size >    Specifies the local work size 2 for the test.\n");
    printf("  -prime         Runs the test proper function only once to prime the device and TLB.\n");
    printf("  -fast          Build the kernel with -cl-fast-relaxed-math.\n");
}

CLST_API int clst_sigfig(char * str, double value, int num_sig_figs)
{
    double   right_shift, multiplier, signage;
    int      num_decimals;

    /** if the number is negative, make positive first. **/
    signage = ((value < 0) ? -1 : 1);
    value *= signage;

    /** right_shift is how many times we need to move the decimal
        point to the right before rounding.  For large numbers, right_shift
        will be negative, meaning the decimal will actually get shifted left. **/
    right_shift = num_sig_figs - ceil(log10(value));

    /** shift the decimal point and round.
        For large values, right_shift will be negative and multiplier will be
        0.1, 0.01, etc.
        For small values, right_shift will be positive and multiplier will be
        1, 10, 100, etc. **/
    multiplier = pow(10, right_shift);
    value *= multiplier;
    value = floor(value + 0.5);
    value /= multiplier;

    /** Make it negative again if it started that way. **/
    value *= signage;

    /** Now the number has a value such that it will only display N sig figs. **/
    num_decimals = (int) (right_shift <= 0 ? 0 : right_shift);

    return sprintf(str, "%.*f", num_decimals, value);
}

CLST_API int clst_main(int argc, char* argv[],
                       clst_suite_type suite_type,
                       const char* suite_name,
                       const char* suite_description,
                       int num_tests, const clst_test *test)
{
    int           i             = 0;
    int           j             = 0;
    int           d             = 0;
    int           failures      = 0;
    int           show_usage    = 0;
    int           list_tests    = 0;
    int           list_platforms= 0;
    int           list_devices  = 0;
    int           verifyResult  = 1;
    int           wimpy         = 0;
    int           output_csv    = 0;
    int           prime         = 0;
    int           fast_math     = 0;
    const char*   test_name     = NULL;
    cl_platform_id platform[CLST_MAX_PLATFORMS] = {0};
    cl_uint        num_platforms = 0;
    cl_device_type device_type = CL_DEVICE_TYPE_DEFAULT;
    size_t        global_work_size[3] = {0, 0, 0};
    size_t        local_work_size[3]  = {0, 0, 0};
    size_t        buffer_size = 0;
    clst_test_env env           = NULL;
    double        acc_time      = 0.0;
    double        last_time     = 0.0;
    double        min_time      = 0.0;
    double        max_time      = 0.0;
    char          sigfig_buf[CLST_SIGFIG_BUF_SIZE];
    clst_context  ctx           = clst_create_context(suite_type, suite_name, suite_description);

    if(!ctx)
    {
        printf("Out of memory!");
        return -1;
    }

    //
    // Parse the command-line parameters
    //
#define OPTION_IS(op) !strcmp(argv[i], op)
    for(i = 1; i < argc; ++i)
    {
        //
        // Query options
        //
        if(OPTION_IS("-tlist"))
        {
            list_tests = 1;
        }
        else
        if(OPTION_IS("-plist"))
        {
            list_platforms = 1;
        }
        else
        if(OPTION_IS("-dlist"))
        {
            list_devices  = 1;

            if((i + 1 >= argc) || argv[i+1][0] == '-')
            {
                // No platform specified. Select all available platforms.
                break;
            }

            num_platforms = 1;
            platform[0] = clst_find_platform_by_name(argv[++i]);

            if(!platform[0])
            {
                printf("Could not find the desired platform. Try -plist to list all available platforms.");
                goto unknown_option;
            }
        }
        else
        //
        // Test options
        //
        if(OPTION_IS("-t"))
        {
            if(i + 1 >= argc)
            {
                goto unknown_option;
            }

            test_name = argv[++i];
        }
        else
        if(OPTION_IS("-p"))
        {
            if(i + 1 >= argc)
            {
                goto unknown_option;
            }

            num_platforms = 1;
            platform[0] = clst_find_platform_by_name(argv[++i]);

            if(!platform[0])
            {
                printf("Could not find the desired platform. Try -plist to list all available platforms.");
                goto unknown_option;
            }
        }
        else
        if(OPTION_IS("-device"))
        {
            if(i + 1 >= argc)
            {
                goto unknown_option;
            }

            ++i;

            if(OPTION_IS("CPU") || OPTION_IS("cpu"))
            {
                printf("Requesting device_type CL_DEVICE_TYPE_CPU\n");
                device_type = CL_DEVICE_TYPE_CPU;
            }
            else
            if(OPTION_IS("GPU")  || OPTION_IS("gpu"))
            {
                printf("Requesting device_type CL_DEVICE_TYPE_GPU\n");
                device_type = CL_DEVICE_TYPE_GPU;
            }
#ifdef FUTURE_SUPPORT
            else
            if(OPTION_IS("ACCELERATOR"))
            {
                device_type = CL_DEVICE_TYPE_ACCELERATOR;
            }
            else
            if(OPTION_IS("DEFAULT"))
            {
                device_type = CL_DEVICE_TYPE_DEFAULT;
            }
            else
            if(OPTION_IS("ALL"))
            {
                device_type = CL_DEVICE_TYPE_ALL;
            }
#endif
            else
            {
                goto unknown_option;
            }
        }
        else
        if (OPTION_IS("-b"))
        {
            if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            buffer_size = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-w"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            global_work_size[0] = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-h"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            global_work_size[1] = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-d"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            global_work_size[2] = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-lw"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            local_work_size[0] = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-lh"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            local_work_size[1] = atoi(argv[++i]);
        }
        else
        if (OPTION_IS("-ld"))
        {
             if(i + 1 >= argc)
            {
                goto unknown_option;
            }
            local_work_size[2] = atoi(argv[++i]);
        }
        else
        if(OPTION_IS("-wimpy"))
        {
            wimpy = 1;
        }
        else
        if(OPTION_IS("-noverify"))
        {
            verifyResult = 0;
        }
        else
        if(OPTION_IS("-rep"))
        {
            if(i + 1 >= argc)
            {
                goto unknown_option;
            }

            ctx->reps = atoi(argv[++i]);
        }
        else
        //
        // Display options
        //
        if(OPTION_IS("-perf"))
        {
            ctx->display = CLST_DISPLAY_PERFORMANCE;
        }
        else
        if(OPTION_IS("-regress"))
        {
            ctx->display = CLST_DISPLAY_RESULT;
        }
        else
        if(OPTION_IS("-csv"))
        {
            output_csv = 1;
        }
        else
        if(OPTION_IS("-prime"))
        {
            prime = 1;
        }
        else
        if(OPTION_IS("-fast"))
        {
            fast_math = 1;
        }
        else
        {
unknown_option:
            printf("Unknown option %s or missing argument to that option.\n", argv[i]);
            show_usage = 1;
        }
    }
#undef OPTION_IS

    if(!num_platforms)
    {
        // No platform specified. Select all the available platforms.
        clGetPlatformIDs(sizeof(platform)/sizeof(platform[0]), platform, &num_platforms);

        if(!num_platforms && !list_tests)
        {
            printf("Could not find any OpenCL platforms on the system!");
            return -1;
        }
    }

    //
    // Service queries if any.
    //
    if(list_tests)
    {
        clst_list_tests(ctx, num_tests, test);
        return -1;
    }
    else
    if(list_platforms)
    {
        clst_list_platforms(ctx);
        return -1;
    }
    else
    if(list_devices)
    {
        for(i = 0; i < (int)num_platforms; ++i)
        {
            clst_list_devices(ctx, platform[i]);
        }
        return -1;
    }

    //
    // Set up defaults
    //
    if(ctx->reps == -1)
    {
        ctx->reps = (ctx->display == CLST_DISPLAY_PERFORMANCE)? CLST_DEFAULT_REPS_PERFORMANCE : 1;
    }

    if(test_name)
    {
        // Try to find the test in the list
        for(i = 0; i < num_tests; ++i)
        {
            if(!strcmp(test[i].test_name, test_name))
            {
                ctx->num_tests = 1;
                ctx->test      = &test[i];
                break;
            }
        }

        if(!ctx->num_tests)
        {
            printf("Could not find test %s. Use -tlist to list all tests.\n\n", test_name);
            show_usage = 1;
        }
    }
    else
    {
        // No test specified. Run all tests.
        ctx->num_tests = num_tests;
        ctx->test      = test;
    }

    if(show_usage)
    {
        clst_show_usage(ctx);
        return -1;
    }

    //
    // Set up the test environment
    //
    env = clst_calloc(sizeof(struct _clst_test_env));
    if(!env)
    {
        printf("Out of memory!");
        return -1;
    }
    env->private_ctx  = ctx;
    env->wimpy        = wimpy;
    env->output_csv   = output_csv;
    env->verifyResult = verifyResult;
    env->fast_math    = fast_math;

    //
    // Select the desired device
    //
    for(i = 0; i < (int)num_platforms; ++i)
    {
        cl_device_id dev[CLST_MAX_DEVICES];
        cl_uint      n = 0;

        clGetDeviceIDs(platform[i], device_type, sizeof(dev)/sizeof(dev[0]), dev, &n);

        for(j = 0; j < (int)n; ++j)
        {
            ctx->device[ctx->num_devices++] = dev[j];
        }
    }

    if(ctx->num_devices == 0)
    {
        printf("Could not find any devices of the desired type. Try -dlist to see all devices.");
        return -1;
    }

    //
    // Run the selected tests.
    //
    printf("bitness: %lu\n\n", (unsigned long)(8 * sizeof(void*)));
    if(env->output_csv)
    {
        printf("Test name%s,", env->wimpy? " (running in wimpy mode)" : "");

        for(d = 0; d < ctx->num_devices; ++d)
        {
            cl_char devname[CLST_MAX_STR_LEN];
            clGetDeviceInfo(ctx->device[d], CL_DEVICE_NAME, sizeof(devname), devname, NULL);
            printf("\"%s\"%s", devname, (d == ctx->num_devices - 1)? "\n" : ",");
        }
    }
    else
    {
        printf("Running test suite %s%s on ",
            ctx->suite_name,
            env->wimpy? " in wimpy mode" : "");

        for(d = 0; d < ctx->num_devices; ++d)
        {
            cl_char devname[CLST_MAX_STR_LEN];
            clGetDeviceInfo(ctx->device[d], CL_DEVICE_NAME, sizeof(devname), devname, NULL);
            printf("\"%s\"%s", devname, (d == ctx->num_devices - 1)? "\n" : ", ");
        }
    }

    fflush(stdout);

    for(i = 0; i < ctx->num_tests; ++i)
    {
        int d           = 0;
        int r           = 0;

        // to store csv output
        char csvstr[CSV_SIZE] = {0};

        env->test_name   = ctx->test[i].test_name;

        if(env->output_csv)
        {
            clst_snprintf(csvstr + strlen(csvstr), CSV_SIZE - strlen(csvstr), "%s.%s,", ctx->suite_name, ctx->test[i].test_name);
            if (ctx->display == CLST_DISPLAY_PERFORMANCE)
            {
                clst_snprintf(csvstr + strlen(csvstr), CSV_SIZE - strlen(csvstr), "%s,", ctx->test[i].return_value_description);
            }
        }
        else
        {
            printf("  %-30s - ", ctx->test[i].test_name);
            if (ctx->display == CLST_DISPLAY_PERFORMANCE)
            {
                printf("%-18s : ", ctx->test[i].return_value_description);
            }
            else
            {
                printf("%-18s : ", " ");
            }
        }

        env->buffer_size = buffer_size;
        for (r = 0; r < 3; r++)
        {
            env->global_work_size[r] = global_work_size[r];
            env->local_work_size[r]  = local_work_size[r];
        }

        for(d = 0; d < ctx->num_devices; ++d)
        {
            void *user_data = NULL;
            int last_failed = 0;

            // Reset the timers.
            acc_time = 0.0;
            min_time = CL_FLT_MAX;
            max_time = -CL_FLT_MAX;

            // Seed the prng even before calling into the preamble
            clst_srand(i);

            env->num_devices = 1;
            env->device[0] = ctx->device[d];
            clGetDeviceInfo(env->device[0], CL_DEVICE_PLATFORM, sizeof(env->platform), &env->platform, NULL);

            // Run the test preamble once.
            if(ctx->test[i].test_preamble_fn)
            {
                ctx->test[i].test_preamble_fn(env, &user_data);
            }
            else
            {
                user_data = NULL;
            }
            // Run the test proper function once to prime the TLB and the device.
            if(prime)
            {
                last_failed  |= !!ctx->test[i].test_fn(env, user_data, &last_time);
            }

            for(r = 0; r < ctx->reps; ++r)
            {
                // Seed the random number generator in a deterministic way.
                clst_srand(r);

                // Run the test!
                last_time = 0.0;
                last_failed  |= !!ctx->test[i].test_fn(env, user_data, &last_time);

                if(last_failed)
                {
                    // There's no point in continuing to run the test.
                    break;
                }

                if(last_time > max_time)
                {
                    max_time = last_time;
                }
                if(last_time < min_time)
                {
                    min_time = last_time;
                }

                acc_time += last_time;
            }

            // Run the test postamble once.
            if(ctx->test[i].test_postamble_fn)
            {
                ctx->test[i].test_postamble_fn(env, user_data);
            }
            user_data = NULL;

            // Measure the performance trimming outliers if possible.
            switch(ctx->reps)
            {
            case 0:
                // For debugging - set acc_time to -1.
                acc_time = -1;
                break;
            case 1:
                // Do not trim any results
                break;
            case 2:
                // Trim only the maximum time
                acc_time = (acc_time - max_time) / (ctx->reps - 1);
                break;
            default:
                // Trim both the maximum and the minimum times
                acc_time = (acc_time - min_time - max_time) / (ctx->reps - 2);
                break;
            }

            if(last_failed)
            {
                // Highlight the failure with a very visible time.
                // Could use NaN here but it might confuse Excel or the CSV file reader.
                acc_time = -9.999999;
            }

            failures += last_failed;

            // Print value with CLST_DEFAULT_SIG_FIGS significant figures
            clst_sigfig(sigfig_buf, acc_time, CLST_DEFAULT_SIG_FIGS);
            if(env->output_csv)
            {
                if(ctx->display == CLST_DISPLAY_PERFORMANCE)
                {
                    clst_snprintf(csvstr + strlen(csvstr), CSV_SIZE - strlen(csvstr), "%s", sigfig_buf);
                }
                else
                {
                    clst_snprintf(csvstr + strlen(csvstr), CSV_SIZE - strlen(csvstr), "%s", last_failed? "FAIL" : "PASS");
                }

                clst_snprintf(csvstr + strlen(csvstr), CSV_SIZE - strlen(csvstr), "%s", (d == ctx->num_devices - 1)? "\n" : ",");

            }
            else
            {
                if(ctx->display == CLST_DISPLAY_PERFORMANCE)
                {
                    printf("%10s", sigfig_buf);
                }
                else
                {
                    if(last_failed)
                    {
                        clst_set_console_color(CLST_COLOR_RED);
                        printf("FAIL");
                    }
                    else
                    {
                        clst_set_console_color(CLST_COLOR_GREEN);
                        printf("PASS");
                    }
                    clst_set_console_color(CLST_COLOR_DEFAULT);
                }

                printf("%s", (d == ctx->num_devices - 1)? "\n" : " ");
            }
        }
        // print csv output
        printf("%s", csvstr);
        fflush(stdout);
    }

    if(!env->output_csv)
    {
        printf("\nFinished test suite %s. ", ctx->suite_name);

        if(failures)
        {
            clst_set_console_color(CLST_COLOR_RED);
            printf("%d tests out of %d failed.\n", failures, ctx->num_tests);
        }
        else
        {
            clst_set_console_color(CLST_COLOR_GREEN);
            printf("All tests passed (%3d total).\n", ctx->num_tests);
        }

        clst_set_console_color(CLST_COLOR_DEFAULT);
        fflush(stdout);
    }

    clst_free(ctx);
    clst_free(env);

    return failures;
}


CLST_API void clst_test_log(const clst_test_env env, const char* fmt, ...)
{
    clst_context ctx = (clst_context)env->private_ctx;
    va_list args;

    (void)ctx;

    va_start(args, fmt);

    fprintf(stderr, "\n*** %s ERROR: ", env->test_name);
    vfprintf(stderr, fmt, args);

    va_end(args);
}


CLST_API int clst_test_error(const clst_test_env env, const char *filename, unsigned line, cl_int errcode)
{
    const char *error = "";

    switch(errcode)
    {
    case CL_DEVICE_NOT_FOUND                         : error = "DEVICE_NOT_FOUND"; break;
    case CL_DEVICE_NOT_AVAILABLE                     : error = "DEVICE_NOT_AVAILABLE"; break;
    case CL_COMPILER_NOT_AVAILABLE                   : error = "COMPILER_NOT_AVAILABLE"; break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE            : error = "MEM_OBJECT_ALLOCATION_FAILURE"; break;
    case CL_OUT_OF_RESOURCES                         : error = "OUT_OF_RESOURCES"; break;
    case CL_OUT_OF_HOST_MEMORY                       : error = "OUT_OF_HOST_MEMORY"; break;
    case CL_PROFILING_INFO_NOT_AVAILABLE             : error = "PROFILING_INFO_NOT_AVAILABLE"; break;
    case CL_MEM_COPY_OVERLAP                         : error = "MEM_COPY_OVERLAP"; break;
    case CL_IMAGE_FORMAT_MISMATCH                    : error = "IMAGE_FORMAT_MISMATCH"; break;
    case CL_IMAGE_FORMAT_NOT_SUPPORTED               : error = "IMAGE_FORMAT_NOT_SUPPORTED"; break;
    case CL_BUILD_PROGRAM_FAILURE                    : error = "BUILD_PROGRAM_FAILURE"; break;
    case CL_MAP_FAILURE                              : error = "MAP_FAILURE"; break;
    case CL_INVALID_VALUE                            : error = "INVALID_VALUE"; break;
    case CL_INVALID_DEVICE_TYPE                      : error = "INVALID_DEVICE_TYPE"; break;
    case CL_INVALID_PLATFORM                         : error = "INVALID_PLATFORM"; break;
    case CL_INVALID_DEVICE                           : error = "INVALID_DEVICE"; break;
    case CL_INVALID_CONTEXT                          : error = "INVALID_CONTEXT"; break;
    case CL_INVALID_QUEUE_PROPERTIES                 : error = "INVALID_QUEUE_PROPERTIES"; break;
    case CL_INVALID_COMMAND_QUEUE                    : error = "INVALID_COMMAND_QUEUE"; break;
    case CL_INVALID_HOST_PTR                         : error = "INVALID_HOST_PTR"; break;
    case CL_INVALID_MEM_OBJECT                       : error = "INVALID_MEM_OBJECT"; break;
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR          : error = "INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
    case CL_INVALID_IMAGE_SIZE                       : error = "INVALID_IMAGE_SIZE"; break;
    case CL_INVALID_SAMPLER                          : error = "INVALID_SAMPLER"; break;
    case CL_INVALID_BINARY                           : error = "INVALID_BINARY"; break;
    case CL_INVALID_BUILD_OPTIONS                    : error = "INVALID_BUILD_OPTIONS"; break;
    case CL_INVALID_PROGRAM                          : error = "INVALID_PROGRAM"; break;
    case CL_INVALID_PROGRAM_EXECUTABLE               : error = "INVALID_PROGRAM_EXECUTABLE"; break;
    case CL_INVALID_KERNEL_NAME                      : error = "INVALID_KERNEL_NAME"; break;
    case CL_INVALID_KERNEL_DEFINITION                : error = "INVALID_KERNEL_DEFINITION"; break;
    case CL_INVALID_KERNEL                           : error = "INVALID_KERNEL"; break;
    case CL_INVALID_ARG_INDEX                        : error = "INVALID_ARG_INDEX"; break;
    case CL_INVALID_ARG_VALUE                        : error = "INVALID_ARG_VALUE"; break;
    case CL_INVALID_ARG_SIZE                         : error = "INVALID_ARG_SIZE"; break;
    case CL_INVALID_KERNEL_ARGS                      : error = "INVALID_KERNEL_ARGS"; break;
    case CL_INVALID_WORK_DIMENSION                   : error = "INVALID_WORK_DIMENSION"; break;
    case CL_INVALID_WORK_GROUP_SIZE                  : error = "INVALID_WORK_GROUP_SIZE"; break;
    case CL_INVALID_WORK_ITEM_SIZE                   : error = "INVALID_WORK_ITEM_SIZE"; break;
    case CL_INVALID_GLOBAL_OFFSET                    : error = "INVALID_GLOBAL_OFFSET"; break;
    case CL_INVALID_EVENT_WAIT_LIST                  : error = "INVALID_EVENT_WAIT_LIST"; break;
    case CL_INVALID_EVENT                            : error = "INVALID_EVENT"; break;
    case CL_INVALID_OPERATION                        : error = "INVALID_OPERATION"; break;
    case CL_INVALID_GL_OBJECT                        : error = "INVALID_GL_OBJECT"; break;
    case CL_INVALID_BUFFER_SIZE                      : error = "INVALID_BUFFER_SIZE"; break;
    case CL_INVALID_MIP_LEVEL                        : error = "INVALID_MIP_LEVEL"; break;
    case CL_INVALID_GLOBAL_WORK_SIZE                 : error = "INVALID_GLOBAL_WORK_SIZE"; break;
    default:
        error = "Unknown error";
    }

    clst_test_log(env, "In file '%s', line %4d, CL returned error %s\n", filename, line, error);

    return errcode;
}

CLST_API void CL_CALLBACK clst_notify(const char *msg, const void *data, size_t data_size, void *_env)
{
    clst_test_env env = (clst_test_env)_env;

    (void)data;
    (void)data_size;

    clst_test_log(env, "CL notifies that %s", msg);
}

CLST_API cl_uint clst_rand(void)
{
    // Constants taken from Knuth's MMIX
    g_rand = 6364136223846793005ULL * g_rand + 1442695040888963407ULL;

    return (cl_uint)(g_rand >> 32);
}

CLST_API cl_int clst_basic_test_setup(const clst_test_env    env,
                                      int                    num_sources,
                                      const char**           sources,
                                      const char*            options,
                                      clst_basic_test_state* state,
                                      cl_context_properties  priority)
{
    cl_int                errcode       = CL_SUCCESS;
    cl_context_properties properties[5] = {CL_CONTEXT_PLATFORM, 0, 0 /* CL_CONTEXT_PRIORITY_HINT_QCOM */, 0, 0};
    int                   i             = 0;
    int                   j             = 0;
    cl_build_status       build_status  = CL_BUILD_ERROR;
    cl_char              *build_log     = NULL;
    size_t                build_log_len = 0;

    // In case we need to append the build option for fast-relaxed math.
    const char           *FAST_MATH_FLAG        = " -cl-fast-relaxed-math";
    const size_t          FAST_MATH_FLAG_LEN    = sizeof(char)*strlen(FAST_MATH_FLAG);

    size_t                options_len           = options ? sizeof(char)*strlen(options) : 0; // Don't assume strlen can handle null pointers.
    size_t                build_options_len     = sizeof(char)*1 + options_len; // At least one character for the null terminator.
    char                 *build_options;

    // Allocate more space if we need the fast math option.
    if (env->fast_math)
    {
        build_options_len += FAST_MATH_FLAG_LEN;
    }

    build_options = clst_calloc(build_options_len);
    if (options)
    {
        strncpy(build_options, options, build_options_len);
    }
    if (env->fast_math)
    {
        strncat(build_options, FAST_MATH_FLAG, FAST_MATH_FLAG_LEN);
    }

    state->clctx   = NULL;
    for(i = 0; i < env->num_devices; ++i)
    {
        state->queue[i] = NULL;
    }
    state->program = NULL;

    properties[1] = (cl_context_properties)env->platform;

#if 0 // CL_CONTEXT_PRIORITY_HINT_QCOM doesn't build
    if(priority != 0)
    {
        properties[2] = CL_CONTEXT_PRIORITY_HINT_QCOM;
        properties[3] = priority;
    }
#endif

    state->clctx = clCreateContext(properties, env->num_devices, env->device, clst_notify, env, &errcode);
    if(errcode)
    {
        LOG_ERROR_LOCATION_STR(env,"Failed clCreateContext");
        goto cleanup;
    }

    for(i = 0; i < env->num_devices; ++i)
    {
        cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
        state->queue[i] = clCreateCommandQueue(state->clctx, env->device[i], properties, &errcode);

        if(errcode == CL_INVALID_QUEUE_PROPERTIES)
        {
            clst_test_log(env, "Device %d does not support out-of-order command queues. Using in-order queues.\n", i);
            properties = 0;
            state->queue[i] = clCreateCommandQueue(state->clctx, env->device[i], properties, &errcode);
            if(errcode)
            {
                LOG_ERROR_LOCATION_STR(env,"Failed clCreateCommandQueue");
                goto cleanup;
            }
        }
        else
        if(errcode)
        {
            LOG_ERROR_LOCATION(env);
            goto cleanup;
        }
    }

    if(!num_sources)
    {
        // Bypass creating the programs
        state->num_programs = 0;
        state->program = NULL;
        return CL_SUCCESS;
    }

    state->num_programs = num_sources;
    state->program      = clst_calloc(state->num_programs * sizeof(cl_program));

    if(!state->program)
    {
        LOG_ERROR_LOCATION(env);
        goto cleanup;
    }

    for(i = 0; i < num_sources; ++i)
    {
        state->program[i] = clCreateProgramWithSource(state->clctx, 1, &sources[i], NULL, &errcode);
        if(errcode)
        {
            LOG_ERROR_LOCATION(env);
            goto cleanup;
        }

        errcode = clBuildProgram(state->program[i], env->num_devices, env->device, build_options, NULL, NULL);

        if(errcode && errcode != CL_BUILD_PROGRAM_FAILURE)
        {
            LOG_ERROR_LOCATION(env);
            goto cleanup;
        }

        for(j = 0; j < env->num_devices; ++j)
        {
            // Attempt to build the program and wait until finished.
            do
            {
                errcode = clGetProgramBuildInfo(state->program[i], env->device[j], CL_PROGRAM_BUILD_STATUS,
                                                sizeof(build_status), &build_status, NULL);
                if(errcode)
                {
                    LOG_ERROR_LOCATION(env);
                    goto cleanup;
                }

                if(build_status == CL_BUILD_IN_PROGRESS)
                {
                    // Wait a bit for the program to finish building.
                    clst_usleep(10);
                }
            }
            while(build_status == CL_BUILD_IN_PROGRESS);

            // Print the build log if there are warnings or errors.
            errcode = clGetProgramBuildInfo(state->program[i], env->device[j], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
            if(errcode)
            {
                LOG_ERROR_LOCATION_STR(env, "clGetProgramBuildInfo failed.");
                errcode = CL_BUILD_PROGRAM_FAILURE;
                goto cleanup;
            }
            if(build_log_len > 1) // If the build log is more than a NULL terminator...
            {
                build_log = clst_calloc(build_log_len);
                if(!build_log)
                {
                    LOG_ERROR_LOCATION(env);
                    errcode = CL_BUILD_PROGRAM_FAILURE;
                    goto cleanup;
                }

                errcode = clGetProgramBuildInfo(state->program[i], env->device[j], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, NULL);
                if(errcode)
                {
                    LOG_ERROR_LOCATION(env);
                    errcode = CL_BUILD_PROGRAM_FAILURE;
                    goto cleanup;
                }

                /* Print the build log only if it does not contain "Pass" */
                if(strncmp((const char *)build_log, "Pass", strlen("Pass")))
                {
                    clst_test_log(env, "Build log follows:\n%s", build_log);
                }

                clst_free(build_log);
                clst_free(build_options);
            }

            if(build_status == CL_BUILD_ERROR)
            {
                LOG_ERROR_LOCATION_STR(env, "Program build failed.");
                errcode = CL_BUILD_PROGRAM_FAILURE;
                goto cleanup;
            }
        }
    }

    return CL_SUCCESS;

cleanup:
    clst_free(build_log);
    clst_free(build_options);
    clst_basic_test_release(env, state);

    return errcode;
}

CLST_API void clst_basic_test_release(const clst_test_env env, clst_basic_test_state *state)
{
    int i = 0;

    if(!state)
    {
        return;
    }


    if(state->program)
    {
        for(i = 0; i < state->num_programs; ++i)
        {
            if(state->program[i])
            {
                clReleaseProgram(state->program[i]);
            }
        }
        clst_free(state->program);
        state->program = NULL;
    }

    for(i = 0; i < env->num_devices; ++i)
    {
        if(state->queue[i])
        {
            clReleaseCommandQueue(state->queue[i]);
            state->queue[i] = NULL;
        }
    }

    if(state->clctx)
    {
        clReleaseContext(state->clctx);
        state->clctx   = NULL;
    }
}
