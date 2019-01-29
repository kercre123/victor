#ifndef __EVALUATOR_SERIALIZATION_H__
#define __EVALUATOR_SERIALIZATION_H__

#include <CL/cl.hpp>

#include <ostream>

std::ostream& operator<<(std::ostream& os, const cl::Platform& platform);
std::ostream& operator<<(std::ostream& os, const cl::Device& device);
std::ostream& operator<<(std::ostream& os, const cl::Context& context);
std::ostream& operator<<(std::ostream& os, const cl::Program& program);
std::ostream& operator<<(std::ostream& os, const cl::ImageFormat& format);


#endif