/*
 * File: imageProcessorRunner.cpp
 *
 * Author: Al Chaussee
 * Created: 4/15/19
 *
 * Description: Container class for managing and running generic Processors
 *
 * Copyright: Anki, Inc. 2019
 */

#include "coretech/vision/engine/imageProcessorRunner.h"

#include "coretech/vision/engine/iImageProcessor.h"

namespace Anki {
namespace Vision {

// Helper macro to call a function on all processors
#define CALL_PROCESSORS(procs, func) {          \
  for(auto proc : procs)                        \
  {                                             \
    proc->func;                                 \
  }                                             \
}

void ImageProcessorRunner::AddProcessor(std::shared_ptr<ProcessorBase> processor)
{
  if(processor->HasAsyncInput())
  {
    _asyncProcessors.insert(processor);
  }
  else
  {
    _processors.insert(processor);
  }
}

void PopulateOutput(const ProcessorBase::UpdateReturn& procOutput,
                    uint32_t procMode,
                    ImageProcessorRunnerOutput* output)
{
  if(output != nullptr)
  {
    output->AddOutput(procOutput.output);
  }

  if(procOutput.notifiedOfInput)
  {
    output->AddProcessorToNotifiedSet(procMode);
  }

  if(procOutput.stillProcessingPrevInput)
  {
    output->AddProcessorToStillProcessingSet(procMode);
  }
}

void ImageProcessorRunner::Update(const ImageProcessorRunnerInput& input, ImageProcessorRunnerOutput* output)
{
  // Try to update each processor with each input, will do nothing
  // if the input is not meant for the processor
  for(auto processor : _processors)
  {
    for(auto in : input.GetInputs())
    {
      auto procOutput = processor->Update(in);
      PopulateOutput(procOutput, processor->GetMode(), output);
    }
  }

  for(auto asyncProcessor : _asyncProcessors)
  {
    // Processors with async input only need to be updated in order
    // to get output. Output may be "dropped" from these processors if they
    // run faster that the runner's update rate
    auto procOutput = asyncProcessor->Update(nullptr);
    PopulateOutput(procOutput, asyncProcessor->GetMode(), output);
  }
}

void ImageProcessorRunner::Start()
{
  CALL_PROCESSORS(_processors, Start());
  CALL_PROCESSORS(_asyncProcessors, Start());
}

void ImageProcessorRunner::Stop()
{
  CALL_PROCESSORS(_processors, Stop());
  CALL_PROCESSORS(_asyncProcessors, Stop());
}

void ImageProcessorRunner::SetIsSynchronous(bool isSynchronous)
{
  CALL_PROCESSORS(_processors, SetIsSynchronous(isSynchronous));
  CALL_PROCESSORS(_asyncProcessors, SetIsSynchronous(isSynchronous));
}

}
}
