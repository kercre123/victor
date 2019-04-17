/*
 * File: imageProcessorRunner.h
 *
 * Author: Al Chaussee
 * Created: 4/15/19
 *
 * Description: Container class for managing and running generic Processors
 *
 * Copyright: Anki, Inc. 2019
 */

#include "coretech/vision/engine/iImageProcessor_fwd.h"
#include <set>

namespace Anki {
namespace Vision {

// Container of inputs for any number of processors
class ImageProcessorRunnerInput
{
public:
  void AddInput(std::shared_ptr<InputBase> input) { _inputs.insert(input); }
  const std::set<std::shared_ptr<InputBase>>& GetInputs() const { return _inputs; }

private:
  std::set<std::shared_ptr<InputBase>> _inputs;
};

// Partial abstract container class for outputs from any number of processors
// Similar to struct ProcessorBase::UpdateReturn
class ImageProcessorRunnerOutput
{
public:
  // Implementation of AddOutput is specific to the types of processors and their outputs
  // that any given ImageProcessorRunner contains. Typically this is due to the need to
  // downcast output to a derived class with which the owner of the ImageProcessorRunner
  // knows how to use
  // See imageProcessorTests.cpp for example
  virtual void AddOutput(std::shared_ptr<OutputBase> output) = 0;
  virtual bool HasOutput() const = 0;

  void AddProcessorToNotifiedSet(uint32_t p) { _processorsNotified.insert(p); }
  const std::set<uint32_t>& GetProcessorsNotifiedSet() const { return _processorsNotified; }

  void AddProcessorToStillProcessingSet(uint32_t p) { _procsStillProcessingPrevInput.insert(p); }
  const std::set<uint32_t>& GetProcStillProcessingPrevInputSet() const { return _procsStillProcessingPrevInput; }

protected:
  std::set<uint32_t> _processorsNotified;
  std::set<uint32_t> _procsStillProcessingPrevInput;
};

// Container to manage and run processors
class ImageProcessorRunner
{
public:
  ImageProcessorRunner() = default;
  ~ImageProcessorRunner() { Stop(); }

  // Add a new processor to the runner
  void AddProcessor(std::shared_ptr<ProcessorBase> processor);

  // Starts all processors that are not already running
  void Start();

  // Stops all processors that are not already stopped
  void Stop();

  // Sets all processors to run synchronously or not
  void SetIsSynchronous(bool isSynchronous);

  // Updates all processors with a container of inputs and returns back appropriate outputs
  void Update(const ImageProcessorRunnerInput& input, ImageProcessorRunnerOutput* output);

  size_t GetNumProcessors() const { return (_processors.size() + _asyncProcessors.size()); }

private:
  std::set<std::shared_ptr<ProcessorBase>> _processors;
  std::set<std::shared_ptr<ProcessorBase>> _asyncProcessors;
};

}
}
