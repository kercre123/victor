#ifndef __EVALUATOR_EVALUATOR_H__
#define __EVALUATOR_EVALUATOR_H__

#include <string>

#ifndef __CL_ENABLE_EXCEPTIONS
  #define __CL_ENABLE_EXCEPTIONS
#endif
#include <CL/cl.hpp>

#include "IONMemory.h"
#include "Profile.h"

namespace Anki
{

class EvaluatorException : public std::runtime_error {
public:
  EvaluatorException(const std::string& what) : std::runtime_error(what) {}
};

// Forward declare
class Evaluator
{
public:
  struct TestConfig {
    std::string name;
    bool enable;
    int repetitions;

    struct {
      int width;
      int height;
      std::vector<std::string> paths;
    } input;

    struct {
      bool save;
      int width;
      int height;
      std::vector<std::string> paths;
    } output;
  };

  struct TestKernelConfig : public TestConfig {
    struct {
      std::string name;
      int width;
      int height;
    } kernel;
  };

  struct NodeConfig {
    std::string kernel;
    int width;
    int height;
    bool save;
    std::vector<std::string> paths;
  };

  struct Config {

    //! gpu config
    struct {
      //! Paths to the programs to load
      std::vector<std::string> programs;
      std::vector<std::string> options;

      TestKernelConfig debayer;
      TestKernelConfig debayer_half;
      TestKernelConfig debayer_img;
      TestKernelConfig debayer_img_half;
      TestKernelConfig debayer_zcp;
      TestKernelConfig debayer_zcp_half;
      TestKernelConfig squash_zcp;
    } gpu;

    //! neon config
    struct {
      TestConfig debayer;
      TestConfig debayer_half;
    } neon;

    struct {
      std::string name;
      bool enable;
      int repetitions;

      struct {
        int width;
        int height;
        std::vector<std::string> paths;
      } input;

      NodeConfig rgb_full;
      NodeConfig rgb_half;
      NodeConfig grey_full;
      NodeConfig grey_half_convert;
      NodeConfig grey_half_resize;

    } pipeline;

    struct {
      std::string name;
      bool enable;
      int repetitions;

      struct {
        int width;
        int height;
        std::vector<std::string> paths;
      } input;

      struct {
        std::string name;
        int width;
        int height;
      } kernel;

      NodeConfig rgb_full;
      NodeConfig rgb_half;
      NodeConfig grey_full;
      NodeConfig grey_half;

    } pipeline_kernel;

    struct {
      std::string name;
      bool enable;
      int repetitions;
      int copies;

      struct {
        int width;
        int height;
        std::string path;
      } input;

      struct {
        int width;
        int height;
      } output;
    } power_neon;

    struct {
      std::string name;
      bool enable;
      int repetitions;
      int copies;

      struct {
        int width;
        int height;
        std::string path;
      } input;

      struct {
        int width;
        int height;
      } output;

      struct {
        std::string name;
        int width;
        int height;
      } kernel;
    } power_zcp;

    struct 
    {
      std::string name;
      bool enable;
      int copies;           //! Number of copies of the input to make
      int fps;              //! Target frames per second (e.g. 30, 15, 10, 5)
      double seconds;       //! Number of seconds to run the test

      struct {
        int width;
        int height;
        std::string path;
      } input;

      struct {
        int width;
        int height;
      } output;

      struct {
        std::string name;
        int width;
        int height;
      } kernel;
      
    } longevity_neon;

    struct 
    {
      std::string name;
      bool enable;
      int copies;           //! Number of copies of the input to make
      int fps;              //! Target frames per second (e.g. 30, 15, 10, 5)
      double seconds;       //! Number of seconds to run the test

      struct {
        int width;
        int height;
        std::string path;
      } input;

      struct {
        int width;
        int height;
      } output;

      struct {
        std::string name;
        int width;
        int height;
      } kernel;
      
    } longevity_zcp;
  };

  Evaluator(const Config& config);
  ~Evaluator();

  void run();
  void print();

private:

  struct ZCPImage
  {
    //! Buffer of ION allocated memory
    IONBuffer ion_buffer;

    //! OpenCL Image2D wrapping this buffer
    cl::Image2D ocl_image;

    //! Pointer to 
    cl_mem_ion_host_ptr ocl_ion_mem;
    
    //! Value return by clEnqueueMapImage
    void * ocl_mapping;

    size_t ocl_row_pitch;

    size_t ocl_n_channels;
  };

  void init();
  void deinit();

  void gpu_debayer();
  void gpu_debayer_half();
  void gpu_debayer_img();
  void gpu_debayer_img_half();
  void gpu_debayer_zcp();
  void gpu_debayer_zcp_half();
  void gpu_squash_zcp();

  void neon_debayer();
  void neon_debayer_half();

  void pipeline();
  void pipeline_kernel();

  void power_neon();
  void power_zcp();

  void longevity_neon();
  void longevity_zcp();

  void zcp_allocate(ZCPImage& image, size_t width, size_t height, cl::ImageFormat format);
  /**
   * @brief Copies continguous vector of bytes into an image created using zcp_allocate
   */
  void zcp_copy(const std::vector<unsigned char>& bytes, ZCPImage& image);

  /**
   * @brief Copies continguous vector of bytes from an image created using zcp_allocate
   */
  void zcp_copy(const ZCPImage& image, std::vector<unsigned char>& bytes);

  Config _config;
  cl::Platform _platform;
  cl::Device _device;
  cl::Program _program;
  cl::Context _context;
  cl::CommandQueue _queue;

  std::vector<Profile> _profiles;
};

} /* namespace Anki */

#endif /* __EVALUATOR_H__ */