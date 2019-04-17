/*
 * File: iImageProcessor_fwd.h
 *
 * Author: Al Chaussee
 * Created: 4/15/19
 *
 * Description: Generic threaded Processor class with Input and Outputs linked via template Enum values
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Vision_iImageProcessor_fwd_H__
#define __Anki_Vision_iImageProcessor_fwd_H__

#include <thread>
#include <mutex>
#include <set>
#include <string>

namespace Anki {
namespace Vision {

// Abstract Output base class so we can utilize polymorphism to store
// multiple different Outputs for various types of Processors
class OutputBase
{
public:
  virtual uint32_t GetMode() const = 0;
};

// Output of a Processor templated on enum class and value
template<typename ModeClass, ModeClass mode>
class Output : public OutputBase
{
public:
  // Underlying type of template enum must be uint32_t
  using ModeClass_underlyingType = typename std::underlying_type<ModeClass>::type;
  virtual ModeClass_underlyingType GetMode() const override final
  {
    static_assert(std::is_same<ModeClass_underlyingType, uint32_t>::value,
                  "Output.GetMode template type ModeClass underlying type not same as uint32_t");
    return static_cast<ModeClass_underlyingType>(mode);
  }
};

// Abstract Input base class so we can utilize polymorphism to store
// multiple different Inputs for various types of Processors
class InputBase
{
public:
  virtual uint32_t GetMode() const = 0;
};

// Input to a Processor templated on enum class and value
template<typename ModeClass, ModeClass mode>
class Input : public InputBase
{
public:
  // Underlying type of template enum must be uint32_t
  using ModeClass_underlyingType = typename std::underlying_type<ModeClass>::type;
  virtual ModeClass_underlyingType GetMode() const override final
  {
    static_assert(std::is_same<ModeClass_underlyingType, uint32_t>::value,
                  "Input.GetMode template type ModeClass underlying type not same as uint32_t");
    return static_cast<ModeClass_underlyingType>(mode);
  }
};

// Abstract Processor base class so we can utilize polymorphism to store
// multiple different Processors in one ProcessorRunner
class ProcessorBase
{
public:
  struct UpdateReturn
  {
    // Latest output from the processor
    std::shared_ptr<OutputBase> output = nullptr;

    // Whether or not the processor was able to accept the latest input
    // (ie input was valid and processor will start to process it)
    bool notifiedOfInput = false;

    // Whether or not the processor is still processing the previous input
    bool stillProcessingPrevInput = false;
  };
  // Update the processor with input, should return the output of the last thing it processed
  virtual UpdateReturn Update(std::shared_ptr<InputBase> inputBase) = 0;

  // Start the processor, may or may not immediately start processing input
  virtual void Start() = 0;

  // Stop the processor
  virtual void Stop() = 0;

  // Asynchronous processors should support being able to run synchronously
  virtual void SetIsSynchronous(bool isSynchronous) = 0;

  // Whether or not this processor acquires its input asynchronously (not from the Update call)
  virtual bool HasAsyncInput() const = 0;

  // Return what type of processor this is, used to link it to same type of Input and Output
  virtual uint32_t GetMode() const = 0;
};

// An asynchronous Processor templated on enum class, value, and its corresponding Input class
template<typename ModeClass, ModeClass mode, class InputClass>
class Processor : public ProcessorBase
{
public:

  Processor(const std::string& name);
  ~Processor();

  constexpr ModeClass GetMode() { return mode; }

  virtual void Start() override final;
  virtual void Stop() override final;

  bool IsRunning() const { return _processorRunning; }

  virtual void SetIsSynchronous(bool isSynchronous) override final;
  bool IsRunningSynchronously() const { return _isSynchronous; }

  // Wait for the processor to start and be ready to accept input
  // This is a blocking call
  void WaitForStart();

  // Wait for the processor to finish processing its current input
  // This is a blocking call
  void WaitUntilDone();

  // Update function to pass new input to the processor and get the output from
  // the previous input. If input is null, output will still be returned
  virtual UpdateReturn Update(std::shared_ptr<InputBase> inputBase) override final;

  // Underlying type of template enum must be uint32_t
  using ModeClass_underlyingType = typename std::underlying_type<ModeClass>::type;
  virtual ModeClass_underlyingType GetMode() const override final
  {
    static_assert(std::is_same<ModeClass_underlyingType, uint32_t>::value,
                  "Processor.GetMode template type ModeClass underlying type not same as uint32_t");
    return static_cast<ModeClass_underlyingType>(mode);
  }

protected:

  // Processors support getting Input asynchronously outside of the Update call
  // A derived class should override these functions if it needs that functionality
  virtual bool HasAsyncInput() const override { return false; }

  // Specific implementation should block until input has been acquired
  // Return true if _input was updated
  // Note: _input will be null on first call. Called from processor thread
  virtual bool AsyncAcquireInput() { return false; }

  // This function should wake/unblock AsyncAquireInput in the case of the processor
  // being stop
  virtual void AsyncWake() { }

  // Latest input to the processor, protected by _inputReadyMutex
  std::shared_ptr<InputClass> _input;

private:

  // Tries to get async input if implemented
  bool TryGetInput();

  // Main thread processing loop
  void ProcessorLoop();

  // Derived class should implement this function to process input and return output
  // Processors are discourage from having state so this function is const
  virtual std::shared_ptr<Output<ModeClass, mode>> Process(InputClass* input) const = 0;

  // Latest output from the processor, protected by _inputReadyMutex
  std::shared_ptr<OutputBase> _output;

  // Whether or not the processor is has been started
  std::atomic<bool> _processorRunning;

  // Processing thread
  std::thread _processingThread;

  // Standard condition variables to wake up processing thread when
  // new input is ready
  bool _inputReady = false;
  std::mutex _inputReadyMutex;
  std::condition_variable _inputReadyCondition;

  // Whether or not the processor is running synchronously
  // If true, calls to Update will trigger immediate input processing
  // on caller thread
  bool _isSynchronous = false;

  // Whether or not the processor is done processing the latest input
  std::atomic<bool> _processingDone;

  // Whether or not the processor thread has completely started and is now
  // waiting for first input
  std::atomic<bool> _threadStarted;

  std::string _name = "";
};

}
}

#endif
