/**
 * File: faceEvalInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/tools/faceEval/faceEvalInterface.h"
#include "coretech/vision/tools/faceEval/simpleArgParser.h"

#include "coretech/vision/engine/faceTracker.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"

#include <random>

namespace Anki {
namespace Vision {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Specialization for private type DisplayMode
template<>
IFaceEval::DisplayMode SimpleArgParser::GetArgHelper<IFaceEval::DisplayMode>(const std::string& value) const
{
  using DisplayMode = IFaceEval::DisplayMode;
  static const std::map<std::string, DisplayMode> LUT{
    {"Off",             DisplayMode::Off},
    {"All",             DisplayMode::All},
    {"AnyFailure",      DisplayMode::AnyFailure},
    {"NoFailure",       DisplayMode::NoFailure},
    {"FalsePositives",  DisplayMode::FalsePositives},
    {"FalseNegatives",  DisplayMode::FalseNegatives},
    {"MissingParts",    DisplayMode::MissingParts},
  };
  
  auto iter = LUT.find(value);
  
  if(iter == LUT.end())
  {
    LOG_ERROR("IFaceEval.GetArgHelper.BadDisplayModeValue", "%s", value.c_str());
    return DisplayMode::Off;
  }
  
  return iter->second;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IFaceEval::IFaceEval(const SimpleArgParser& argParser)
{
  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
    -0.2198782133395603,
    0.001435740245449692,
    0.001523365725052927,
    0.1341471670512819,
    0, 0, 0}};
  auto calib = std::make_shared<CameraCalibration>(360,640,
                                                   362.8773099149878, 366.7347434532929, 302.2888225643724,
                                                   200.012543449327, 0, distortionCoeffs);
  _camera.SetCalibration(calib);
  
  argParser.GetArg("--enableDisplay", _params.enableDisplay);
  argParser.GetArg("--outputDir", _params.outputDir);
  argParser.GetArg("--randomSeed", _params.randomSeed);
  
  if(!Util::FileUtils::DirectoryExists(_params.outputDir))
  {
    const bool success = Util::FileUtils::CreateDirectory(_params.outputDir);
    if(!success)
    {
      LOG_ERROR("IFaceEval.Constructor.CreateOutputDirFailed", "%s", _params.outputDir.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Needed b/c of use of unique_ptr<FaceTracker> w/ forward declaration
IFaceEval::~IFaceEval() = default;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceTracker& IFaceEval::GetFaceTracker()
{
  if(_faceTracker == nullptr)
  {
    Json::Value config;
    _faceTracker.reset( new FaceTracker(_camera, "", config) );
    _faceTracker->SetRecognitionIsSynchronous(true);
  }
  
  return *_faceTracker;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IFaceEval::DrawFaceOnImage(const TrackedFace& face, Vision::ImageRGB& img)
{
  img.DrawRect(face.GetRect(), NamedColors::RED);
  
  const auto& kFeatureColor = NamedColors::GREEN;
  Point2f leftEye, rightEye;
  if(face.GetEyeCenters(leftEye, rightEye)) {
    img.DrawCircle(leftEye, kFeatureColor, 1);
    img.DrawCircle(rightEye, kFeatureColor, 1);
  }
  
  // Draw features
  for(s32 iFeature=0; iFeature<(s32)Vision::TrackedFace::NumFeatures; ++iFeature)
  {
    Vision::TrackedFace::FeatureName featureName = (Vision::TrackedFace::FeatureName)iFeature;
    const Vision::TrackedFace::Feature& feature = face.GetFeature(featureName);
    
    for(size_t crntPoint=0, nextPoint=1; nextPoint < feature.size(); ++crntPoint, ++nextPoint) {
      img.DrawLine(feature[crntPoint], feature[nextPoint], kFeatureColor);
    }
  }

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IFaceEval::DrawFaces(const Image& img, const std::list<TrackedFace>& faces,
                          const FailureFlags& failureFlags,
                          const std::string& displayStr,
                          int pauseTime_ms, const char* windowName,
                          const std::string& saveFilename)
{
  const bool doSave = !saveFilename.empty();
  bool doDisplay = false;
  switch(GetDisplayMode())
  {
    case DisplayMode::Off:
      doDisplay = false;
      break;
      
    case DisplayMode::All:
      doDisplay = true;
      break;
      
    case DisplayMode::AnyFailure:
      doDisplay = failureFlags.AreAnyFlagsSet();
      break;
      
    case DisplayMode::NoFailure:
      doDisplay = !failureFlags.AreAnyFlagsSet();
      break;
      
    case DisplayMode::FalseNegatives:
      doDisplay = failureFlags.IsBitFlagSet(FailureType::FalseNegative);
      break;
      
    case DisplayMode::FalsePositives:
      doDisplay = failureFlags.IsBitFlagSet(FailureType::FalsePositive);
      break;
      
    case DisplayMode::MissingParts:
      doDisplay = failureFlags.IsBitFlagSet(FailureType::MissingParts);
      break;
  }
  
  if(doDisplay || doSave)
  {
    _dispImg.SetFromGray(img);
    for(const auto& face : faces)
    {
      DrawFaceOnImage(face, _dispImg);
    }
    
#if ANKI_DEVELOPER_CODE
    if(!displayStr.empty())
    {
      _dispImg.DrawText({1, _dispImg.GetNumRows()-1}, displayStr, NamedColors::RED, 0.75f);
    }
#endif
    
    if(doDisplay)
    {
      _dispImg.Display(windowName, pauseTime_ms);
    }
    
    if(doSave)
    {
      _dispImg.Save(Util::FileUtils::FullFilePath({GetOutputDir(), saveFilename}));
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IFaceEval::ShuffleHelper(std::vector<std::string>& strList)
{
  static std::random_device randomDevice;
  static std::mt19937       randomEngine((_params.randomSeed >=0 ? _params.randomSeed : randomDevice()));
  std::shuffle(strList.begin(), strList.end(), randomEngine);
}
  
} // namespace Vision
} // namespace Anki

