
#include "camera/cameraService.h"

#include "coretech/vision/engine/iImageProcessor.h"
#include "coretech/vision/engine/imageProcessorRunner.h"
#include "coretech/vision/engine/imageCacheProvider.h"

#include "coretech/common/shared/types.h"

#include <iostream>
#include <chrono>

using namespace Anki::Vision;
using namespace std::chrono_literals;

namespace {

enum Threads
{
   Camera,
   RGB,
   Gray
};


class CameraProcessor : public Processor<Threads, Camera, Input<Threads, Camera>>
{
public:
  CameraProcessor(ImageCacheProvider& icp) : Processor("Camera"), _icp(icp) { }

protected:

  virtual bool HasAsyncInput() const override { return true; }

  virtual bool AsyncAcquireInput() override
  {
    // std::unique_lock<std::mutex> lock(_mutex);
    // _cond.wait_for(lock, 30ms);

    Anki::Vector::CameraService::getInstance()->Update();

    if(_input == nullptr)
    {
      _input.reset(new Input<Threads, Camera>());
    }

    printf("capture\n");
    ImageBuffer buffer;
    const bool ret = Anki::Vector::CameraService::getInstance()->CameraGetFrame(0, buffer);
    if(ret)
    {
      printf("got %u\n", buffer.GetImageId());
      std::vector<uint32_t> cleanedIds;
      _icp.Reset(buffer, cleanedIds);

      for(auto id : cleanedIds)
      {
        Anki::Vector::CameraService::getInstance()->CameraReleaseFrame(id);
      }
    }
    return ret;
  }

  virtual void AsyncWake() override
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.notify_all();
  }


private:
  virtual std::shared_ptr<Output<Threads, Camera>> Process(Input<Threads, Camera>* input) const override
  {
    std::shared_ptr<Output<Threads, Camera>> output = std::make_shared<Output<Threads, Camera>>();
    return output;
  }

  ImageCacheProvider& _icp;

  std::mutex _mutex;
  std::condition_variable _cond;
};


class InputRGB : public Input<Threads, RGB>
{
public:
  std::shared_ptr<const ImageRGB> _img = nullptr;
};

class RGBProcessor : public Processor<Threads, RGB, InputRGB>
{
public:
  RGBProcessor(ImageCacheProvider& icp) : Processor("RGB"), _icp(icp) { }

protected:

  virtual bool HasAsyncInput() const override { return true; }

  virtual bool AsyncAcquireInput() override
  {
    // std::unique_lock<std::mutex> lock(_mutex);
    // _cond.wait_for(lock, 30ms);

    if(_input == nullptr)
    {
      _input.reset(new InputRGB());
    }

    printf("getting rgb %u\n", _icp.GetLatestImageId());
    _input->_img = _icp.GetRGB();

    return true;
  }

  virtual void AsyncWake() override
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.notify_all();
  }

private:
  virtual std::shared_ptr<Output<Threads, RGB>> Process(InputRGB* input) const override
  {
    if(input != nullptr && input->_img != nullptr)
    {
      printf("saving rgb\n");
      input->_img->Save("/test/rgb_" + std::to_string(input->_img->GetImageId()) + ".png");
    }

    std::shared_ptr<Output<Threads, RGB>> output = std::make_shared<Output<Threads, RGB>>();
    return output;
  }

  ImageCacheProvider& _icp;

  std::mutex _mutex;
  std::condition_variable _cond;
};


class InputGray : public Input<Threads, Gray>
{
public:
  std::shared_ptr<const Image> _img = nullptr;
};

class GrayProcessor : public Processor<Threads, Gray, InputGray>
{
public:
  GrayProcessor(ImageCacheProvider& icp) : Processor("Gray"), _icp(icp) { }

protected:

  virtual bool HasAsyncInput() const override { return true; }

  virtual bool AsyncAcquireInput() override
  {
    // std::unique_lock<std::mutex> lock(_mutex);
    // _cond.wait_for(lock, 30ms);

    if(_input == nullptr)
    {
      _input.reset(new InputGray());
    }

    printf("getting gray %u\n", _icp.GetLatestImageId());
    _input->_img = _icp.GetGray();

    return true;
  }

  virtual void AsyncWake() override
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.notify_all();
  }

private:
  virtual std::shared_ptr<Output<Threads, Gray>> Process(InputGray* input) const override
  {
    if(input != nullptr && input->_img != nullptr)
    {
      printf("saving gray\n");
      input->_img->Save("/test/gray_" + std::to_string(input->_img->GetImageId()) + ".png");
    }

    std::shared_ptr<Output<Threads, Gray>> output = std::make_shared<Output<Threads, Gray>>();
    return output;
  }

  ImageCacheProvider& _icp;

  std::mutex _mutex;
  std::condition_variable _cond;
};

}

int main(int argc, char** argv)
{
  ImageProcessorRunner ipr;
  ImageCacheProvider icp;

  std::shared_ptr<CameraProcessor> camera = std::make_shared<CameraProcessor>(icp);
  std::shared_ptr<RGBProcessor> rgb = std::make_shared<RGBProcessor>(icp);
  std::shared_ptr<GrayProcessor> gray = std::make_shared<GrayProcessor>(icp);

  ipr.AddProcessor(camera);
  ipr.AddProcessor(rgb);
  ipr.AddProcessor(gray);

  ipr.Start();

  sleep(10);

  ipr.Stop();
}
