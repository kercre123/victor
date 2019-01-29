#include "Serialization.h"

#include "Util.h"

#include <sstream>
#include <iterator>

#define BOOL_TO_STR(x) ((x)? "true" : "false")

std::ostream& operator<<(std::ostream& os, const cl::Platform& platform)
{
  std::vector<std::string> extensions = Anki::split(std::string(platform.getInfo<CL_PLATFORM_EXTENSIONS>()));

  os<<"PLATFORM: "<<std::endl;
  os<<"  NAME:         "<<platform.getInfo<CL_PLATFORM_NAME>()<<std::endl;
  os<<"  PROFILE:      "<<platform.getInfo<CL_PLATFORM_PROFILE>()<<std::endl;
  os<<"  VERSION:      "<<platform.getInfo<CL_PLATFORM_VERSION>()<<std::endl;
  os<<"  VENDOR:       "<<platform.getInfo<CL_PLATFORM_VENDOR>()<<std::endl;
  os<<"  EXTENSIONS:   "<<std::endl;
  for (const auto& extension : extensions){
    os<<"     "<<extension<<std::endl;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const cl::Context& context)
{
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
  
  os<<"CONTEXT:  "<<std::endl;
  os<<"  REFERENCE_COUNT:  "<<context.getInfo<CL_CONTEXT_REFERENCE_COUNT>()<<std::endl;
  os<<"  NUM_DEVICES:      "<<context.getInfo<CL_CONTEXT_NUM_DEVICES>()<<std::endl;
  os<<"  DEVICES:          "<<std::endl;
  for (cl::Device& device: devices){
    os<<"     "<<device.getInfo<CL_DEVICE_NAME>()<<std::endl;
  }

  // os<<"  PROPERTIES: "<<std::endl;
  // std::vector<cl_contect_properties> properties = context.getInfo<CL_CONTEXT_PROPERTIES>()

  // std::cerr<<"    D3D10_PREFER_SHARED_RESOURCES_KHR:  "<<_context.getInfo<CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR>()<<std::endl;

  return os;
}

std::ostream& operator<<(std::ostream& os, const cl::Program& program)
{
  cl::Context context = program.getInfo<CL_PROGRAM_CONTEXT>();
  std::vector<cl::Device> devices = program.getInfo<CL_PROGRAM_DEVICES>();
  std::vector<size_t> binary_sizes = program.getInfo<CL_PROGRAM_BINARY_SIZES>();


  os<<"PROGRAM: "<<std::endl;
  os<<"  REFERENCE_COUNT:    "<<program.getInfo<CL_PROGRAM_REFERENCE_COUNT>()<<std::endl;
  os<<"  NUM_DEVICES:        "<<program.getInfo<CL_PROGRAM_NUM_DEVICES>()<<std::endl;
  os<<"  DEVICES: "<<std::endl;
  for (cl::Device& device: devices){
    std::string build_status;
    switch(program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device))
    { 
      case CL_BUILD_NONE: build_status = "NONE"; break;
      case CL_BUILD_ERROR: build_status = "ERROR"; break;
      case CL_BUILD_SUCCESS: build_status = "SUCCESS"; break;
      case CL_BUILD_IN_PROGRESS: build_status = "IN_PROGRESS"; break;
    }

    os<<"     NAME: "<<device.getInfo<CL_DEVICE_NAME>()<<std::endl;
    os<<"     BUILD_STATUS:       "<<build_status<<std::endl;
    os<<"     BUILD_OPTIONS:      "<<program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(device)<<std::endl;
    os<<"     BUILD_LOG:          "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)<<std::endl;
  }
  os<<"  BINARY_SIZES:       "<<std::endl;
  for (auto index = 0; index < binary_sizes.size(); ++index){
    os<<"      Size["<<index<<"]: "<<binary_sizes[index]<<std::endl;
  }
  os<<"  SOURCE: "<<std::endl;
  os<<program.getInfo<CL_PROGRAM_SOURCE>()<<std::endl;

  // os<<"    BINARIES:           "<<program.getInfo<CL_PROGRAM_BINARIES>()<<std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const cl::Device& device)
{
  std::vector<std::string> extensions = Anki::split(std::string(device.getInfo<CL_DEVICE_EXTENSIONS>()));
  cl_device_type device_type = device.getInfo<CL_DEVICE_TYPE>();
  cl_device_fp_config double_fp_config = device.getInfo<CL_DEVICE_DOUBLE_FP_CONFIG>();
  cl_device_exec_capabilities execution_capabilities = device.getInfo<CL_DEVICE_EXECUTION_CAPABILITIES>();
  std::string global_mem_cache_type;
  switch(device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>())
  {
    case CL_NONE: global_mem_cache_type = "NONE"; break;
    case CL_READ_ONLY_CACHE: global_mem_cache_type = "READ_ONLY_CACHE"; break;
    case CL_READ_WRITE_CACHE: global_mem_cache_type = "READ_WRITE_CACHE"; break;
    default: global_mem_cache_type = "UNEXPECTED VALUE"; break;
  }
  cl_device_fp_config half_fp_config = device.getInfo<CL_DEVICE_HALF_FP_CONFIG>();
  std::string local_mem_type;
  switch(device.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>())
  {
    case CL_LOCAL: local_mem_type = "LOCAL"; break;
    case CL_GLOBAL: local_mem_type = "GLOBAL"; break;
    default: local_mem_type = "UNEXPECTED VALUE"; break;
  }
  std::string max_work_item_sizes;
  {
    std::vector<size_t> sizes = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    std::stringstream ss;
    std::copy(sizes.begin(), sizes.end(), std::ostream_iterator<size_t>(ss, " "));
    max_work_item_sizes = ss.str();
  }

  cl_device_fp_config single_fp_config = device.getInfo<CL_DEVICE_SINGLE_FP_CONFIG>();
  cl_command_queue_properties queue_properties = device.getInfo<CL_DEVICE_QUEUE_PROPERTIES>();


  os<<"DEVICE: "<<std::endl;
  os<<"  NAME:                                "<<device.getInfo<CL_DEVICE_NAME>()<<std::endl;
  os<<"  TYPE:                                "<<std::endl;
  os<<"     CPU:                              "<<BOOL_TO_STR(device_type | CL_DEVICE_TYPE_CPU)<<std::endl;
  os<<"     GPU:                              "<<BOOL_TO_STR(device_type | CL_DEVICE_TYPE_GPU)<<std::endl;
  os<<"     ACCELERATOR:                      "<<BOOL_TO_STR(device_type | CL_DEVICE_TYPE_ACCELERATOR)<<std::endl;
  os<<"     DEFAULT:                          "<<BOOL_TO_STR(device_type | CL_DEVICE_TYPE_DEFAULT)<<std::endl;
  os<<"  VENDOR:                              "<<device.getInfo<CL_DEVICE_VENDOR>()<<std::endl;
  os<<"  VENDOR_ID:                           "<<device.getInfo<CL_DEVICE_VENDOR_ID>()<<std::endl;
  os<<"  VERSION:                             "<<device.getInfo<CL_DEVICE_VERSION>()<<std::endl;
  os<<"  DRIVER_VERSION:                      "<<device.getInfo<CL_DRIVER_VERSION>()<<std::endl;
  os<<"  OPENCL_C_VERSION:                    "<<device.getInfo<CL_DEVICE_OPENCL_C_VERSION>()<<std::endl;
  os<<"  PROFILE:                             "<<device.getInfo<CL_DEVICE_PROFILE>()<<std::endl;
  os<<"  PLATFORM:                            "<<cl::Platform(device.getInfo<CL_DEVICE_PLATFORM>()).getInfo<CL_PLATFORM_NAME>()<<std::endl;
  os<<"  EXTENSIONS:   "<<std::endl;
  for (const auto& extension : extensions){
    os<<"     "<<extension<<std::endl;
  }
  os<<"  ------------------------------------------"<<std::endl;
  os<<"  ADDRESS_BITS:                        "<<device.getInfo<CL_DEVICE_ADDRESS_BITS>()<<std::endl;
  os<<"  AVAILABLE:                           "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_AVAILABLE>())<<std::endl;
  os<<"  COMPILER_AVAILABLE:                  "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_COMPILER_AVAILABLE>())<<std::endl;
  os<<"  ENDIAN_LITTLE:                       "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_ENDIAN_LITTLE>())<<std::endl;
  os<<"  ERROR_CORRECTION_SUPPORT:            "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_ERROR_CORRECTION_SUPPORT>())<<std::endl;
  os<<"  EXECUTION_CAPABILITIES:              "<<std::endl;
  os<<"     KERNEL:                           "<<BOOL_TO_STR(execution_capabilities | CL_EXEC_KERNEL)<<std::endl;
  os<<"     NATIVE_KERNEL:                    "<<BOOL_TO_STR(execution_capabilities | CL_EXEC_NATIVE_KERNEL)<<std::endl;
  os<<"  GLOBAL_MEM_CACHE_SIZE:               "<<device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>()<<std::endl;
  os<<"  GLOBAL_MEM_CACHE_TYPE:               "<<global_mem_cache_type<<std::endl;
  os<<"  GLOBAL_MEM_CACHELINE_SIZE:           "<<device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>()<<std::endl;
  os<<"  GLOBAL_MEM_SIZE:                     "<<device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()<<std::endl;
  os<<"  HOST_UNIFIED_MEMORY:                 "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_HOST_UNIFIED_MEMORY>())<<std::endl;
  os<<"  IMAGE_SUPPORT:                       "<<BOOL_TO_STR(device.getInfo<CL_DEVICE_IMAGE_SUPPORT>())<<std::endl;
  os<<"  IMAGE2D_MAX_HEIGHT:                  "<<device.getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>()<<std::endl;
  os<<"  IMAGE2D_MAX_WIDTH:                   "<<device.getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>()<<std::endl;
  os<<"  IMAGE3D_MAX_DEPTH:                   "<<device.getInfo<CL_DEVICE_IMAGE3D_MAX_DEPTH>()<<std::endl;
  os<<"  IMAGE3D_MAX_HEIGHT:                  "<<device.getInfo<CL_DEVICE_IMAGE3D_MAX_HEIGHT>()<<std::endl;
  os<<"  IMAGE3D_MAX_WIDTH:                   "<<device.getInfo<CL_DEVICE_IMAGE3D_MAX_WIDTH>()<<std::endl;
  os<<"  LOCAL_MEM_SIZE:                      "<<device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()<<std::endl;
  os<<"  LOCAL_MEM_TYPE:                      "<<local_mem_type<<std::endl;
  os<<"  MAX_CLOCK_FREQUENCY:                 "<<device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>()<<std::endl;
  os<<"  MAX_COMPUTE_UNITS:                   "<<device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()<<std::endl;
  os<<"  MAX_CONSTANT_ARGS:                   "<<device.getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>()<<std::endl;
  os<<"  MAX_CONSTANT_BUFFER_SIZE:            "<<device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>()<<std::endl;
  os<<"  MAX_MEM_ALLOC_SIZE:                  "<<device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()<<std::endl;
  os<<"  MAX_PARAMETER_SIZE:                  "<<device.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>()<<std::endl;
  os<<"  MAX_READ_IMAGE_ARGS:                 "<<device.getInfo<CL_DEVICE_MAX_READ_IMAGE_ARGS>()<<std::endl;
  os<<"  MAX_SAMPLERS:                        "<<device.getInfo<CL_DEVICE_MAX_SAMPLERS>()<<std::endl;
  os<<"  MAX_SAMPLERS:                        "<<device.getInfo<CL_DEVICE_MAX_SAMPLERS>()<<std::endl;
  os<<"  MAX_WORK_ITEM_DIMENSIONS:            "<<device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>()<<std::endl;
  os<<"  MAX_WORK_ITEM_SIZES:                 "<<max_work_item_sizes<<std::endl;
  os<<"  MAX_WRITE_IMAGE_ARGS:                "<<device.getInfo<CL_DEVICE_MAX_WRITE_IMAGE_ARGS>()<<std::endl;
  os<<"  MEM_BASE_ADDR_ALIGN:                 "<<device.getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>()<<std::endl;
  os<<"  MIN_DATA_TYPE_ALIGN_SIZE:            "<<device.getInfo<CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_CHAR:            "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_SHORT:           "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_INT:             "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_LONG:            "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_FLOAT:           "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_DOUBLE:          "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>()<<std::endl;
  os<<"  NATIVE_VECTOR_WIDTH_HALF:            "<<device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_CHAR:         "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_SHORT:        "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_INT:          "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_LONG:         "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_FLOAT:        "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_DOUBLE:       "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>()<<std::endl;
  os<<"  PREFFERED_VECTOR_WIDTH_HALF:         "<<device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF>()<<std::endl;
  os<<"  PROFILING_TIMER_RESOLUTION:          "<<device.getInfo<CL_DEVICE_PROFILING_TIMER_RESOLUTION>()<<std::endl;
  os<<"  QUEUE_PROPERTIES:                    "<<std::endl;
  os<<"     OUT_OF_ORDER_EXEC_MODE_ENABLE:    "<<BOOL_TO_STR(queue_properties | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)<<std::endl;
  os<<"     QUEUE_PROFILING_ENABLE:           "<<BOOL_TO_STR(queue_properties | CL_QUEUE_PROFILING_ENABLE)<<std::endl;
  os<<"  HALF_FP_CONFIG:                      "<<std::endl;
  os<<"     DENORM:                           "<<BOOL_TO_STR(half_fp_config | CL_FP_DENORM)<<std::endl;
  os<<"     INF_NAN:                          "<<BOOL_TO_STR(half_fp_config | CL_FP_INF_NAN)<<std::endl;
  os<<"     ROUND_TO_NEAREST:                 "<<BOOL_TO_STR(half_fp_config | CL_FP_ROUND_TO_NEAREST)<<std::endl;
  os<<"     ROUND_TO_ZERO:                    "<<BOOL_TO_STR(half_fp_config | CL_FP_ROUND_TO_ZERO)<<std::endl;
  os<<"     ROUND_TO_INF:                     "<<BOOL_TO_STR(half_fp_config | CL_FP_ROUND_TO_INF)<<std::endl;
  os<<"     FMA:                              "<<BOOL_TO_STR(half_fp_config | CL_FP_FMA)<<std::endl;
  os<<"     SOFT_FLOAT:                       "<<BOOL_TO_STR(half_fp_config | CL_FP_SOFT_FLOAT)<<std::endl;
  os<<"  SINGLE_FP_CONFIG:                    "<<std::endl;
  os<<"     DENORM:                           "<<BOOL_TO_STR(single_fp_config | CL_FP_DENORM)<<std::endl;
  os<<"     INF_NAN:                          "<<BOOL_TO_STR(single_fp_config | CL_FP_INF_NAN)<<std::endl;
  os<<"     ROUND_TO_NEAREST:                 "<<BOOL_TO_STR(single_fp_config | CL_FP_ROUND_TO_NEAREST)<<std::endl;
  os<<"     ROUND_TO_ZERO:                    "<<BOOL_TO_STR(single_fp_config | CL_FP_ROUND_TO_ZERO)<<std::endl;
  os<<"     ROUND_TO_INF:                     "<<BOOL_TO_STR(single_fp_config | CL_FP_ROUND_TO_INF)<<std::endl;
  os<<"     FMA:                              "<<BOOL_TO_STR(single_fp_config | CL_FP_FMA)<<std::endl;
  os<<"     SOFT_FLOAT:                       "<<BOOL_TO_STR(single_fp_config | CL_FP_SOFT_FLOAT)<<std::endl;
  os<<"  DOUBLE_FP_CONFIG:                    "<<std::endl;
  os<<"     DENORM:                           "<<BOOL_TO_STR(double_fp_config | CL_FP_DENORM)<<std::endl;
  os<<"     INF_NAN:                          "<<BOOL_TO_STR(double_fp_config | CL_FP_INF_NAN)<<std::endl;
  os<<"     ROUND_TO_NEAREST:                 "<<BOOL_TO_STR(double_fp_config | CL_FP_ROUND_TO_NEAREST)<<std::endl;
  os<<"     ROUND_TO_ZERO:                    "<<BOOL_TO_STR(double_fp_config | CL_FP_ROUND_TO_ZERO)<<std::endl;
  os<<"     ROUND_TO_INF:                     "<<BOOL_TO_STR(double_fp_config | CL_FP_ROUND_TO_INF)<<std::endl;
  os<<"     FMA:                              "<<BOOL_TO_STR(double_fp_config | CL_FP_FMA)<<std::endl;

  return os;
}

