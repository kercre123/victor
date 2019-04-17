/*
 * File: imageProcessorTests.h
 *
 * Author: Al Chaussee
 * Created: 4/15/19
 *
 * Description: Test suite for Processors and ProcessorRunners
 *
 * Copyright: Anki, Inc. 2019
 */

// Used in place of gTest/gTest.h directly to suppress warnings in the header
#include "util/helpers/includeGTest.h"

#include "coretech/vision/engine/iImageProcessor.h"
#include "coretech/vision/engine/imageProcessorRunner.h"

#include "coretech/common/shared/types.h"

#include <unistd.h>

using namespace Anki::Vision;

namespace {

// Types of test processors
enum VisionMode
{
  Simple,
  Simple2,
  Async
};

// Simple processor with input and output containing one bool
class InputSimple : public Input<VisionMode, Simple>
{
public:
  InputSimple(bool b) : _b(b) { }
  bool _b = false;
};

class OutputSimple : public Output<VisionMode, Simple>
{
public:
  OutputSimple(bool b) : _b(b) { }
  bool _b = false;
};

class ProcessorSimple : public Processor<VisionMode, Simple, InputSimple>
{
public:
  ProcessorSimple() : Processor("Simple") { }

  mutable bool hasProcessed = false;

  bool IsInputAsync() const { return HasAsyncInput(); } // HasAsyncInput is protected

private:
  virtual std::shared_ptr<Output<VisionMode, Simple>> Process(InputSimple* input) const override
  {
    hasProcessed = true;

    std::shared_ptr<Output<VisionMode, Simple>> output = std::make_shared<OutputSimple>(input->_b);
    return output;
  }
};

// Second simple processor with input and output containing one string
class InputSimple2 : public Input<VisionMode, Simple2>
{
public:
  InputSimple2(std::string s) : _s(s) { }
  std::string _s = "inputSimple2";
};

class OutputSimple2 : public Output<VisionMode, Simple2>
{
public:
  OutputSimple2(std::string s) : _s(s) { }
  std::string _s = "outputSimple2";
};

class ProcessorSimple2 : public Processor<VisionMode, Simple2, InputSimple2>
{
public:
  ProcessorSimple2() : Processor("Simple2") { }

  mutable bool hasProcessed = false;

  bool IsInputAsync() const { return HasAsyncInput(); } // HasAsyncInput is protected

private:
  virtual std::shared_ptr<Output<VisionMode, Simple2>> Process(InputSimple2* input) const override
  {
    hasProcessed = true;

    std::shared_ptr<Output<VisionMode, Simple2>> output = std::make_shared<OutputSimple2>(input->_s);
    return output;
  }
};

// More complex processor that has asynchronous input
class InputAsync : public Input<VisionMode, Async>
{
public:
  int _g = -1;
};

class OutputAsync : public Output<VisionMode, Async>
{
public:
  OutputAsync(int g) : _g(g) { }
  int _g = 0;
};

class ProcessorAsync : public Processor<VisionMode, Async, InputAsync>
{
public:
  ProcessorAsync() : Processor("Async"), _g(-1) {}
  bool IsInputAsync() const { return HasAsyncInput(); } // HasAsyncInput is protected
  void SetAsyncInput(int g)
  {
    _mutex.lock();
    _g = g;
    _ready = true;
    _mutex.unlock();

    _cond.notify_all();
  }

  mutable bool hasProcessed = false;

protected:

  virtual bool HasAsyncInput() const override { return true; }

  virtual bool AsyncAcquireInput() override
  {
    // Block waiting for input to be set by SetAsyncInput
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]{ return _ready || _stop; });

    if(_stop)
    {
      return false;
    }

    _ready = false;

    // Create new input if needed
    if(_input == nullptr)
    {
      _input.reset(new InputAsync());
    }

    // Set input
    _input->_g = _g;

    return true;
  }

  virtual void AsyncWake() override
  {
    _mutex.lock();
    _stop = true;
    _mutex.unlock();

    _cond.notify_all();
  }

private:
  virtual std::shared_ptr<Output<VisionMode, Async>> Process(InputAsync* input) const override
  {
    hasProcessed = true;

    std::shared_ptr<Output<VisionMode, Async>> output = std::make_shared<OutputAsync>(input->_g);
    return output;
  }

  std::mutex _mutex;
  std::condition_variable _cond;
  bool _ready = false;
  bool _stop = false;

  // Stores asynchronous input
  std::atomic<int> _g;
};

