#include "Evaluator.h"

#include "Util.h"

#include "Serialization.h"

#include <CL/cl.hpp>
#include <CL/cl_ext.h>
#include <CL/cl_ext_qcom.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "IONMemory.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
#include <chrono>
#include <tuple>
#include <functional>

namespace Anki
{

Evaluator::Evaluator(const Config& config)
  : _config(config)
{

}

Evaluator::~Evaluator()
{
}

void Evaluator::run()
{
  try {
    init();
  } catch (cl::Error& e){
    std::cerr<<"Init error: "<<e.what()<<"("<<e.err()<<"): "<<cl_error_to_string(e.err())<<std::endl;
    throw e;
  }

  std::vector<std::tuple<std::string, std::function<void()> > > tests = {
    std::make_tuple(_config.gpu.debayer.name,             std::bind(&Evaluator::gpu_debayer, this)),
    std::make_tuple(_config.gpu.debayer_half.name,        std::bind(&Evaluator::gpu_debayer_half, this)),
    std::make_tuple(_config.gpu.debayer_img.name,         std::bind(&Evaluator::gpu_debayer_img, this)),
    std::make_tuple(_config.gpu.debayer_img_half.name,    std::bind(&Evaluator::gpu_debayer_img_half, this)),
    std::make_tuple(_config.gpu.debayer_zcp.name,         std::bind(&Evaluator::gpu_debayer_zcp, this)),
    std::make_tuple(_config.gpu.debayer_zcp_half.name,    std::bind(&Evaluator::gpu_debayer_zcp_half, this)),
    std::make_tuple(_config.gpu.squash_zcp.name,          std::bind(&Evaluator::gpu_squash_zcp, this)),
    // std::make_tuple(_config.gpu.resize.name,              std::bind(&Evaluator::gpu_resize, this)),
    // std::make_tuple(_config.gpu.rgb2yuv.name,             std::bind(&Evaluator::gpu_rgb2yuv, this)),
    std::make_tuple(_config.neon.debayer.name,            std::bind(&Evaluator::neon_debayer, this)),
    std::make_tuple(_config.neon.debayer_half.name,       std::bind(&Evaluator::neon_debayer_half, this)),
    std::make_tuple(_config.pipeline.name,                std::bind(&Evaluator::pipeline, this)),
    std::make_tuple(_config.pipeline_kernel.name,         std::bind(&Evaluator::pipeline_kernel, this)),
    std::make_tuple(_config.power_neon.name,              std::bind(&Evaluator::power_neon, this)),
    std::make_tuple(_config.power_zcp.name,               std::bind(&Evaluator::power_zcp, this))
  };

  for (auto& test : tests) {
    try {
      std::string name;
      std::function<void()> func;
      std::tie(name, func) = test;
      std::cerr<<"Running: "<<name<<std::endl;
      func();
    } catch (cl::Error& e){
      std::cerr<<"Test error: "<<e.what()<<"("<<e.err()<<"): "<<cl_error_to_string(e.err())<<std::endl;
    }
  }

  try {
    deinit();
  } catch (cl::Error& e){
    std::cerr<<"Deinit error: "<<e.what()<<"("<<e.err()<<"): "<<cl_error_to_string(e.err())<<std::endl;
    throw e;
  }

}

void Evaluator::init()
{
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  if (platforms.empty()){
    throw EvaluatorException("Unexpected: no platforms");
  } else if (platforms.size() > 1){
    throw EvaluatorException("Unexpected: more than one platform");
  }
  cl::Platform platform = platforms[0];

  std::vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  if (devices.empty()){
    throw EvaluatorException("Unexpected: no devices");
  } else if (devices.size() > 1) {
    throw EvaluatorException("Unexpected: more than one device");
  }
  cl::Device device = devices[0];

  cl::Context context({device});

  cl::Program program;
  {
    std::vector<std::string> paths = _config.gpu.programs;
    std::vector<std::string> texts;
    for (const std::string& path : paths)
    {
      std::ifstream ifs(path);
      std::stringstream ss;
      ss << ifs.rdbuf();
      texts.push_back(ss.str());
    }

    cl::Program::Sources sources;
    for (const std::string& text : texts){
      sources.push_back({ text.c_str(), text.size() });
    }
    program = cl::Program(context, sources);
  }

  try {
    program.build({device});
  } catch (cl::Error& e){
    std::stringstream ss;
    ss << "Failed to build CL program: "<<std::endl
      << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)
      <<std::endl;
    std::string message = ss.str();
    throw EvaluatorException(message);
  }

  // Qualcomm Extensions
  clSetPerfHintQCOM(_context(), CL_PERF_HINT_LOW_QCOM);

  _platform = platform;
  _device = device;
  _context = context;
  _program = program;
  _queue = cl::CommandQueue(_context, _device, CL_QUEUE_PROFILING_ENABLE);
}

void Evaluator::deinit()
{

}

void Evaluator::print()
{
#if 0
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
  std::cerr<<_platform<<std::endl;
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
  std::cerr<<_context<<std::endl;
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
  std::cerr<<_device<<std::endl;
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
  std::cerr<<_program<<std::endl;
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
#endif
#if 0
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
  std::vector<cl::ImageFormat> formats;
  _context.getSupportedImageFormats(
    CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY | CL_MEM_READ_WRITE,
    CL_MEM_OBJECT_IMAGE2D,
    &formats
  );
  std::cerr<<"Formats: "<<std::endl;
  for (const auto& format : formats){
    std::cerr<<"    "<<format<<std::endl;
  }
  std::cerr<<"--------------------------------------------------------------------------------"<<std::endl;
#endif
  for (auto& profile : _profiles){
    std::cerr<<profile.to_string()<<std::endl;
  }


}

