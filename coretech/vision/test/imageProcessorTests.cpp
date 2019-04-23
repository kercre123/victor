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
#include "coretech/vision/engine/imageCacheProvider.h"

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

GTEST_TEST(ImageCacheProvider, CacheCleaning)
{
  ImageCacheProvider icp;

  ASSERT_EQ(icp.GetNumCaches(), 0);

  u8 bayerData[6][40] = {{0}};

  bayerData[2][5] = 255; //b
  bayerData[2][6] = 0;   //g
  bayerData[3][5] = 0;   //g
  bayerData[3][6] = 0;   //r

  bayerData[2][15] = 0;   //b
  bayerData[2][16] = 255; //g
  bayerData[3][15] = 255; //g
  bayerData[3][16] = 0;   //r

  bayerData[2][25] = 0;   //b
  bayerData[2][26] = 0;   //g
  bayerData[3][25] = 0;   //g
  bayerData[3][26] = 255; //r

  const s32 nrows = 6;
  const s32 ncols = 32;
  const Anki::TimeStamp_t t = 1;
  const s32 id = 2;
  ImageBuffer buffer(&(bayerData[0][0]), nrows, ncols, ImageEncoding::BAYER, t, id);
  buffer.SetSensorResolution(nrows, ncols);

  // Resetting with the first buffer should result in one cache being added to the icp
  icp.Reset(buffer);
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Resetting with the buffer again will result in the previous cache being cleaned up
  // due to no one using one of its images and a new cache with buffer being added
  // So we should still have just one cache
  icp.Reset(buffer);
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Get some kind of image from the icp, this will result in
  // a cache being "in use" as some one has a reference (shared_ptr) to one of its
  // images
  auto img = icp.GetGray();

  // Reset with another buffer and we should now have two caches
  icp.Reset(buffer);
  ASSERT_EQ(icp.GetNumCaches(), 2);

  // Clean up unused caches which should remove the most recent added cache
  // as no one is using its images
  icp.CleanCaches();
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Reset our shared image pointer which will result in
  // the last cache becoming unused
  img.reset();

  // Clean up the now unused last cache
  icp.CleanCaches();
  ASSERT_EQ(icp.GetNumCaches(), 0);
}

// Test class to spawn a thread the runs func when triggered
class TestThread
{
public:
  TestThread(const std::function<void(void)>& func)
  : _stop(false)
  , _processing(false)
  , _func(func)
  {
    _thread = std::thread(&TestThread::Loop, this);
  };

  void Loop()
  {
    while(!_stop)
    {
      std::unique_lock<std::mutex> lock(_triggerMutex);
      _processing = false;

      _cond.wait(lock, [this]{ return _trigger; });
      _trigger = false;

      _func();
    }
  }

  void Stop()
  {
    _stop = true;

    Trigger();

    if(_thread.joinable())
    {
      _thread.join();
    }
  }

  void Trigger()
  {
    {
      std::unique_lock<std::mutex> lock(_triggerMutex);
      _trigger = true;
      _processing = true;
    }
    _cond.notify_all();
  }

  bool IsProcessing()
  {
    std::lock_guard<std::mutex> lock(_triggerMutex);
    return _processing;
  }

  void WaitForProcessing()
  {
    while(IsProcessing())
    {
      usleep(10000);
    }
  }

  std::atomic<bool>         _stop;
  std::thread               _thread;
  std::mutex                _triggerMutex;
  std::condition_variable   _cond;
  bool                      _trigger    = false;
  bool                      _processing = false;
  std::function<void(void)> _func;
};


