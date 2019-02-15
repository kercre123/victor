#ifndef __PDORAN_ACCELERATOR_H__
#define __PDORAN_ACCELERATOR_H__

#include <vector>
#include <cfloat>

#ifndef __CL_ENABLE_EXCEPTIONS
  #define __CL_ENABLE_EXCEPTIONS
#endif
#include <CL/cl.hpp>


class AcceleratorException : public std::runtime_error {
public:
  AcceleratorException(const std::string& what) : std::runtime_error(what) {}
};

class Accelerator
{
public:
  static Accelerator& instance();
  void hello(const float* input_arg1, const float* input_arg2, float* output, std::vector<size_t> dims);
  void profile();

private:
  struct Event {
    Event() : name(""), event(), sum(0.0), count(0), average(0), min(DBL_MAX), max(DBL_MIN){}
    std::string name;
    cl::Event event;
    double sum;
    uint64_t count;
    double average;
    double min;
    double max;
  };

  Accelerator();
  void init();
  void deinit();

  cl::Platform _platform;
  cl::Device _device;
  cl::Program _program;
  cl::Context _context;
  cl::CommandQueue _queue;
  std::vector<Event> _events;
};

#endif /* __PDORAN_ACCELERATOR_H__ */