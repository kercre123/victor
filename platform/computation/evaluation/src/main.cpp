#include "Evaluator.h"
#include "Util.h"

#include <iostream>

using namespace Anki;


int main(int argc, char** argv)
{
  Evaluator::Config config;

  // Other random things to try
  // GPU Frequency Control via opencl
  //    HIGH = Max frequency
  //    LOW = Min frequency
  //    NORMAL = dynamic clock and voltage
  //    This is hint, thermal controls and external apps/services can override.
  // cl_qcom_perf_hint = [HIGH,NORMAL,LOW]

  // Qualcomm mentions there is a "Snapdragon Profiler"
  config.gpu.programs = {
    "resources/programs/global.cl", // Include order matters, I put the global sampler here. 
    "resources/programs/debayer.cl",
    "resources/programs/greyscale.cl",
    "resources/programs/pipeline.cl",
    "resources/programs/resize.cl",
    "resources/programs/squash.cl"
    // NOTE: Don't forget to add a comma when adding a new file to this list!
  };

  // NOTE: None of these options are required
  config.gpu.options = {
    // "-D <name>",                        // Predefine name as a macro, with definition 1.
    // "-D <name>=<definition>",           // Predefine name as a macro
    // "-I <dir>",                         // Include directory to search for header files
    "-cl-single-precision-constant",    // Treat double precision floating-point constant as single precision constant.
    "-cl-denorms-are-zero",             // Flush denormalized single/double precision numbers to 0
    // "-cl-opt-disable",                  // Disable all optimizations
    "-cl-mad-enable",                   // Allow a * b + c to be replaced by a mad, reducing accuracy
    "-cl-no-signed-zeros",              // Allow optimizations that ignore the signedness of floating point zero
    // "-cl-unsafe-math-optimizations",    // Allow floating point math optimizations that may violate stanadards/compliance
    // "-cl-finite-math-only",             // Allow optimizations for floating point math that assume args/results aren't NaN or Inf
    "-cl-fast-relaxed-math",            // Equivalent to "-cl-finite-math-only -cl-unsafe-math-optimizations", sets __FAST_RELAXED_MATH__
    // "-w",                               // Inhibit all warning messages
    "-Werror",                          // Make all warnings into errors
    // "-cl-std=C1.1"                      // Sets OpenCL language version
  };

  //====================================================================================================================
  // GPU Config
  
  //==================
  // Debayer config
  config.gpu.debayer.name = "gpu_debayer";
  config.gpu.debayer.enable = false;
  config.gpu.debayer.repetitions = 100;
  config.gpu.debayer.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer.input.height = 720;
  config.gpu.debayer.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer.output.width = 1280;
  config.gpu.debayer.output.height = 720;
  config.gpu.debayer.output.save = false;
  config.gpu.debayer.output.paths = {
    "output/gpu/debayer/image0000.rgb",
  };
  config.gpu.debayer.kernel.name = "debayer";
  config.gpu.debayer.kernel.width = 320;
  config.gpu.debayer.kernel.height = 360;

  //==================
  // Debayer_half config
  config.gpu.debayer_half.name = "gpu_debayer_half";
  config.gpu.debayer_half.enable = false;
  config.gpu.debayer_half.repetitions = 100;
  config.gpu.debayer_half.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer_half.input.height = 720;
  config.gpu.debayer_half.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer_half.output.width = 640;
  config.gpu.debayer_half.output.height = 360;
  config.gpu.debayer_half.output.save = false;
  config.gpu.debayer_half.output.paths = {
    "output/gpu/debayer_half/image0000.rgb",
  };
  config.gpu.debayer_half.kernel.name = "debayer_half";
  config.gpu.debayer_half.kernel.width = 160;
  config.gpu.debayer_half.kernel.height = 180;

  //==================
  // debayer_img
  config.gpu.debayer_img.name = "gpu_debayer_img";
  config.gpu.debayer_img.enable = false;
  config.gpu.debayer_img.repetitions = 100;
  config.gpu.debayer_img.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer_img.input.height = 720;
  config.gpu.debayer_img.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer_img.output.width = 1280;
  config.gpu.debayer_img.output.height = 720;
  config.gpu.debayer_img.output.save = false;
  config.gpu.debayer_img.output.paths = {
    "output/gpu/debayer_img/image0000.rgb",
  };
  config.gpu.debayer_img.kernel.name = "debayer_img";
  config.gpu.debayer_img.kernel.width = 320;
  config.gpu.debayer_img.kernel.height = 360;

  //==================
  // debayer_img_half
  config.gpu.debayer_img_half.name = "gpu_debayer_img_half";
  config.gpu.debayer_img_half.enable = false;
  config.gpu.debayer_img_half.repetitions = 100;
  config.gpu.debayer_img_half.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer_img_half.input.height = 720;
  config.gpu.debayer_img_half.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer_img_half.output.width = 640;
  config.gpu.debayer_img_half.output.height = 360;
  config.gpu.debayer_img_half.output.save = false;
  config.gpu.debayer_img_half.output.paths = {
    "output/gpu/debayer_img_half/image0000.rgb",
  };
  config.gpu.debayer_img_half.kernel.name = "debayer_img_half";
  config.gpu.debayer_img_half.kernel.width = 160;
  config.gpu.debayer_img_half.kernel.height = 180;

  //==================
  // debayer_zcp
  config.gpu.debayer_zcp.name = "gpu_debayer_zcp";
  config.gpu.debayer_zcp.enable = false;
  config.gpu.debayer_zcp.repetitions = 100;
  config.gpu.debayer_zcp.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer_zcp.input.height = 720;
  config.gpu.debayer_zcp.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer_zcp.output.width = 1280;
  config.gpu.debayer_zcp.output.height = 720;
  config.gpu.debayer_zcp.output.save = false;
  config.gpu.debayer_zcp.output.paths = {
    "output/gpu/debayer_zcp/image0000.rgb",
  };
  // NOTE: Zero copy can use the same kernel; only difference is in the setup of memory at run time.
  config.gpu.debayer_zcp.kernel.name = "debayer_img";
  config.gpu.debayer_zcp.kernel.width = 320;
  config.gpu.debayer_zcp.kernel.height = 360;

  //==================
  // debayer_zcp_half
  config.gpu.debayer_zcp_half.name = "gpu_debayer_zcp_half";
  config.gpu.debayer_zcp_half.enable = false;
  config.gpu.debayer_zcp_half.repetitions = 100;
  config.gpu.debayer_zcp_half.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.debayer_zcp_half.input.height = 720;
  config.gpu.debayer_zcp_half.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.debayer_zcp_half.output.width = 640;
  config.gpu.debayer_zcp_half.output.height = 360;
  config.gpu.debayer_zcp_half.output.save = false;
  config.gpu.debayer_zcp_half.output.paths = {
    "output/gpu/debayer_zcp_half/image0000.rgb",
  };
  // NOTE: Zero copy can use the same kernel; only difference is in the setup of memory at run time.
  config.gpu.debayer_zcp_half.kernel.name = "debayer_img_half";
  config.gpu.debayer_zcp_half.kernel.width = 160;
  config.gpu.debayer_zcp_half.kernel.height = 180;

  //=====================
  // squash_zcp
  config.gpu.squash_zcp.name = "gpu_squash_zcp";
  config.gpu.squash_zcp.enable = false;
  config.gpu.squash_zcp.repetitions = 100;
  config.gpu.squash_zcp.input.width = 1600; // 1280*5/4, RAW10 format
  config.gpu.squash_zcp.input.height = 720;
  config.gpu.squash_zcp.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.gpu.squash_zcp.output.width = 1280;
  config.gpu.squash_zcp.output.height = 720;
  config.gpu.squash_zcp.output.save = false;
  config.gpu.squash_zcp.output.paths = {
    "output/gpu/squash_zcp/image0000.bggr8",
  };
  // NOTE: Zero copy can use the same kernel; only difference is in the setup of memory at run time.
  config.gpu.squash_zcp.kernel.name = "squash";
  config.gpu.squash_zcp.kernel.width = 320;
  config.gpu.squash_zcp.kernel.height = 360;

  //====================================================================================================================
  // NEON Config

  //==================
  // Debayer config
  config.neon.debayer.name = "neon_debayer";
  config.neon.debayer.enable = false;
  config.neon.debayer.repetitions = 100;
  config.neon.debayer.input.width = 1600; // 1280*5/4, RAW10 format
  config.neon.debayer.input.height = 720;
  config.neon.debayer.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.neon.debayer.output.width = 1280;
  config.neon.debayer.output.height = 720;
  config.neon.debayer.output.save = false;
  config.neon.debayer.output.paths = {
    "output/neon/debayer/image0000.rgb",
  };

  //==================
  // Debayer_half config
  config.neon.debayer_half.name = "neon_debayer_half";
  config.neon.debayer_half.enable = false;
  config.neon.debayer_half.repetitions = 100;
  config.neon.debayer_half.input.width = 1280;
  config.neon.debayer_half.input.height = 720;
  config.neon.debayer_half.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.neon.debayer_half.output.width = 640;
  config.neon.debayer_half.output.height = 360;
  config.neon.debayer_half.output.save = false;
  config.neon.debayer_half.output.paths = {
    "output/neon/debayer_half/image0000.rgb",
  };

  //====================================================================================================================
  // Pipeline Config

  config.pipeline.name = "pipeline";
  config.pipeline.enable = false;
  config.pipeline.repetitions = 100;
  config.pipeline.input.width = 1600; // 1280*5/4, RAW10 format
  config.pipeline.input.height = 720;
  config.pipeline.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.pipeline.rgb_full.kernel = "debayer_img";
  config.pipeline.rgb_full.width = 1280;
  config.pipeline.rgb_full.height = 720;
  config.pipeline.rgb_full.save = false;
  config.pipeline.rgb_full.paths = {
    "output/pipeline/rgb_full_0000.rgb",
  };
  config.pipeline.rgb_half.kernel = "resize";
  config.pipeline.rgb_half.width = 640;
  config.pipeline.rgb_half.height = 360;
  config.pipeline.rgb_half.save = false;
  config.pipeline.rgb_half.paths = {
    "output/pipeline/rgb_half_0000.rgb",
  };
  config.pipeline.grey_full.kernel = "greyscale";
  config.pipeline.grey_full.width = 1280;
  config.pipeline.grey_full.height = 720;
  config.pipeline.grey_full.save = false;
  config.pipeline.grey_full.paths = {
    "output/pipeline/grey_full_0000.grey",
  };
  config.pipeline.grey_half_convert.kernel = "greyscale";
  config.pipeline.grey_half_convert.width = 640;
  config.pipeline.grey_half_convert.height = 360;
  config.pipeline.grey_half_convert.save = false;
  config.pipeline.grey_half_convert.paths = {
    "output/pipeline/grey_half_convert_0000.grey",
  };
  config.pipeline.grey_half_resize.kernel = "resize";
  config.pipeline.grey_half_resize.width = 640;
  config.pipeline.grey_half_resize.height = 360;
  config.pipeline.grey_half_resize.save = false;
  config.pipeline.grey_half_resize.paths = {
    "output/pipeline/grey_half_resize_0000.grey",
  };

  //====================================================================================================================
  // Pipeline Kernel Config

  config.pipeline_kernel.name = "pipeline_kernel";
  config.pipeline_kernel.enable = false;
  config.pipeline_kernel.repetitions = 100;
  config.pipeline_kernel.input.width = 1600; // 1280*5/4, RAW10 format
  config.pipeline_kernel.input.height = 720;
  config.pipeline_kernel.input.paths = {
    "resources/images/debayer/image0000.raw",
  };
  config.pipeline_kernel.rgb_full.kernel = "";
  config.pipeline_kernel.rgb_full.width = 1280;
  config.pipeline_kernel.rgb_full.height = 720;
  config.pipeline_kernel.rgb_full.save = false;
  config.pipeline_kernel.rgb_full.paths = {
    "output/pipeline_kernel/rgb_full_0000.rgb",
  };
  config.pipeline_kernel.rgb_half.kernel = "";
  config.pipeline_kernel.rgb_half.width = 640;
  config.pipeline_kernel.rgb_half.height = 360;
  config.pipeline_kernel.rgb_half.save = false;
  config.pipeline_kernel.rgb_half.paths = {
    "output/pipeline_kernel/rgb_half_0000.rgb",
  };
  config.pipeline_kernel.grey_full.kernel = "";
  config.pipeline_kernel.grey_full.width = 1280;
  config.pipeline_kernel.grey_full.height = 720;
  config.pipeline_kernel.grey_full.save = false;
  config.pipeline_kernel.grey_full.paths = {
    "output/pipeline_kernel/grey_full_0000.grey",
  };
  config.pipeline_kernel.grey_half.kernel = "";
  config.pipeline_kernel.grey_half.width = 640;
  config.pipeline_kernel.grey_half.height = 360;
  config.pipeline_kernel.grey_half.save = false;
  config.pipeline_kernel.grey_half.paths = {
    "output/pipeline_kernel/grey_half_0000.grey",
  };

  config.pipeline_kernel.kernel.name = "pipeline";
  config.pipeline_kernel.kernel.width = 160;
  config.pipeline_kernel.kernel.height = 180;

  //====================================================================================================================
  // Power Config

  // NEON
  config.power_neon.name = "power_neon";
  config.power_neon.enable = false;
  config.power_neon.repetitions = 1000;
  config.power_neon.copies = 5;
  config.power_neon.input.width = 1600;
  config.power_neon.input.height  = 720;
  config.power_neon.input.path = "resources/images/debayer/image0000.raw";

  config.power_neon.output.width = 1280;
  config.power_neon.output.height = 720;

  // ZCP
  config.power_zcp.name = "power_zcp";
  config.power_zcp.enable = false;
  config.power_zcp.repetitions = 1000;
  config.power_zcp.copies = 5;
  config.power_zcp.input.width = 1600;
  config.power_zcp.input.height  = 720;
  config.power_zcp.input.path = "resources/images/debayer/image0000.raw";

  config.power_zcp.output.width = 1280;
  config.power_zcp.output.height = 720;

  // NOTE: Zero copy can use the same kernel; only difference is in the setup of memory at run time.
  config.power_zcp.kernel.name = "debayer_img";
  config.power_zcp.kernel.width = 320;
  config.power_zcp.kernel.height = 360;


  //====================================================================================================================
  // Longevity Config

  // NEON
  config.longevity_neon.name = "longevity_neon";
  config.longevity_neon.enable = true;
  config.longevity_neon.copies = 5;
  config.longevity_neon.fps = 5;
  config.longevity_neon.seconds = 300;
  config.longevity_neon.input.width = 1600;
  config.longevity_neon.input.height  = 720;
  config.longevity_neon.input.path = "resources/images/debayer/image0000.raw";
  config.longevity_neon.output.width = 1280;
  config.longevity_neon.output.height = 720;

  // ZCP
  config.longevity_zcp.name = "longevity_zcp";
  config.longevity_zcp.enable = true;
  config.longevity_zcp.copies = 5;
  config.longevity_zcp.fps = 5;
  config.longevity_zcp.seconds = 300;
  config.longevity_zcp.input.width = 1600;
  config.longevity_zcp.input.height  = 720;
  config.longevity_zcp.input.path = "resources/images/debayer/image0000.raw";
  config.longevity_zcp.output.width = 1280;
  config.longevity_zcp.output.height = 720;
  config.longevity_zcp.kernel.name = "debayer_img";
  config.longevity_zcp.kernel.width = 320;
  config.longevity_zcp.kernel.height = 360;

  //====================================================================================================================

  directory_create("output");
  directory_create("output/gpu");
  directory_create("output/gpu/debayer");
  directory_create("output/gpu/debayer_half");
  directory_create("output/gpu/debayer_img");
  directory_create("output/gpu/debayer_img_half");
  directory_create("output/gpu/debayer_zcp");
  directory_create("output/gpu/debayer_zcp_half");
  directory_create("output/gpu/squash_zcp");
  directory_create("output/neon");
  directory_create("output/neon/debayer");
  directory_create("output/neon/debayer_half");
  directory_create("output/pipeline");
  directory_create("output/pipeline_kernel");

  Evaluator evaluator(config);
  try {
    evaluator.run();
    evaluator.print();
  } catch (EvaluatorException& e){
    std::cerr<<e.what()<<std::endl;
  }

}