// ImageProcessorRunnerOutput for our specific ImageProcessorRunner that contains
// output from processors of type VisionMode::Simple, Simple2, Async
class IPROutput : public ImageProcessorRunnerOutput
{
public:
  virtual void AddOutput(std::shared_ptr<OutputBase> output) override final
  {
    if(output == nullptr)
    {
      return;
    }

    outputSet = true;

    const VisionMode mode = static_cast<VisionMode>(output->GetMode());
    switch(mode)
    {
      case VisionMode::Simple:
        outputSimple = std::static_pointer_cast<OutputSimple>(output);
        break;
      case VisionMode::Simple2:
        outputSimple2 = std::static_pointer_cast<OutputSimple2>(output);
        break;
      case VisionMode::Async:
        outputAsync = std::static_pointer_cast<OutputAsync>(output);
        break;
    }
  }

  virtual bool HasOutput() const override final
  {
    return outputSet;
  }

  std::shared_ptr<OutputSimple> outputSimple = nullptr;
  std::shared_ptr<OutputSimple2> outputSimple2 = nullptr;
  std::shared_ptr<OutputAsync> outputAsync = nullptr;

private:
  bool outputSet = false;
};

}

// Single processor running in a thread asynchronously
GTEST_TEST(ImageProcessor, OneProcessorAsync)
{
  // Check that a newly constructed processor and processor runner have
  // correct state
  std::shared_ptr<ProcessorSimple> tp1 = std::make_shared<ProcessorSimple>();
  ImageProcessorRunner ipr;

  ASSERT_EQ(VisionMode::Simple, tp1->GetMode());
  ASSERT_EQ(false, tp1->IsInputAsync());
  ASSERT_EQ(false, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);
  ASSERT_EQ(0, ipr.GetNumProcessors());

  // Add the processor to the runner
  ipr.AddProcessor(tp1);

  ASSERT_EQ(1, ipr.GetNumProcessors());
  ASSERT_EQ(false, tp1->IsRunning());

  // Start all processors
  ipr.Start();

  tp1->WaitForStart();

  // Processor should be running but should not have processed anything
  // yet due to no input
  ASSERT_EQ(true, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Create input and output
  auto ti1 = std::make_shared<InputSimple>(true);
  ImageProcessorRunnerInput input;
  IPROutput output;

  ASSERT_EQ(0, input.GetInputs().size());

  // Try to update with empty input
  ipr.Update(input, &output);

  // Should be no output and nothing should have been processed
  ASSERT_EQ(false, output.HasOutput());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Add the input...
  input.AddInput(ti1);
  ASSERT_EQ(1, input.GetInputs().size());

  //...and process
  ipr.Update(input, &output);

  // Still no output as the processor is just now processing ti1 (asynchronous)
  ASSERT_EQ(false, output.HasOutput());

  // Wait for the processor to finish
  tp1->WaitUntilDone();

  // Update processor runner with empty input in order to get
  // the previous output
  ImageProcessorRunnerInput emptyInput;
  emptyInput.AddInput(nullptr);
  ipr.Update(emptyInput, &output);

  ASSERT_EQ(true, output.HasOutput());
  ASSERT_NE(nullptr, output.outputSimple);
  ASSERT_EQ(ti1->_b, output.outputSimple->_b);

  ipr.Stop();
}

// Single processor running synchronously
GTEST_TEST(ImageProcessor, OneProcessorSync)
{
  // Check that a newly constructed processor and processor runner have
  // correct state
  std::shared_ptr<ProcessorSimple> tp1 = std::make_shared<ProcessorSimple>();
  ImageProcessorRunner ipr;

  ASSERT_EQ(VisionMode::Simple, tp1->GetMode());
  ASSERT_EQ(false, tp1->IsInputAsync());
  ASSERT_EQ(false, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);
  ASSERT_EQ(0, ipr.GetNumProcessors());

  // Add the processor to the runner
  ipr.AddProcessor(tp1);

  ASSERT_EQ(1, ipr.GetNumProcessors());
  ASSERT_EQ(false, tp1->IsRunning());

  // Make processing synchronous
  ipr.SetIsSynchronous(true);

  ASSERT_EQ(true, tp1->IsRunningSynchronously());

  // Start all processors
  ipr.Start();

  // Processor should be running but should not have processed anything
  // yet due to no input
  ASSERT_EQ(true, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Create input and output
  auto ti1 = std::make_shared<InputSimple>(true);
  ImageProcessorRunnerInput input;
  IPROutput output;

  ASSERT_EQ(0, input.GetInputs().size());

  // Try to update with empty input
  ipr.Update(input, &output);

  // Should be no output and nothing should have been processed
  ASSERT_EQ(false, output.HasOutput());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Add the input and process
  input.AddInput(ti1);
  ipr.Update(input, &output);

  // No output yet, but the processor should have run
  ASSERT_EQ(false, output.HasOutput());
  ASSERT_EQ(true, tp1->hasProcessed);

  // Update processor runner with empty input in order to get
  // the previous output
  ImageProcessorRunnerInput emptyInput;
  emptyInput.AddInput(nullptr);
  ipr.Update(emptyInput, &output);

  ASSERT_EQ(true, output.HasOutput());
  ASSERT_NE(nullptr, output.outputSimple);
  ASSERT_EQ(ti1->_b, output.outputSimple->_b);

  ipr.Stop();
}

// Single processor with asynchronous input
GTEST_TEST(ImageProcessor, OneProcessorAsyncInput)
{
  // Check that a newly constructed processor and processor runner have
  // correct state
  std::shared_ptr<ProcessorAsync> tp1 = std::make_shared<ProcessorAsync>();
  ImageProcessorRunner ipr;

  ASSERT_EQ(VisionMode::Async, tp1->GetMode());
  ASSERT_EQ(true, tp1->IsInputAsync());
  ASSERT_EQ(false, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);
  ASSERT_EQ(0, ipr.GetNumProcessors());

  // Add the processor to the runner
  ipr.AddProcessor(tp1);

  ASSERT_EQ(1, ipr.GetNumProcessors());
  ASSERT_EQ(false, tp1->IsRunning());

  // Start all processors
  ipr.Start();

  // Processor should be running but should not have processed anything
  // yet due to its async input blocking
  ASSERT_EQ(true, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Create input and output, no matter what this input should do nothing
  // as the processor gets its own input asynchronously
  auto ti1 = std::make_shared<InputAsync>();
  ImageProcessorRunnerInput input;
  IPROutput output;

  ASSERT_EQ(0, input.GetInputs().size());

  // Try to update with empty input
  ipr.Update(input, &output);

  // Should be no output and nothing should have been processed
  ASSERT_EQ(false, output.HasOutput());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Add the input and process
  input.AddInput(ti1);
  ipr.Update(input, &output);

  // Still shouldn't have output and the processor won't have run
  ASSERT_EQ(false, output.HasOutput());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Trigger processor's async input...
  const int inputValue = 10;
  tp1->SetAsyncInput(inputValue);

  // ...and wait until done
  tp1->WaitUntilDone();

  ASSERT_EQ(true, tp1->hasProcessed);

  // Update processor runner with empty input in order to get
  // the previous output
  ImageProcessorRunnerInput emptyInput;
  emptyInput.AddInput(nullptr);
  ipr.Update(emptyInput, &output);

  ASSERT_EQ(true, output.HasOutput());
  ASSERT_NE(nullptr, output.outputAsync);
  ASSERT_EQ(inputValue, output.outputAsync->_g);

  ipr.Stop();
}

// Single processor with asynchronous input running synchronously
GTEST_TEST(ImageProcessor, OneProcessorAsyncInputRunSync)
{
  // Check that a newly constructed processor and processor runner have
  // correct state
  std::shared_ptr<ProcessorAsync> tp1 = std::make_shared<ProcessorAsync>();
  ImageProcessorRunner ipr;

  ASSERT_EQ(VisionMode::Async, tp1->GetMode());
  ASSERT_EQ(true, tp1->IsInputAsync());
  ASSERT_EQ(false, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);
  ASSERT_EQ(0, ipr.GetNumProcessors());

  // Add the processor to the runner
  ipr.AddProcessor(tp1);

  ASSERT_EQ(1, ipr.GetNumProcessors());
  ASSERT_EQ(false, tp1->IsRunning());

  // Make synchronous
  ipr.SetIsSynchronous(true);

  ASSERT_EQ(true, tp1->IsRunningSynchronously());

  // Start all processors
  ipr.Start();

  // Processor should be running but should not have processed anything
  // yet due to running synchronously
  ASSERT_EQ(true, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Create input and output, no matter what this input should do nothing
  // as the processor gets its own input asynchronously
  auto ti1 = std::make_shared<InputAsync>();
  ImageProcessorRunnerInput input;
  IPROutput output;

  ASSERT_EQ(0, input.GetInputs().size());

  // Trigger processor's async input
  // We are running synchronously so we need to do this
  // before Update() otherwise Update() will block this thread
  // as the processor waits for async input
  const int inputValue = 10;
  tp1->SetAsyncInput(inputValue);

  // input is empty
  ipr.Update(input, &output);

  ASSERT_EQ(true, tp1->hasProcessed);

  // Update once more to get output from previous Update
  tp1->SetAsyncInput(0);
  ipr.Update(input, &output);

  ASSERT_EQ(true, output.HasOutput());
  ASSERT_NE(nullptr, output.outputAsync);
  ASSERT_EQ(inputValue, output.outputAsync->_g);

  ipr.Stop();
}

// Multi-processor test
GTEST_TEST(ImageProcessor, MultiProcessor)
{
  std::shared_ptr<ProcessorSimple> tp1 = std::make_shared<ProcessorSimple>();
  ImageProcessorRunner ipr;

  ASSERT_EQ(0, ipr.GetNumProcessors());

  // Add the processor to the runner
  ipr.AddProcessor(tp1);

  ASSERT_EQ(1, ipr.GetNumProcessors());
  ASSERT_EQ(false, tp1->IsRunning());

  // Start all processors
  ipr.Start();

  tp1->WaitForStart();

  // Processor should be running but should not have processed anything
  ASSERT_EQ(true, tp1->IsRunning());
  ASSERT_EQ(false, tp1->hasProcessed);

  // Create input and output
  auto inputSimple = std::make_shared<InputSimple>(true);
  ImageProcessorRunnerInput input;
  IPROutput output;

  ASSERT_EQ(0, input.GetInputs().size());

  // Add the input and process
  input.AddInput(inputSimple);
  ipr.Update(input, &output);

  // Still no output as the processor is just now processing ti1 (asynchronous)
  ASSERT_EQ(false, output.HasOutput());

  // Wait for the processor to finish
  tp1->WaitUntilDone();

  // Add another processor
  std::shared_ptr<ProcessorSimple2> tp2 = std::make_shared<ProcessorSimple2>();
  ipr.AddProcessor(tp2);
  ipr.Start();

  tp2->WaitForStart();

  // Update both processors
  ImageProcessorRunnerInput input2;
  IPROutput output2;
  auto input2Simple = std::make_shared<InputSimple>(false);
  auto input2Simple2 = std::make_shared<InputSimple2>("42");
  input2.AddInput(input2Simple);
  input2.AddInput(input2Simple2);

  ipr.Update(input2, &output2);

  // Should have output only from the first processor, ProcessorSimple
  ASSERT_EQ(true, output2.HasOutput());
  ASSERT_NE(nullptr, output2.outputSimple);
  ASSERT_EQ(nullptr, output2.outputSimple2);
  ASSERT_EQ(nullptr, output2.outputAsync);
  ASSERT_EQ(inputSimple->_b, output2.outputSimple->_b);

  // Add a third processor
  std::shared_ptr<ProcessorAsync> tp3 = std::make_shared<ProcessorAsync>();
  ipr.AddProcessor(tp3);
  ipr.Start();

  // Trigger tp3's async input
  const int inputValue = 99;
  tp3->SetAsyncInput(inputValue);

  // Wait for all processors to complete
  tp1->WaitUntilDone();
  tp2->WaitUntilDone();
  tp3->WaitUntilDone();

  ImageProcessorRunnerInput input3;
  IPROutput output3;
  input3.AddInput(nullptr);
  ipr.Update(input3, &output3);

  ASSERT_EQ(true, output3.HasOutput());
  ASSERT_NE(nullptr, output3.outputSimple);
  ASSERT_NE(nullptr, output3.outputSimple2);
  ASSERT_NE(nullptr, output3.outputAsync);
  ASSERT_EQ(input2Simple->_b, output3.outputSimple->_b);
  ASSERT_EQ(input2Simple2->_s, output3.outputSimple2->_s);
  ASSERT_EQ(inputValue, output3.outputAsync->_g);

  ipr.Stop();
}