GTEST_TEST(ImageCacheProvider, MultipleCaches)
{
  // Create ImageCacheProvider
  ImageCacheProvider icp;

  // Atomic variables to control state/get output of fake processing threads
  std::atomic<bool> proc1Wait(true);
  std::atomic<bool> proc1GotImage(false);
  std::atomic<int> proc1Id(-1);
  std::atomic<ImageCache::GetType> proc1GetType;

  std::atomic<bool> proc2Wait(true);
  std::atomic<bool> proc2GotImage(false);
  std::atomic<int> proc2Id(-2);
  std::atomic<ImageCache::GetType> proc2GetType;

  // Fake processing thread 1 gets a gray image from the icp
  auto getImg = [&icp, &proc1Wait, &proc1GotImage, &proc1Id, &proc1GetType]() -> void {
                  ImageCache::GetType getType = proc1GetType;
                  auto img = icp.GetGray(ImageCache::GetDefaultImageCacheSize(), &getType);
                  proc1GetType = getType;
                  proc1GotImage = true;

                  if(img != nullptr)
                  {
                    proc1Id = img->GetImageId();

                    while(proc1Wait)
                    {
                      usleep(10000);
                    }
                  }
                  else
                  {
                    proc1Id = -1;
                    usleep(10000);
                  }
                  proc1GotImage = false;
                };
  TestThread proc1(getImg);

  // Fake processing thread 2 gets a rgb image from the icp
  auto getImg2 = [&icp, &proc2Wait, &proc2GotImage, &proc2Id, &proc2GetType]() -> void {
                   ImageCache::GetType getType = proc2GetType;
                   auto img = icp.GetRGB(ImageCache::GetDefaultImageCacheSize(), &getType);
                   proc2GetType = getType;
                   proc2GotImage = true;

                   if(img != nullptr)
                   {
                     proc2Id = img->GetImageId();

                     while(proc2Wait)
                     {
                       usleep(10000);
                     }
                   }
                   else
                   {
                     proc2Id = -2;
                     usleep(10000);
                   }
                   proc2GotImage = false;
                 };
  TestThread proc2(getImg2);


  // Setup fake camera thread which resets the icp with a new image
  // when triggered
  s32 id = 0;
  auto setImg = [&icp, &id]() -> void {
                  u8 bayerData[6][40] = {{0}};
                  const s32 nrows = 6;
                  const s32 ncols = 32;
                  const Anki::TimeStamp_t t = 1;
                  ImageBuffer buffer(&(bayerData[0][0]), nrows, ncols, ImageEncoding::BAYER, t, id++);
                  buffer.SetSensorResolution(nrows, ncols);
                  icp.Reset(buffer);
                };
  TestThread camera(setImg);

  // Should have no caches
  ASSERT_EQ(icp.GetNumCaches(), 0);

  // Trigger the camera thread to add an image to the icp
  camera.Trigger();
  // Need to wait for it to finish
  camera.WaitForProcessing();

  // We should have one cache now
  ASSERT_EQ(icp.GetNumCaches(), 1);
  ASSERT_EQ(icp.GetGray()->GetImageId(), id-1);

  // Add another image to the cache
  camera.Trigger();
  camera.WaitForProcessing();

  // The previous cache will be cleaned as no one was using it
  // so we will again only have one cache but it will be a different image
  ASSERT_EQ(icp.GetNumCaches(), 1);
  ASSERT_EQ(icp.GetGray()->GetImageId(), id-1);

  // Forcibly clean the caches
  icp.CleanCaches();

  // No caches left as no one is using them
  ASSERT_EQ(icp.GetNumCaches(), 0);

  // Add image
  camera.Trigger();
  camera.WaitForProcessing();

  // Trigger proc1 to start and wait for it to actually wake up and acquire an image
  // from the icp
  proc1.Trigger();
  while(!proc1GotImage)
  {
    usleep(10000);
  }

  // Still only one cache
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Trigger proc2 to start and wait for it to actually wake up and acquire an image
  // from the icp
  proc2.Trigger();
  while(!proc2GotImage)
  {
    usleep(10000);
  }

  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Add a second image
  camera.Trigger();
  camera.WaitForProcessing();

  // There should be 2 caches now that both processors are using the first cache
  ASSERT_EQ(icp.GetNumCaches(), 2);
  ASSERT_EQ(proc1Id, id-2);
  ASSERT_EQ(proc2Id, id-2);
  // Proc1 will have to compute a new entry while Proc2 will resize into the now existing
  // gray entry from proc1 (special case of ImageCache, valid entry at size but we wanted color and
  // the cache was reset with color so we will resize from the original color data instead of using the
  // existing gray)
  ASSERT_EQ(proc1GetType, ImageCache::GetType::NewEntry);
  ASSERT_EQ(proc2GetType, ImageCache::GetType::ResizeIntoExisting);

  // Clean the caches
  icp.CleanCaches();

  // There should be only one cache and it should contain
  // the image that both processors are using
  ASSERT_EQ(icp.GetNumCaches(), 1);
  ASSERT_EQ(icp.GetGray()->GetImageId(), proc1Id);
  ASSERT_EQ(icp.GetGray()->GetImageId(), proc2Id);

  // Have proc1 complete processing so it releases
  // its hold on the image
  proc1Wait = false;
  proc1.WaitForProcessing();
  proc1Wait = true;

  // The cache should still exist as proc2 is still holding the image
  ASSERT_EQ(icp.GetNumCaches(), 1);
  icp.CleanCaches();
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Have proc2 complete
  proc2Wait = false;
  proc2.WaitForProcessing();
  proc2Wait = true;

  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Clean the caches now that nothing should be using them
  icp.CleanCaches();

  ASSERT_EQ(icp.GetNumCaches(), 0);

  // Add image
  camera.Trigger();
  camera.WaitForProcessing();

  // Trigger proc1 to get an image
  proc1.Trigger();
  while(!proc1GotImage)
  {
    usleep(10000);
  }

  // Add second image
  camera.Trigger();
  camera.WaitForProcessing();

  // Trigger proc2 to get the second image
  proc2.Trigger();
  while(!proc2GotImage)
  {
    usleep(10000);
  }

  // Add a third image
  camera.Trigger();
  camera.WaitForProcessing();

  // Should have 3 caches, and each processor should
  // have gotten the right image
  ASSERT_EQ(icp.GetNumCaches(), 3);
  ASSERT_EQ(proc1Id, id-3);
  ASSERT_EQ(proc2Id, id-2);
  ASSERT_EQ(icp.GetGray()->GetImageId(), id-1);

  // Clean caches, should remove the most recently added cache
  // as it was unused
  icp.CleanCaches();

  ASSERT_EQ(icp.GetNumCaches(), 2);
  ASSERT_EQ(proc1Id, id-3);
  ASSERT_EQ(proc2Id, id-2);
  ASSERT_EQ(icp.GetGray()->GetImageId(), id-2);

  // Have proc1 finish processing
  proc1Wait = false;
  proc1.WaitForProcessing();

  icp.CleanCaches();

  // Just one cache left which is held by proc2
  ASSERT_EQ(icp.GetNumCaches(), 1);

  // Have proc2 finish processing
  proc2Wait = false;
  proc2.WaitForProcessing();

  icp.CleanCaches();

  // No caches left
  ASSERT_EQ(icp.GetNumCaches(), 0);

  // Stop threads
  proc1.Stop();
  proc2.Stop();
  camera.Stop();
}

