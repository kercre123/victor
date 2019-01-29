#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define __CL_ENABLE_EXCEPTIONS  // Turn on exceptions
#include <CL/cl.hpp>

void rot13(const char* input, char* output)
{
  int index=0;
  char c = input[index];
  while (c!=0) {
    if (c<'A' || c>'z' || (c>'Z' && c<'a')) {
        output[index] = input[index];
    } else {
        if (c>'m' || (c>'M' && c<'a')) {
            output[index] = input[index]-13;
        } else {
            output[index] = input[index]+13;
        }
    }
    c = input[++index];
  }
}

std::string rot13_cpu(const std::string& message)
{
  size_t length = message.length();
  char output[length+1]; // add one for null character
  memset((void*)output, 0, sizeof(output));
  rot13(message.c_str(), output);
  return std::string(output);
}

std::string rot13_gpu(const std::string& filename, const std::string& message){
  // Fetch the Platform and Device IDs; we only want one.
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  if (platforms.empty()){
    return "NO PLATFORMS";
  }
  cl::Platform platform = platforms[0];

  std::vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  if (devices.empty()){
    return "NO DEVICES";
  }
  cl::Device device = devices[0];

  cl::Context context({device});

  cl::Program::Sources sources;
  std::string text;
  {
    std::ifstream ifs(filename);
    std::stringstream ss;
    ss << ifs.rdbuf();
    text = ss.str();
  }
  sources.push_back({ text.c_str(), text.size() });

  cl::Program program(context, sources);
  try {
    program.build({device});
  } catch (cl::Error& e){
    std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device)<<std::endl;
    std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)<<std::endl;
    return "FAILED TO BUILD PROGRAM";
  }

  // add one for null character
  size_t size = message.size()+1;

  cl::Buffer buffer_A(context, CL_MEM_READ_WRITE, size);
  cl::Buffer buffer_B(context, CL_MEM_READ_WRITE, size);

  cl::CommandQueue queue(context, device);

  queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, size, (void*)message.c_str());
  
  cl::Kernel kernel_rot13(program, "rot13");
  kernel_rot13.setArg(0, buffer_A);
  kernel_rot13.setArg(1, buffer_B);
  queue.enqueueNDRangeKernel(kernel_rot13,
    cl::NullRange,
    cl::NDRange(size),
    cl::NullRange);

  char output[size];
  memset((void*)output, 0, sizeof(output));
  queue.enqueueReadBuffer(buffer_B, CL_TRUE, 0, size, output);

  return std::string(output);
}

int main(int argc, char** argv)
{
  if (argc != 3){
    std::cerr<<"Usage: rot13 <rot13.cl> <message>"<<std::endl;
    return -1;
  }

  {
    std::string message(argv[2]);
    std::string rotated = rot13_cpu(message);

    std::cerr<<message<<" <-- rot13_cpu --> "<<rotated<<std::endl;
  }
  
  {
    std::string filename(argv[1]);
    std::string message(argv[2]);
    std::string rotated = rot13_gpu(filename, message);

    std::cerr<<message<<" <-- rot13_gpu --> "<<rotated<<std::endl;
  }

  
  
}