//======================================================================================================================
void Evaluator::gpu_debayer()
{
  if (!_config.gpu.debayer.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer.name);
  profile.append("WriteBuffer_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer.kernel.name);
  profile.append("ReadBuffer_output");

  for (int repetition = 0; repetition < _config.gpu.debayer.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());

      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer.output.width * _config.gpu.debayer.output.height);

      size_t in_size = sizeof(unsigned char) * _config.gpu.debayer.input.width * _config.gpu.debayer.input.height;
      size_t out_size = 3 * sizeof(unsigned char) * _config.gpu.debayer.output.width * _config.gpu.debayer.output.height;

      cl::Buffer in_buffer = cl::Buffer(_context, CL_MEM_READ_ONLY, in_size);
      cl::Buffer out_buffer = cl::Buffer(_context, CL_MEM_WRITE_ONLY, out_size);

      _queue.enqueueWriteBuffer(in_buffer, CL_TRUE, 0, in_size, (void*)in_bytes.data(),
        NULL, &events[0]);

      cl::Kernel kernel(_program, _config.gpu.debayer.kernel.name.c_str());
      kernel.setArg(0, in_buffer);
      kernel.setArg(1, out_buffer);
      kernel.setArg(2, _config.gpu.debayer.input.width);
      kernel.setArg(3, _config.gpu.debayer.output.width);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer.kernel.width, _config.gpu.debayer.kernel.height),
        cl::NullRange,
        NULL, &events[1]);

      _queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, out_size, (void*)out_bytes.data(),
        NULL, &events[2]);

      if (_config.gpu.debayer.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      _queue.finish();

      // Add profiling information
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii
    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::gpu_debayer_half()
{
  if (!_config.gpu.debayer_half.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer_half.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer_half.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer_half.name);
  profile.append("WriteBuffer_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer_half.kernel.name);
  profile.append("ReadBuffer_output");

  for (int repetition = 0; repetition < _config.gpu.debayer_half.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());

      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer_half.output.width * _config.gpu.debayer_half.output.height);

      size_t in_size = sizeof(unsigned char) * _config.gpu.debayer_half.input.width * _config.gpu.debayer_half.input.height;
      size_t out_size = 3 * sizeof(unsigned char) * _config.gpu.debayer_half.output.width * _config.gpu.debayer_half.output.height;

      cl::Buffer in_buffer = cl::Buffer(_context, CL_MEM_READ_ONLY, in_size);
      cl::Buffer out_buffer = cl::Buffer(_context, CL_MEM_WRITE_ONLY, out_size);

      _queue.enqueueWriteBuffer(in_buffer, CL_TRUE, 0, in_size, (void*)in_bytes.data(),
        NULL, &events[0]);


      cl::Kernel kernel(_program, _config.gpu.debayer_half.kernel.name.c_str());
      kernel.setArg(0, in_buffer);
      kernel.setArg(1, out_buffer);
      kernel.setArg(2, _config.gpu.debayer_half.input.width);
      kernel.setArg(3, _config.gpu.debayer_half.output.width);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer_half.kernel.width, _config.gpu.debayer_half.kernel.height),
        cl::NullRange,
        NULL, &events[1]);

      _queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, out_size, (void*)out_bytes.data(),
        NULL, &events[2]);

      if (_config.gpu.debayer_half.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      _queue.finish();

      // Add profiling information
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii
    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::gpu_debayer_img()
{
  if (!_config.gpu.debayer_img.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer_img.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer_img.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer_img.name);
  profile.append("WriteImage_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer_img.kernel.name);
  profile.append("ReadImage_output");

  for (int repetition = 0; repetition < _config.gpu.debayer_img.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());

      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer_img.output.width * _config.gpu.debayer_img.output.height);

      size_t in_size = sizeof(unsigned char) * _config.gpu.debayer_img.input.width * _config.gpu.debayer_img.input.height;
      size_t out_size = 3 * sizeof(unsigned char) * _config.gpu.debayer_img.output.width * _config.gpu.debayer_img.output.height;

      cl::Image2D in_image2D = cl::Image2D(_context,  CL_MEM_READ_ONLY, cl::ImageFormat(CL_R, CL_UNORM_INT8),
        _config.gpu.debayer_img.input.width, _config.gpu.debayer_img.input.height, 0);
      cl::Image2D out_image2D = cl::Image2D(_context,  CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGB, CL_UNORM_INT8),
        _config.gpu.debayer_img.output.width, _config.gpu.debayer_img.output.height, 0);

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.gpu.debayer_img.input.width;
      in_region[1] = _config.gpu.debayer_img.input.height;
      in_region[2] = 1;
      _queue.enqueueWriteImage(in_image2D, CL_TRUE, in_origin, in_region, 0, 0, (void*)in_bytes.data(), NULL, &events[0]);


      cl::Kernel kernel(_program, _config.gpu.debayer_img.kernel.name.c_str());
      kernel.setArg(0, in_image2D);
      kernel.setArg(1, out_image2D);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer_img.kernel.width, _config.gpu.debayer_img.kernel.height),
        cl::NullRange,
        NULL, &events[1]);

      cl::size_t<3> out_origin;
      out_origin[0] = 0;
      out_origin[1] = 0;
      out_origin[2] = 0;
      cl::size_t<3> out_region;
      out_region[0] = _config.gpu.debayer_img.output.width;
      out_region[1] = _config.gpu.debayer_img.output.height;
      out_region[2] = 1;
      _queue.enqueueReadImage(out_image2D, CL_TRUE, out_origin, out_region, 0, 0, (void*)out_bytes.data(), NULL, &events[2]);

      if (_config.gpu.debayer_img.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      _queue.finish();

      // Add profiling information
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii
    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::gpu_debayer_img_half()
{
  if (!_config.gpu.debayer_img_half.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer_img_half.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer_img_half.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer_img_half.name);
  profile.append("WriteImage_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer_img_half.kernel.name);
  profile.append("ReadImage_output");

  for (int repetition = 0; repetition < _config.gpu.debayer_img_half.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());

      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer_img_half.output.width * _config.gpu.debayer_img_half.output.height);

      size_t in_size = sizeof(unsigned char) * _config.gpu.debayer_img_half.input.width * _config.gpu.debayer_img_half.input.height;
      size_t out_size = 3 * sizeof(unsigned char) * _config.gpu.debayer_img_half.output.width * _config.gpu.debayer_img_half.output.height;

      cl::Image2D in_image2D = cl::Image2D(_context,  CL_MEM_READ_ONLY, cl::ImageFormat(CL_R, CL_UNORM_INT8),
        _config.gpu.debayer_img_half.input.width, _config.gpu.debayer_img_half.input.height, 0);
      cl::Image2D out_image2D = cl::Image2D(_context,  CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGB, CL_UNORM_INT8),
        _config.gpu.debayer_img_half.output.width, _config.gpu.debayer_img_half.output.height, 0);

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.gpu.debayer_img_half.input.width;
      in_region[1] = _config.gpu.debayer_img_half.input.height;
      in_region[2] = 1;
      _queue.enqueueWriteImage(in_image2D, CL_TRUE, in_origin, in_region, 0, 0, (void*)in_bytes.data(), NULL, &events[0]);


      cl::Kernel kernel(_program, _config.gpu.debayer_img_half.kernel.name.c_str());
      kernel.setArg(0, in_image2D);
      kernel.setArg(1, out_image2D);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer_img_half.kernel.width, _config.gpu.debayer_img_half.kernel.height),
        cl::NullRange,
        NULL, &events[1]);

      cl::size_t<3> out_origin;
      out_origin[0] = 0;
      out_origin[1] = 0;
      out_origin[2] = 0;
      cl::size_t<3> out_region;
      out_region[0] = _config.gpu.debayer_img_half.output.width;
      out_region[1] = _config.gpu.debayer_img_half.output.height;
      out_region[2] = 1;
      _queue.enqueueReadImage(out_image2D, CL_TRUE, out_origin, out_region, 0, 0, (void*)out_bytes.data(), NULL, &events[2]);

      if (_config.gpu.debayer_img_half.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      _queue.finish();

      // Add profiling information
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii
    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::zcp_allocate(ZCPImage& image, size_t width, size_t height, cl::ImageFormat format)
{
    size_t              imgw                      = width;
    size_t              imgh                      = height;
    cl_image_format     img_fmt                   = format;
    cl_mem              image_object              = NULL;
    size_t              ext_mem_padding_in_bytes  = 0;
    size_t              device_page_size          = 0;
    size_t              image_row_pitch           = 0;
    cl_int              errcode                   = 0;

    memset(&image.ocl_ion_mem, 0, sizeof(cl_mem_ion_host_ptr));

    // Query the device's page size and the amount of padding necessary at
    // the end of the buffer.
    clGetDeviceInfo(_device(), CL_DEVICE_PAGE_SIZE_QCOM,
      sizeof(device_page_size), &device_page_size, NULL);
    clGetDeviceInfo(_device(), CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM,
      sizeof(ext_mem_padding_in_bytes), &ext_mem_padding_in_bytes, NULL);

    // Query the device supported row and slice pitch using
    // clGetDeviceImageInfoQCOM
    // imgw - image width
    // imgh - image height
    // img_fmt - image format
    clGetDeviceImageInfoQCOM(_device(), imgw, imgh, &img_fmt,
      CL_IMAGE_ROW_PITCH, sizeof(image_row_pitch), &image_row_pitch,
      NULL);

    // Use the image height, row pitch obtained above and element size to
    // compute the size of the buffer
    size_t buffer_size_in_bytes = imgh * image_row_pitch;

    // Compute amount of memory that needs to be allocated for the buffer
    // including padding.
    size_t buffer_size_with_padding = buffer_size_in_bytes + ext_mem_padding_in_bytes;

    // Make an ION memory allocation of size buffer_size_with_padding here.
    // Note that allocating buffer_size_in_bytes instead would be a mistake.
    // It's important to allocate the extra padding. Let's say the
    // parameters of the allocation are stored in a struct named ion_info
    // that we will use below.
    image.ion_buffer.resize(buffer_size_with_padding);

    // Create an OpenCL image object that uses ion_info as its data store.
    image.ocl_ion_mem.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
    // NOTE: Use in conjunction with correct ion flags, @see IONMemory::allocate 
    // image.ocl_ion_mem.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
    image.ocl_ion_mem.ext_host_ptr.host_cache_policy = CL_MEM_HOST_WRITEBACK_QCOM;
    // file descriptor for ION
    image.ocl_ion_mem.ion_filedesc = image.ion_buffer.fd();
    // hostptr returned by ION which is device page size aligned
    image.ocl_ion_mem.ion_hostptr = image.ion_buffer.data();

    if( ((unsigned long)image.ocl_ion_mem.ion_hostptr) % device_page_size)
    {
      throw EvaluatorException("Host pointer must be aligned to device_page_size!");
    }

    switch (format.image_channel_order)
    {
      case CL_R:
        image.ocl_n_channels  = 1;
        break;
      case CL_RGB:
        image.ocl_n_channels  = 3;
        break;
      default:
        throw EvaluatorException("Unsupported cl::Image format");
    }

#if 0
    std::cerr<<"-----------------------------------"<<std::endl;
    std::cerr<<"Allocate image"<<std::endl;
    std::cerr<<"imgh:                       "<<imgh<<std::endl;
    std::cerr<<"imgw:                       "<<imgw<<std::endl;
    std::cerr<<"n_channels:                 "<<image.ocl_n_channels<<std::endl;
    std::cerr<<"image_row_pitch:            "<<image_row_pitch<<std::endl;
    std::cerr<<"device_page_size:           "<<device_page_size<<std::endl;
    std::cerr<<"ext_mem_padding_in_bytes:   "<<ext_mem_padding_in_bytes<<std::endl;
    std::cerr<<"buffer_size_in_bytes:       "<<buffer_size_in_bytes<<std::endl;
    std::cerr<<"buffer_size_with_padding:   "<<buffer_size_with_padding<<std::endl;
    std::cerr<<"ion_buffer.size:            "<<image.ion_buffer.size()<<std::endl;
#endif

    image.ocl_image = cl::Image2D(_context,
      CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
      format,
      imgw,
      imgh,
      image_row_pitch,
      &image.ocl_ion_mem);
}

void Evaluator::zcp_copy(const std::vector<unsigned char>& bytes, ZCPImage& image)
{
  size_t height = image.ocl_image.getImageInfo<CL_IMAGE_HEIGHT>();
  size_t width = image.ocl_image.getImageInfo<CL_IMAGE_WIDTH>();
  size_t element_size = image.ocl_image.getImageInfo<CL_IMAGE_ELEMENT_SIZE>();
  size_t n_channels = image.ocl_n_channels;
  size_t channel_size = element_size / n_channels;
  size_t img_pitch = image.ocl_image.getImageInfo<CL_IMAGE_ROW_PITCH>();
  size_t pitch = image.ocl_row_pitch;
  unsigned char* data = static_cast<unsigned char*>(image.ocl_mapping);
  size_t count = width * element_size;

  // Element size = number of bytes per pixel
  // Channels = number of channels for pixel
  // e.g. RGB has element size of 3 bytes and 3 channels

#if 0
  std::cerr<<"--------------------------"<<std::endl;
  std::cerr<<"Copy vector to image"<<std::endl;
  std::cerr<<"Height:         "<<height<<std::endl;
  std::cerr<<"Width:          "<<width<<std::endl;
  std::cerr<<"Element Size:   "<<element_size<<std::endl;
  std::cerr<<"Channels:       "<<n_channels<<std::endl;
  std::cerr<<"Channel Size:   "<<channel_size<<std::endl;
  std::cerr<<"Pitch:          "<<pitch<<std::endl;
  std::cerr<<"Img Pitch:      "<<img_pitch<<std::endl;
  std::cerr<<"Bytes:          "<<bytes.size()<<std::endl;
  std::cerr<<"Count:          "<<count<<std::endl;
#endif

  for (int row = 0; row < height; ++row)
  {
    size_t index_data = row * pitch;
    size_t index_bytes = row * count;
    std::memcpy((void*)(data+index_data), (void*)(&bytes[index_bytes]), count);
  }

}

void Evaluator::zcp_copy(const ZCPImage& image, std::vector<unsigned char>& bytes)
{
  size_t height = image.ocl_image.getImageInfo<CL_IMAGE_HEIGHT>();
  size_t width = image.ocl_image.getImageInfo<CL_IMAGE_WIDTH>();
  size_t element_size = image.ocl_image.getImageInfo<CL_IMAGE_ELEMENT_SIZE>();
  size_t n_channels = image.ocl_n_channels;
  size_t channel_size = element_size / n_channels;
  size_t img_pitch = image.ocl_image.getImageInfo<CL_IMAGE_ROW_PITCH>();
  size_t pitch = image.ocl_row_pitch;
  const unsigned char* data = static_cast<const unsigned char*>(image.ocl_mapping);
  size_t count = width * element_size;

  // Element size = number of bytes per pixel
  // Channels = number of channels for pixel
  // e.g. RGB has element size of 3 bytes and 3 channels, so one byte per channel

#if 0
  std::cerr<<"--------------------------"<<std::endl;
  std::cerr<<"Copy image to vector"<<std::endl;
  std::cerr<<"Height:         "<<height<<std::endl;
  std::cerr<<"Width:          "<<width<<std::endl;
  std::cerr<<"Channels:       "<<n_channels<<std::endl;
  std::cerr<<"Element Size:   "<<element_size<<std::endl;
  std::cerr<<"Channel Size:   "<<channel_size<<std::endl;
  std::cerr<<"Pitch:          "<<pitch<<std::endl;
  std::cerr<<"Img Pitch:      "<<img_pitch<<std::endl;
  std::cerr<<"Bytes:          "<<bytes.size()<<std::endl;
  std::cerr<<"Count:          "<<count<<std::endl;
#endif

  for (int row = 0; row < height; ++row)
  {
    size_t index_data = row * pitch;
    size_t index_bytes = row * count;
    std::memcpy((void*)(&bytes[index_bytes]), (void*)(data+index_data), count);
  }
}

void Evaluator::gpu_debayer_zcp()
{
  if (!_config.gpu.debayer_zcp.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer_zcp.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer_zcp.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer_zcp.name);
  profile.append("MapImage_input");
  profile.append("UnmapImage_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer_zcp.kernel.name);
  profile.append("MapImage_output");
  profile.append("UnmapImage_output");

  for (int repetition = 0; repetition < _config.gpu.debayer_zcp.repetitions; ++repetition)
  {
    
    // Note, mapped images don't need to be copied over. They're always accessible by both CPU and GPU, however
    // care must be taken to not use the memory simultaneously.

    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer_zcp.output.width * _config.gpu.debayer_zcp.output.height);

      ZCPImage in_image2D;
      zcp_allocate(in_image2D,
        _config.gpu.debayer_zcp.input.width,
        _config.gpu.debayer_zcp.input.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));
      ZCPImage out_image2D;
      zcp_allocate(out_image2D,
        _config.gpu.debayer_zcp.output.width,
        _config.gpu.debayer_zcp.output.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.gpu.debayer_zcp.input.width;
      in_region[1] = _config.gpu.debayer_zcp.input.height;
      in_region[2] = 1;

      in_image2D.ocl_mapping = _queue.enqueueMapImage(in_image2D.ocl_image, CL_TRUE, CL_MAP_WRITE,
        in_origin, in_region, &in_image2D.ocl_row_pitch, NULL, NULL, &events[0], NULL);

      zcp_copy(in_bytes, in_image2D);

      _queue.enqueueUnmapMemObject(in_image2D.ocl_image, in_image2D.ocl_mapping, NULL, &events[1]);

      cl::Kernel kernel(_program, _config.gpu.debayer_zcp.kernel.name.c_str());
      kernel.setArg(0, in_image2D.ocl_image);
      kernel.setArg(1, out_image2D.ocl_image);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer_zcp.kernel.width, _config.gpu.debayer_zcp.kernel.height),
        cl::NullRange,
        NULL, &events[2]);

      _queue.finish();

      cl::size_t<3> out_origin;
      out_origin[0] = 0;
      out_origin[1] = 0;
      out_origin[2] = 0;
      cl::size_t<3> out_region;
      out_region[0] = _config.gpu.debayer_zcp.output.width;
      out_region[1] = _config.gpu.debayer_zcp.output.height;
      out_region[2] = 1;

      out_image2D.ocl_mapping = _queue.enqueueMapImage(out_image2D.ocl_image, CL_TRUE, CL_MAP_READ,
        out_origin, out_region, &out_image2D.ocl_row_pitch, NULL, NULL, &events[3], NULL);

      zcp_copy(out_image2D, out_bytes);

      _queue.enqueueUnmapMemObject(out_image2D.ocl_image, out_image2D.ocl_mapping, NULL, &events[4]);

      _queue.finish();

      if (_config.gpu.debayer_zcp.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      // Add profiling information for the inner loop
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii

    } // for-loop: path_index

  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::gpu_debayer_zcp_half()
{
  if (!_config.gpu.debayer_zcp_half.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.debayer_zcp_half.input.paths;
  std::vector<std::string> out_paths = _config.gpu.debayer_zcp_half.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.debayer_zcp_half.name);
  profile.append("MapImage_input");
  profile.append("UnmapImage_input");
  profile.append("NDRangeKernel_" + _config.gpu.debayer_zcp_half.kernel.name);
  profile.append("MapImage_output");
  profile.append("UnmapImage_output");

  for (int repetition = 0; repetition < _config.gpu.debayer_zcp_half.repetitions; ++repetition)
  {
    
    // Note, mapped images don't need to be copied over. They're always accessible by both CPU and GPU, however
    // care must be taken to not use the memory simultaneously.

    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.gpu.debayer_zcp_half.output.width * _config.gpu.debayer_zcp_half.output.height);

      ZCPImage in_image2D;
      zcp_allocate(in_image2D,
        _config.gpu.debayer_zcp_half.input.width,
        _config.gpu.debayer_zcp_half.input.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));
      ZCPImage out_image2D;
      zcp_allocate(out_image2D,
        _config.gpu.debayer_zcp_half.output.width,
        _config.gpu.debayer_zcp_half.output.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.gpu.debayer_zcp_half.input.width;
      in_region[1] = _config.gpu.debayer_zcp_half.input.height;
      in_region[2] = 1;

      in_image2D.ocl_mapping = _queue.enqueueMapImage(in_image2D.ocl_image, CL_TRUE, CL_MAP_WRITE,
        in_origin, in_region, &in_image2D.ocl_row_pitch, NULL, NULL, &events[0], NULL);

      zcp_copy(in_bytes, in_image2D);

      _queue.enqueueUnmapMemObject(in_image2D.ocl_image, in_image2D.ocl_mapping, NULL, &events[1]);

      cl::Kernel kernel(_program, _config.gpu.debayer_zcp_half.kernel.name.c_str());
      kernel.setArg(0, in_image2D.ocl_image);
      kernel.setArg(1, out_image2D.ocl_image);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.debayer_zcp_half.kernel.width, _config.gpu.debayer_zcp_half.kernel.height),
        cl::NullRange,
        NULL, &events[2]);

      _queue.finish();


      cl::size_t<3> out_origin;
      out_origin[0] = 0;
      out_origin[1] = 0;
      out_origin[2] = 0;
      cl::size_t<3> out_region;
      out_region[0] = _config.gpu.debayer_zcp_half.output.width;
      out_region[1] = _config.gpu.debayer_zcp_half.output.height;
      out_region[2] = 1;

      out_image2D.ocl_mapping = _queue.enqueueMapImage(out_image2D.ocl_image, CL_TRUE, CL_MAP_READ,
        out_origin, out_region, &out_image2D.ocl_row_pitch, NULL, NULL, &events[3], NULL);

      zcp_copy(out_image2D, out_bytes);

      _queue.enqueueUnmapMemObject(out_image2D.ocl_image, out_image2D.ocl_mapping, NULL, &events[4]);

      _queue.finish();

      if (_config.gpu.debayer_zcp_half.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      // Add profiling information for the inner loop
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii

    } // for-loop: path_index

  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::gpu_squash_zcp()
{
  if (!_config.gpu.squash_zcp.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to BGGR8 image

  // Get the image paths
  std::vector<std::string> in_paths = _config.gpu.squash_zcp.input.paths;
  std::vector<std::string> out_paths = _config.gpu.squash_zcp.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.gpu.squash_zcp.name);
  profile.append("MapImage_input");
  profile.append("UnmapImage_input");
  profile.append("NDRangeKernel_" + _config.gpu.squash_zcp.kernel.name);
  profile.append("MapImage_output");
  profile.append("UnmapImage_output");

  for (int repetition = 0; repetition < _config.gpu.squash_zcp.repetitions; ++repetition)
  {
    // Note, mapped images don't need to be copied over. They're always accessible by both CPU and GPU, however
    // care must be taken to not use the memory simultaneously.

    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(_config.gpu.squash_zcp.output.width * _config.gpu.squash_zcp.output.height);

      // Note: In practice you would not allocate and map the image every time for each image; you'd allocate once
      // and map once for the entirety of the program (or as long as you need it). This should be done outside any
      // of the for loops. But for profiling and simplicity based on the implementing the other algorithms first, 
      // I've done it in the innermost loop.
      ZCPImage in_image2D;
      zcp_allocate(in_image2D,
        _config.gpu.squash_zcp.input.width,
        _config.gpu.squash_zcp.input.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));
      ZCPImage out_image2D;
      zcp_allocate(out_image2D,
        _config.gpu.squash_zcp.output.width,
        _config.gpu.squash_zcp.output.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.gpu.squash_zcp.input.width;
      in_region[1] = _config.gpu.squash_zcp.input.height;
      in_region[2] = 1;

      in_image2D.ocl_mapping = _queue.enqueueMapImage(in_image2D.ocl_image, CL_TRUE, CL_MAP_WRITE,
        in_origin, in_region, &in_image2D.ocl_row_pitch, NULL, NULL, &events[0], NULL);

      _queue.finish();

      zcp_copy(in_bytes, in_image2D);

      _queue.enqueueUnmapMemObject(in_image2D.ocl_image, in_image2D.ocl_mapping, NULL, &events[1]);
      in_image2D.ocl_mapping = nullptr;

      _queue.finish();

      cl::Kernel kernel(_program, _config.gpu.squash_zcp.kernel.name.c_str());
      kernel.setArg(0, in_image2D.ocl_image);
      kernel.setArg(1, out_image2D.ocl_image);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.gpu.squash_zcp.kernel.width, _config.gpu.squash_zcp.kernel.height),
        cl::NullRange,
        NULL, &events[2]);

      _queue.finish();

      cl::size_t<3> out_origin;
      out_origin[0] = 0;
      out_origin[1] = 0;
      out_origin[2] = 0;
      cl::size_t<3> out_region;
      out_region[0] = _config.gpu.squash_zcp.output.width;
      out_region[1] = _config.gpu.squash_zcp.output.height;
      out_region[2] = 1;

      out_image2D.ocl_mapping = _queue.enqueueMapImage(out_image2D.ocl_image, CL_TRUE, CL_MAP_READ,
        out_origin, out_region, &out_image2D.ocl_row_pitch, NULL, NULL, &events[3], NULL);

      _queue.finish();

      zcp_copy(out_image2D, out_bytes);

      _queue.enqueueUnmapMemObject(out_image2D.ocl_image, out_image2D.ocl_mapping, NULL, &events[4]);
      out_image2D.ocl_mapping = nullptr;

      _queue.finish();

      if (_config.gpu.squash_zcp.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

      // Add profiling information for the inner loop
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii

    } // for-loop: path_index

  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::neon_debayer()
{
  if (!_config.neon.debayer.enable){
    return;
  }
#ifdef __ARM_NEON__
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.neon.debayer.input.paths;
  std::vector<std::string> out_paths = _config.neon.debayer.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.neon.debayer.name);
  profile.append("debayer");

  for (int repetition = 0; repetition < _config.neon.debayer.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.neon.debayer.output.width * _config.neon.debayer.output.height);

      size_t out_size = 3 * sizeof(unsigned char) * _config.neon.debayer.output.width * _config.neon.debayer.output.height;


      const uint8_t* bayer_in = in_bytes.data();
      int32_t rows = _config.neon.debayer.output.height;
      int32_t cols = _config.neon.debayer.output.width;
      cv::Mat_<cv::Vec<uint8_t,3>> rgb(rows,cols);

      // Code
      auto tStart = std::chrono::steady_clock::now();
      cv::Mat bayer(rows, cols, CV_8UC1);

      const uint8_t* bufferPtr = bayer_in;
      uint8_t* bayerPtr = static_cast<uint8_t*>(bayer.ptr());
      for(int ii = 0; ii < (cols*rows); ii+=8)
      {
        uint8x8_t data = vld1_u8(bufferPtr);
        bufferPtr += 5;
        uint8x8_t data2 = vld1_u8(bufferPtr);
        bufferPtr += 5;
        data = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(data), 32));
        data = vext_u8(data, data2, 4);
        data = vqshl_n_u8(data, 2);
        vst1_u8(bayerPtr, data);
        bayerPtr += 8;
      }
      cv::cvtColor(bayer, rgb, cv::COLOR_BayerRG2BGR);


      auto tEnd = std::chrono::steady_clock::now();
      uint64_t elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tStart).count();
      profile.add(0, elapsed);

      std::memcpy((void*)(out_bytes.data()), rgb.data, out_size);

      if (_config.neon.debayer.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }

    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
