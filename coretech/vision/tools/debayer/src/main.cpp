/**
 * File: main.cpp
 *
 * Author: Patrick Doran
 * Date:   3/25/2019
 *
 * Description: Program to run debayering, compare to reference images, and create output images.
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "coretech/vision/engine/debayer.h"

#include "util/fileUtils/fileUtils.h"

#include <json/json.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using Anki::Vision::Debayer;
using Anki::Util::FileUtils;

Debayer::Method StringToMethod(const std::string& value)
{
  if (value == "PHOTO") return Debayer::Method::PHOTO;
  if (value == "PERCEPTION") return Debayer::Method::PERCEPTION;
  throw std::runtime_error("Invalid value: "+value);
}

Debayer::Layout StringToLayout(const std::string& value)
{
  if (value == "RAW10") return Debayer::Layout::RAW10;
  throw std::runtime_error("Invalid value: "+value);
}

Debayer::Pattern StringToPattern(const std::string& value)
{
  if (value == "RGGB") return Debayer::Pattern::RGGB;
  if (value == "BGGR") return Debayer::Pattern::BGGR;
  if (value == "GRBG") return Debayer::Pattern::GRBG;
  if (value == "GBRG") return Debayer::Pattern::GBRG;
  throw std::runtime_error("Invalid value: "+value);
}

Debayer::Scale StringToScale(const std::string& value)
{
  if (value == "FULL")    return Debayer::Scale::FULL;
  if (value == "HALF")    return Debayer::Scale::HALF;
  if (value == "QUARTER") return Debayer::Scale::QUARTER;
  if (value == "EIGHTH")   return Debayer::Scale::EIGHTH;
  throw std::runtime_error("Invalid value: "+value);
}

Debayer::OutputFormat StringToOutputFormat(const std::string& value)
{
  if (value == "RGB24") return Debayer::OutputFormat::RGB24;
  if (value == "Y8")    return Debayer::OutputFormat::Y8;
  throw std::runtime_error("Invalid value: "+value);
}

float ScaleToFloat(Debayer::Scale scale)
{
  switch (scale)
  {
    case Debayer::Scale::FULL: return 1.0f;
    case Debayer::Scale::HALF: return 0.5f;
    case Debayer::Scale::QUARTER: return 0.25f;
    case Debayer::Scale::EIGHTH: return 0.125f;
    default:
      throw std::runtime_error("Unrecognized scale");
  }
}

uint32_t OutputFormatToChannels(Debayer::OutputFormat OutputFormat)
{
  switch(OutputFormat)
  {
    case Debayer::OutputFormat::RGB24: return 3;
    case Debayer::OutputFormat::Y8: return 1;
    default:
      throw std::runtime_error("Unrecognized OutputFormat");
  }
}

struct Config {
  struct Compare {
    Compare() = default;
    Compare(const Json::Value& root);
    bool enable;
    std::string directory;
  };
  struct Image {
    Image() = default;
    Image(const Json::Value& root);

    std::string filename;
    Debayer::Method method;
    Debayer::Scale scale;
    Debayer::OutputFormat format;
  };

  struct Input {
    Input() = default;
    Input(const Json::Value& root);

    std::string filename;
    uint32_t width;
    uint32_t height;
    Debayer::Layout layout;
    Debayer::Pattern pattern;
  };

  struct Output {
    Output() = default;
    Output(const Json::Value& root);
    bool save;
    std::string directory;
    std::vector<Image> images;
  };

  Config() = default;
  Config(const Json::Value& root);

  Compare compare;
  Input input;
  Output output;

};

Config::Config(const Json::Value& root)
  : compare(root.get("compare", Json::nullValue))
  , input(root.get("input", Json::nullValue))
  , output(root.get("output", Json::nullValue))
{
}

Config::Compare::Compare(const Json::Value& root)
{
  {
    Json::Value value = root.get("enable",Json::nullValue);
    this->enable = value.asBool();
  }
  {
    Json::Value value = root.get("directory",Json::nullValue);
    this->directory = value.asString();
  }
}

Config::Input::Input(const Json::Value& root)
{
  {
    Json::Value value = root.get("filename",Json::nullValue);
    this->filename = value.asString();
  }
  {
    Json::Value value = root.get("width",Json::nullValue);
    this->width = value.asUInt();
  }
  {
    Json::Value value = root.get("height",Json::nullValue);
    this->height = value.asUInt();
  }
  {
    Json::Value value = root.get("layout",Json::nullValue);
    this->layout = StringToLayout(value.asString());
  }
  {
    Json::Value value = root.get("pattern",Json::nullValue);
    this->pattern = StringToPattern(value.asString());
  }
}

Config::Output::Output(const Json::Value& root)
{
  {
    Json::Value value = root.get("save",Json::nullValue);
    this->save = value.asBool();
  }
  {
    Json::Value value = root.get("directory",Json::nullValue);
    this->directory = value.asString();
  }
  {
    Json::Value value = root.get("images",Json::nullValue);
    this->images.reserve(value.size());
    for (uint32_t ii = 0; ii < value.size(); ++ii)
    {
      this->images.emplace_back(value[ii]);
    }
  }
}

Config::Image::Image(const Json::Value& root)
{
  {
    Json::Value value = root.get("filename",Json::nullValue);
    this->filename = value.asString();
  }
  {
    Json::Value value = root.get("method",Json::nullValue);
    this->method = StringToMethod(value.asString());
  }
  {
    Json::Value value = root.get("scale",Json::nullValue);
    this->scale = StringToScale(value.asString());
  }
  {
    Json::Value value = root.get("output_format",Json::nullValue);
    this->format = StringToOutputFormat(value.asString());
  }
}

int main(int argc, char** argv)
{
  if (argc != 2){
    std::cout<<"Usage: cti_vision_debayer <config>"<<std::endl;
    return -1;
  }
  Json::Value root;
  
  {
    std::ifstream ifs(argv[1]);
    Json::Reader reader;
    bool parsed = reader.parse(ifs, root);
    if (!parsed)
    {
      std::cout << "Error reading config: " << reader.getFormattedErrorMessages()<<std::endl;
      return -1;
    }
  }

  Config config(root);

  if (config.output.save)
  {
    FileUtils::CreateDirectory(config.output.directory);
  }

  Debayer debayer;

  std::ifstream ifs(config.input.filename);
  std::vector<uint8_t> inBytes(std::istreambuf_iterator<char>(ifs),{});

  Debayer::InArgs inArgs(
    inBytes.data(),
    config.input.height,
    config.input.width,
    config.input.layout,
    config.input.pattern
  );

  for (uint32_t ii = 0; ii < config.output.images.size(); ++ii)
  {
    const float scale = ScaleToFloat(config.output.images[ii].scale);
    s32 width = inArgs.width * scale;
    s32 height = inArgs.height * scale;
    const uint32_t channels = OutputFormatToChannels(config.output.images[ii].format);
    const uint32_t size = channels * width * height;
    std::vector<uint8_t> outBytes(size);

    Debayer::OutArgs outArgs(
      outBytes.data(),
      inArgs.height * scale,
      inArgs.width * scale,
      config.output.images[ii].scale,
      config.output.images[ii].format
    );
    
    const Debayer::Method method = config.output.images[ii].method;
    
    Anki::Result result = debayer.Invoke(method, inArgs, outArgs);
    if (result != Anki::Result::RESULT_OK)
    {
      std::cout<<"Error debayering "<<config.output.images[ii].filename<<std::endl;
    }

    if (config.output.save)
    {
      std::string path = FileUtils::FullFilePath({config.output.directory, config.output.images[ii].filename});
      std::ofstream ofs(path, std::ios::out | std::ios::binary);
      ofs.write(reinterpret_cast<const char*>(outBytes.data()), outBytes.size());
    }

    if (config.compare.enable)
    {
      std::string path = FileUtils::FullFilePath({config.compare.directory,config.output.images[ii].filename});
      std::ifstream stream(path);
      std::vector<uint8_t> refBytes(std::istreambuf_iterator<char>(stream),{});

      // This is a lazy comparison of bytes. We assume that the output bytes will exactly match the reference bytes.
      // In the future it may be worth allowing for some amount of error between the output bytes and reference bytes.
      if (outBytes != refBytes)
      {
        std::cout<<"Error comparing "<<config.output.images[ii].filename<<std::endl;
      }
    }
  }
  return 0;
}