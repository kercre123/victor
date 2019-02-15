#include "accelerator.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

std::string command2string(cl_command_type ctype)
{
    std::string command;
    switch(ctype)
    {
    case CL_COMMAND_NDRANGE_KERNEL: return "NDRANGE_KERNEL";
    case CL_COMMAND_TASK: return "TASK";
    case CL_COMMAND_NATIVE_KERNEL: return "NATIVE_KERNEL";
    case CL_COMMAND_READ_BUFFER: return "READ_BUFFER";
    case CL_COMMAND_WRITE_BUFFER: return "WRITE_BUFFER";
    case CL_COMMAND_COPY_BUFFER: return "COPY_BUFFER";
    case CL_COMMAND_READ_IMAGE: return "READ_IMAGE";
    case CL_COMMAND_WRITE_IMAGE: return "WRITE_IMAGE";
    case CL_COMMAND_COPY_IMAGE: return "COPY_IMAGE";
    case CL_COMMAND_COPY_BUFFER_TO_IMAGE: return "COPY_BUFFER_TO_IMAGE";
    case CL_COMMAND_COPY_IMAGE_TO_BUFFER: return "COPY_IMAGE_TO_BUFFER";
    case CL_COMMAND_MAP_BUFFER: return "MAP_BUFFER";
    case CL_COMMAND_MAP_IMAGE: return "MAP_IMAGE";
    case CL_COMMAND_UNMAP_MEM_OBJECT: return "UNMAP_MEM_OBJECT";
    case CL_COMMAND_MARKER: return "MARKER";
    case CL_COMMAND_ACQUIRE_GL_OBJECTS: return "ACQUIRE_GL_OBJECTS";
    case CL_COMMAND_RELEASE_GL_OBJECTS: return "RELEASE_GL_OBJECTS";
    case CL_COMMAND_READ_BUFFER_RECT: return "READ_BUFFER_RECT";
    case CL_COMMAND_WRITE_BUFFER_RECT: return "WRITE_BUFFER_RECT";
    case CL_COMMAND_COPY_BUFFER_RECT: return "COPY_BUFFER_RECT";
    case CL_COMMAND_USER: return "USER";
    default: return "UNKNOWN";
    }
}

Accelerator::Accelerator()
{
  init();
}

void Accelerator::init()
{
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  if (platforms.empty()){
    throw AcceleratorException("Unexpected: no platforms");
  } else if (platforms.size() > 1){
    throw AcceleratorException("Unexpected: more than one platform");
  }
  cl::Platform platform = platforms[0];

  std::vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  if (devices.empty()){
    throw AcceleratorException("Unexpected: no devices");
  } else if (devices.size() > 1) {
    throw AcceleratorException("Unexpected: more than one device");
  }
  cl::Device device = devices[0];

  cl::Context context({device});

  cl::Program program;
  {
    std::vector<std::string> paths = {
      "resources/hello.cl",
    };
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
    throw AcceleratorException(message);
  }

  _platform = platform;
  _device = device;
  _context = context;
  _program = program;
  _queue = cl::CommandQueue(_context, _device, CL_QUEUE_PROFILING_ENABLE);

  _events.resize(4);
  _events[0].name = "EnqueueWriteBuffer_1";
  _events[1].name = "EnqueueWriteBuffer_2";
  _events[2].name = "EnqueueNDRangeKernel";
  _events[3].name = "EnqueueReadBuffer";
}

void Accelerator::deinit()
{
}

Accelerator& Accelerator::instance()
{
  static Accelerator gAccelerator;
  return gAccelerator;
}

void Accelerator::hello(const float* input_arg1, const float* input_arg2, float* output, std::vector<size_t> dims)
{
  size_t size = 1;
  for (int i = 0; i < dims.size(); ++i){
    size *= dims[i];
  }
  size *= sizeof(float);

  cl::Buffer buffer_A(_context, CL_MEM_READ_WRITE, size);
  cl::Buffer buffer_B(_context, CL_MEM_READ_WRITE, size);
  cl::Buffer buffer_C(_context, CL_MEM_READ_WRITE, size);

  _queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, size, (void*)input_arg1, nullptr, &_events[0].event);
  _queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, size, (void*)input_arg2, nullptr, &_events[1].event);
  
  cl::Kernel kernel(_program, "hello");
  kernel.setArg(0, buffer_A);
  kernel.setArg(1, buffer_B);
  kernel.setArg(2, buffer_C);
  _queue.enqueueNDRangeKernel(kernel,
    cl::NullRange,
    cl::NDRange(size),
    cl::NullRange,
    nullptr,
    &_events[2].event);

  _queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, size, output, nullptr, &_events[3].event);

  for (int i = 0; i < _events.size(); ++i){
    cl::Event& evt = _events[i].event;
    uint64_t elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
      evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    double seconds = elapsed/1000000000.0;
    _events[i].sum += seconds;
    _events[i].min = std::min(_events[i].min, seconds);
    _events[i].max = std::max(_events[i].min, seconds);
    _events[i].count += 1;
    _events[i].average = _events[i].sum / _events[i].count;
  }
}

void Accelerator::profile()
{
  std::cerr<<std::setw(32)<<"Name"
    <<std::setw(15)<<"Min"
    <<std::setw(14)<<"Max"
    <<std::setw(14)<<"Avg"
    <<std::setw(14)<<"Count"
    <<std::endl;
  for (int i = 0; i < _events.size(); ++i){
    std::cerr<<std::setw(32)<<_events[i].name<<": "
      <<std::setprecision(6)
      <<std::fixed
      <<std::setw(14)<<_events[i].min
      <<std::setw(14)<<_events[i].max
      <<std::setw(14)<<_events[i].average
      <<std::setw(14)<<_events[i].count
      <<std::endl;
  }
}