#endif
}

//======================================================================================================================
void Evaluator::neon_debayer_half()
{
  if (!_config.neon.debayer_half.enable){
    return;
  }
#ifdef __ARM_NEON__
  // Covert a RAW10 BGGR10 format image to RGB888

  // Get the image paths
  std::vector<std::string> in_paths = _config.neon.debayer_half.input.paths;
  std::vector<std::string> out_paths = _config.neon.debayer_half.output.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.neon.debayer_half.name);
  profile.append("debayer_half");

  for (int repetition = 0; repetition < _config.neon.debayer_half.repetitions; ++repetition)
  {
    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
      std::vector<unsigned char> out_bytes(3 * _config.neon.debayer_half.output.width * _config.neon.debayer_half.output.height);

      size_t out_size = 3 * sizeof(unsigned char) * _config.neon.debayer_half.output.width * _config.neon.debayer_half.output.height;

      // Setup to match arguments
      const uint8_t* bayer_in = in_bytes.data();
      int32_t rows = _config.neon.debayer_half.input.height;
      int32_t cols = _config.neon.debayer_half.input.width;
      uint8_t* rgb = out_bytes.data();
      int32_t bayer_sx = (cols*10)/8;
      int32_t bayer_sy = rows;
      int bpp = 10;

      auto tStart = std::chrono::steady_clock::now();
      // Code
      {
        // input width must be divisble by bpp
        assert((bayer_sx % bpp) == 0);

        // Each iteration of the inline assembly processes 20 bytes at a time
        const uint8_t  kNumBytesProcessedPerLoop = 20;
        assert((bayer_sx % kNumBytesProcessedPerLoop) == 0);

        const uint32_t kNumInnerLoops = bayer_sx / kNumBytesProcessedPerLoop;
        const uint32_t kNumOuterLoops = bayer_sy >> 1;

        uint8_t* bayer = (uint8_t*)bayer_in;
        uint8_t* bayer2 = bayer + bayer_sx;

        uint32_t i, j;
        for (i = 0; i < kNumOuterLoops; ++i) {
          for (j = 0; j < kNumInnerLoops; ++j) {
            __asm__ volatile
            (
              "VLD1.8 {d0}, [%[ptr]] \n\t"  // Load 8 bytes from raw bayer image
              "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer by 5 (5 byte packing)
              "VLD1.8 {d1}, [%[ptr]] \n\t"  // Load 8 more bytes
              "VSHL.I64 d0, d0, #32 \n\t"   // Shift out the partial packed bytes from d0
              "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer again, after ld and shl so they can be dual issued
              "VEXT.8 d0, d0, d1, #4 \n\t"  // Extract first 4 bytes from d0 and following 4 bytes from d1
              // d0 is now alternating red and green bytes

              // Repeat above steps for next 8 bytes
              "VLD1.8 {d1}, [%[ptr]] \n\t"
              "ADD %[ptr], %[ptr], #5 \n\t"
              "VLD1.8 {d2}, [%[ptr]] \n\t"
              "VSHL.I64 d1, d1, #32 \n\t"
              "ADD %[ptr], %[ptr], #5 \n\t"
              "VEXT.8 d1, d1, d2, #4 \n\t" // d1 is alternating red and green

              "VUZP.8 d0, d1 \n\t" // Unzip alternating bytes so d0 is all red bytes and d1 is all green bytes

              // Repeat above for the second row containing green and blue bytes
              "VLD1.8 {d2}, [%[ptr2]] \n\t"
              "ADD %[ptr2], %[ptr2], #5 \n\t"
              "VLD1.8 {d3}, [%[ptr2]] \n\t"
              "VSHL.I64 d2, d2, #32 \n\t"
              "ADD %[ptr2], %[ptr2], #5 \n\t"
              "VEXT.8 d2, d2, d3, #4 \n\t" // d2 is alternating green and blue

              "VLD1.8 {d3}, [%[ptr2]] \n\t"
              "ADD %[ptr2], %[ptr2], #5 \n\t"
              "VLD1.8 {d4}, [%[ptr2]] \n\t"
              "VSHL.I64 d3, d3, #32 \n\t"
              "ADD %[ptr2], %[ptr2], #5 \n\t"
              "VEXT.8 d3, d3, d4, #4 \n\t" // d3 is alternating green and blue

              "VUZP.8 d2, d3 \n\t" // d2 is green, d3 is blue
              "VSWP.8 d2, d3 \n\t" // Due to required register stride on saving need to have blue in d2 so swap

              // Average the green data with a vector halving add
              // Could save an instruction by just ignoring the second set of green data (d3)
              "VHADD.U8 d1, d1, d3 \n\t"

              // Perform a saturating left shift on all elements since each color byte is the 8 high bits
              // from the 10 bit data. This is like adding 00 as the two low bits
              "VQSHL.U8 d0, d0, #2 \n\t"
              "VQSHL.U8 d1, d1, #2 \n\t"
              "VQSHL.U8 d2, d2, #2 \n\t"

              // Interleaving store of red, green, and blue bytes into output rgb image
              "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

              : [ptr] "+r"(bayer),   // Output list because we want the output of the pointer adds, + since we are reading
                [out] "+r"(rgb),     //   and writing from these registers
                [ptr2] "+r"(bayer2)
              : // empty input list
              : "d0", "d1", "d2", "d3", "d4", "memory" // Clobber list of registers used and memory since it is being written to
            );
          }

          // Skip a row
          bayer += bayer_sx;
          bayer2 += bayer_sx;
        }
      }

      // End profiling
      auto tEnd = std::chrono::steady_clock::now();
      uint64_t elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tStart).count();
      profile.add(0, elapsed);

      if (_config.neon.debayer_half.output.save){
        std::string out_path = out_paths[path_index];
        std::ofstream ofs(out_path, std::ios::out | std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
      }



    } // for-loop: path_index
  } // for-loop: repetition

  _profiles.push_back(profile);
