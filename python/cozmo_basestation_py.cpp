/**
 * File: cozmo_basestation_py
 *
 * Author: Nishkar Grover
 * Created: 05/25/17
 *
 * Description: Cozmo basestation functionality that is exposed for use in Python.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include <vector>
#include <boost/python.hpp>
#include <boost/python/init.hpp>
#include "anki/cozmo/basestation/proceduralFaceDrawer.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/common/basestation/array2d_impl.h"


using namespace Anki;
using namespace Vector;


ProceduralFace GetProcFaceFromVals(boost::python::list& leftEyeList, boost::python::list& rightEyeList,
                                   double faceAngle_deg, double faceCenterX, double faceCenterY,
                                   double faceScaleX, double faceScaleY)
{
  ProceduralFace procFace;
  std::vector<f32> leftEyeVector;
  std::vector<f32> rightEyeVector;

  for (int idx=0; idx<len(leftEyeList); ++idx) {
    leftEyeVector.push_back(boost::python::extract<f32>(leftEyeList[idx]));
  }

  for (int idx=0; idx<len(rightEyeList); ++idx) {
    rightEyeVector.push_back(boost::python::extract<f32>(rightEyeList[idx]));
  }

  procFace.SetFromValues(leftEyeVector, rightEyeVector, static_cast<f32>(faceAngle_deg),
                         static_cast<f32>(faceCenterX), static_cast<f32>(faceCenterY),
                         static_cast<f32>(faceScaleX), static_cast<f32>(faceScaleY));
  return procFace;
}


class FaceImage
{
private:
  ProceduralFace _procFace;
  Vision::Image _faceImg;
public:
  FaceImage(boost::python::list&, boost::python::list&, double, double, double, double, double);
  void Interpolate(boost::python::list&, boost::python::list&, double, double, double, double, double, double);
  void Reset();
  bool Save(const std::string&);
  boost::python::tuple GetPixelList();
  boost::python::tuple GetPixelMemory();
};

FaceImage::FaceImage(boost::python::list& leftEye, boost::python::list& rightEye, double faceAngle_deg,
                     double faceCenterX, double faceCenterY, double faceScaleX, double faceScaleY)
{
  _procFace = GetProcFaceFromVals(leftEye, rightEye, faceAngle_deg, faceCenterX, faceCenterY, faceScaleX, faceScaleY);
  _faceImg = ProceduralFaceDrawer::DrawFace(_procFace);
}

void FaceImage::Interpolate(boost::python::list& leftEye, boost::python::list& rightEye, double faceAngle_deg,
                            double faceCenterX, double faceCenterY, double faceScaleX, double faceScaleY,
                            double blendFraction)
{
  ProceduralFace interpolatedProcFace;
  ProceduralFace secondProcFace = GetProcFaceFromVals(leftEye, rightEye, faceAngle_deg, faceCenterX, faceCenterY,
                                                      faceScaleX, faceScaleY);
  interpolatedProcFace.Interpolate(_procFace, secondProcFace, static_cast<float>(blendFraction));
  _faceImg = ProceduralFaceDrawer::DrawFace(interpolatedProcFace);
}

// Set '_faceImg' to the face that was passed into the FaceImage constructor.
// This essentially resets any prior interpolation with a second face.
void FaceImage::Reset()
{
  _faceImg = ProceduralFaceDrawer::DrawFace(_procFace);
}

bool FaceImage::Save(const std::string& fileName)
{
  Result saveResult = _faceImg.Save(fileName);
  if (RESULT_OK == saveResult) {
    return true;
  } else {
    return false;
  }
}

boost::python::tuple FaceImage::GetPixelList()
{
  boost::python::list pixels;
  s32 height = _faceImg.GetNumRows();
  s32 width = _faceImg.GetNumCols();
  if (_faceImg.IsContinuous()) {
    // This should help improve efficiency for "continuous" images
    width *= height;
    height = 1;
  }
  for (int i=0; i<height; ++i) {
    const uint8_t* imageRow = _faceImg.GetRow(i);
    for (int j=0; j<width; ++j) {
      pixels.append(imageRow[j]);
    }
  }
  return boost::python::make_tuple(_faceImg.GetNumRows(), _faceImg.GetNumCols(), pixels);
}

boost::python::tuple FaceImage::GetPixelMemory()
{
  const s32 height = _faceImg.GetNumRows();
  const s32 width = _faceImg.GetNumCols();
  const s32 size = _faceImg.GetNumElements();
  DEV_ASSERT(_faceImg.IsContinuous(), "FaceImage.GetPixelMemory.FaceImageNotContinuous");
  const uint8_t* imageData = _faceImg.GetDataPointer();
  PyObject* pyObj = PyMemoryView_FromObject(PyBuffer_FromMemory(const_cast<uint8_t*>(imageData), size));
  boost::python::object memoryView(boost::python::handle<>(boost::python::borrowed(pyObj)));
  return boost::python::make_tuple(height, width, memoryView);
}


BOOST_PYTHON_MODULE(cozmo_basestation_py)
{
  using namespace boost::python;

  // The methods that are exposed for use in Python use the lowercase_with_underscore
  // naming convention to adhere to the PEP-8 style guide for Python code.

  class_<FaceImage>("FaceImage", init<boost::python::list&, boost::python::list&,
                                      double, double, double, double, double>())
    .def("interpolate", &FaceImage::Interpolate)
    .def("reset", &FaceImage::Reset)
    .def("save", &FaceImage::Save)
    .def("get_pixel_list", &FaceImage::GetPixelList)
    .def("get_pixel_memory", &FaceImage::GetPixelMemory)
  ;
}


