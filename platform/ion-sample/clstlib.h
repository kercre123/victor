/* Copyright (c) 2014, 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef CLSTLIB_H
#define CLSTLIB_H

#ifdef __cplusplus
extern "C" {
#endif


#include "osal.h"
#include <CL/cl.h>
#include <CL/cl_ext.h>

#if 0 // The below files have EGL dependency which isn't in Drones
#include <CL/cl_ext_qcom.h>
##include <CL/cl_ext_qcom_private_internal.h>
#endif

#ifdef CLST_EXPORTS
    #define CLST_API            OSAL_DLLEXPORT
#else
    #define CLST_API            OSAL_DLLIMPORT
#endif // CLST_EXPORTS

#define CLST_MAX_PLATFORMS              16
#define CLST_MAX_DEVICES                16
#define CLST_MAX_STR_LEN               128
#define CLST_DEFAULT_REPS_PERFORMANCE   10
#define CLST_DEFAULT_SIG_FIGS            4
#define CLST_SIGFIG_BUF_SIZE           256

/** Type of system test: performance, regression or stress */
enum _clst_suite_type
{
    CLST_PERFORMANCE_TESTS = 1,
    CLST_REGRESSION_TESTS  = 2,
};

typedef struct _clst_test_env*        clst_test_env;
typedef enum   _clst_suite_type       clst_suite_type;
typedef struct _clst_test             clst_test;
typedef struct _clst_basic_test_state clst_basic_test_state;

/** The environment in which a test is run. */
struct _clst_test_env
{
    /** Name of the test. */
    const char*            test_name;

    /** Whether to run the test in wimpy mode */
    int                    wimpy;

    /** Whether to verify the result */
    int                    verifyResult;

    size_t                 buffer_size;

    /** global work dimensions */
    size_t                 global_work_size[3];

    /** local work dimensions */
    size_t                 local_work_size[3];

    /** Platform that must be used for this test */
    cl_platform_id         platform;

    /** Devices that must be used to create the CL context for this test */
    /*@{*/
    int                    num_devices;
    cl_device_id           device[CLST_MAX_DEVICES];
    /*@}*/

    /** Whether to output results in CSV format or in a human-readable form. */
    int                    output_csv;

    void*                  private_ctx;

    /** Whether to run the test with -cl-fast-relaxed-math */
    int                    fast_math;
};

/** Basic state for a test. See also clst_basic_test_setup */
struct _clst_basic_test_state
{
    cl_context              clctx;
    cl_command_queue        queue[CLST_MAX_DEVICES]; // in order command queue
    cl_program             *program;

    // application-specified properties
    int                     num_programs;
    cl_context_properties   priority;
};

/** Prototypes for test functions. */
/*@{*/
/** Preamble: creates CL objects necessary to run the test.
  * It can transfer data to the test proper function via #user_data.
  * This function is called only once per test case.
  */
typedef  void (*clst_test_preamble_fn)(const clst_test_env env, void** user_data);

/** Test proper. This is where the test is actually run, and this is the only function whose execution time is measured.
  * Note that this function may be executed multiple times per test case,
  * while the preamble and postamble are only executed once.
  */
typedef  int (*clst_test_proper_fn)(const clst_test_env env, void* user_data, double *elapsed_time);

/** Postample: destroys CL objects created in the preamble.
  * Frees memory and resources associated with user_data.
  * This function is called only once per test case.
  */
typedef  void (*clst_test_postamble_fn)(const clst_test_env env, void* user_data);
/*@}*/

/** A CLST test is composed of a name, a test function and a short description. */
struct _clst_test
{
    /** Test identifier. It must be unique and reasonably short (32 characters max.) */
    const char*     test_name;

    /** Functions to run the test. */
    /*@{*/
    clst_test_preamble_fn  test_preamble_fn;
    clst_test_proper_fn    test_fn;
    clst_test_postamble_fn test_postamble_fn;
    /*@}*/

    /** One-liner. Can be NULL or empty. */
    const char*     test_description;

    /** What the test is measuring. */
    const char*     return_value_description;
};

/** clst_sigfig prints value into str, with num_sig_figs number of significant figures,
    such that if num_sig_figs == 4:
    54623.871 -> "54620"
    54.623871 -> "54.62"
    0.0546238 -> "0.05462"
  */
CLST_API int clst_sigfig(char * str, double value, int num_sig_figs);

/** All tests must immediately call this function in their main() routine to transfer the control to the CLST framework. */
CLST_API int clst_main(int argc, char* argv[], clst_suite_type suite_type,
                       const char* suite_name,
                       const char* suite_description,
                       int num_tests, const clst_test *test);


/* *** Utility macros and functions that the tests can use *** */

CLST_API cl_int clst_basic_test_setup(const clst_test_env    env,
                                      int                    num_sources,
                                      const char**           sources,
                                      const char*            options,
                                      clst_basic_test_state* state,
                                      cl_context_properties  priority);

CLST_API void clst_basic_test_release(const clst_test_env env, clst_basic_test_state *state);

#define CLST_CHECK_ERRCODE(clctx, env, errcode)                        \
    if((errcode) != CL_SUCCESS) {                                      \
        clReleaseContext((clctx));                                     \
        return clst_test_error((env), __FILE__, __LINE__, (errcode));  \
    }

#define CLST_CHECK_ERRCODE_NORC(clctx, env, errcode)                        \
    if((errcode) != CL_SUCCESS) {                                      \
        clReleaseContext((clctx));                                     \
    }

#define LOG_ERROR_LOCATION(env) \
    clst_test_log(env, "Error on function %s, line %d!\n", __FUNCTION__, __LINE__);

#define LOG_ERROR_LOCATION_STR(env,str) \
    clst_test_log(env, "Error %s on function %s, line %d!\n", str,__FUNCTION__, __LINE__);

/** Appends an error or warning message to the log file.
  * The error message needs to be properly terminated with a newline.
  */
CLST_API void clst_test_log(const clst_test_env env, const char* fmt, ...);

/** Generates an error message according to the CL errcode and returns #errcode */
CLST_API int  clst_test_error(const clst_test_env env, const char *filename, unsigned line, cl_int errcode);

/** Notify function that can be used when the CL context is created for a test.
  * Use the pointer to the test environment as the 'user_data' argument to clCreateContext.
  */
CLST_API void CL_CALLBACK clst_notify(const char *msg, const void *data, size_t data_size, void *_env);

/** Resets the state of the random number generator indirectly through a seed. Use judiciously!
  * Setting up the prng is generally the responsibility of the clstlib, not the responsibility of the test!
  */
CLST_API void clst_srand(cl_ulong seed);

/** Resets the state of the random number generator directly to the given value. Use judiciously!
  * Setting up the prng is generally the responsibility of the clstlib, not the responsibility of the test!
  */
CLST_API void clst_set_rand_state(cl_ulong rand_state);

/** Returns the internal state of the random number generator */
CLST_API cl_ulong clst_get_rand_state(void);

/** Returns a 32-bit unsigned value that is uniformly distributed.
  * Seeding the prng is generally the responsibility of the clstlib, not the responsibility of the test!
  * XXX Not thread safe.
  */
CLST_API cl_uint clst_rand(void);

#ifdef __cplusplus
}
#endif

#endif // CLSTLIB_H