#endif
}

//======================================================================================================================
void Evaluator::pipeline()
{

  if (!_config.pipeline.enable){
    return;
  }
  // Covert a RAW10 BGGR10 format image to BGGR8 image

  // Get the image paths
  std::vector<std::string> in_paths = _config.pipeline.input.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.pipeline.name);
  profile.append("MapImage_input");
  profile.append("UnmapImage_input");
  profile.append("NDRangeKernel_debayer_zcp");
  profile.append("NDRangeKernel_resize_rgb");
  profile.append("NDRangeKernel_greyscale_full");
  profile.append("NDRangeKernel_greyscale_half");
  profile.append("NDRangeKernel_resize_grey");
  profile.append("MapImage_rgb_full");
  profile.append("UnmapImage_rgb_full");
  profile.append("MapImage_rgb_half");
  profile.append("UnmapImage_rgb_half");
  profile.append("MapImage_grey_full");
  profile.append("UnmapImage_grey_full");
  profile.append("MapImage_grey_half_convert");
  profile.append("UnmapImage_grey_half_convert");
  profile.append("MapImage_grey_half_resize");
  profile.append("UnmapImage_grey_half_resize");

  auto save = [this](ZCPImage& image, 
    cl::Event* eventMap, 
    cl::Event* eventUnmap, 
    NodeConfig& node, 
    const std::string& path,
    size_t num_bytes) -> void 
  {
    std::vector<unsigned char> out_bytes(num_bytes);

    cl::size_t<3> out_origin;
    out_origin[0] = 0;
    out_origin[1] = 0;
    out_origin[2] = 0;
    cl::size_t<3> out_region;
    out_region[0] = node.width;
    out_region[1] = node.height;
    out_region[2] = 1;

    image.ocl_mapping = this->_queue.enqueueMapImage(image.ocl_image, CL_TRUE, CL_MAP_READ,
      out_origin, out_region, &image.ocl_row_pitch, NULL, NULL, eventMap, NULL);

    this->zcp_copy(image, out_bytes);

    this->_queue.enqueueUnmapMemObject(image.ocl_image, image.ocl_mapping, NULL, eventUnmap);
      image.ocl_mapping = nullptr;

    if (node.save){
      std::ofstream ofs(path, std::ios::out | std::ios::binary);
      ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
    }
  };

  for (int repetition = 0; repetition < _config.pipeline.repetitions; ++repetition)
  {
    // Note, mapped images don't need to be copied over. They're always accessible by both CPU and GPU, however
    // care must be taken to not use the memory simultaneously.

    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});

      // Note: In practice you would not allocate and map the image every time for each image; you'd allocate once
      // and map once for the entirety of the program (or as long as you need it). This should be done outside any
      // of the for loops. But for profiling and simplicity based on the implementing the other algorithms first, 
      // I've done it in the innermost loop.
      ZCPImage source_image;
      zcp_allocate(source_image,
        _config.pipeline.input.width,
        _config.pipeline.input.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));
        
      ZCPImage out_rgb_full;
      zcp_allocate(out_rgb_full,
        _config.pipeline.rgb_full.width,
        _config.pipeline.rgb_full.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      ZCPImage out_rgb_half;
      zcp_allocate(out_rgb_half,
        _config.pipeline.rgb_half.width,
        _config.pipeline.rgb_half.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      ZCPImage out_grey_full;
      zcp_allocate(out_grey_full,
        _config.pipeline.grey_full.width,
        _config.pipeline.grey_full.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      ZCPImage out_grey_half_convert;
      zcp_allocate(out_grey_half_convert,
        _config.pipeline.grey_half_convert.width,
        _config.pipeline.grey_half_convert.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      ZCPImage out_grey_half_resize;
      zcp_allocate(out_grey_half_resize,
        _config.pipeline.grey_half_resize.width,
        _config.pipeline.grey_half_resize.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.pipeline.input.width;
      in_region[1] = _config.pipeline.input.height;
      in_region[2] = 1;

      source_image.ocl_mapping = _queue.enqueueMapImage(source_image.ocl_image, CL_TRUE, CL_MAP_WRITE,
        in_origin, in_region, &source_image.ocl_row_pitch, NULL, NULL, &events[0], NULL);

      zcp_copy(in_bytes, source_image);

      _queue.enqueueUnmapMemObject(source_image.ocl_image, source_image.ocl_mapping, NULL, &events[1]);
      source_image.ocl_mapping = nullptr;

      _queue.finish();

      cl::Kernel kernel_debayer(_program, _config.pipeline.rgb_full.kernel.c_str());
      kernel_debayer.setArg(0, source_image.ocl_image);
      kernel_debayer.setArg(1, out_rgb_full.ocl_image);

      cl::Kernel kernel_rgb_half(_program, _config.pipeline.rgb_half.kernel.c_str());
      kernel_rgb_half.setArg(0, out_rgb_full.ocl_image);
      kernel_rgb_half.setArg(1, out_rgb_half.ocl_image);
    
      cl::Kernel kernel_grey_full(_program, _config.pipeline.grey_full.kernel.c_str());
      kernel_grey_full.setArg(0, out_rgb_full.ocl_image);
      kernel_grey_full.setArg(1, out_grey_full.ocl_image);
    
      cl::Kernel kernel_grey_half_convert(_program, _config.pipeline.grey_half_convert.kernel.c_str());
      kernel_grey_half_convert.setArg(0, out_rgb_half.ocl_image);
      kernel_grey_half_convert.setArg(1, out_grey_half_convert.ocl_image);
    
      cl::Kernel kernel_grey_half_resize(_program, _config.pipeline.grey_half_resize.kernel.c_str());
      kernel_grey_half_resize.setArg(0, out_grey_full.ocl_image);
      kernel_grey_half_resize.setArg(1, out_grey_half_resize.ocl_image);

      _queue.enqueueNDRangeKernel(kernel_debayer,
        cl::NullRange,
        cl::NDRange(_config.pipeline.rgb_full.width/4, _config.pipeline.rgb_full.height/2), // iterate 4x2
        cl::NullRange,
        NULL, &events[2]);

      _queue.enqueueNDRangeKernel(kernel_rgb_half,
        cl::NullRange,
        cl::NDRange(_config.pipeline.rgb_half.width, _config.pipeline.rgb_half.height),
        cl::NullRange,
        NULL, &events[3]);

      _queue.enqueueNDRangeKernel(kernel_grey_full,
        cl::NullRange,
        cl::NDRange(_config.pipeline.grey_full.width, _config.pipeline.grey_full.height),
        cl::NullRange,
        NULL, &events[4]);

      _queue.enqueueNDRangeKernel(kernel_grey_half_convert,
        cl::NullRange,
        cl::NDRange(_config.pipeline.grey_half_convert.width, _config.pipeline.grey_half_convert.height),
        cl::NullRange,
        NULL, &events[5]);

      _queue.enqueueNDRangeKernel(kernel_grey_half_resize,
        cl::NullRange,
        cl::NDRange(_config.pipeline.grey_half_resize.width, _config.pipeline.grey_half_resize.height),
        cl::NullRange,
        NULL, &events[6]);

      _queue.finish();


      save(out_rgb_full, &events[7], &events[8], _config.pipeline.rgb_full, _config.pipeline.rgb_full.paths[path_index],
        3*_config.pipeline.rgb_full.width*_config.pipeline.rgb_full.height);
      save(out_rgb_half, &events[9], &events[10], _config.pipeline.rgb_half, _config.pipeline.rgb_half.paths[path_index],
        3*_config.pipeline.rgb_half.width*_config.pipeline.rgb_half.height);
      save(out_grey_full, &events[11], &events[12], _config.pipeline.grey_full, _config.pipeline.grey_full.paths[path_index],
        _config.pipeline.grey_full.width*_config.pipeline.grey_full.height);
      save(out_grey_half_convert, &events[13], &events[14], _config.pipeline.grey_half_convert, _config.pipeline.grey_half_convert.paths[path_index],
        _config.pipeline.grey_half_convert.width*_config.pipeline.grey_half_convert.height);
      save(out_grey_half_resize, &events[15], &events[16], _config.pipeline.grey_half_resize, _config.pipeline.grey_half_resize.paths[path_index],
        _config.pipeline.grey_half_resize.width*_config.pipeline.grey_half_resize.height);

      _queue.finish();

      // Add profiling information for the inner loop
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii

    } // for-loop: path_index

  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::pipeline_kernel()
{

  if (!_config.pipeline_kernel.enable){
    return;
  }

  // Get the image paths
  std::vector<std::string> in_paths = _config.pipeline_kernel.input.paths;

  // TODO: Handle all images
  if (in_paths.empty()){
    return;
  }

  Profile profile(_config.pipeline_kernel.name);
  profile.append("MapImage_input");
  profile.append("UnmapImage_input");
  profile.append("NDRangeKernel_pipeline_kernel");
  profile.append("MapImage_rgb_full");
  profile.append("UnmapImage_rgb_full");
  profile.append("MapImage_rgb_half");
  profile.append("UnmapImage_rgb_half");
  profile.append("MapImage_grey_full");
  profile.append("UnmapImage_grey_full");
  profile.append("MapImage_grey_half_resize");
  profile.append("UnmapImage_grey_half_resize");

  auto save = [this](ZCPImage& image, 
    cl::Event* eventMap, 
    cl::Event* eventUnmap, 
    NodeConfig& node, 
    const std::string& path,
    size_t num_bytes) -> void 
  {
    std::vector<unsigned char> out_bytes(num_bytes);

    cl::size_t<3> out_origin;
    out_origin[0] = 0;
    out_origin[1] = 0;
    out_origin[2] = 0;
    cl::size_t<3> out_region;
    out_region[0] = node.width;
    out_region[1] = node.height;
    out_region[2] = 1;

    image.ocl_mapping = this->_queue.enqueueMapImage(image.ocl_image, CL_TRUE, CL_MAP_READ,
      out_origin, out_region, &image.ocl_row_pitch, NULL, NULL, eventMap, NULL);

    this->zcp_copy(image, out_bytes);

    this->_queue.enqueueUnmapMemObject(image.ocl_image, image.ocl_mapping, NULL, eventUnmap);
      image.ocl_mapping = nullptr;

    if (node.save){
      std::ofstream ofs(path, std::ios::out | std::ios::binary);
      ofs.write(reinterpret_cast<const char*>(out_bytes.data()), out_bytes.size());
    }
  };

  for (int repetition = 0; repetition < _config.pipeline_kernel.repetitions; ++repetition)
  {
    // Note, mapped images don't need to be copied over. They're always accessible by both CPU and GPU, however
    // care must be taken to not use the memory simultaneously.

    for (int path_index = 0; path_index < in_paths.size(); ++path_index)
    {
      std::vector<cl::Event> events(profile.num_events());
      std::string in_path = in_paths[path_index];
      std::ifstream ifs(in_path, std::ios::binary);
      std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});

      // Note: In practice you would not allocate and map the image every time for each image; you'd allocate once
      // and map once for the entirety of the program (or as long as you need it). This should be done outside any
      // of the for loops. But for profiling and simplicity based on the implementing the other algorithms first, 
      // I've done it in the innermost loop.
      ZCPImage source_image;
      zcp_allocate(source_image,
        _config.pipeline_kernel.input.width,
        _config.pipeline_kernel.input.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));
        
      ZCPImage out_rgb_full;
      zcp_allocate(out_rgb_full,
        _config.pipeline_kernel.rgb_full.width,
        _config.pipeline_kernel.rgb_full.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      ZCPImage out_rgb_half;
      zcp_allocate(out_rgb_half,
        _config.pipeline_kernel.rgb_half.width,
        _config.pipeline_kernel.rgb_half.height,
        cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

      ZCPImage out_grey_full;
      zcp_allocate(out_grey_full,
        _config.pipeline_kernel.grey_full.width,
        _config.pipeline_kernel.grey_full.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      ZCPImage out_grey_half;
      zcp_allocate(out_grey_half,
        _config.pipeline_kernel.grey_half.width,
        _config.pipeline_kernel.grey_half.height,
        cl::ImageFormat(CL_R, CL_UNORM_INT8));

      cl::size_t<3> in_origin;
      in_origin[0] = 0;
      in_origin[1] = 0;
      in_origin[2] = 0;
      cl::size_t<3> in_region;
      in_region[0] = _config.pipeline_kernel.input.width;
      in_region[1] = _config.pipeline_kernel.input.height;
      in_region[2] = 1;

      source_image.ocl_mapping = _queue.enqueueMapImage(source_image.ocl_image, CL_TRUE, CL_MAP_WRITE,
        in_origin, in_region, &source_image.ocl_row_pitch, NULL, NULL, &events[0], NULL);

      zcp_copy(in_bytes, source_image);

      _queue.enqueueUnmapMemObject(source_image.ocl_image, source_image.ocl_mapping, NULL, &events[1]);
      source_image.ocl_mapping = nullptr;

      _queue.finish();

      cl::Kernel kernel(_program, _config.pipeline_kernel.kernel.name.c_str());
      kernel.setArg(0, source_image.ocl_image);
      kernel.setArg(1, out_rgb_full.ocl_image);
      kernel.setArg(2, out_grey_full.ocl_image);
      kernel.setArg(3, out_rgb_half.ocl_image);
      kernel.setArg(4, out_grey_half.ocl_image);

      _queue.enqueueNDRangeKernel(kernel,
        cl::NullRange,
        cl::NDRange(_config.pipeline_kernel.kernel.width, _config.pipeline_kernel.kernel.height),
        cl::NullRange,
        NULL, &events[2]);

      _queue.finish();

      save(out_rgb_full, &events[3], &events[4], _config.pipeline_kernel.rgb_full, _config.pipeline_kernel.rgb_full.paths[path_index],
        3*_config.pipeline_kernel.rgb_full.width*_config.pipeline_kernel.rgb_full.height);
      save(out_rgb_half, &events[5], &events[6], _config.pipeline_kernel.rgb_half, _config.pipeline_kernel.rgb_half.paths[path_index],
        3*_config.pipeline_kernel.rgb_half.width*_config.pipeline_kernel.rgb_half.height);
      save(out_grey_full, &events[7], &events[8], _config.pipeline_kernel.grey_full, _config.pipeline_kernel.grey_full.paths[path_index],
        _config.pipeline_kernel.grey_full.width*_config.pipeline_kernel.grey_full.height);
      save(out_grey_half, &events[9], &events[10], _config.pipeline_kernel.grey_half, _config.pipeline_kernel.grey_half.paths[path_index],
        _config.pipeline_kernel.grey_half.width*_config.pipeline_kernel.grey_half.height);

      _queue.finish();

      // Add profiling information for the inner loop
      for (int ii = 0; ii < events.size(); ++ii)
      {
        cl::Event& evt = events[ii];
        uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
          evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        profile.add(ii, elapsed);
      } // for-loop: ii

    } // for-loop: path_index

  } // for-loop: repetition

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::power_neon()
{
  std::string in_path = _config.power_neon.input.path;
  std::ifstream ifs(in_path, std::ios::binary);
  std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
  std::vector<unsigned char> out_bytes(3 * _config.power_neon.output.width * _config.power_neon.output.height);
  size_t out_size = 3 * sizeof(unsigned char) * _config.power_neon.output.width * _config.power_neon.output.height;

  const uint8_t* bayer_in = in_bytes.data();
  int32_t rows = _config.power_neon.output.height;
  int32_t cols = _config.power_neon.output.width;
  cv::Mat_<cv::Vec<uint8_t,3>> rgb(rows,cols);

  cv::Mat bayer(rows, cols, CV_8UC1);

  Profile profile(_config.power_neon.name);
  profile.append("debayer");
  
  // Code to test power - look at multimeter while this is running
  for (int repetition = 0; repetition < _config.power_neon.repetitions; ++repetition)
  {
    auto tStart = std::chrono::steady_clock::now();

    const uint8_t* bufferPtr = bayer_in;
    uint8_t* bayerPtr = static_cast<uint8_t*>(bayer.ptr());
    for(int ii = 0; ii < (cols*rows); ii+=8)
    {
      uint8x8_t data = vld1_u8(bufferPtr);
      bufferPtr += 5;
      uint8x8_t data2 = vld1_u8(bufferPtr);
      bufferPtr += 5;
      data = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(data), 32));
      data = vext_u8(data, data2, 4);
      data = vqshl_n_u8(data, 2);
      vst1_u8(bayerPtr, data);
      bayerPtr += 8;
    }
    cv::cvtColor(bayer, rgb, cv::COLOR_BayerRG2BGR);

    auto tEnd = std::chrono::steady_clock::now();
    uint64_t elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tStart).count();
    profile.add(0, elapsed);
  }

  _profiles.push_back(profile);
}

//======================================================================================================================
void Evaluator::power_zcp()
{
  std::string in_path = _config.power_neon.input.path;
  std::ifstream ifs(in_path, std::ios::binary);
  std::vector<unsigned char> in_bytes(std::istreambuf_iterator<char>(ifs), {});
  std::vector<unsigned char> out_bytes(3 * _config.power_zcp.output.width * _config.power_zcp.output.height);

  ZCPImage in_image2D;
  zcp_allocate(in_image2D,
    _config.power_zcp.input.width,
    _config.power_zcp.input.height,
    cl::ImageFormat(CL_R, CL_UNORM_INT8));
  ZCPImage out_image2D;
  zcp_allocate(out_image2D,
    _config.power_zcp.output.width,
    _config.power_zcp.output.height,
    cl::ImageFormat(CL_RGB, CL_UNORM_INT8));

  cl::size_t<3> in_origin;
  in_origin[0] = 0;
  in_origin[1] = 0;
  in_origin[2] = 0;
  cl::size_t<3> in_region;
  in_region[0] = _config.power_zcp.input.width;
  in_region[1] = _config.power_zcp.input.height;
  in_region[2] = 1;

  // Copy the bayer image into the buffer. Do this once just to set up the input
  in_image2D.ocl_mapping = _queue.enqueueMapImage(in_image2D.ocl_image, CL_TRUE, CL_MAP_WRITE,
    in_origin, in_region, &in_image2D.ocl_row_pitch, NULL, NULL, NULL, NULL);

  zcp_copy(in_bytes, in_image2D);

  _queue.enqueueUnmapMemObject(in_image2D.ocl_image, in_image2D.ocl_mapping, NULL, NULL);

  _queue.finish();

  cl::size_t<3> out_origin;
  out_origin[0] = 0;
  out_origin[1] = 0;
  out_origin[2] = 0;
  cl::size_t<3> out_region;
  out_region[0] = _config.power_zcp.output.width;
  out_region[1] = _config.power_zcp.output.height;
  out_region[2] = 1;

  Profile profile(_config.power_zcp.name);
  profile.append("debayer");

  // Code to test power - look at multimeter while this is running
  for (int repetition = 0; repetition < _config.power_zcp.repetitions; ++repetition)
  {
    auto tStart = std::chrono::steady_clock::now();

    in_image2D.ocl_mapping = _queue.enqueueMapImage(in_image2D.ocl_image, CL_TRUE, CL_MAP_WRITE,
      in_origin, in_region, &in_image2D.ocl_row_pitch, NULL, NULL, NULL, NULL);

    // Don't actually do anything, we're not measuring writing data into the buffer; assume it's already done. However,
    // we have to do the map/unmap because those are relevant operations.

    _queue.enqueueUnmapMemObject(in_image2D.ocl_image, in_image2D.ocl_mapping, NULL, NULL);

    // Not sure if we have to make a new on each time or just set the args.
    cl::Kernel kernel(_program, _config.power_zcp.kernel.name.c_str());
    kernel.setArg(0, in_image2D.ocl_image);
    kernel.setArg(1, out_image2D.ocl_image);

    _queue.enqueueNDRangeKernel(kernel,
      cl::NullRange,
      cl::NDRange(_config.power_zcp.kernel.width, _config.power_zcp.kernel.height),
      cl::NullRange,
      NULL, NULL);

    _queue.finish();

    out_image2D.ocl_mapping = _queue.enqueueMapImage(out_image2D.ocl_image, CL_TRUE, CL_MAP_READ,
      out_origin, out_region, &out_image2D.ocl_row_pitch, NULL, NULL, NULL, NULL);

    // Don't actually do anything, we don't care about this when measuring power. The data is usable outside of 
    // debayering at this point.

    _queue.enqueueUnmapMemObject(out_image2D.ocl_image, out_image2D.ocl_mapping, NULL, NULL);

    _queue.finish();

    auto tEnd = std::chrono::steady_clock::now();
    uint64_t elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(tEnd - tStart).count();
    profile.add(0, elapsed);
  }

  _profiles.push_back(profile);
}

}