std::ostream& operator<<(std::ostream& os, const cl::ImageFormat& format)
{
  std::string order, type;
  switch (format.image_channel_order)
  {
    case CL_R:          order = "CL_R"; break;
    case CL_Rx:         order = "CL_Rx"; break;
    case CL_A:          order = "CL_A"; break;
    case CL_INTENSITY:  order = "CL_INTENSITY"; break;
    case CL_LUMINANCE:  order = "CL_LUMINANCE"; break;
    case CL_RG:         order = "CL_RG"; break;
    case CL_RGx:        order = "CL_RGx"; break;
    case CL_RA:         order = "CL_RA"; break;
    case CL_RGB:        order = "CL_RGB"; break;
    case CL_RGBx:       order = "CL_RGBx"; break;
    case CL_RGBA:       order = "CL_RGBA"; break;
    case CL_ARGB:       order = "CL_ARGB"; break;
    case CL_BGRA:       order = "CL_BGRA"; break;
    default:            order = "UNKNOWN"; break;
  };

  switch(format.image_channel_data_type)
  {
    case CL_SNORM_INT8:         type = "CL_SNORM_INT8"; break;
    case CL_SNORM_INT16:        type = "CL_SNORM_INT16"; break;
    case CL_UNORM_INT8:         type = "CL_UNORM_INT8"; break;
    case CL_UNORM_INT16:        type = "CL_UNORM_INT16"; break;
    case CL_UNORM_SHORT_565:    type = "CL_UNORM_SHORT_565"; break;
    case CL_UNORM_SHORT_555:    type = "CL_UNORM_SHORT_555"; break;
    case CL_UNORM_INT_101010:   type = "CL_UNORM_INT_101010"; break;
    case CL_SIGNED_INT8:        type = "CL_SIGNED_INT8"; break;
    case CL_SIGNED_INT16:       type = "CL_SIGNED_INT16"; break;
    case CL_SIGNED_INT32:       type = "CL_SIGNED_INT32"; break;
    case CL_UNSIGNED_INT8:      type = "CL_UNSIGNED_INT8"; break;
    case CL_UNSIGNED_INT16:     type = "CL_UNSIGNED_INT16"; break;
    case CL_UNSIGNED_INT32:     type = "CL_UNSIGNED_INT32"; break;
    case CL_HALF_FLOAT:         type = "CL_HALF_FLOAT"; break;
    case CL_FLOAT:              type = "CL_FLOAT"; break;
    default:                    type = "UNKNOWN"; break;
  }

  os<<"["<<order<<", "<<type<<"]";
  return os;
}