GTEST_TEST(ImageCacheProvider, ParallelReads)
{
  // Create ImageCacheProvider
  ImageCacheProvider icp;

  // Atomic variables to control state/get output of fake processing threads
  std::atomic<bool> proc1Wait(true);
  std::atomic<bool> proc1GotImage(false);
  std::atomic<int> proc1Id(-1);
  std::atomic<ImageCache::GetType> proc1GetType;

  std::atomic<bool> proc2Wait(true);
  std::atomic<bool> proc2GotImage(false);
  std::atomic<int> proc2Id(-2);
  std::atomic<ImageCache::GetType> proc2GetType;

  // Both fake processing threads are setup to request the same type of image in order to test
  // multiple threads requesting the same thing from the cache
  auto getImg = [&icp, &proc1Wait, &proc1GotImage, &proc1Id, &proc1GetType]() -> void {
                  ImageCache::GetType getType = proc1GetType;
                  auto img = icp.GetGray(ImageCache::GetDefaultImageCacheSize(), &getType);
                  proc1GetType = getType;
                  proc1GotImage = true;

                  if(img != nullptr)
                  {
                    proc1Id = img->GetImageId();

                    while(proc1Wait)
                    {
                      usleep(10000);
                    }
                  }
                  else
                  {
                    proc1Id = -1;
                    usleep(10000);
                  }
                  proc1GotImage = false;
                };
  TestThread proc1(getImg);

  auto getImg2 = [&icp, &proc2Wait, &proc2GotImage, &proc2Id, &proc2GetType]() -> void {
                   ImageCache::GetType getType = proc2GetType;
                   auto img = icp.GetGray(ImageCache::GetDefaultImageCacheSize(), &getType);
                   proc2GetType = getType;
                   proc2GotImage = true;

                   if(img != nullptr)
                   {
                     proc2Id = img->GetImageId();

                     while(proc2Wait)
                     {
                       usleep(10000);
                     }
                   }
                   else
                   {
                     proc2Id = -2;
                     usleep(10000);
                   }
                   proc2GotImage = false;
                 };
  TestThread proc2(getImg2);


  // Setup fake camera thread which resets the icp with a new image
  // when triggered
  s32 id = 0;
  auto setImg = [&icp, &id]() -> void {
                  u8 bayerData[6][40] = {{0}};
                  const s32 nrows = 6;
                  const s32 ncols = 32;
                  const Anki::TimeStamp_t t = 1;
                  ImageBuffer buffer(&(bayerData[0][0]), nrows, ncols, ImageEncoding::BAYER, t, id++);
                  buffer.SetSensorResolution(nrows, ncols);
                  icp.Reset(buffer);
                };
  TestThread camera(setImg);

  // Add image
  camera.Trigger();
  camera.WaitForProcessing();

  // Trigger both processors to get an image from the icp
  proc1.Trigger();
  proc2.Trigger();

  // Wait for both the processors to acquire the image
  while(!proc1GotImage)
  {
    usleep(10000);
  }

  while(!proc2GotImage)
  {
    usleep(10000);
  }

  // Both processors should have the same image
  ASSERT_EQ(icp.GetNumCaches(), 1);
  ASSERT_EQ(proc1Id, id-1);
  ASSERT_EQ(proc2Id, id-1);

  // This test is intended to test what happens when two things simultaneously try to access the same
  // sized image from the ICP. Due to the asynchronous nature of this test there are two possible outcome paths.
  // Either Proc1 asks for the image first and the conversion runs on its thread. Meanwhile Proc2 asks for the same
  // image which should cause it to block due to the conversion that is already in progress on Proc1's thread.
  // Proc1 will have computed a NewEntry while Proc2 will have gotten a FullyCached image.
  // The second outcome is the opposite of the first in which Proc2 manages to ask for the image before Proc1
  if(proc1GetType == ImageCache::GetType::NewEntry)
  {
    ASSERT_EQ(proc1GetType, ImageCache::GetType::NewEntry);
    ASSERT_EQ(proc2GetType, ImageCache::GetType::FullyCached);
  }
  else if(proc2GetType == ImageCache::GetType::NewEntry)
  {
    ASSERT_EQ(proc2GetType, ImageCache::GetType::NewEntry);
    ASSERT_EQ(proc1GetType, ImageCache::GetType::FullyCached);
  }
  else
  {
    // Some unexpected result from ImageCacheProvider.Get()
    PRINT_NAMED_ERROR("ImageProcessorTests.ImageCacheProvider.ParallelReads",
                      "Unexpected ImageCache::GetType from parallel processors %d and %d",
                      (int)(proc1GetType.load()),
                      (int)(proc2GetType.load()));
    ASSERT_TRUE(false);
  }

  // Stop the threads
  proc1Wait = false;
  proc2Wait = false;
  proc1.Stop();
  proc2.Stop();
  camera.Stop();
}
