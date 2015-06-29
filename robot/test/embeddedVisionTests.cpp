/**
File: embeddedVisionTests.cpp
Author: Peter Barnum
Created: 2013

Various tests of the coretech vision embedded library.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

//#define RUN_PC_ONLY_TESTS
//#define RUN_HIGH_MEMORY_TESTS
//#define JUST_FIDUCIAL_DETECTION

#include "anki/common/robot/config.h"
#include "anki/common/robot/gtestLight.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/arrayPatterns.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/integralImage.h"
#include "anki/vision/robot/draw_vision.h"
#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/transformations.h"
#include "anki/vision/robot/binaryTracker.h"
#include "anki/vision/robot/decisionTree_vision.h"
#include "anki/vision/robot/perspectivePoseEstimation.h"
#include "anki/vision/robot/classifier.h"
#include "anki/vision/robot/cameraImagingPipeline.h"
#include "anki/vision/robot/opencvLight_vision.h"
#include "anki/vision/robot/features.h"
#include "anki/vision/robot/recognize.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "data/simpleFiducials_320x240.h"
#include "data/newFiducials_320x240.h"

#if !defined(JUST_FIDUCIAL_DETECTION)
#include "data/blockImage50_320x240.h"
#include "data/blockImages00189_80x60.h"
#include "data/blockImages00190_80x60.h"
#include "data/cozmo_date2014_01_29_time11_41_05_frame10_320x240.h"
#include "data/cozmo_date2014_01_29_time11_41_05_frame12_320x240.h"
#include "data/cozmo_date2014_04_04_time17_40_08_frame0.h"

#include "anki/vision/robot/lbpcascade_frontalface.h"

#include "data/cozmo_date2014_04_10_time16_15_40_frame0.h"
#endif

#include "embeddedTests.h"

#include <cmath>

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include <iostream>
#include <fstream>
#include "opencv2/video/tracking.hpp"
#include "opencv2/contrib/contrib.hpp"
#endif

using namespace Anki;
using namespace Anki::Embedded;

#if defined(RUN_PC_ONLY_TESTS) || defined(RUN_HIGH_MEMORY_TESTS)
#define HUGE_BUFFER_SIZE 100000000
static char hugeBuffer[HUGE_BUFFER_SIZE];
#endif

#if !defined(JUST_FIDUCIAL_DETECTION)

#if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)
static Result FaceRecognizer_LoadTrainingImages(
  FixedLengthList<Array<u8> > &trainingImages,
  FixedLengthList<Rectangle<s32> > &faceLocations,
  std::vector<int> &trainingLabels,
  std::vector<std::string> &trainingLabelNames,
  MemoryStack scratch, MemoryStack &memory)
{
  const s32 faceRecognizer_numTrainingImages = 8;

  char * faceRecognizer_trainingImageFilenames[faceRecognizer_numTrainingImages] = {
    "C:/tmp/faces320/andrew1.jpg",
    "C:/tmp/faces320/andrew2.jpg",
    "C:/tmp/faces320/bryan1.jpg",
    "C:/tmp/faces320/bryan2.jpg",
    "C:/tmp/faces320/kevin1.jpg",
    "C:/tmp/faces320/kevin2.jpg",
    "C:/tmp/faces320/peter1.jpg",
    "C:/tmp/faces320/peter2.jpg"};

  const s32 numLabelNames = 4;
  const char * faceRecognizer_labelNames[numLabelNames] = {"Andrew", "Bryan", "Kevin", "Peter"};

  const s32 faceRecognizer_trainingLabels_data[faceRecognizer_numTrainingImages] = {0, 0, 1, 1, 2, 2, 3, 3};

  trainingImages = FixedLengthList<Array<u8> >(faceRecognizer_numTrainingImages, memory, Flags::Buffer(true, false, true));
  faceLocations = FixedLengthList<Rectangle<s32> >(faceRecognizer_numTrainingImages, memory, Flags::Buffer(true, false, true));

  AnkiConditionalErrorAndReturnValue(AreValid(trainingImages, faceLocations),
    RESULT_FAIL, "FaceRecognizer_LoadTrainingImages", "Image error");

  AnkiConditionalErrorAndReturnValue(NotAliased(scratch, memory),
    RESULT_FAIL, "FaceRecognizer_LoadTrainingImages", "Aliased");

  // TODO: are these const casts okay?
  const FixedLengthList<Classifier::CascadeClassifier::Stage> &stages = FixedLengthList<Classifier::CascadeClassifier::Stage>(lbpcascade_frontalface_stages_length, const_cast<Classifier::CascadeClassifier::Stage*>(&lbpcascade_frontalface_stages_data[0]), lbpcascade_frontalface_stages_length*sizeof(Classifier::CascadeClassifier::Stage) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTree> &classifiers = FixedLengthList<Classifier::CascadeClassifier::DTree>(lbpcascade_frontalface_classifiers_length, const_cast<Classifier::CascadeClassifier::DTree*>(&lbpcascade_frontalface_classifiers_data[0]), lbpcascade_frontalface_classifiers_length*sizeof(Classifier::CascadeClassifier::DTree) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTreeNode> &nodes =  FixedLengthList<Classifier::CascadeClassifier::DTreeNode>(lbpcascade_frontalface_nodes_length, const_cast<Classifier::CascadeClassifier::DTreeNode*>(&lbpcascade_frontalface_nodes_data[0]), lbpcascade_frontalface_nodes_length*sizeof(Classifier::CascadeClassifier::DTreeNode) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));;
  const FixedLengthList<f32> &leaves = FixedLengthList<f32>(lbpcascade_frontalface_leaves_length, const_cast<f32*>(&lbpcascade_frontalface_leaves_data[0]), lbpcascade_frontalface_leaves_length*sizeof(f32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<s32> &subsets = FixedLengthList<s32>(lbpcascade_frontalface_subsets_length, const_cast<s32*>(&lbpcascade_frontalface_subsets_data[0]), lbpcascade_frontalface_subsets_length*sizeof(s32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Rectangle<s32> > &featureRectangles = FixedLengthList<Rectangle<s32> >(lbpcascade_frontalface_featureRectangles_length, const_cast<Rectangle<s32>*>(reinterpret_cast<const Rectangle<s32>*>(&lbpcascade_frontalface_featureRectangles_data[0])), lbpcascade_frontalface_featureRectangles_length*sizeof(Rectangle<s32>) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const s32 minHeight = 30;
  const s32 minWidth = 30;
  const s32 trainingWidth = 64;
  const bool useHistogramPreprocess = false;

  trainingLabels.clear();
  trainingLabelNames.clear();

  for(s32 iFace=0; iFace<faceRecognizer_numTrainingImages; iFace++) {
    trainingImages[iFace] = Array<u8>::LoadImage(faceRecognizer_trainingImageFilenames[iFace], memory);

    AnkiConditionalErrorAndReturnValue(trainingImages[iFace].IsValid(),
      RESULT_FAIL, "FaceRecognizer_LoadTrainingImages", "Image error");

    cv::Mat_<u8> arrayCopy;
    ArrayToCvMat(trainingImages[iFace], &arrayCopy);

    //// Preprocess
    //if(useHistogramPreprocess) {
    //  // Histogram preprocess
    //  cv::equalizeHist(arrayCopy, arrayCopy);
    //} else {
    //  // DoG preprocess
    //  cv::Mat_<u8> dog1(arrayCopy.rows, arrayCopy.cols);
    //  cv::Mat_<u8> dog2(arrayCopy.rows, arrayCopy.cols);

    //  cv::GaussianBlur(arrayCopy, dog1, cv::Size(7,7), 1.0);
    //  cv::GaussianBlur(arrayCopy, dog2, cv::Size(13,13), 2.0);

    //  for(s32 i=0; i<arrayCopy.rows*arrayCopy.cols; i++) {
    //    arrayCopy.data[i] = (static_cast<s32>(dog1.data[i]) - static_cast<s32>(dog2.data[i]) + 255) >> 1;
    //  }
    //}

    trainingLabels.push_back(faceRecognizer_trainingLabels_data[iFace]);
  }

  for(s32 i=0; i<numLabelNames; i++) {
    trainingLabelNames.push_back(std::string(faceRecognizer_labelNames[i]));
  }

  {
    PUSH_MEMORY_STACK(memory);

    Classifier::CascadeClassifier_LBP cc(
      lbpcascade_frontalface_isStumpBased,
      lbpcascade_frontalface_stageType,
      lbpcascade_frontalface_featureType,
      lbpcascade_frontalface_ncategories,
      lbpcascade_frontalface_origWinHeight,
      lbpcascade_frontalface_origWinWidth,
      stages,
      classifiers,
      nodes,
      leaves,
      subsets,
      featureRectangles,
      memory);

    const s32 MAX_CANDIDATES = 5000;

    FixedLengthList<Rectangle<s32> > detectedFaces(MAX_CANDIDATES, memory);

    for(s32 iFace=0; iFace<faceRecognizer_numTrainingImages; iFace++) {
      detectedFaces.Clear();

      const Result result = cc.DetectMultiScale(
        trainingImages[iFace],
        static_cast<f32>(scaleFactor),
        minNeighbors,
        minHeight, minWidth,
        trainingImages[iFace].get_size(0), trainingImages[iFace].get_size(1),
        detectedFaces,
        scratch,
        memory);

      AnkiConditionalErrorAndReturnValue(detectedFaces.get_size() == 1,
        RESULT_FAIL, "FaceRecognizer_LoadTrainingImages", "Only one face should be detected");

      //Rectangle<f32> detectedFaceF32(static_cast<f32>(detectedFaces[0].left), static_cast<f32>(detectedFaces[0].right), static_cast<f32>(detectedFaces[0].top), static_cast<f32>(detectedFaces[0].bottom));

      faceLocations[iFace] = detectedFaces[0];
    }
  }

  return RESULT_OK;
}
#endif // #if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)

#if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)
GTEST_TEST(CoreTech_Vision, TrainFaceRecognizer)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  //FixedLengthList<Array<u8> > trainingImages(faceRecognizer_numTrainingImages, scratchHuge, Flags::Buffer(true, false, true));

  //std::vector<int> cvTrainingLabels(&faceRecognizer_trainingLabels_data[0], &faceRecognizer_trainingLabels_data[0]+faceRecognizer_numTrainingImages);

  //FixedLengthList<Rectangle<s32> > detectedFaces(MAX_CANDIDATES, scratchOffchip);

  // TODO: are these const casts okay?
  const FixedLengthList<Classifier::CascadeClassifier::Stage> &stages = FixedLengthList<Classifier::CascadeClassifier::Stage>(lbpcascade_frontalface_stages_length, const_cast<Classifier::CascadeClassifier::Stage*>(&lbpcascade_frontalface_stages_data[0]), lbpcascade_frontalface_stages_length*sizeof(Classifier::CascadeClassifier::Stage) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTree> &classifiers = FixedLengthList<Classifier::CascadeClassifier::DTree>(lbpcascade_frontalface_classifiers_length, const_cast<Classifier::CascadeClassifier::DTree*>(&lbpcascade_frontalface_classifiers_data[0]), lbpcascade_frontalface_classifiers_length*sizeof(Classifier::CascadeClassifier::DTree) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTreeNode> &nodes =  FixedLengthList<Classifier::CascadeClassifier::DTreeNode>(lbpcascade_frontalface_nodes_length, const_cast<Classifier::CascadeClassifier::DTreeNode*>(&lbpcascade_frontalface_nodes_data[0]), lbpcascade_frontalface_nodes_length*sizeof(Classifier::CascadeClassifier::DTreeNode) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));;
  const FixedLengthList<f32> &leaves = FixedLengthList<f32>(lbpcascade_frontalface_leaves_length, const_cast<f32*>(&lbpcascade_frontalface_leaves_data[0]), lbpcascade_frontalface_leaves_length*sizeof(f32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<s32> &subsets = FixedLengthList<s32>(lbpcascade_frontalface_subsets_length, const_cast<s32*>(&lbpcascade_frontalface_subsets_data[0]), lbpcascade_frontalface_subsets_length*sizeof(s32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Rectangle<s32> > &featureRectangles = FixedLengthList<Rectangle<s32> >(lbpcascade_frontalface_featureRectangles_length, const_cast<Rectangle<s32>*>(reinterpret_cast<const Rectangle<s32>*>(&lbpcascade_frontalface_featureRectangles_data[0])), lbpcascade_frontalface_featureRectangles_length*sizeof(Rectangle<s32>) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));

  Classifier::CascadeClassifier_LBP cc(
    lbpcascade_frontalface_isStumpBased,
    lbpcascade_frontalface_stageType,
    lbpcascade_frontalface_featureType,
    lbpcascade_frontalface_ncategories,
    lbpcascade_frontalface_origWinHeight,
    lbpcascade_frontalface_origWinWidth,
    stages,
    classifiers,
    nodes,
    leaves,
    subsets,
    featureRectangles,
    scratchCcm);

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const s32 minHeight = 30;
  const s32 minWidth = 30;
  const s32 trainingWidth = 64;

  cv::Ptr<cv::FaceRecognizer> model = cv::createLBPHFaceRecognizer();
  //cv::Ptr<cv::FaceRecognizer> model = cv::createFisherFaceRecognizer();
  std::vector<cv::Mat> cvTrainingImages;

  //const bool showImages = false;
  const bool showImages = true;

  FixedLengthList<Array<u8> > trainingImages;
  FixedLengthList<Rectangle<s32> > faceLocations;
  std::vector<int> trainingLabels;
  std::vector<std::string> trainingLabelNames;

  ASSERT_TRUE(FaceRecognizer_LoadTrainingImages(trainingImages, faceLocations, trainingLabels, trainingLabelNames, scratchOffchip, scratchHuge) == RESULT_OK);

  for(s32 iFace=0; iFace<trainingImages.get_size(); iFace++) {
    //trainingImages[iFace].Show("face", true);

    Array<u8> clippedImage(faceLocations[iFace].get_height()+1, faceLocations[iFace].get_width()+1, scratchHuge);
    clippedImage(0,-1,0,-1).Set(trainingImages[iFace](faceLocations[iFace].top, faceLocations[iFace].bottom, faceLocations[iFace].left, faceLocations[iFace].right));

    {
      cv::Mat_<u8> arrayCopy;
      ArrayToCvMat(clippedImage, &arrayCopy);
      cv::Mat_<u8> arrayCopyResized = cvCreateMat(trainingWidth, trainingWidth, CV_8U);
      cv::resize(arrayCopy, arrayCopyResized, arrayCopyResized.size());
      cvTrainingImages.push_back(arrayCopyResized);
    }

    if(showImages) {
      printf("%d) ", iFace);
      faceLocations[iFace].Print();

      cv::Mat_<u8> arrayCopy;
      ArrayToCvMat(trainingImages[iFace], &arrayCopy);

      cv::Point center( Round<s32>((faceLocations[iFace].left + faceLocations[iFace].right)*0.5), Round<s32>((faceLocations[iFace].top + faceLocations[iFace].bottom)*0.5) );
      cv::ellipse( arrayCopy, center, cv::Size( Round<s32>((faceLocations[iFace].right-faceLocations[iFace].left)*0.5), Round<s32>((faceLocations[iFace].bottom-faceLocations[iFace].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );

      cv::imshow("detect", arrayCopy);
      clippedImage.Show("clippedImage", false);
      cv::waitKey();
    } // if(showImages)
  } // for(s32 iFace=0; iFace<faceRecognizer_numTrainingImages; iFace++)

  //for(s32 iFace=0; iFace<faceRecognizer_numTrainingImages; iFace++) {
  //  cv::imshow("detect", cvTrainingImages[iFace]);
  //  cv::waitKey();
  //}

  model->train(cvTrainingImages, trainingLabels);

  model->save("c:/tmp/recognizer.yml");

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TrainFaceRecognizer)
#endif // #if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)

#if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)
GTEST_TEST(CoreTech_Vision, TestFaceRecognizer)
{
  const s32 trainingWidth = 64;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  FixedLengthList<Array<u8> > trainingImages;
  FixedLengthList<Rectangle<s32> > trainingFaceLocations;
  std::vector<int> trainingLabels;
  std::vector<std::string> trainingLabelNames;
  ASSERT_TRUE(FaceRecognizer_LoadTrainingImages(trainingImages, trainingFaceLocations, trainingLabels, trainingLabelNames, scratchOffchip, scratchHuge) == RESULT_OK);

  cv::Ptr<cv::FaceRecognizer> model = cv::createLBPHFaceRecognizer();
  //cv::Ptr<cv::FaceRecognizer> model = cv::createFisherFaceRecognizer();

  model->load("c:/tmp/recognizer.yml");

  cv::Mat cvImage;

  const s32 MAX_CANDIDATES = 5000;

  FixedLengthList<Rectangle<s32> > detectedFaces(MAX_CANDIDATES, scratchOffchip);

  // TODO: are these const casts okay?
  const FixedLengthList<Classifier::CascadeClassifier::Stage> &stages = FixedLengthList<Classifier::CascadeClassifier::Stage>(lbpcascade_frontalface_stages_length, const_cast<Classifier::CascadeClassifier::Stage*>(&lbpcascade_frontalface_stages_data[0]), lbpcascade_frontalface_stages_length*sizeof(Classifier::CascadeClassifier::Stage) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTree> &classifiers = FixedLengthList<Classifier::CascadeClassifier::DTree>(lbpcascade_frontalface_classifiers_length, const_cast<Classifier::CascadeClassifier::DTree*>(&lbpcascade_frontalface_classifiers_data[0]), lbpcascade_frontalface_classifiers_length*sizeof(Classifier::CascadeClassifier::DTree) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTreeNode> &nodes =  FixedLengthList<Classifier::CascadeClassifier::DTreeNode>(lbpcascade_frontalface_nodes_length, const_cast<Classifier::CascadeClassifier::DTreeNode*>(&lbpcascade_frontalface_nodes_data[0]), lbpcascade_frontalface_nodes_length*sizeof(Classifier::CascadeClassifier::DTreeNode) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));;
  const FixedLengthList<f32> &leaves = FixedLengthList<f32>(lbpcascade_frontalface_leaves_length, const_cast<f32*>(&lbpcascade_frontalface_leaves_data[0]), lbpcascade_frontalface_leaves_length*sizeof(f32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<s32> &subsets = FixedLengthList<s32>(lbpcascade_frontalface_subsets_length, const_cast<s32*>(&lbpcascade_frontalface_subsets_data[0]), lbpcascade_frontalface_subsets_length*sizeof(s32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Rectangle<s32> > &featureRectangles = FixedLengthList<Rectangle<s32> >(lbpcascade_frontalface_featureRectangles_length, const_cast<Rectangle<s32>*>(reinterpret_cast<const Rectangle<s32>*>(&lbpcascade_frontalface_featureRectangles_data[0])), lbpcascade_frontalface_featureRectangles_length*sizeof(Rectangle<s32>) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));

  Classifier::CascadeClassifier_LBP cc(
    lbpcascade_frontalface_isStumpBased,
    lbpcascade_frontalface_stageType,
    lbpcascade_frontalface_featureType,
    lbpcascade_frontalface_ncategories,
    lbpcascade_frontalface_origWinHeight,
    lbpcascade_frontalface_origWinWidth,
    stages,
    classifiers,
    nodes,
    leaves,
    subsets,
    featureRectangles,
    scratchCcm);

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const s32 minHeight = 30;
  const s32 minWidth = 30;

  const f32 eyeQualityThreshold = 1.0f / 5000.0f; //< A value between about 2000 to 5000 is good

  Array<u8> faceImage = Array<u8>::LoadImage("C:/tmp/faces320/peter2.jpg", scratchHuge);

  cv::CascadeClassifier eyeCascade;
  /*ASSERT_TRUE(eyeCascade.load("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml"));*/
  ASSERT_TRUE(eyeCascade.load("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_eye.xml"));

  cv::VideoCapture cap(0);
  while ( cap.isOpened() ) {
    PUSH_MEMORY_STACK(scratchHuge);

    cv::Mat_<u8> cvImageGray;

    //const bool useSavedImage = true;
    const bool useSavedImage = false;
    if(!useSavedImage) {
      cap >> cvImage;

      if(cvImage.empty())
        break;

      cv::Mat_<u8> cvImageSmall(240,320);

      cvtColor(cvImage, cvImageGray, CV_BGRA2GRAY);

      cv::resize(cvImageGray, cvImageSmall, cvImageSmall.size());

      cvImageGray = cvImageSmall;

      cv::equalizeHist(cvImageGray, cvImageGray);
    } else {
      ArrayToCvMat(faceImage, &cvImageGray);
      cv::equalizeHist(cvImageGray, cvImageGray);
    }

    Array<u8> image(cvImageGray.rows, cvImageGray.cols, scratchHuge);

    image.Set(cvImageGray);

    const Result result = cc.DetectMultiScale(
      image,
      static_cast<f32>(scaleFactor),
      minNeighbors,
      minHeight, minWidth,
      image.get_size(0), image.get_size(1),
      detectedFaces,
      scratchOffchip,
      scratchHuge);

    cv::Mat_<u8> arrayCopy;
    ArrayToCvMat(image, &arrayCopy);

    cv::Mat arrayCopyColor = cvCreateMat(arrayCopy.rows, arrayCopy.cols, CV_8UC3);

    std::vector<cv::Mat> channels;
    channels.push_back(arrayCopy);
    channels.push_back(arrayCopy);
    channels.push_back(arrayCopy);
    cv::merge(channels, arrayCopyColor);

    if(detectedFaces.get_size() > 0) {
      for( s32 iFace = 0; iFace < detectedFaces.get_size(); iFace++ ) {
        //for( s32 iFace = 1; iFace < 2; iFace++ ) {
        //std::vector<cv::Rect> detectedEyes;
        //s32 leftEyeIndex;
        //s32 rightEyeIndex;
        //f32 eyeQuality;
        s32 faceId;
        f64 confidence;

        //const Result result = Recognize::RecognizeFace(
        //  image,
        //  detectedFaces[iFace],
        //  eyeCascade,
        //  eyeQualityThreshold,
        //  detectedEyes,
        //  leftEyeIndex,
        //  rightEyeIndex,
        //  eyeQuality,
        //  faceId,
        //  confidence);

        const Result result = Recognize::RecognizeFace(
          image,
          detectedFaces[iFace],
          trainingImages,
          trainingFaceLocations,
          faceId,
          confidence,
          scratchHuge);

        ASSERT_TRUE(result == RESULT_OK);

        cv::Point center( Round<s32>((detectedFaces[iFace].left + detectedFaces[iFace].right)*0.5), Round<s32>((detectedFaces[iFace].top + detectedFaces[iFace].bottom)*0.5) );
        cv::ellipse( arrayCopyColor, center, cv::Size( Round<s32>((detectedFaces[iFace].right-detectedFaces[iFace].left)*0.5), Round<s32>((detectedFaces[iFace].bottom-detectedFaces[iFace].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );

        //for(s32 iEye=0; iEye<detectedEyes.size(); iEye++) {
        //  //printf("eye (%d,%d) %d %d\n", detectedEyes[iEye].x, detectedEyes[iEye].y, detectedEyes[iEye].width, detectedEyes[iEye].height);
        //  cv::Rect shiftedRect(detectedEyes[iEye].x, detectedEyes[iEye].y, detectedEyes[iEye].width, detectedEyes[iEye].height);

        //  if(iEye == leftEyeIndex || iEye == rightEyeIndex) {
        //    if(eyeQuality > eyeQualityThreshold) {
        //      cv::rectangle(arrayCopyColor, shiftedRect, cv::Scalar(0,255,0), 3);
        //    } else {
        //      cv::rectangle(arrayCopyColor, shiftedRect, cv::Scalar(0,0,255), 3);
        //    }
        //  } else {
        //    cv::rectangle(arrayCopyColor, shiftedRect, cv::Scalar(255,255,255), 1);
        //  }
        //}

        Array<u8> clippedImage(detectedFaces[iFace].get_height()+1, detectedFaces[iFace].get_width()+1, scratchHuge);
        clippedImage(0,-1,0,-1).Set(image(detectedFaces[iFace].top, detectedFaces[iFace].bottom, detectedFaces[iFace].left, detectedFaces[iFace].right));

        cv::Mat_<u8> cvClippedImage;
        ArrayToCvMat(clippedImage, &cvClippedImage);

        s32 predictedLabel;
        //f64 confidence;

        cv::Mat_<u8> cvClippedImageResized = cvCreateMat(trainingWidth, trainingWidth, CV_8U);
        cv::resize(cvClippedImage, cvClippedImageResized, cvClippedImageResized.size());

        model->predict(cvClippedImageResized, predictedLabel, confidence);

        char text[1024];
        snprintf(text, 1024, "%s %0.0f", trainingLabelNames[predictedLabel].data(), confidence);
        //snprintf(text, 1024, "%f", eyeQuality);

        //printf("Predicted label: %d %s %f\n", predictedLabel, faceRecognizer_labelNames[predictedLabel], confidence);

        cv::putText(arrayCopyColor, text, cv::Point((detectedFaces[iFace].left + detectedFaces[iFace].right)/2, (detectedFaces[iFace].top + detectedFaces[iFace].bottom)/2), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255, 255, 255));
      } // for( s32 i = 0; i < detectedFaces.get_size(); i++ )
    } // if(detectedFaces.get_size() > 0)

    cv::imshow("detect", arrayCopyColor);

    //image.Show("image", false);

    cv::waitKey(30);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TestFaceRecognizer)
#endif // #if defined(RUN_HIGH_MEMORY_TESTS) && defined(RUN_PC_ONLY_TESTS)

GTEST_TEST(CoreTech_Vision, UpsampleByPowerOfTwoBilinear)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 downsamplePower = 4;
  const s32 upsamplePower = 4;

  const Array<u8> in(240, 320, const_cast<u8*>(&cozmo_date2014_04_10_time16_15_40_frame0[0]), 320*240, Flags::Buffer(false, false, true)); // = Array<u8>::LoadImage("C:/Anki/products-cozmo-large-files/systemTestsData/images/cozmo_date2014_06_04_time16_52_36_frame0.png", scratchHuge);

  Array<u8> inSmall(in.get_size(0)>>downsamplePower, in.get_size(1)>>downsamplePower, scratchOffchip);

  InitBenchmarking();

  BeginBenchmark("UpsampleByPowerOfTwoBilinear_DownsampleByPowerOfTwo");
  ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in, downsamplePower, inSmall, scratchOffchip);
  EndBenchmark("UpsampleByPowerOfTwoBilinear_DownsampleByPowerOfTwo");

  Array<u8> out(inSmall.get_size(0)<<upsamplePower, inSmall.get_size(1)<<upsamplePower, scratchOffchip);

  BeginBenchmark("UpsampleByPowerOfTwoBilinear_UpsampleByPowerOfTwoBilinear");
  /*const Result result =*/ ImageProcessing::UpsampleByPowerOfTwoBilinear<upsamplePower>(inSmall, out, scratchOffchip);
  EndBenchmark("UpsampleByPowerOfTwoBilinear_UpsampleByPowerOfTwoBilinear");

  //matlab.PutArray(inSmall, "inSmall");
  //matlab.PutArray(out, "out");
  //matlab.EvalStringEcho("outB = imresize(inSmall, size(inSmall)*%d, 'bilinear', 'Antialiasing', false); close all; imshows(2*out, 2*outB, 'maximize');", 1<<upsamplePower);
  //in.Show("in", false);
  //out.Show("out", true);

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, UpsampleByPowerOfTwoBilinear)

#ifdef RUN_HIGH_MEMORY_TESTS
GTEST_TEST(CoreTech_Vision, KLT)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  const Array<u8> image1(240, 320, const_cast<u8*>(&cozmo_date2014_01_29_time11_41_05_frame10_320x240[0]), 320*240, Flags::Buffer(false, false, true)); // = Array<u8>::LoadImage("Z:/Box Sync/Cozmo SE/systemTestImages_all/cozmo_date2014_01_23_time15_02_46_frame23.png", scratchHuge);
  const Array<u8> image2(240, 320, const_cast<u8*>(&cozmo_date2014_01_29_time11_41_05_frame12_320x240[0]), 320*240, Flags::Buffer(false, false, true)); // = Array<u8>::LoadImage("Z:/Box Sync/Cozmo SE/systemTestImages_all/cozmo_date2014_01_23_time15_02_46_frame24.png", scratchHuge);

  ASSERT_TRUE(AreValid(image1, image2));

  const s32 numPyramidLevels = 2;
  const s32 maxCorners = 100;
  const f32 qualityLevel = 0.1f;
  const int blockSize = 11;
  const double harrisK = 0.04;
  const f32 minEigThreshold = 0.001f;
  const s32 windowWidth = 11;
  const s32 termination_maxCount = 20;
  const f32 termination_epsilon = 0.3f;

  //#define TEST_OPENCV_KLT
#ifdef TEST_OPENCV_KLT
  cv::Mat image1Cv;
  cv::Mat image2Cv;

  ArrayToCvMat<u8>(image1, &image1Cv);
  ArrayToCvMat<u8>(image2, &image2Cv);

  std::vector<cv::Point2f> pointsCv[2];

  std::vector<cv::Point2f> cornersCv;
  cv::goodFeaturesToTrack(image1Cv, pointsCv[0], maxCorners, qualityLevel, 0, cv::Mat(), blockSize, true, harrisK);

  cv::TermCriteria termcrit(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, termination_maxCount, termination_epsilon);

  cv::Size winSize(windowWidth,windowWidth);

  std::vector<uchar> statusCv;
  std::vector<float> errCv;

  calcOpticalFlowPyrLK(image1Cv, image2Cv, pointsCv[0], pointsCv[1], statusCv, errCv, winSize, numPyramidLevels, termcrit, 0, minEigThreshold);
#endif // #ifdef TEST_OPENCV_KLT

  InitBenchmarking();

  BeginBenchmark("KLT_Pyramid");
  FixedLengthList<Array<u8> > pyramid1 = ImageProcessing::BuildPyramid<u8,u32,u8>(image1, numPyramidLevels, scratchHuge);
  EndBenchmark("KLT_Pyramid");

  BeginBenchmark("KLT_Pyramid");
  FixedLengthList<Array<u8> > pyramid2 = ImageProcessing::BuildPyramid<u8,u32,u8>(image2, numPyramidLevels, scratchHuge);
  EndBenchmark("KLT_Pyramid");

  ASSERT_TRUE(pyramid1.IsValid());
  ASSERT_TRUE(pyramid2.IsValid());

  //pyramid1[0].Show("p1 0", false, false);
  //pyramid1[1].Show("p1 1", false, false);
  //pyramid1[2].Show("p1 2", false, false);
  //pyramid1[3].Show("p1 3", false, false);

  //pyramid2[0].Show("p2 0", false, false);
  //pyramid2[1].Show("p2 1", false, false);
  //pyramid2[2].Show("p2 2", false, false);
  //pyramid2[3].Show("p2 3", true, false);

  BeginBenchmark("KLT_GoodFeaturesToTrack");
  const s32 numPointsMax = 50000;
  FixedLengthList<Point<s16> > points1S16(numPointsMax, scratchHuge);
  FixedLengthList<Point<f32> > points1F32(numPointsMax, scratchHuge);
  const Result gftResult = Features::GoodFeaturesToTrack(image1, points1S16, maxCorners, qualityLevel, blockSize, true, static_cast<f32>(harrisK), scratchOffchip, scratchHuge);
  EndBenchmark("KLT_GoodFeaturesToTrack");

  ASSERT_TRUE(gftResult == RESULT_OK);

  points1F32.set_size(points1S16.get_size());
  for(s32 i=0; i<numPointsMax; i++) {
    points1F32[i].SetCast(points1S16[i]);
  }

  FixedLengthList<Point<f32> > points2F32(numPointsMax, scratchHuge);

  FixedLengthList<bool> status(numPointsMax, scratchHuge);
  FixedLengthList<f32> err(numPointsMax, scratchHuge);

  BeginBenchmark("KLT_CalcOpticalFlowPyrLK");

  const Result kltResult = TemplateTracker::CalcOpticalFlowPyrLK(
    pyramid1, pyramid2,
    points1F32, points2F32,
    status, err,
    windowWidth, windowWidth,
    termination_maxCount, termination_epsilon,
    minEigThreshold,
    false,
    scratchHuge);

  EndBenchmark("KLT_CalcOpticalFlowPyrLK");

#ifdef TEST_OPENCV_KLT
  {
    Matlab matlab(false);
    matlab.PutArray(image1, "image1");
    matlab.PutArray(image2, "image2");

    matlab.EvalString("points1 = zeros(0,2); points2 = zeros(0,4); points1Cv = zeros(0,2); points2Cv = zeros(0,2);");
    for(s32 i=0; i<points1F32.get_size(); i++) {
      matlab.EvalString("points1(end+1,:) = [%f,%f];", points1F32[i].x, points1F32[i].y);
    }

    for(s32 i=0; i<points2F32.get_size(); i++) {
      matlab.EvalString("points2(end+1,:) = [%f,%f,%d,%f];", points2F32[i].x, points2F32[i].y, status[i], err[i]);
    }

    for(s32 i=0; i<pointsCv[0].size(); i++) {
      matlab.EvalString("points1Cv(end+1,:) = [%f,%f];", pointsCv[0][i].x, pointsCv[0][i].y);
    }

    for(s32 i=0; i<pointsCv[1].size(); i++) {
      matlab.EvalString("points2Cv(end+1,:) = [%f,%f];", pointsCv[1][i].x, pointsCv[1][i].y);
    }

    matlab.EvalString("figure(1); hold off; imshow(image1); hold on; scatter(points1(:,1), points1(:,2), 'g+');");
    matlab.EvalString("figure(2); hold off; imshow(image2); hold on; scatter(points2(points2(:,4)<10, 1), points2(points2(:,4)<10,2), 'g+'); scatter(points2(points2(:,4)>=10, 1), points2(points2(:,4)>=10,2), 'r+');");
    matlab.EvalString("figure(3); hold off; imshow(image1); hold on; scatter(points1Cv(:,1), points1Cv(:,2), 'g+');");
    matlab.EvalString("figure(4); hold off; imshow(image2); hold on; scatter(points2Cv(:,1), points2Cv(:,2), 'r+');");
  }
#endif // #ifdef TEST_OPENCV_KLT

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  //points1F32.Print("points1F32");
  //points2F32.Print("points2F32");
  //status.Print("status");
  //err.Print("err");

  ASSERT_TRUE(points1F32.get_size() == 72);
  ASSERT_TRUE(points2F32.get_size() == 72);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, KLT)
#endif // #ifdef RUN_HIGH_MEMORY_TESTS

#ifdef RUN_HIGH_MEMORY_TESTS
GTEST_TEST(CoreTech_Vision, Harris)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  const Array<u8> image(240, 320, const_cast<u8*>(simpleFiducials_320x240), 320*240); // = Array<u8>::LoadImage("Z:/Documents/Anki/products-cozmo-large-files/systemTestsData/images/cozmo_date2014_06_04_time16_52_38_frame0.png", scratchOffchip);
  Array<f32> harrisImage(image.get_size(0), image.get_size(1), scratchOffchip);

  ASSERT_TRUE(AreValid(image, harrisImage));

  const int blockSize = 11;
  const double harrisK = 0.04;

#ifdef TEST_OPENCV_HARRIS
  cv::Mat_<u8> imageCv;

  ArrayToCvMat<u8>(image, &imageCv);

  cv::Mat_<f32> harrisImageCv;
  cv::cornerHarris(imageCv, harrisImageCv, blockSize, 3, harrisK, cv::BORDER_DEFAULT);

  matlab.PutOpencvMat(imageCv, "imageCv");
  matlab.PutOpencvMat(harrisImageCv, "harrisImageCv");

#endif

  const Result cornerResult = Features::CornerHarris(image, harrisImage, blockSize, static_cast<f32>(harrisK), scratchOffchip);

  ASSERT_TRUE(cornerResult == RESULT_OK);

#if ANKICORETECH_EMBEDDED_USE_MATLAB
  {
    matlab.PutArray(harrisImage, "harrisImage");
  }
#endif // #if ANKICORETECH_EMBEDDED_USE_MATLAB

  const s32 maxCorners = 100;
  const f32 qualityLevel = 0.3f;

  FixedLengthList<Point<s16> > corners(5000, scratchHuge);

  InitBenchmarking();

  BeginBenchmark("GoodFeaturesToTrack Harris");
  const Result gftResult = Features::GoodFeaturesToTrack(image, corners, maxCorners, qualityLevel, blockSize, true, static_cast<f32>(harrisK), scratchOffchip, scratchHuge);
  EndBenchmark("GoodFeaturesToTrack Harris");

  ASSERT_TRUE(gftResult == RESULT_OK);

#ifdef TEST_OPENCV_HARRIS
  std::vector<cv::Point2f> cornersCv;
  cv::goodFeaturesToTrack(imageCv, cornersCv, maxCorners, qualityLevel, 0, cv::Mat(), blockSize, true, harrisK);

  cv::Mat drawnCorners(image.get_size(0), image.get_size(1), CV_8UC3);
  cv::Mat drawnCornersCv(image.get_size(0), image.get_size(1), CV_8UC3);

  matlab.EvalStringEcho("corners=zeros(0,2); cornersCv=zeros(0,2);");

  for(s32 i=0; i<corners.get_size(); i++) {
    cv::circle(drawnCorners, cv::Point(corners[i].x, corners[i].y), 2, cv::Scalar(255,0,0), -1);
    matlab.EvalStringEcho("corners(end+1,:) = [%d,%d];", s32(corners[i].x), s32(corners[i].y));
  }

  for(s32 i=0; i<(s32)(cornersCv.size()); i++) {
    cv::circle(drawnCornersCv, cv::Point(s32(cornersCv[i].x), s32(cornersCv[i].y)), 2, cv::Scalar(255,0,0), -1);
    matlab.EvalStringEcho("cornersCv(end+1,:) = [%d,%d];", s32(cornersCv[i].x), s32(cornersCv[i].y));
  }

  cv::imshow("image", imageCv);
  cv::imshow("drawnCorners", drawnCorners);
  cv::imshow("drawnCornersCv", drawnCornersCv);

  cv::waitKey();
#endif // #ifdef TEST_OPENCV_HARRIS

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LocalMaxima)
#endif // #ifdef RUN_HIGH_MEMORY_TESTS

GTEST_TEST(CoreTech_Vision, LocalMaxima)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 imageHeight = 10;
  const s32 imageWidth = 10;

  const u8 image_data[imageHeight*imageWidth] = {
    5, 5, 5, 5, 5, 5, 5, 5, 9, 5,
    9, 9, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    9, 5, 5, 5, 9, 5, 5, 5, 5, 5,
    5, 5, 9, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 9, 5, 5, 5,
    5, 9, 5, 5, 5, 5, 9, 5, 5, 5,
    5, 5, 9, 5, 5, 5, 5, 5, 5, 9,
    5, 9, 5, 5, 9, 9, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 9, 5};

  Array<u8> image(imageHeight, imageWidth, scratchCcm);
  FixedLengthList<Point<s16> > points(100, scratchCcm);

  ASSERT_TRUE(image.IsValid());

  image.Set(image_data, imageHeight*imageWidth);

  const Result result = ImageProcessing::LocalMaxima<u8>(image, 3, 3, points);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(points.get_size() == 2);

  ASSERT_TRUE(points[0] == Point<s16>(4, 3));
  ASSERT_TRUE(points[1] == Point<s16>(2, 4));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LocalMaxima)

#ifdef RUN_PC_ONLY_TESTS
GTEST_TEST(CoreTech_Vision, VisionMarkerImages)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  // Generate the list of filenames in Matlab:
  // patterns = {'Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/*.png'};
  // allFiles = {};
  // for iPattern = 1:length(patterns)
  //   files = dir(patterns{iPattern});
  //   for iFile = 1:length(files)
  //     allFiles{end+1} = [strrep(patterns{iPattern}, '*.png', ''), files(iFile).name];
  //   end
  // end
  // fprintf('const char * allFilenames[%d] = {', length(allFiles)); for i=1:length(allFiles) fprintf('"%s", ', allFiles{i}); end; disp('};');

  const char * allFilenames[113] = {"Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits001.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits002.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits003.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits004.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits005.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits006.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits007.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits008.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits009.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits010.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits011.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits012.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits013.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits014.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits015.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits016.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits017.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits018.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits019.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits020.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits021.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits022.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits023.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits024.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits025.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits026.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits027.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits028.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits029.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits030.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits031.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits032.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits033.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits034.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits035.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits036.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits037.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits038.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits039.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits040.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits041.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits042.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits043.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits044.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits045.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits046.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits047.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits048.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits049.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits050.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits051.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits052.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits053.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits054.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits055.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits056.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits057.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits058.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits059.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits060.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits061.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits062.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits063.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/ankiLogoWithBits064.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice1.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice2.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice3.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice4.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice5.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/dice6.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/0.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/1.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/2.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/3.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/4.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/5.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/6.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/7.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/8.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/A.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/B.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/C.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/D.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/E.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/F.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/G.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/H.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/J.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/K.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/L.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/M.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/N.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/P.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/Q.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/R.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/T.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/X.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/Y.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/angryFace.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/ankiLogo.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/arrow.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/bangBangBang.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/bullseye2.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/circularArrow.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/fire.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/questionMark.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rampBack.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rampFront.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rampLeft.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rampRight.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/squarePlusCorners.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/star5.png", "Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/stopWithHand.png", };

  FixedLengthList<const char*> imageFilenames(113, scratchOffchip, Flags::Buffer(true, false, true));
  imageFilenames.Set(allFilenames, 113);
  //imageFilenames.Print("imageFilenames");

  const f64 t0 = GetTimeF64();
  VisionMarkerImages vmi(imageFilenames, scratchHuge);
  const f64 t1 = GetTimeF64();

  //vmi.Show(50);

  const char * queryImageFilename = "Z:/Documents/Anki/products-cozmo-large-files/systemTestsData/images/cozmo_date2014_06_04_time16_52_38_frame0.png";
  Array<u8> queryImage = Array<u8>::LoadImage(queryImageFilename, scratchHuge);
  //queryImage.Show("queryImage", true);

  Quadrilateral<f32> quad(
    Point<f32>(64.1757f, 46.0676f),
    Point<f32>(61.9054f, 114.8243f),
    Point<f32>(134.8784f, 44.7703f),
    Point<f32>(133.9054f, 115.1486f));

  VisionMarker extractedMarker;
  f32 matchQuality;

  const f64 t2 = GetTimeF64();
  vmi.MatchExhaustive(queryImage, quad, extractedMarker, matchQuality, scratchOffchip, scratchHuge);
  const f64 t3 = GetTimeF64();

  printf("Database loaded in %f seconds and quad matched in %f seconds\n", t1-t0, t3-t2);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, VisionMarkerImages)
#endif // #ifdef RUN_PC_ONLY_TESTS

GTEST_TEST(CoreTech_Vision, LookupMarkerType)
{
  const s32 numNames = 18;

  const char *inputNames[numNames] = {
    "MARKER_0",
    "marker_0",
    "mARKER_0",
    "0",
    "c:\\path\\0.png",
    "~/path/0.png",
    "MARKER_Y",
    "MarKER_Y",
    "Y",
    "y",
    "c:\\y.png.out.mat.toast",
    "/home/users\\crazy\\/y.hello",
    "MARKER_UNKNOWN",
    "UNKNOWN",
    "unknown",
    "UNknoOWN",
    "\\unknown",
    "//unknown"};

  const Anki::Vision::MarkerType inputTypes[numNames] = {
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_0,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_Y,
    Anki::Vision::MARKER_UNKNOWN,
    Anki::Vision::MARKER_UNKNOWN,
    Anki::Vision::MARKER_UNKNOWN,
    Anki::Vision::MARKER_UNKNOWN,
    Anki::Vision::MARKER_UNKNOWN,
    Anki::Vision::MARKER_UNKNOWN};

  for(s32 i=0; i<numNames; i++) {
    const Anki::Vision::MarkerType lookedUp = LookupMarkerType(inputNames[i]);
    ASSERT_TRUE(lookedUp == inputTypes[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LookupMarkerType)

GTEST_TEST(CoreTech_Vision, IntegerCounts_Quadrilateral)
{
  const Quadrilateral<f32> quad(Point<f32>(1, 5), Point<f32>(6, 6), Point<f32>(7, 11), Point<f32>(0, 11));

  const s32 arrayHeight = 15;
  const s32 arrayWidth = 15;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(arrayHeight, arrayWidth, scratchCcm);

  s32 color = 0;
  for(s32 y=0; y<arrayHeight; y++) {
    for(s32 x=0; x<arrayWidth; x++) {
      image[y][x] = color;
      color++;
    }
  }

  //image.Print("image");

  {
    PUSH_MEMORY_STACK(scratchCcm);
    const IntegerCounts countsSkip2(image, quad, 2, 1, scratchCcm);

    ASSERT_TRUE(countsSkip2.IsValid());

    //countsSkip2.get_counts().Print("countsSkip2");

    // counts = zeros(256,1); counts(1+(76:78))=1; counts(1+(106:110))=1;counts(1+(135:141))=1; toArray(counts')
    const s32 countsSkip2_groundTruthData[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for(s32 i=0; i<256; i++) {
      //printf("%d) %d %d\n", i, countsSkip2.get_counts()[i], countsSkip2_groundTruthData[i]);
      ASSERT_TRUE(countsSkip2.get_counts()[i] == countsSkip2_groundTruthData[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(scratchCcm);
    const IntegerCounts countsSkip1(image, quad, 1, 1, scratchCcm);

    ASSERT_TRUE(countsSkip1.IsValid());

    //countsSkip1.get_counts().Print("countsSkip1");

    // counts = zeros(256,1); counts(1+(76:78))=1; counts(1+(91:95))=1; counts(1+(106:110))=1; counts(1+(120:126))=1; counts(1+(135:141))=1; counts(1+(150:156))=1; toArray(counts')
    const s32 countsSkip1_groundTruthData[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for(s32 i=0; i<256; i++) {
      //printf("%d) %d %d\n", i, countsSkip1.get_counts()[i], countsSkip1_groundTruthData[i]);
      ASSERT_TRUE(countsSkip1.get_counts()[i] == countsSkip1_groundTruthData[i]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, IntegerCounts_Quadrilateral)

GTEST_TEST(CoreTech_Vision, DistanceTransform)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Correctness test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 6;
    const s32 imageWidth = 8;
    const s32 numFractionalBits = 3;

    Array<u8> image(imageHeight, imageWidth, scratchOffchip);
    FixedPointArray<s16> distance(imageHeight, imageWidth, numFractionalBits, scratchOffchip);

    const u8 image_data[imageHeight*imageWidth] = {
      9, 9, 9, 9, 9, 4, 9, 9,
      3, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 2,
      9, 9, 9, 1, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 0};

    const u8 backgroundThreshold = 5;

    image.Set(image_data, imageHeight*imageWidth);

    const Result result = ImageProcessing::DistanceTransform(image, backgroundThreshold, distance);

    //image.Print("image");
    //distance.Print("distance");

    ASSERT_TRUE(result == RESULT_OK);

    FixedPointArray<s16> distance_groundTruth(imageHeight, imageWidth, numFractionalBits, scratchOffchip);

    const s16 distance_groundTruthData[imageHeight*imageWidth] = {
      8, 11, 19, 16, 8, 0, 8, 16,
      0, 8, 16, 16, 11, 8, 11, 8,
      8, 11, 11, 8, 11, 16, 8, 0,
      16, 16, 8, 0, 8, 16, 11, 8,
      24, 19, 11, 8, 11, 19, 11, 8,
      30, 22, 19, 16, 19, 16, 8, 0};

    distance_groundTruth.Set(distance_groundTruthData, imageHeight*imageWidth);

    ASSERT_TRUE(AreElementwiseEqual<s16>(distance, distance_groundTruth));
  } // Correctness test

  // Benchmarking test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 320;
    const s32 imageWidth = 120;
    const s32 numFractionalBits = 3;

    Array<u8> image(imageHeight, imageWidth, scratchOnchip);
    FixedPointArray<s16> distance(imageHeight, imageWidth, numFractionalBits, scratchOnchip);

    for(s32 y=0; y<imageHeight; y++) {
      u8 * restrict pImage = image.Pointer(y,0);
      for(s32 x=0; x<imageWidth; x++) {
        pImage[x] = static_cast<u8>(30*x + 10*y);
      }
    }

    const u8 backgroundThreshold = 128;

    InitBenchmarking();

    BeginBenchmark("DistanceTransform");

    const Result result = ImageProcessing::DistanceTransform(image, backgroundThreshold, distance);

    EndBenchmark("DistanceTransform");

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    ASSERT_TRUE(result == RESULT_OK);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DistanceTransform)

GTEST_TEST(CoreTech_Vision, FastGradient)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Correctness test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 5;
    const s32 imageWidth = 8;

    Array<u8> image(imageHeight, imageWidth, scratchOffchip);
    Array<s8> dx(imageHeight, imageWidth, scratchOffchip);
    Array<s8> dy(imageHeight, imageWidth, scratchOffchip);

    for(s32 y=0; y<imageHeight; y++) {
      for(s32 x=0; x<imageWidth; x++) {
        image[y][x] = static_cast<u8>(x*x*x + y);
      }
    }

    image[2][2] = 50;

    const Result result = ImageProcessing::FastGradient(
      image,
      dx, dy,
      scratchCcm);

    //image.Print("image");
    //dx.Print("dx");
    //dy.Print("dy");

    ASSERT_TRUE(result == RESULT_OK);

    Array<s8> dx_groundTruth(imageHeight, imageWidth, scratchOffchip);
    Array<s8> dy_groundTruth(imageHeight, imageWidth, scratchOffchip);

    const s8 dx_groundTruthData[imageHeight*imageWidth] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 4, 13, 28, 49, 76, -19, 0,
      0, 24, 13, 8, 49, 76, -19, 0,
      0, 4, 13, 28, 49, 76, -19, 0,
      0, 0, 0, 0, 0, 0, 0, 0};

    const s8 dy_groundTruthData[imageHeight*imageWidth] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 21, 1, 1, 1, 1, 0,
      0, 1, 1, 1, 1, 1, 1, 0,
      0, 1, -19, 1, 1, 1, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0};

    dx_groundTruth.Set(dx_groundTruthData, imageHeight*imageWidth);
    dy_groundTruth.Set(dy_groundTruthData, imageHeight*imageWidth);

    ASSERT_TRUE(AreElementwiseEqual<s8>(dx, dx_groundTruth));
    ASSERT_TRUE(AreElementwiseEqual<s8>(dy, dy_groundTruth));
  } // Correctness test

  // Benchmarking test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 120;
    const s32 imageWidth = 320;

    Array<u8> image(imageHeight, imageWidth, scratchOnchip);
    Array<s8> dx(imageHeight, imageWidth, scratchOnchip);
    Array<s8> dy(imageHeight, imageWidth, scratchOnchip);

    InitBenchmarking();

    BeginBenchmark("FastGradient");

    const Result result = ImageProcessing::FastGradient(
      image,
      dx, dy,
      scratchCcm);

    EndBenchmark("FastGradient");

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    ASSERT_TRUE(result == RESULT_OK);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, FastGradient)

GTEST_TEST(CoreTech_Vision, Canny)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  InitBenchmarking();

  //Array<u8> im(240, 320, scratchOffchip);
  //Array<u8> canny(240, 320, scratchOffchip);
  //cv::Mat imCv = cv::imread("Z:/Documents/Box Documents/Cozmo SE/blockImages/Screen Shot 2014-05-13 at 6.35.03 PM gray.png");
  //im.Set(imCv);

  //const Result result = CannyEdgeDetection(
  //  im, canny,
  //  low_thresh, high_thresh,
  //  aperture_size,
  //  scratchOffchip);

  //im.Show("im", false);
  //canny.Show("canny", true);

  //printf("");

  // Correctness test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 low_thresh = 50;
    const s32 high_thresh = 100;
    const s32 aperture_size = 3;

    const s32 imageHeight = 5;
    const s32 imageWidth = 8;

    Array<u8> image(imageHeight, imageWidth, scratchOffchip);
    Array<u8> canny(imageHeight, imageWidth, scratchOffchip);

    for(s32 y=0; y<imageHeight; y++) {
      for(s32 x=0; x<imageWidth; x++) {
        image[y][x] = static_cast<u8>(x*x*x + y);
      }
    }

    const Result result = CannyEdgeDetection(
      image, canny,
      low_thresh, high_thresh,
      aperture_size,
      scratchOffchip);

    //image.Print("image");
    //canny.Print("canny");

    ASSERT_TRUE(result == RESULT_OK);

    Array<u8> canny_groundTruth(imageHeight, imageWidth, scratchOffchip);

    const u8 canny_groundTruthData[imageHeight*imageWidth] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 255, 0, 0,
      0, 0, 0, 0, 0, 255, 0, 0,
      0, 0, 0, 0, 0, 255, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0};

    canny_groundTruth.Set(canny_groundTruthData, imageHeight*imageWidth);

    //canny_groundTruth.Print("canny_groundTruth");

    ASSERT_TRUE(AreElementwiseEqual<u8>(canny, canny_groundTruth));
  } // Correctness test

  // Benchmarking test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 low_thresh = 50;
    const s32 high_thresh = 100;
    const s32 aperture_size = 3;

    const s32 imageHeight = 240;
    const s32 imageWidth = 320;

    Array<u8> image(imageHeight, imageWidth, scratchOnchip);
    Array<u8> canny(imageHeight, imageWidth, scratchOffchip);

    image.Set(&cozmo_date2014_04_10_time16_15_40_frame0[0], imageHeight*imageWidth);

    BeginBenchmark("CannyEdgeDetection");

    const Result result = CannyEdgeDetection(
      image, canny,
      low_thresh, high_thresh,
      aperture_size,
      scratchOffchip);

    EndBenchmark("CannyEdgeDetection");

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    ASSERT_TRUE(result == RESULT_OK);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Canny)

GTEST_TEST(CoreTech_Vision, BoxFilterU8U16)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  InitBenchmarking();

  // Correctness test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 5;
    const s32 imageWidth = 8;

    const s32 boxHeight = 3;
    const s32 boxWidth = 3;

    Array<u8> image(imageHeight, imageWidth, scratchOffchip);

    for(s32 y=0; y<imageHeight; y++) {
      for(s32 x=0; x<imageWidth; x++) {
        image[y][x] = static_cast<u8>(x + y);
      }
    }

    Array<u16> filtered(imageHeight, imageWidth, scratchOffchip);
    filtered.Set(0xFFFF);

    const Result result = ImageProcessing::BoxFilter<u8,u16,u16>(image, boxHeight, boxWidth, filtered, scratchOnchip);

    //filtered.Print("filtered");

    //Matlab matlab(false);
    //matlab.PutArray(image, "image");
    //matlab.PutArray(filtered, "filtered");

    Array<u16> filtered_groundTruth(imageHeight, imageWidth, scratchOffchip);

    const u16 filtered_groundTruthData[imageHeight*imageWidth] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 18, 27, 36, 45, 54, 63, 0,
      0, 27, 36, 45, 54, 63, 72, 0,
      0, 36, 45, 54, 63, 72, 81, 0,
      0, 0, 0, 0, 0, 0, 0, 0};

    filtered_groundTruth.Set(filtered_groundTruthData, imageHeight*imageWidth);

    //filtered_groundTruth.Print("filtered_groundTruth");

    ASSERT_TRUE(AreElementwiseEqual<u16>(filtered, filtered_groundTruth));

    ASSERT_TRUE(result == RESULT_OK);
  } // Correctness test

  // Benchmarking test
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    const s32 imageHeight = 120;
    const s32 imageWidth = 320;

    const s32 boxHeight = 15;
    const s32 boxWidth = 15;

    Array<u8> image(imageHeight, imageWidth, scratchOnchip);

    for(s32 y=0; y<imageHeight; y++) {
      for(s32 x=0; x<imageWidth; x++) {
        image[y][x] = static_cast<u8>(x + y);
      }
    }

    Array<u16> filtered(imageHeight, imageWidth, scratchOnchip);
    filtered.Set(0xFFFF);

    BeginBenchmark("BoxFilter_u8u16u16");

    ImageProcessing::BoxFilter<u8,u16,u16>(image, boxHeight, boxWidth, filtered, scratchOnchip);

    EndBenchmark("BoxFilter_u8u16u16");

    BeginBenchmark("BoxFilter_u8u32u16");

    ImageProcessing::BoxFilter<u8,u32,u16>(image, boxHeight, boxWidth, filtered, scratchOnchip);

    EndBenchmark("BoxFilter_u8u32u16");

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    //    ASSERT_TRUE(result == RESULT_OK);
  } // Benchmarking test

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BoxFilterU8U16)

//GTEST_TEST(CoreTech_Vision, IntegralImageU16)
//{
//  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
//  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
//  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
//
//  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));
//
//  const s32 imageHeight = 200;
//  const s32 imageWidth = 200;
//
//  Array<u8> im(imageHeight, imageWidth, scratchOffchip);
//  im.Set(255);
//
//  Array<u16> filtered(imageHeight, imageWidth, scratchOffchip);
//
//  IntegralImage_u8_u16 ii(im, scratchOffchip);
//
//  const s32 averagingWidth = 21;
//  const s32 averagingWidth2 = averagingWidth / 2;
//
//  for(s32 y=averagingWidth2+1; y<(imageHeight-averagingWidth2-1); y++) {
//    for(s32 x=averagingWidth2+1; x<(imageWidth-averagingWidth2-1); x++) {
//      filtered[y][x] =
//        ii[y+averagingWidth2][x+averagingWidth2] -
//        ii[y+averagingWidth2][x-averagingWidth2] +
//        ii[y-averagingWidth2][x-averagingWidth2] -
//        ii[y-averagingWidth2][x+averagingWidth2];
//    }
//  }
//
//  Matlab matlab(false);
//  matlab.PutArray(im, "im");
//  matlab.PutArray(filtered, "filtered");
//
//  printf("");
//
//  GTEST_RETURN_HERE;
//}

GTEST_TEST(CoreTech_Vision, Vignetting)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 imageHeight = 4;
  const s32 imageWidth = 16;

  Array<u8> image(imageHeight,imageWidth,scratchOnchip);
  image.Set(128);

  FixedLengthList<f32> polynomialParameters(5, scratchOnchip, Flags::Buffer(false, false, true));
  const f32 parameters[5] = {1.0f, 0.01f, 0.03f, 0.01f, -0.01f};

  for(s32 i=0; i<5; i++)
    polynomialParameters[i] = parameters[i];

  const Result result = CorrectVignetting(image, polynomialParameters);

  ASSERT_TRUE(result == RESULT_OK);

  image.Print("image");

  Array<u8> image_groundTruth(imageHeight,imageWidth,scratchOnchip);

  const u8 image_groundTruthData[imageHeight*imageWidth] = {
    133, 133, 145, 145, 168, 168, 202, 202, 245, 245, 255, 255, 255, 255, 255, 255,
    133, 133, 145, 145, 168, 168, 202, 202, 245, 245, 255, 255, 255, 255, 255, 255,
    130, 130, 143, 143, 166, 166, 199, 199, 243, 243, 255, 255, 255, 255, 255, 255,
    130, 130, 143, 143, 166, 166, 199, 199, 243, 243, 255, 255, 255, 255, 255, 255};

  image_groundTruth.Set(image_groundTruthData, imageHeight*imageWidth);

  ASSERT_TRUE(AreElementwiseEqual<u8>(image, image_groundTruth));

  // Just benchmarks
  {
    Array<u8> imageOffchip(240,320,scratchOffchip);
    Array<u8> imageOnchip(240,320,scratchOnchip);

    InitBenchmarking();

    BeginBenchmark("CorrectVignetting_offchip");
    CorrectVignetting(imageOffchip, polynomialParameters);
    EndBenchmark("CorrectVignetting_offchip");

    BeginBenchmark("CorrectVignetting_onchip");
    CorrectVignetting(imageOnchip, polynomialParameters);
    EndBenchmark("CorrectVignetting_onchip");

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Vignetting)

#if defined(RUN_PC_ONLY_TESTS)
GTEST_TEST(CoreTech_Vision, FaceDetection_All)
{
  using namespace std;

  cv::CascadeClassifier face_cascade;

  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt.xml";
  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt2.xml";
  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt_tree.xml";
  const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml";

  //Classifier::CascadeClassifier cc(face_cascade_name, scratchOffchip);
  //cc.SaveAsHeader("c:/tmp/cascade.h", "lbpcascade_frontalface");

  if( !face_cascade.load( face_cascade_name ) ) {
    CoreTechPrint("Could not load %s\n", face_cascade_name);
    return;
  }

  std::ifstream faceFilenames("C:/datasets/faces/lfw/allFiles.txt");

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const cv::Size minSize = cv::Size(30,30);

  const s32 MAX_CANDIDATES = 5000;

  FixedLengthList<Rectangle<s32> > detectedFaces_anki(MAX_CANDIDATES, scratchOffchip);

  Classifier::CascadeClassifier_LBP cc(face_cascade_name, scratchOffchip);

  //s32 curSet = 0;
  while(!faceFilenames.eof()) {
    const s32 numImagesAtATime = 25;
    //const s32 numImagesAtATime = 1;
    std::string lines[numImagesAtATime];

    const s32 maxX = 1800;
    const s32 borderPixelsX = 10;
    const s32 borderPixelsY = 20;
    s32 largestY = 0;
    s32 curX = 0;
    s32 curY = 0;

    for(s32 in=0; in<numImagesAtATime; in++) {
      PUSH_MEMORY_STACK(scratchOffchip);

      getline(faceFilenames, lines[in]);

      /*curSet++;

      if(curSet < 47)
      continue;*/

      vector<cv::Rect> detectedFaces_opencv;

      cv::Mat image = cv::imread(lines[in]);

      cv::Mat grayImage = image;
      if( grayImage.channels() > 1 )
      {
        cv::Mat temp;
        cvtColor(grayImage, temp, CV_BGR2GRAY);
        grayImage = temp;
      }

      Array<u8> imageArray(grayImage.rows, grayImage.cols, scratchOffchip);
      imageArray.Set(grayImage);

      const f64 t0 = GetTimeF64();

      face_cascade.detectMultiScale(
        grayImage,
        detectedFaces_opencv,
        1.1, // double scaleFactor=1.1,
        2, // int minNeighbors=3,
        0|CV_HAAR_SCALE_IMAGE, // int flags=0,
        cv::Size(30, 30), // Size minSize=Size(),
        cv::Size() // Size maxSize=Size()
        );

      const f64 t1 = GetTimeF64();

      const cv::Size maxSize = cv::Size(imageArray.get_size(1), imageArray.get_size(0));

      cc.DetectMultiScale(
        imageArray,
        static_cast<f32>(scaleFactor),
        minNeighbors,
        minSize.height, minSize.width,
        maxSize.height, maxSize.width,
        detectedFaces_anki,
        scratchOnchip,
        scratchOffchip);

      const f64 t2 = GetTimeF64();

      CoreTechPrint("OpenCV took %f seconds and Anki took %f seconds\n", t1-t0, t2-t1);

      cv::Mat toShow;

      if(image.channels() == 1) {
        (image.rows, image.cols, CV_8UC3);

        vector<cv::Mat> channels;
        channels.push_back(image);
        channels.push_back(image);
        channels.push_back(image);
        cv::merge(channels, toShow);
      } else {
        toShow = image;
      }

      for( s32 i = 0; i < detectedFaces_anki.get_size(); i++ )
      {
        cv::Point center( Round<s32>((detectedFaces_anki[i].left + detectedFaces_anki[i].right)*0.5), Round<s32>((detectedFaces_anki[i].top + detectedFaces_anki[i].bottom)*0.5) );
        cv::ellipse( toShow, center, cv::Size( Round<s32>((detectedFaces_anki[i].right-detectedFaces_anki[i].left)*0.5), Round<s32>((detectedFaces_anki[i].bottom-detectedFaces_anki[i].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );
      }

      for( size_t i = 0; i < detectedFaces_opencv.size(); i++ )
      {
        cv::Point center( Round<s32>(detectedFaces_opencv[i].x + detectedFaces_opencv[i].width*0.5), Round<s32>(detectedFaces_opencv[i].y + detectedFaces_opencv[i].height*0.5) );
        cv::ellipse( toShow, center, cv::Size( Round<s32>(detectedFaces_opencv[i].width*0.5), Round<s32>(detectedFaces_opencv[i].height*0.5)), 0, 0, 360, cv::Scalar( 0, 0, 255 ), 1, 8, 0 );
      }

      const s32 maxChars = 1024;
      char outname[maxChars];
      snprintf(outname, maxChars, "Detected faces %d", in);

      //-- Show what you got
      cv::imshow(outname, toShow);

      cv::moveWindow(outname, curX, curY);

      largestY = MAX(largestY, toShow.rows);

      curX += toShow.cols + borderPixelsX;

      if(curX > maxX) {
        curX = 0;
        curY += largestY + borderPixelsY;
      }
    }

    cv::waitKey();
  } // while(!faceFilenames.eof())

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, FaceDetection_All)
#endif // #if defined(RUN_PC_ONLY_TESTS)

GTEST_TEST(CoreTech_Vision, FaceDetection)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt";
  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt2";
  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt_tree";
  //const char * face_cascade_name = "C:/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml";

  const s32 imageHeight = 240;
  const s32 imageWidth = 320;

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const s32 minHeight = 30;
  const s32 minWidth = 30;
  const s32 maxHeight = imageHeight;
  const s32 maxWidth = imageWidth;

  const s32 MAX_CANDIDATES = 5000;

  Array<u8> image(imageHeight, imageWidth, scratchOnchip);
  image.Set(&cozmo_date2014_04_10_time16_15_40_frame0[0], imageHeight*imageWidth);

  FixedLengthList<Rectangle<s32> > detectedFaces_anki(MAX_CANDIDATES, scratchOffchip);

  // Load from the openCV XML file
  // Classifier::CascadeClassifier_LBP cc(face_cascade_name, scratchOffchip);
  // cc.SaveAsHeader("c:/tmp/lbpcascade_frontalface.h", "lbpcascade_frontalface");

  const f64 t0 = GetTimeF64();

  // TODO: are these const casts okay?
  const FixedLengthList<Classifier::CascadeClassifier::Stage> &stages = FixedLengthList<Classifier::CascadeClassifier::Stage>(lbpcascade_frontalface_stages_length, const_cast<Classifier::CascadeClassifier::Stage*>(&lbpcascade_frontalface_stages_data[0]), lbpcascade_frontalface_stages_length*sizeof(Classifier::CascadeClassifier::Stage) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTree> &classifiers = FixedLengthList<Classifier::CascadeClassifier::DTree>(lbpcascade_frontalface_classifiers_length, const_cast<Classifier::CascadeClassifier::DTree*>(&lbpcascade_frontalface_classifiers_data[0]), lbpcascade_frontalface_classifiers_length*sizeof(Classifier::CascadeClassifier::DTree) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Classifier::CascadeClassifier::DTreeNode> &nodes =  FixedLengthList<Classifier::CascadeClassifier::DTreeNode>(lbpcascade_frontalface_nodes_length, const_cast<Classifier::CascadeClassifier::DTreeNode*>(&lbpcascade_frontalface_nodes_data[0]), lbpcascade_frontalface_nodes_length*sizeof(Classifier::CascadeClassifier::DTreeNode) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));;
  const FixedLengthList<f32> &leaves = FixedLengthList<f32>(lbpcascade_frontalface_leaves_length, const_cast<f32*>(&lbpcascade_frontalface_leaves_data[0]), lbpcascade_frontalface_leaves_length*sizeof(f32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<s32> &subsets = FixedLengthList<s32>(lbpcascade_frontalface_subsets_length, const_cast<s32*>(&lbpcascade_frontalface_subsets_data[0]), lbpcascade_frontalface_subsets_length*sizeof(s32) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));
  const FixedLengthList<Rectangle<s32> > &featureRectangles = FixedLengthList<Rectangle<s32> >(lbpcascade_frontalface_featureRectangles_length, const_cast<Rectangle<s32>*>(reinterpret_cast<const Rectangle<s32>*>(&lbpcascade_frontalface_featureRectangles_data[0])), lbpcascade_frontalface_featureRectangles_length*sizeof(Rectangle<s32>) + MEMORY_ALIGNMENT_RAW, Flags::Buffer(false,false,true));

  InitBenchmarking();

  BeginBenchmark("CascadeClassifier_LBP constructor");

  Classifier::CascadeClassifier_LBP cc(
    lbpcascade_frontalface_isStumpBased,
    lbpcascade_frontalface_stageType,
    lbpcascade_frontalface_featureType,
    lbpcascade_frontalface_ncategories,
    lbpcascade_frontalface_origWinHeight,
    lbpcascade_frontalface_origWinWidth,
    stages,
    classifiers,
    nodes,
    leaves,
    subsets,
    featureRectangles,
    scratchCcm);

  EndBenchmark("CascadeClassifier_LBP constructor");

  const f64 t1 = GetTimeF64();

  const Result result = cc.DetectMultiScale(
    image,
    static_cast<f32>(scaleFactor),
    minNeighbors,
    minHeight, minWidth,
    maxHeight, maxWidth,
    detectedFaces_anki,
    scratchOnchip,
    scratchOffchip);

  ASSERT_TRUE(result == RESULT_OK);

  const f64 t2 = GetTimeF64();

  CoreTechPrint("Detection took %f seconds (setup time %f seconds)\n", t2-t1, t1-t0);

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  //detectedFaces_anki[0].Print();

  ASSERT_TRUE(detectedFaces_anki.get_size() == 1);
  ASSERT_TRUE(detectedFaces_anki[0] == Rectangle<s32>(102, 219, 40, 156));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, FaceDetection)

GTEST_TEST(CoreTech_Vision, ResizeImage)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<f32> in(2, 5, scratchOnchip);
  Array<f32> outBig(3, 6, scratchOnchip);
  Array<f32> outSmall(2, 5, scratchOnchip);

  in[0][0] = 1; in[0][1] = 2; in[0][2] = 3; in[0][3] = 4; in[0][4] = 5;
  in[1][0] = 6; in[1][1] = 7; in[1][2] = 8; in[1][3] = 9; in[1][4] = 5;

  {
    const Result result = ImageProcessing::Resize<f32,f32>(in, outBig);

    outBig.Print("outBig");

    ASSERT_TRUE(result == RESULT_OK);
  }

  Array<f32> outBig_groundTruth(3, 6, scratchOnchip);

  outBig_groundTruth[0][0] = 1.0000f; outBig_groundTruth[0][1] = 1.7500f; outBig_groundTruth[0][2] = 2.5833f; outBig_groundTruth[0][3] = 3.4167f; outBig_groundTruth[0][4] = 4.2500f; outBig_groundTruth[0][5] = 5.0000f;
  outBig_groundTruth[1][0] = 3.5000f; outBig_groundTruth[1][1] = 4.2500f; outBig_groundTruth[1][2] = 5.0833f; outBig_groundTruth[1][3] = 5.9167f; outBig_groundTruth[1][4] = 6.1250f; outBig_groundTruth[1][5] = 5.0000f;
  outBig_groundTruth[2][0] = 6.0000f; outBig_groundTruth[2][1] = 6.7500f; outBig_groundTruth[2][2] = 7.5833f; outBig_groundTruth[2][3] = 8.4167f; outBig_groundTruth[2][4] = 8.0000f; outBig_groundTruth[2][5] = 5.0000f;

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(outBig, outBig_groundTruth, .01, .01));

  {
    const Result result = ImageProcessing::Resize<f32,f32>(outBig_groundTruth, outSmall);

    outSmall.Print("outSmall");

    ASSERT_TRUE(result == RESULT_OK);
  }

  Array<f32> outSmall_groundTruth(2, 5, scratchOnchip);

  outSmall_groundTruth[0][0] = 1.7000f; outSmall_groundTruth[0][1] = 2.6250f; outSmall_groundTruth[0][2] = 3.6250f; outSmall_groundTruth[0][3] = 4.5156f; outSmall_groundTruth[0][4] = 4.9719f;
  outSmall_groundTruth[1][0] = 5.4500f; outSmall_groundTruth[1][1] = 6.3750f; outSmall_groundTruth[1][2] = 7.3750f; outSmall_groundTruth[1][3] = 7.6094f; outSmall_groundTruth[1][4] = 5.2531f;

  ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(outSmall, outSmall_groundTruth, .01, .01));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ResizeImage)

GTEST_TEST(CoreTech_Vision, DecisionTreeVision)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 numNodes = 7;
  const s32 treeDataLength = numNodes * sizeof(FiducialMarkerDecisionTree::Node);
  FiducialMarkerDecisionTree::Node treeData[numNodes];
  const s32 treeDataNumFractionalBits = 0;
  const s32 treeMaxDepth = 2;
  const s32 numProbeOffsets = 1;
  const s16 probeXOffsets[numProbeOffsets] = {0};
  const s16 probeYOffsets[numProbeOffsets] = {0};
  const u8 grayvalueThreshold = 128;

  treeData[0].probeXCenter = 0;
  treeData[0].probeYCenter = 0;
  treeData[0].leftChildIndex = 1;
  treeData[0].label = 0;

  treeData[1].probeXCenter = 1;
  treeData[1].probeYCenter = 0;
  treeData[1].leftChildIndex = 3;
  treeData[1].label = 1;

  treeData[2].probeXCenter = 2;
  treeData[2].probeYCenter = 0;
  treeData[2].leftChildIndex = 5;
  treeData[2].label = 2;

  treeData[3].probeXCenter = 0x7FFF;
  treeData[3].probeYCenter = 0x7FFF;
  treeData[3].leftChildIndex = 0xFFFF;
  treeData[3].label = 3 + (1<<15);

  treeData[4].probeXCenter = 0x7FFF;
  treeData[4].probeYCenter = 0x7FFF;
  treeData[4].leftChildIndex = 0xFFFF;
  treeData[4].label = 4 + (1<<15);

  treeData[5].probeXCenter = 0x7FFF;
  treeData[5].probeYCenter = 0x7FFF;
  treeData[5].leftChildIndex = 0xFFFF;
  treeData[5].label = 5 + (1<<15);

  treeData[6].probeXCenter = 0x7FFF;
  treeData[6].probeYCenter = 0x7FFF;
  treeData[6].leftChildIndex = 0xFFFF;
  treeData[6].label = 6 + (1<<15);

  FiducialMarkerDecisionTree tree(reinterpret_cast<u8*>(&treeData[0]), treeDataLength, treeDataNumFractionalBits, treeMaxDepth, probeXOffsets, probeYOffsets, numProbeOffsets, NULL, 0);

  Array<f32> homography = Eye<f32>(3,3,scratchOnchip);
  Array<u8> image(1,3,scratchOnchip);

  image[0][0] = grayvalueThreshold; // black
  image[0][1] = grayvalueThreshold + 1; // white

  s32 label = -1;
  const Result result = tree.Classify(image, homography, grayvalueThreshold, label);

  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(label == 4);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DecisionTreeVision)

/*
TODO: Re-enable this test once the binary tracker isn't hard-coded to use the old battery marker.

GTEST_TEST(CoreTech_Vision, BinaryTrackerHeaderTemplate)
{
const s32 imageHeight = 240;
const s32 imageWidth = 320;

MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

Array<u8> templateImage(imageHeight, imageWidth, scratchOnchip);

const Quadrilateral<f32> templateQuad(
Point<f32>(90, 100),
Point<f32>(91, 228),
Point<f32>(218, 100),
Point<f32>(217, 227));

TemplateTracker::BinaryTracker::EdgeDetectionParameters edgeDetectionParams_template(
TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE,
4,    // s32 threshold_yIncrement; //< How many pixels to use in the y direction (4 is a good value?)
4,    // s32 threshold_xIncrement; //< How many pixels to use in the x direction (4 is a good value?)
0.1f, // f32 threshold_blackPercentile; //< What percentile of histogram energy is black? (.1 is a good value)
0.9f, // f32 threshold_whitePercentile; //< What percentile of histogram energy is white? (.9 is a good value)
0.8f, // f32 threshold_scaleRegionPercent; //< How much to scale template bounding box (.8 is a good value)
2,    // s32 minComponentWidth; //< The smallest horizontal size of a component (1 to 4 is good)
500,  // s32 maxDetectionsPerType; //< As many as you have memory and time for (500 is good)
1,    // s32 combHalfWidth; //< How far apart to compute the derivative difference (1 is good)
20,   // s32 combResponseThreshold; //< The minimum absolute-value response to start an edge component (20 is good)
1     // s32 everyNLines; //< As many as you have time for
);

//TemplateTracker::BinaryTracker::EdgeDetectionParameters edgeDetectionParams_update = edgeDetectionParams_template;
//edgeDetectionParams_update.maxDetectionsPerType = 2500;

//const s32 updateEdgeDetection_maxDetectionsPerType = 2500;

//const s32 normal_matching_maxTranslationDistance = 7;
//const s32 normal_matching_maxProjectiveDistance = 7;

const f32 scaleTemplateRegionPercent = 1.05f;

//const s32 verify_maxTranslationDistance = 1;

//const u8 verify_maxPixelDifference = 30;

//const s32 verify_coordinateIncrement = 3;

//const s32 ransac_matching_maxProjectiveDistance = normal_matching_maxProjectiveDistance;

//const s32 ransac_maxIterations = 20;
//const s32 ransac_numSamplesPerType = 8;
//const s32 ransac_inlinerDistance = verify_maxTranslationDistance;

templateImage.Set(&cozmo_date2014_04_04_time17_40_08_frame0[0], imageHeight*imageWidth);

// Skip zero rows/columns (non-list)
{
PUSH_MEMORY_STACK(scratchCcm);
PUSH_MEMORY_STACK(scratchOnchip);
PUSH_MEMORY_STACK(scratchOffchip);

CoreTechPrint("Skip 0 nonlist\n");
edgeDetectionParams_template.everyNLines = 1;

InitBenchmarking();

BeginBenchmark("BinaryTracker init");

TemplateTracker::BinaryTracker tracker(
Anki::Vision::MARKER_ANGRYFACE,
templateImage, templateQuad,
scaleTemplateRegionPercent,
edgeDetectionParams_template,
scratchOnchip, scratchOffchip);

EndBenchmark("BinaryTracker init");

//templateImage.Show("templateImage",false);
//nextImage.Show("nextImage",false);
//tracker.ShowTemplate(true, false);

const s32 numTemplatePixels = tracker.get_numTemplatePixels();

ASSERT_TRUE(numTemplatePixels == 1588);

ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
} // Skip zero rows/columns (non-list)

GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BinaryTrackerHeaderTemplate)
*/

GTEST_TEST(CoreTech_Vision, BinaryTracker)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> templateImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOnchip);
  Array<u8> nextImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOnchip);

  const Quadrilateral<f32> templateQuad(Point<f32>(128,78), Point<f32>(220,74), Point<f32>(229,167), Point<f32>(127,171));

  TemplateTracker::BinaryTracker::EdgeDetectionParameters edgeDetectionParams_template(
    TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE,
    //TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE,
    4,    // s32 threshold_yIncrement; //< How many pixels to use in the y direction (4 is a good value?)
    4,    // s32 threshold_xIncrement; //< How many pixels to use in the x direction (4 is a good value?)
    0.1f, // f32 threshold_blackPercentile; //< What percentile of histogram energy is black? (.1 is a good value)
    0.9f, // f32 threshold_whitePercentile; //< What percentile of histogram energy is white? (.9 is a good value)
    0.8f, // f32 threshold_scaleRegionPercent; //< How much to scale template bounding box (.8 is a good value)
    2,    // s32 minComponentWidth; //< The smallest horizontal size of a component (1 to 4 is good)
    500,  // s32 maxDetectionsPerType; //< As many as you have memory and time for (500 is good)
    1,    // s32 combHalfWidth; //< How far apart to compute the derivative difference (1 is good)
    20,   // s32 combResponseThreshold; //< The minimum absolute-value response to start an edge component (20 is good)
    1     // s32 everyNLines; //< As many as you have time for
    );

  TemplateTracker::BinaryTracker::EdgeDetectionParameters edgeDetectionParams_update = edgeDetectionParams_template;
  edgeDetectionParams_update.maxDetectionsPerType = 2500;

  const s32 normal_matching_maxTranslationDistance = 7;
  const s32 normal_matching_maxProjectiveDistance = 7;

  const f32 scaleTemplateRegionPercent = 1.05f;

  const s32 verify_maxTranslationDistance = 1;

  const u8 verify_maxPixelDifference = 30;

  const s32 verify_coordinateIncrement = 3;

  const s32 ransac_matching_maxProjectiveDistance = normal_matching_maxProjectiveDistance;

  const s32 ransac_maxIterations = 20;
  const s32 ransac_numSamplesPerType = 8;
  const s32 ransac_inlinerDistance = verify_maxTranslationDistance;

  templateImage.Set(&cozmo_date2014_01_29_time11_41_05_frame10_320x240[0], cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT);
  nextImage.Set(&cozmo_date2014_01_29_time11_41_05_frame12_320x240[0], cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT);

  // Skip zero rows/columns (non-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("Skip 0 nonlist\n");
    edgeDetectionParams_template.everyNLines = 1;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");

    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);
    //tracker.ShowTemplate(true, false);

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      ASSERT_TRUE(numTemplatePixels == 1292);
    } else if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
      ASSERT_TRUE(numTemplatePixels == 1366);
    }

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_Normal(
      nextImage,
      edgeDetectionParams_update,
      normal_matching_maxTranslationDistance, normal_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    // TODO: verify this number manually
    /*ASSERT_TRUE(verify_numMatches == 1241);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 6);
    ASSERT_TRUE(verify_numInBounds == 1155);
    ASSERT_TRUE(verify_numSimilarPixels == 1137);*/

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip zero rows/columns (non-list)

  // Skip one row/column (non-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("\nSkip 1 nonlist\n");
    edgeDetectionParams_template.everyNLines = 2;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");
    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      ASSERT_TRUE(numTemplatePixels == 647);
    } else if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
      ASSERT_TRUE(numTemplatePixels == 678);
    }

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_Normal(
      nextImage,
      edgeDetectionParams_update,
      normal_matching_maxTranslationDistance, normal_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    // TODO: verify this number manually
    /*ASSERT_TRUE(verify_numMatches == 622);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 6);
    ASSERT_TRUE(verify_numInBounds == 1155);
    ASSERT_TRUE(verify_numSimilarPixels == 1143);*/

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip one row/column (non-list)

  // Skip zero rows/columns (with-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("\nSkip 0 list\n");
    edgeDetectionParams_template.everyNLines = 1;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");
    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      ASSERT_TRUE(numTemplatePixels == 1292);
    } else if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
      ASSERT_TRUE(numTemplatePixels == 1366);
    }

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_List(
      nextImage,
      edgeDetectionParams_update,
      normal_matching_maxTranslationDistance, normal_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    // TODO: verify this number manually
    /*ASSERT_TRUE(verify_numMatches == 1241);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 6);
    ASSERT_TRUE(verify_numInBounds == 1155);
    ASSERT_TRUE(verify_numSimilarPixels == 1137);*/

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.068f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip zero rows/columns (with-list)

  // Skip one row/column (with-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("\nSkip 1 list\n");
    edgeDetectionParams_template.everyNLines = 2;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");
    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      ASSERT_TRUE(numTemplatePixels == 647);
    } else if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
      ASSERT_TRUE(numTemplatePixels == 678);
    }

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_List(
      nextImage,
      edgeDetectionParams_update,
      normal_matching_maxTranslationDistance, normal_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    // TODO: verify this number manually
    /*ASSERT_TRUE(verify_numMatches == 622);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 6);
    ASSERT_TRUE(verify_numInBounds == 1155);
    ASSERT_TRUE(verify_numSimilarPixels == 1143);*/

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.060f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip one row/column (with-list)

  // Skip zero rows/columns (with-ransac)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("\nSkip 0 ransac\n");
    edgeDetectionParams_template.everyNLines = 1;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");
    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    //ASSERT_TRUE(numTemplatePixels == );

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_Ransac(
      nextImage,
      edgeDetectionParams_update,
      ransac_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      ransac_maxIterations, ransac_numSamplesPerType, ransac_inlinerDistance,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    CoreTechPrint("numMatches = %d / %d\n", verify_numMatches, numTemplatePixels);

    // TODO: verify this number manually
    //ASSERT_TRUE(verify_numMatches == );

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.068f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      //ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip zero rows/columns (with-ransac)

  // Skip one row/column (with-ransac)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    CoreTechPrint("\nSkip 1 ransac\n");
    edgeDetectionParams_template.everyNLines = 2;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");
    TemplateTracker::BinaryTracker tracker(
      templateImage, templateQuad,
      scaleTemplateRegionPercent,
      edgeDetectionParams_template,
      scratchOnchip, scratchOffchip);
    EndBenchmark("BinaryTracker init");

    const s32 numTemplatePixels = tracker.get_numTemplatePixels();

    //ASSERT_TRUE(numTemplatePixels == );

    //templateImage.Show("templateImage",false);
    //nextImage.Show("nextImage",false);

    BeginBenchmark("BinaryTracker update fixed-float");

    s32 verify_numMatches;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    const Result result = tracker.UpdateTrack_Ransac(
      nextImage,
      edgeDetectionParams_update,
      ransac_matching_maxProjectiveDistance,
      verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
      ransac_maxIterations, ransac_numSamplesPerType, ransac_inlinerDistance,
      verify_numMatches, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
      scratchCcm, scratchOffchip);
    EndBenchmark("BinaryTracker update fixed-float");

    ASSERT_TRUE(result == RESULT_OK);

    CoreTechPrint("numMatches = %d / %d\n", verify_numMatches, numTemplatePixels);

    // TODO: verify this number manually
    //ASSERT_TRUE(verify_numMatches == );

    //Array<u8> warpedTemplateImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.060f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      //ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
  } // Skip one row/column (with-ransac)

  //tracker.get_transformation().Transform(templateImage, warpedTemplateImage, scratchOffchip, 1.0f);

  //templateImage.Show("templateImage", false, false, true);
  //nextImage.Show("nextImage", false, false, true);
  //warpedTemplateImage.Show("warpedTemplateImage", false, false, true);
  //tracker.ShowTemplate(false, true);
  //cv::waitKey();

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BinaryTracker)

GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_DerivativeThreshold)
{
  const s32 combHalfWidth = 1;
  const s32 combResponseThreshold = 20;
  const s32 maxExtrema = 500;
  const s32 everyNLines = 1;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 imageHeight = 48;
  const s32 imageWidth = 64;

  Array<u8> image(imageHeight, imageWidth, scratchOffchip);

  EdgeLists edges;

  edges.xDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.xIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.yDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.yIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);

  for(s32 y=0; y<24; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = (y)*8;
    }
  }

  for(s32 y=24; y<48; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = 250 - (((y)*4));
    }
  }

  for(s32 x=31; x<48; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = (x-31)*10;
    }
  }
  for(s32 x=48; x<64; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = 250 - (((x-31)*6) - (x+1)/2);
    }
  }

  //Matlab matlab(false);
  //matlab.PutArray(image, "image");

  const Result result = DetectBlurredEdges_DerivativeThreshold(image, combHalfWidth, combResponseThreshold, everyNLines, edges);

  ASSERT_TRUE(result == RESULT_OK);

  //cv::Mat drawEdges = edges.DrawIndexes(imageHeight, imageWidth, edges.xDecreasing, edges.xIncreasing, edges.yDecreasing, edges.yIncreasing);
  //image.Show("image", false, false, true);
  //cv::namedWindow("drawEdges", CV_WINDOW_NORMAL);
  //cv::imshow("drawEdges", drawEdges);
  //cv::waitKey();

  //xDecreasing.Print("xDecreasing");
  //xIncreasing.Print("xIncreasing");
  //yDecreasing.Print("yDecreasing");
  //yIncreasing.Print("yIncreasing");

  ASSERT_TRUE(edges.xDecreasing.get_size() == 5);
  ASSERT_TRUE(edges.xIncreasing.get_size() == 42);
  ASSERT_TRUE(edges.yDecreasing.get_size() == 30);
  ASSERT_TRUE(edges.yIncreasing.get_size() == 0);

  ASSERT_TRUE(edges.xDecreasing[0] == Point<s16>(30,3));

  for(s32 i=1;i<=4;i++) {
    ASSERT_TRUE(edges.xDecreasing[i] == Point<s16>(38,19+i));
  }

  for(s32 i=0;i<3;i++) {
    ASSERT_TRUE(edges.xIncreasing[i] == Point<s16>(39,i+1));
  }

  for(s32 i=3;i<19;i++) {
    ASSERT_TRUE(edges.xIncreasing[i] == Point<s16>(38,i+1));
  }

  for(s32 i=19;i<42;i++) {
    ASSERT_TRUE(edges.xIncreasing[i] == Point<s16>(38,i+5));
  }

  for(s32 i=0;i<30;i++) {
    ASSERT_TRUE(edges.yDecreasing[i] == Point<s16>(i+1,23));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_DerivativeThreshold)

GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_GrayvalueThreshold)
{
  const u8 grayvalueThreshold = 128;
  const s32 minComponentWidth = 3;
  const s32 maxExtrema = 500;
  const s32 everyNLines = 1;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 imageHeight = 48;
  const s32 imageWidth = 64;

  Array<u8> image(imageHeight, imageWidth, scratchOffchip);

  EdgeLists edges;

  edges.xDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.xIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.yDecreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);
  edges.yIncreasing = FixedLengthList<Point<s16> >(maxExtrema, scratchOffchip);

  for(s32 y=0; y<24; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = (y)*8;
    }
  }

  for(s32 y=24; y<48; y++) {
    for(s32 x=0; x<32; x++) {
      image[y][x] = 250 - (((y)*4));
    }
  }

  for(s32 x=31; x<48; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = (x-31)*10;
    }
  }
  for(s32 x=48; x<64; x++) {
    for(s32 y=0; y<48; y++) {
      image[y][x] = 250 - (((x-31)*6) - (x+1)/2);
    }
  }

  //image.Print("image");

  //image.Show("image", true);

  const Result result = DetectBlurredEdges_GrayvalueThreshold(image, grayvalueThreshold, minComponentWidth, everyNLines, edges);

  ASSERT_TRUE(result == RESULT_OK);

  //xDecreasing.Print("xDecreasing");
  //xIncreasing.Print("xIncreasing");
  //yDecreasing.Print("yDecreasing");
  //yIncreasing.Print("yIncreasing");

  ASSERT_TRUE(edges.xDecreasing.get_size() == 62);
  ASSERT_TRUE(edges.xIncreasing.get_size() == 48);
  ASSERT_TRUE(edges.yDecreasing.get_size() == 31);
  ASSERT_TRUE(edges.yIncreasing.get_size() == 31);

  for(s32 i=0; i<=47; i++) {
    bool valueFound = false;

    for(s32 j=0; j<62; j++) {
      if(edges.xDecreasing[j] == Point<s16>(56,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=17; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<62; j++) {
      if(edges.xDecreasing[j] == Point<s16>(31,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=47; i++) {
    bool valueFound = false;

    for(s32 j=0; j<48; j++) {
      if(edges.xIncreasing[j] == Point<s16>(44,i)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<31; j++) {
      if(edges.yDecreasing[j] == Point<s16>(i,31)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  for(s32 i=0; i<=30; i++) {
    bool valueFound = false;

    for(s32 j=0; j<31; j++) {
      if(edges.yIncreasing[j] == Point<s16>(i,16)) {
        valueFound = true;
        break;
      }
    }

    ASSERT_TRUE(valueFound == true);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_GrayvalueThreshold)

GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ASSERT_TRUE(blockImage50_320x240_WIDTH % MEMORY_ALIGNMENT == 0);
  ASSERT_TRUE(reinterpret_cast<size_t>(&blockImage50_320x240[0]) % MEMORY_ALIGNMENT == 0);

  Array<u8> in(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, const_cast<u8*>(&blockImage50_320x240[0]), blockImage50_320x240_WIDTH*blockImage50_320x240_HEIGHT, Flags::Buffer(false,false,false));
  Array<u8> out(60, 80, scratchOffchip);
  //in.Print("in");

  ASSERT_TRUE(AreValid(in, out));

  //const f64 t0 = GetTimeF64();

  const Result result = ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in, 2, out, scratchOffchip);

  ASSERT_TRUE(result == RESULT_OK);

  //CoreTechPrint("%d %d %d %d", out[0][0], out[0][17], out[40][80-1], out[59][80-3]);

  ASSERT_TRUE(out[0][0] == 155);
  ASSERT_TRUE(out[0][17] == 157);
  ASSERT_TRUE(out[40][80-1] == 143);
  ASSERT_TRUE(out[59][80-3] == 127);

  out.SetZero();

  Array<u8> tmp(120, 160, scratchOffchip);

  ASSERT_TRUE(AreValid(tmp));

  const Result result2 = ImageProcessing::DownsampleByTwo<u8,u32,u8>(in, tmp);
  ASSERT_TRUE(result2 == RESULT_OK);

  const Result result3 = ImageProcessing::DownsampleByTwo<u8,u32,u8>(tmp, out);
  ASSERT_TRUE(result3 == RESULT_OK);

  printf("%d\n", out[59][80-3]);

  ASSERT_TRUE(out[0][0] == 155);
  ASSERT_TRUE(out[0][17] == 157);
  ASSERT_TRUE(out[40][80-1] == 143);
  ASSERT_TRUE(out[59][80-3] == 126);

  f64 totalTime1 = 0;
  f64 totalTime2 = 0;

  Array<u8> in640(480,640,scratchOffchip,Flags::Buffer(false,false,false));
  Array<u8> out320(240,320,scratchOffchip,Flags::Buffer(false,false,false));

  ASSERT_TRUE(AreValid(in640, out320));

  volatile s32 numLoops = 100;

  const f64 t1a = GetTimeF64();
  ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in640, 1, out320, scratchOffchip);
  const f64 t2a = GetTimeF64();

  volatile s32 i;
  for(i=0; i<numLoops; i++) {
    PUSH_MEMORY_STACK(scratchOffchip);
    const f64 t1 = GetTimeF64();
    ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in640, 1, out320, scratchOffchip);
    const f64 t2 = GetTimeF64();

    in640.Set(out320[5][5]);

    totalTime1 += (t2-t1);
  }

  const f64 t1b = GetTimeF64();
  ImageProcessing::DownsampleByTwo<u8,u32,u8>(in640, out320);
  const f64 t2b = GetTimeF64();

  for(i=0; i<numLoops; i++) {
    const f64 t1 = GetTimeF64();
    ImageProcessing::DownsampleByTwo<u8,u32,u8>(in640, out320);
    const f64 t2 = GetTimeF64();

    in640.Set(out320[5][5]);

    totalTime2 += (t2-t1);
  }

  /*const f64 t3 =*/ GetTimeF64();

  const f64 t1c = GetTimeF64();
  FixedLengthList<Array<u8> > pyramid1 = ImageProcessing::BuildPyramid<u8,u32,u8>(in640, 5, scratchOffchip);
  const f64 t2c = GetTimeF64();
  CoreTechPrint("Pyramid 5 took %f\n", t2c-t1c);

  //  CoreTechPrint("DownsampleByPowerOfTwo took %f seconds per call. DownsampleByTwo took %f seconds per call.\n", (t1-t0) / 100, (t3-t2) / 100);
  CoreTechPrint("DownsampleByPowerOfTwo took %f seconds per call (%f). DownsampleByTwo took %f seconds per call (%f). \n", totalTime1 / numLoops, totalTime2 / numLoops, t2a-t1a, t2b-t1b);

  //{
  //  Matlab matlab(false);
  //  matlab.PutArray(in, "in");
  //  matlab.PutArray(out, "out");
  //}

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo)

bool IsBlockImage50_320x240Valid(const u8 * const imageBuffer, const bool isBigEndian)
{
  //CoreTechPrint("%d %d %d %d\n", imageBuffer[0], imageBuffer[1000], imageBuffer[320*120], imageBuffer[320*240-1]);

  if(isBigEndian) {
    const u8 pixel1 = ((((int*)(&blockImage50_320x240[0]))[0]) & 0xFF000000)>>24;
    const u8 pixel2 = ((((int*)(&blockImage50_320x240[0]))[1000>>2]) & 0xFF000000)>>24;
    const u8 pixel3 = ((((int*)(&blockImage50_320x240[0]))[(320*120)>>2]) & 0xFF000000)>>24;
    const u8 pixel4 = ((((int*)(&blockImage50_320x240[0]))[((320*240)>>2)-1]) & 0xFF);

    if(pixel1 != 157) return false;
    if(pixel2 != 153) return false;
    if(pixel3 != 157) return false;
    if(pixel4 != 130) return false;
  } else {
    const u8 pixel1 = imageBuffer[0];
    const u8 pixel2 = imageBuffer[1000];
    const u8 pixel3 = imageBuffer[320*120];
    const u8 pixel4 = imageBuffer[320*240-1];

    if(pixel1 != 157) return false;
    if(pixel2 != 153) return false;
    if(pixel3 != 157) return false;
    if(pixel4 != 130) return false;
  }

  return true;
} // bool IsBlockImage50_320x240Valid()

//// TODO: Update this to test FillDockErrMsg() in visionSystem.cpp
////This will require exposing more of the internal state of the vision system
////or changing the function's API to pass these things in.
//
//GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine)
//{
//// TODO: make these the real values
//const s32 horizontalTrackingResolution = 80;
//const f32 blockMarkerWidthInMM = 50.0f;
//const f32 horizontalFocalLengthInMM = 5.0f;
//
//MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
//ASSERT_TRUE(ms.IsValid());
//
//const Quadrilateral<f32> initialCorners(Point<f32>(5.0f,5.0f), Point<f32>(100.0f,100.0f), Point<f32>(50.0f,20.0f), Point<f32>(10.0f,80.0f));
//const Transformations::PlanarTransformation_f32 transform(Transformations::TRANSFORM_AFFINE, initialCorners, ms);
//
//f32 rel_x, rel_y, rel_rad;
//ASSERT_TRUE(Docking::ComputeDockingErrorSignal(transform,
//horizontalTrackingResolution, blockMarkerWidthInMM, horizontalFocalLengthInMM,
//rel_x, rel_y, rel_rad, ms) == RESULT_OK);
//
//// TODO: manually compute the correct output
//ASSERT_TRUE(FLT_NEAR(rel_x,2.7116308f));
//ASSERT_TRUE(NEAR(rel_y,-8.1348925f, 0.1f)); // The Myriad inexact floating point mode is imprecise here
//ASSERT_TRUE(FLT_NEAR(rel_rad,-0.87467581f));
//
//GTEST_RETURN_HERE;
//} // GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine)

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledProjective)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> templateImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
  Array<u8> nextImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOnchip);

  const Quadrilateral<f32> templateQuad(Point<f32>(128,78), Point<f32>(220,74), Point<f32>(229,167), Point<f32>(127,171));

  const s32 numPyramidLevels = 4;

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  const f32 scaleTemplateRegionPercent = 1.05f;

  const u8 verify_maxPixelDifference = 30;

  const s32 maxSamplesAtBaseLevel = 2000;

  // TODO: add check that images were loaded correctly

  templateImage.Set(&cozmo_date2014_01_29_time11_41_05_frame10_320x240[0], cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT);
  nextImage.Set(&cozmo_date2014_01_29_time11_41_05_frame12_320x240[0], cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT);

  // Translation-only LK_Projective
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    //tracker.ShowTemplate("tracker", true, true);

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 10);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 115);*/

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Translation-only LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Translation-only LK_SampledProjective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3, 3, scratchOffchip);
    transform_groundTruth[0][2] = 3.143f;;
    transform_groundTruth[1][2] = -4.952f;

    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
    tracker.get_transformation().Transform(templateImage, warpedImage, scratchOffchip);
    //warpedImage.Show("translationWarped", false, false, false);
    //nextImage.Show("nextImage", true, false, false);

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK_SampledProjective
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 119);*/

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Affine LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Affine LK_SampledProjective");

    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
    tracker.get_transformation().Transform(templateImage, warpedImage, scratchOffchip);
    //warpedImage.Show("affineWarped", false, false, false);
    //nextImage.Show("nextImage", true, false, false);

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.064f; transform_groundTruth[0][1] = -0.004f; transform_groundTruth[0][2] = 3.225f;
    transform_groundTruth[1][0] = 0.002f; transform_groundTruth[1][1] = 1.058f;  transform_groundTruth[1][2] = -4.375f;
    transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;    transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK_SampledProjective
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 119);*/

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Projective LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Projective LK_SampledProjective");

    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
    tracker.get_transformation().Transform(templateImage, warpedImage, scratchOffchip);
    //warpedImage.Show("projectiveWarped", false, false, false);
    //nextImage.Show("nextImage", true, false, false);

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.065f;  transform_groundTruth[0][1] = 0.003f; transform_groundTruth[0][2] = 3.215f;
    transform_groundTruth[1][0] = 0.002f; transform_groundTruth[1][1] = 1.059f;   transform_groundTruth[1][2] = -4.453f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledProjective)

//GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof)
//{
//MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
//MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
//MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

//ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));
//
//  Array<u8> templateImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
//  Array<u8> nextImage(cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH, scratchOnchip);
//
//  const Quadrilateral<f32> templateQuad(Point<f32>(128,78), Point<f32>(220,74), Point<f32>(229,167), Point<f32>(127,171));
//
//  const s32 numPyramidLevels = 4;
//
//  const s32 maxIterations = 25;
//  const f32 convergenceTolerance = .05f;
//
//  const f32 scaleTemplateRegionPercent = 1.05f;
//
//  const u8 verify_maxPixelDifference = 30;
//
//  const s32 maxSamplesAtBaseLevel = 2000;
//
//  // Are these correct?
//  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 317.23763f;
//  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 318.38113f;
//  const f32 HEAD_CAM_CALIB_CENTER_X       = 151.88373f;
//  const f32 HEAD_CAM_CALIB_CENTER_Y       = 129.03379f;
//  const f32 DEFAULT_BLOCK_MARKER_WIDTH_MM = 26.f;
//
//  // TODO: add check that images were loaded correctly
//
//  templateImage.Set(&cozmo_date2014_01_29_time11_41_05_frame10_320x240[0], cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT);
//  nextImage.Set(&cozmo_date2014_01_29_time11_41_05_frame12_320x240[0], cozmo_date2014_01_29_time11_41_05_frame12_320x240_WIDTH*cozmo_date2014_01_29_time11_41_05_frame12_320x240_HEIGHT);
//
//  // Translation-only LK_Projective
//  {
//    PUSH_MEMORY_STACK(scratchCcm);
//    PUSH_MEMORY_STACK(scratchOnchip);
//    PUSH_MEMORY_STACK(scratchOffchip);
//
//    InitBenchmarking();
//
//    const f64 time0 = GetTimeF64();
//
//    TemplateTracker::LucasKanadeTracker_SampledPlanar6dof tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, maxSamplesAtBaseLevel,
//      HEAD_CAM_CALIB_FOCAL_LENGTH_X,
//      HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
//      HEAD_CAM_CALIB_CENTER_X,
//      HEAD_CAM_CALIB_CENTER_Y,
//      DEFAULT_BLOCK_MARKER_WIDTH_MM,
//      scratchCcm, scratchOnchip, scratchOffchip);
//
//    ASSERT_TRUE(tracker.IsValid());
//
//    //tracker.ShowTemplate("tracker", true, true);
//
//    const f64 time1 = GetTimeF64();
//
//    bool verify_converged = false;
//    s32 verify_meanAbsoluteDifference;
//    s32 verify_numInBounds;
//    s32 verify_numSimilarPixels;
//
//    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);
//
//    ASSERT_TRUE(verify_converged == true);
//    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 10);
//    ASSERT_TRUE(verify_numInBounds == 121);
//    ASSERT_TRUE(verify_numSimilarPixels == 115);*/
//
//    const f64 time2 = GetTimeF64();
//
//    CoreTechPrint("Translation-only LK_SampledPlanar6dof totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
//    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
//
//    tracker.get_transformation().Print("Translation-only LK_SampledPlanar6dof");
//
//    // This ground truth is from the PC c++ version
//    Array<f32> transform_groundTruth = Eye<f32>(3, 3, scratchOffchip);
//    transform_groundTruth[0][2] = 3.143f;;
//    transform_groundTruth[1][2] = -4.952f;
//
//    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
//    tracker.get_transformation().Transform(templateImage, warpedImage, scratchOffchip);
//    //warpedImage.Show("translationWarped", false, false, false);
//    //nextImage.Show("nextImage", true, false, false);
//
//    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
//  }
//
//#if 0 // Affine updates not supported with planar 6dof tracker (?)
//  // Affine LK_SampledProjective
//  {
//    PUSH_MEMORY_STACK(scratchCcm);
//    PUSH_MEMORY_STACK(scratchOnchip);
//    PUSH_MEMORY_STACK(scratchOffchip);
//
//    InitBenchmarking();
//
//    const f64 time0 = GetTimeF64();
//
//    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);
//
//    ASSERT_TRUE(tracker.IsValid());
//
//    const f64 time1 = GetTimeF64();
//
//    bool verify_converged = false;
//    s32 verify_meanAbsoluteDifference;
//    s32 verify_numInBounds;
//    s32 verify_numSimilarPixels;
//
//    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);
//
//    ASSERT_TRUE(verify_converged == true);
//    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
//    ASSERT_TRUE(verify_numInBounds == 121);
//    ASSERT_TRUE(verify_numSimilarPixels == 119);*/
//
//    const f64 time2 = GetTimeF64();
//
//    CoreTechPrint("Affine LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
//    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
//
//    tracker.get_transformation().Print("Affine LK_SampledProjective");
//
//    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
//    tracker.get_transformation().TransformArray(templateImage, warpedImage, scratchOffchip);
//    //warpedImage.Show("affineWarped", false, false, false);
//    //nextImage.Show("nextImage", true, false, false);
//
//    // This ground truth is from the PC c++ version
//    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
//    transform_groundTruth[0][0] = 1.064f; transform_groundTruth[0][1] = -0.004f; transform_groundTruth[0][2] = 3.225f;
//    transform_groundTruth[1][0] = 0.002f; transform_groundTruth[1][1] = 1.058f;  transform_groundTruth[1][2] = -4.375f;
//    transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;    transform_groundTruth[2][2] = 1.0f;
//
//    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
//  }
//#endif
//
//  // Projective LK_SampledPlanar6dof
//  {
//    PUSH_MEMORY_STACK(scratchCcm);
//    PUSH_MEMORY_STACK(scratchOnchip);
//    PUSH_MEMORY_STACK(scratchOffchip);
//
//    InitBenchmarking();
//
//    const f64 time0 = GetTimeF64();
//
//    TemplateTracker::LucasKanadeTracker_SampledPlanar6dof tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, maxSamplesAtBaseLevel,
//      HEAD_CAM_CALIB_FOCAL_LENGTH_X,
//      HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
//      HEAD_CAM_CALIB_CENTER_X,
//      HEAD_CAM_CALIB_CENTER_Y,
//      DEFAULT_BLOCK_MARKER_WIDTH_MM,
//      scratchCcm, scratchOnchip, scratchOffchip);
//
//    ASSERT_TRUE(tracker.IsValid());
//
//    const f64 time1 = GetTimeF64();
//
//    bool verify_converged = false;
//    s32 verify_meanAbsoluteDifference;
//    s32 verify_numInBounds;
//    s32 verify_numSimilarPixels;
//
//    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);
//
//    ASSERT_TRUE(verify_converged == true);
//    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
//    ASSERT_TRUE(verify_numInBounds == 121);
//    ASSERT_TRUE(verify_numSimilarPixels == 119);*/
//
//    const f64 time2 = GetTimeF64();
//
//    CoreTechPrint("Projective LK_SampledPlanar6dof totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
//    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);
//
//    tracker.get_transformation().Print("Projective LK_SampledPlanar6dof");
//
//    Array<u8> warpedImage(cozmo_date2014_01_29_time11_41_05_frame10_320x240_HEIGHT, cozmo_date2014_01_29_time11_41_05_frame10_320x240_WIDTH, scratchOffchip);
//    tracker.get_transformation().Transform(templateImage, warpedImage, scratchOffchip);
//    //warpedImage.Show("projectiveWarped", false, false, false);
//    //nextImage.Show("nextImage", true, false, false);
//
//    // This ground truth is from the PC c++ version
//    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
//    transform_groundTruth[0][0] = 1.065f;  transform_groundTruth[0][1] = 0.003f; transform_groundTruth[0][2] = 3.215f;
//    transform_groundTruth[1][0] = 0.002f; transform_groundTruth[1][1] = 1.059f;   transform_groundTruth[1][2] = -4.453f;
//    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;
//
//    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
//  }
//GTEST_RETURN_HERE;
//} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof)

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Projective)
{
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  const Rectangle<f32> templateRegion(13*4, 34*4, 22*4, 43*4);
  const Quadrilateral<f32> templateQuad(templateRegion);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  const f32 scaleTemplateRegionPercent = 1.05f;

  const u8 verify_maxPixelDifference = 30;

  // TODO: add check that images were loaded correctly

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratchOffchip);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratchOffchip);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //CoreTechPrint("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK_Projective
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 13);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 474);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Translation-only LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Translation-only LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK_Projective
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Affine LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Affine LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.301f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.101f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK_Projective
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Projective LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Projective LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.342f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.044f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Projective)

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Affine)
{
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  //const Rectangle<f32> templateRegion(13, 34, 22, 43);
  const Rectangle<f32> templateRegion(13*4, 34*4, 22*4, 43*4);
  const Quadrilateral<f32> templateQuad(templateRegion);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  const f32 scaleTemplateRegionPercent = 1.05f;

  const u8 verify_maxPixelDifference = 30;

  // TODO: add check that images were loaded correctly

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratchOffchip);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratchOffchip);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //CoreTechPrint("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Affine tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 13);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 474);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Translation-only FAST-LK totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Translation-only LK_Affine");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Affine tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Affine FAST-LK totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Affine LK_Affine");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.299f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.104f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Affine)

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Slow)
{
  const s32 imageHeight = 60;
  const s32 imageWidth = 80;

  const s32 numPyramidLevels = 2;

  const f32 ridgeWeight = 0.0f;

  const Rectangle<f32> templateRegion(13*4, 34*4, 22*4, 43*4);
  //const Rectangle<f32> templateRegion(13, 34, 22, 43);

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  const f32 scaleTemplateRegionPercent = 1.05f;

  // TODO: add check that images were loaded correctly

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratchOffchip);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratchOffchip);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //CoreTechPrint("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, ridgeWeight, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, verify_converged, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Translation-only LK totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Translation-only LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, ridgeWeight, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, verify_converged, scratchOffchip) == RESULT_OK);
    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Affine LK totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Affine LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.299f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.104f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK
  {
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTimeF64();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, ridgeWeight, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTimeF64();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, true, verify_converged, scratchOffchip) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTimeF64();

    CoreTechPrint("Projective LK totalTime:%dms initTime:%dms updateTrack:%dms\n", Round<s32>(1000*(time2-time0)), Round<s32>(1000*(time1-time0)), Round<s32>(1000*(time2-time1)));
    ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    tracker.get_transformation().Print("Projective LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.339f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.042f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Slow)

GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(3,16,scratchOnchip);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  //image.Print("image");

  Array<u8> image2(4,16,scratchOnchip);
  ASSERT_TRUE(image2.IsValid());

  image2[0][0] = 1; image2[0][1] = 2; image2[0][2] = 3;
  image2[1][0] = 9; image2[1][1] = 9; image2[1][2] = 9;
  image2[2][0] = 0; image2[2][1] = 0; image2[2][2] = 1;
  image2[3][0] = 5; image2[3][1] = 5; image2[3][2] = 5;

  //image2.Print("image2");

  Array<s32> filteredOutput(1, 16, scratchOnchip);

  //
  // Test with border of 2
  //

  ScrollingIntegralImage_u8_s32 ii_border2(4, 16, 2, scratchOnchip);
  ASSERT_TRUE(ii_border2.ScrollDown(image, 4, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_0(-1, 1, -1, 1);
    const s32 imageRow_testBorder2_0 = 0;
    const s32 filteredOutput_groundTruth_testBorder2_0[] = {35, 39, 28};
    ASSERT_TRUE(ii_border2.FilterRow(filter_testBorder2_0, imageRow_testBorder2_0, filteredOutput) == RESULT_OK);

    //ii_border2.Print("ii_border2");
    //filteredOutput.Print("filteredOutput1");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_0[i]);
    }
  }

  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_1(-1, 1, -1, 1);
    const s32 imageRow_testBorder2_1 = 2;
    const s32 filteredOutput_groundTruth_testBorder2_1[] = {27, 29, 20};
    ASSERT_TRUE(ii_border2.FilterRow(filter_testBorder2_1, imageRow_testBorder2_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput2");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_1[i]);
    }
  }

  //
  // Test with border of 1
  //

  // imBig2 = [1,1,2,3,3;1,1,2,3,3;9,9,9,9,9;0,0,0,1,1;0,0,0,1,1];

  ScrollingIntegralImage_u8_s32 ii_border1(4, 16, 1, scratchOnchip);

  ASSERT_TRUE(ii_border1.ScrollDown(image, 4, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder1_0(0, 0, 0, 2);
    const s32 imageRow_testBorder1_0 = 0;
    const s32 filteredOutput_groundTruth_testBorder1_0[] = {10, 11, 13};
    ASSERT_TRUE(ii_border1.FilterRow(filter_testBorder1_0, imageRow_testBorder1_0, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput3");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder1_0[i]);
    }
  }

  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder1_1(0, 1, 0, 2);
    const s32 imageRow_testBorder1_1 = 1;
    const s32 filteredOutput_groundTruth_testBorder1_1[] = {18, 20, 11};
    ASSERT_TRUE(ii_border1.FilterRow(filter_testBorder1_1, imageRow_testBorder1_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput4");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder1_1[i]);
    }
  }

  //
  // Test with border of 0
  //

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, scratchOnchip);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_0(-1, 0, 0, 0);
    const s32 imageRow_testBorder2_0 = 1;
    const s32 filteredOutput_groundTruth_testBorder2_0[] = {0, 0, 18};
    ASSERT_TRUE(ii_border0.FilterRow(filter_testBorder2_0, imageRow_testBorder2_0, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput5");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_0[i]);
    }
  }

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, scratchOnchip) == RESULT_OK);

  {
    Rectangle<s16> filter_testBorder2_1(0, 1, 0, 0);
    const s32 imageRow_testBorder2_1 = 2;
    const s32 filteredOutput_groundTruth_testBorder2_1[] = {0, 1, 1};
    ASSERT_TRUE(ii_border0.FilterRow(filter_testBorder2_1, imageRow_testBorder2_1, filteredOutput) == RESULT_OK);

    //filteredOutput.Print("filteredOutput5");

    for(s32 i=0; i<3; i++) {
      ASSERT_TRUE(filteredOutput[0][i] == filteredOutput_groundTruth_testBorder2_1[i]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering)

GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(3,16,scratchOnchip);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  Array<u8> image2(4,16,scratchOnchip);
  ASSERT_TRUE(image2.IsValid());

  image2[0][0] = 1; image2[0][1] = 2; image2[0][2] = 3;
  image2[1][0] = 9; image2[1][1] = 9; image2[1][2] = 9;
  image2[2][0] = 0; image2[2][1] = 0; image2[2][2] = 1;
  image2[3][0] = 5; image2[3][1] = 5; image2[3][2] = 5;

  //
  // Test with border of 2
  //

  // imBig = [1,1,1,2,3,3,3;1,1,1,2,3,3,3;1,1,1,2,3,3,3;9,9,9,9,9,9,9;0,0,0,0,1,1,1;0,0,0,0,1,1,1;0,0,0,0,1,1,1]
  const s32 border2_groundTruthRows[7][5] = {
    {1, 2, 3, 5, 8},
    {2, 4, 6, 10, 16},
    {3, 6, 9, 15, 24},
    {12, 24, 36, 51, 69},
    {12, 24, 36, 51, 70},
    {12, 24, 36, 51, 71},
    {12, 24, 36, 51, 72}};

  ScrollingIntegralImage_u8_s32 ii_border2(3, 16, 2, scratchOnchip);
  ASSERT_TRUE(ii_border2.get_rowOffset() == -2);

  //Result ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+4,'int32'), 1, 3, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 3, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == -1);
  ASSERT_TRUE(ii_border2.get_rowOffset() == -2);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-2, 2, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 2);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border2.get_rowOffset() == 0);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y+2][x]);
    }
  }

  // scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-2+2, 2, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border2.get_minRow(1) == 2);
  ASSERT_TRUE(ii_border2.get_maxRow(1) == 3);
  ASSERT_TRUE(ii_border2.get_rowOffset() == 2);

  //ii_border2.Print("ii_border2");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<5; x++) {
      ASSERT_TRUE(ii_border2[y][x] == border2_groundTruthRows[y+4][x]);
    }
  }

  //
  // Test with border of 1
  //

  // imBig2 = [1,1,2,3,3;1,1,2,3,3;9,9,9,9,9;0,0,0,1,1;0,0,0,1,1];
  const s32 border1_groundTruthRows[5][4] = {
    {1, 2, 4, 7},
    {2, 4, 8, 14},
    {11, 22, 35, 50},
    {11, 22, 35, 51},
    {11, 22, 35, 52}};

  ScrollingIntegralImage_u8_s32 ii_border1(3, 16, 1, scratchOnchip);

  ASSERT_TRUE(ii_border1.get_rowOffset() == -1);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+2,'int32'), 1, 3, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 3, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_rowOffset() == -1);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-1, 1, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border1.get_rowOffset() == 0);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y+1][x]);
    }
  }

  // scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-1+1, 1, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border1.get_minRow(1) == 1);
  ASSERT_TRUE(ii_border1.get_maxRow(1) == 2);
  ASSERT_TRUE(ii_border1.get_rowOffset() == 1);

  //ii_border1.Print("ii_border1");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<4; x++) {
      ASSERT_TRUE(ii_border1[y][x] == border1_groundTruthRows[y+2][x]);
    }
  }

  //
  // Test with border of 0
  //

  const s32 border0_groundTruthRows[4][3] = {
    {1,  3,  6},
    {10, 21, 33},
    {10, 21, 34},
    {15, 31, 49}};

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, scratchOnchip);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2),'int32'), 1, 3, 0)
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border0.get_minRow(1) == 0);
  ASSERT_TRUE(ii_border0.get_maxRow(1) == 1);
  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  //ii_border0.Print("ii_border0");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<3; x++) {
      ASSERT_TRUE(ii_border0[y][x] == border0_groundTruthRows[y][x]);
    }
  }

  // scrolledII = computeScrollingIntegralImage(im, initialII, 1+3, 1, 0)
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, scratchOnchip) == RESULT_OK);

  //ASSERT_TRUE(ii_border0.get_minRow(1) == 1);
  ASSERT_TRUE(ii_border0.get_maxRow(1) == 2);
  ASSERT_TRUE(ii_border0.get_rowOffset() == 1);

  //ii_border0.Print("ii_border0");

  for(s32 y=0; y<3; y++) {
    for(s32 x=0; x<3; x++) {
      ASSERT_TRUE(ii_border0[y][x] == border0_groundTruthRows[y+1][x]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration)

#endif // #if !defined(JUST_FIDUCIAL_DETECTION)

GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers)
{
  const s32 scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
  //const s32 scaleImage_thresholdMultiplier = 49152; // .75*(2^16)=49152
  const s32 scaleImage_numPyramidLevels = 3;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.01f*MAX(newFiducials_320x240_HEIGHT,newFiducials_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(newFiducials_320x240_HEIGHT,newFiducials_320x240_WIDTH);

  const bool useIntegralImageFiltering = true;

  const s32 component_minimumNumPixels = Round<s32>(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength));
  const s32 component_maximumNumPixels = Round<s32>(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  //const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  //const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8
  const f32 component_minHollowRatio = 1.0f;

  const s32 minLaplacianPeakRatio = 5;

  const s32 maxExtractedQuads = 1000/2;
  const s32 quads_minQuadArea = 100/4;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;
  //const s32 maxConnectedComponentSegments = 5000; // 25000/4 = 6250
  const s32 maxConnectedComponentSegments = 39000; // 322*240/2 = 38640
  //const s32 maxConnectedComponentSegments = 70000;

  const s32 quadRefinementIterations = 5;
  const s32 numRefinementSamples = 100;
  const f32 quadRefinementMaxCornerChange = 5.f;
  const f32 quadRefinementMinCornerChange = .005f;
  
  const CornerMethod cornerMethod = CORNER_METHOD_LAPLACIAN_PEAKS; // {CORNER_METHOD_LAPLACIAN_PEAKS, CORNER_METHOD_LINE_FITS};

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(newFiducials_320x240_HEIGHT, newFiducials_320x240_WIDTH, scratchOffchip);
  image.Set(newFiducials_320x240, newFiducials_320x240_WIDTH*newFiducials_320x240_HEIGHT);

  //Array<u8> image = Array<u8>::LoadImage("c:/Anki/products-cozmo-large-files/systemTestsData//trackingImages/tracking_00000_cozmo1_img_545642.jpg", scratchOffchip);

  //image.Show("image", true);

  // TODO: Check that the image loaded correctly
  //ASSERT_TRUE(IsnewFiducials_320x240Valid(image.Pointer(0,0), false));

  FixedLengthList<VisionMarker> markers(maxMarkers, scratchCcm);
  FixedLengthList<Array<f32> > homographies(maxMarkers, scratchCcm);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, scratchCcm);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  InitBenchmarking();

  {
    const f64 time0 = GetTimeF64();
    // note: arglist is screwed up right now
    const Result result = DetectFiducialMarkers(
      image,
      markers,
      homographies,
      useIntegralImageFiltering,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      cornerMethod, minLaplacianPeakRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      quadRefinementIterations,
      numRefinementSamples,
      quadRefinementMaxCornerChange,
      quadRefinementMinCornerChange,
      false,
      scratchCcm,
      scratchOnchip,
      scratchOffchip);
    const f64 time1 = GetTimeF64();

    CoreTechPrint("totalTime: %dms\n", Round<s32>(1000*(time1-time0)));

    //ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

    ASSERT_TRUE(result == RESULT_OK);
  }

  //markers.Print("markers");
  //markers[0].Print();
  
  // Send stuff to Matlab
  /*{
    Anki::Embedded::Matlab matlab(false);

    matlab.EvalStringEcho("markers = zeros(4,2,0);");
    for(s32 i=0; i<markers.get_size(); i++) {
      matlab.EvalStringEcho("markers(:,:,end+1) = [%f,%f;%f,%f;%f,%f;%f,%f];", markers[i].corners[0].x, markers[i].corners[0].y, markers[i].corners[1].x, markers[i].corners[1].y, markers[i].corners[2].x, markers[i].corners[2].y, markers[i].corners[3].x, markers[i].corners[3].y);
    }

    matlab.PutArray(image, "image");
  } // Send stuff to Matlab
  */
  
  if(scaleImage_thresholdMultiplier == 65536) {
    // Grab the ground truth markers types and locations from the
    // auto-generated header file
#include "data/newFiducials_320x240_markers.h"

    // Make sure the ground truth image only has one of each marker type
    bool seenThisMarkerType[numMarkers_groundTruth];
    for(s32 iMarker=0; iMarker<numMarkers_groundTruth; ++iMarker) {
      seenThisMarkerType[iMarker] = false;
    }
    for(s32 iMarker=0; iMarker<numMarkers_groundTruth; ++iMarker) {
      ASSERT_FALSE(seenThisMarkerType[iMarker]); // if true, we've already seen one of these
      seenThisMarkerType[iMarker] = true;
    }

    CoreTechPrint("Found %d of %d markers.\n", markers.get_size(), numMarkers_groundTruth);
    if(markers.get_size() < numMarkers_groundTruth) {
      s32 iMarker = 0;
      while(markers[iMarker].markerType == markerTypes_groundTruth[iMarker]) {
        ++iMarker;
      }
      CoreTechPrint("Looks like %s was not seen.\n",
        Vision::MarkerTypeStrings[markerTypes_groundTruth[iMarker]]);
    }
    ASSERT_TRUE(markers.get_size() == numMarkers_groundTruth);

    const f32 cornerDistanceTolerance = 2.f*sqrtf(2.f); // in pixels

    // For each detected marker, find the ground truth marker with the same type
    // and check that its corners are in the right place (this avoids the
    // problem that the markers are detected out of order from the way they
    // are provided in the auto-generated ground truth file)
    for(s32 iMarkerDet=0; iMarkerDet<markers.get_size(); ++iMarkerDet)
    {
      ASSERT_TRUE(markers[iMarkerDet].validity == VisionMarker::VALID);

      s32 iMarkerTrue=0;
      while(markers[iMarkerDet].markerType != markerTypes_groundTruth[iMarkerTrue]) {
        ++iMarkerTrue;
        // if this fails, we found the right number of markers (checked above),
        // but we did not actually find each one (i.e. maybe we found two of
        // one)
        ASSERT_TRUE(iMarkerTrue < numMarkers_groundTruth);
      }

      // Sort the quads to ignore differing corner orderings for markers that
      // are rotationally symmetric
      const Quadrilateral<f32> currentCorners = markers[iMarkerDet].corners.ComputeClockwiseCorners<f32>();
      Quadrilateral<f32> trueCorners(Point2f(corners_groundTruth[iMarkerTrue][0][0],
        corners_groundTruth[iMarkerTrue][0][1]),
        Point2f(corners_groundTruth[iMarkerTrue][1][0],
        corners_groundTruth[iMarkerTrue][1][1]),
        Point2f(corners_groundTruth[iMarkerTrue][2][0],
        corners_groundTruth[iMarkerTrue][2][1]),
        Point2f(corners_groundTruth[iMarkerTrue][3][0],
        corners_groundTruth[iMarkerTrue][3][1]));
      trueCorners = trueCorners.ComputeClockwiseCorners<f32>();

      for(s32 iCorner=0; iCorner<4; iCorner++) {
        const Point2f& currentCorner = currentCorners[iCorner];
        const Point2f& trueCorner    = trueCorners[iCorner];

        ASSERT_TRUE( (currentCorner - trueCorner).Length() < cornerDistanceTolerance );
      } // FOR each corner
    } // FOR each detected marker
  } else {
    ASSERT_TRUE(false);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers)

GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark)
{
  // Don't check if the markers were correctly detected, just check the timing

  const s32 scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
  //const s32 scaleImage_thresholdMultiplier = 49152; // .75*(2^16)=49152
  const s32 scaleImage_numPyramidLevels = 3;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.01f*MAX(simpleFiducials_320x240_HEIGHT,simpleFiducials_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(simpleFiducials_320x240_HEIGHT,simpleFiducials_320x240_WIDTH);

  const s32 component_minimumNumPixels = Round<s32>(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength));
  const s32 component_maximumNumPixels = Round<s32>(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const f32 component_minHollowRatio = 1.0f;

  const s32 maxExtractedQuads = 1000/2;
  const s32 quads_minQuadArea = 100/4;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;
  //const s32 maxConnectedComponentSegments = 5000; // 25000/4 = 6250
  const s32 maxConnectedComponentSegments = 39000; // 322*240/2 = 38640
  //const s32 maxConnectedComponentSegments = 70000;

  const CornerMethod cornerMethod = CORNER_METHOD_LAPLACIAN_PEAKS; // {CORNER_METHOD_LAPLACIAN_PEAKS, CORNER_METHOD_LINE_FITS};

  const s32 minLaplacianPeakRatio = 5;

  const s32 quadRefinementIterations = 5;
  const s32 numRefinementSamples = 100;
  const f32 quadRefinementMaxCornerChange = 5.f;
  const f32 quadRefinementMinCornerChange = .005f;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(simpleFiducials_320x240_HEIGHT, simpleFiducials_320x240_WIDTH, scratchOffchip);
  image.Set(simpleFiducials_320x240, simpleFiducials_320x240_WIDTH*simpleFiducials_320x240_HEIGHT);

  //Array<u8> image = Array<u8>::LoadImage("C:/Anki/products-cozmo-large-files/systemTestsData/images/cozmo_date2014_06_04_time16_52_36_frame0.png", scratchOffchip);

  //image.Show("image", true);

  // TODO: Check that the image loaded correctly
  //ASSERT_TRUE(IssimpleFiducials_320x240Valid(image.Pointer(0,0), false));

  FixedLengthList<VisionMarker> markers(maxMarkers, scratchCcm);
  FixedLengthList<Array<f32> > homographies(maxMarkers, scratchCcm);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, scratchCcm);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  const s32 numRuns = 1;
  FixedLengthList<FixedLengthList<BenchmarkElement> > benchmarkElements_integral(numRuns, scratchOffchip);
  FixedLengthList<FixedLengthList<BenchmarkElement> > benchmarkElements_binomial(numRuns, scratchOffchip);

  ASSERT_TRUE(AreValid(benchmarkElements_integral, benchmarkElements_binomial));

  for(s32 iRun=0; iRun<numRuns; iRun++) {
    InitBenchmarking();

    // note: arglist is screwed up right now
    const Result result_integral = DetectFiducialMarkers(
      image,
      markers,
      homographies,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      cornerMethod, minLaplacianPeakRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      quadRefinementIterations,
      numRefinementSamples,
      quadRefinementMaxCornerChange,
      quadRefinementMinCornerChange,
      false,
      scratchCcm,
      scratchOnchip,
      scratchOffchip);

    ASSERT_TRUE(result_integral == RESULT_OK);

    benchmarkElements_integral[iRun] = ComputeBenchmarkResults(scratchOffchip);

    //PrintBenchmarkResults(benchmarkElements_integral[iRun], true, true);
  } // for(s32 iRun=0; iRun<numRuns; iRun++)

  const f32 elementPercentile = 0; // Minimum
  //const f32 elementPercentile = 0.5f; // Median

  printf("Integral image benchmarks:\n");
  ASSERT_TRUE(PrintPercentileBenchmark(benchmarkElements_integral, numRuns, elementPercentile, scratchOffchip) == RESULT_OK);

  //markers.Print();

  const f32 minDifference = 1.0f;
  Point<f32> groundTruth[4];
  groundTruth[0] = Point<f32>(22.2537f,22.2517f);
  groundTruth[1] = Point<f32>(22.2359f,56.4778f);
  groundTruth[2] = Point<f32>(56.4742f,22.2397f);
  groundTruth[3] = Point<f32>(56.4890f,56.4873f);

  for(s32 i=0; i<4; i++) {
    ASSERT_TRUE(ABS(markers[0].corners[i].x - groundTruth[i].x) < minDifference);
    ASSERT_TRUE(ABS(markers[0].corners[i].y - groundTruth[i].y) < minDifference);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark)

#if defined(RUN_HIGH_MEMORY_TESTS)
GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark640)
{
  // Don't check if the markers were correctly detected, just check the timing

  const s32 scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
  //const s32 scaleImage_thresholdMultiplier = 49152; // .75*(2^16)=49152
  const s32 scaleImage_numPyramidLevels = 3;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.01f*MAX(simpleFiducials_320x240_HEIGHT,simpleFiducials_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(simpleFiducials_320x240_HEIGHT,simpleFiducials_320x240_WIDTH);

  const s32 component_minimumNumPixels = Round<s32>(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength));
  const s32 component_maximumNumPixels = Round<s32>(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const f32 component_minHollowRatio = 1.0f;

  const s32 maxExtractedQuads = 1000/2;
  const s32 quads_minQuadArea = 100/4;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;

  const s32 maxConnectedComponentSegments = 39000 * 16; // 322*240/2 = 38640

  const s32 quadRefinementIterations = 5;
  const s32 numRefinementSamples = 100;
  const f32 quadRefinementMaxCornerChange = 5.f;
  const f32 quadRefinementMinCornerChange = .005f;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchHuge(&hugeBuffer[0], HUGE_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip, scratchHuge));

  Array<u8> image320(simpleFiducials_320x240_HEIGHT, simpleFiducials_320x240_WIDTH, scratchOffchip);
  image320.Set(simpleFiducials_320x240, simpleFiducials_320x240_WIDTH*simpleFiducials_320x240_HEIGHT);

  Array<u8> image640(480, 640, scratchHuge);
  ImageProcessing::Resize<u8,u8>(image320, image640);

  //matlab.PutArray(image320, "image320");
  //matlab.PutArray(image640, "image640");

  //image320.Show("image320", false, false);
  //image640.Show("image640", true, false);

  // TODO: Check that the image loaded correctly
  //ASSERT_TRUE(IssimpleFiducials_320x240Valid(image.Pointer(0,0), false));

  FixedLengthList<VisionMarker> markers(maxMarkers, scratchCcm);
  FixedLengthList<Array<f32> > homographies(maxMarkers, scratchCcm);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f32> newArray(3, 3, scratchCcm);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  const s32 numRuns = 2;
  FixedLengthList<FixedLengthList<BenchmarkElement> > benchmarkElements_integral(numRuns, scratchOffchip);
  FixedLengthList<FixedLengthList<BenchmarkElement> > benchmarkElements_binomial(numRuns, scratchOffchip);

  ASSERT_TRUE(AreValid(benchmarkElements_integral, benchmarkElements_binomial));

  for(s32 iRun=0; iRun<numRuns; iRun++) {
    InitBenchmarking();

    const Result result_binomial = DetectFiducialMarkers(
      image640,
      markers,
      homographies,
      false,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      quadRefinementIterations,
      numRefinementSamples,
      quadRefinementMaxCornerChange,
      quadRefinementMinCornerChange,
      false,
      scratchOnchip,
      scratchOffchip,
      scratchHuge);

    ASSERT_TRUE(result_binomial == RESULT_OK);

    benchmarkElements_binomial[iRun] = ComputeBenchmarkResults(scratchHuge);

    //PrintBenchmarkResults(benchmarkElements_binomial[iRun], true, true);

    InitBenchmarking();

    const Result result_integral = DetectFiducialMarkers(
      image640,
      markers,
      homographies,
      true,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      quadRefinementIterations,
      numRefinementSamples,
      quadRefinementMaxCornerChange,
      quadRefinementMinCornerChange,
      false,
      scratchOnchip,
      scratchOffchip,
      scratchHuge);

    ASSERT_TRUE(result_integral == RESULT_OK);

    benchmarkElements_integral[iRun] = ComputeBenchmarkResults(scratchHuge);

    //PrintBenchmarkResults(benchmarkElements_integral[iRun], true, true);
  } // for(s32 iRun=0; iRun<numRuns; iRun++)

  const f32 elementPercentile = 0; // Minimum
  //const f32 elementPercentile = 0.5f; // Median

  printf("Integral image benchmarks:\n");
  ASSERT_TRUE(PrintPercentileBenchmark(benchmarkElements_integral, numRuns, elementPercentile, scratchOffchip) == RESULT_OK);

  printf("Binomial benchmarks:\n");
  ASSERT_TRUE(PrintPercentileBenchmark(benchmarkElements_binomial, numRuns, elementPercentile, scratchOffchip) == RESULT_OK);

  markers[0].Print();

  const f32 minDifference = 1e-4f;
  Point<f32> groundTruth[4];
  groundTruth[0] = Point<f32>(45.0272f,45.0177f);
  groundTruth[1] = Point<f32>(44.9521f,113.3854f);
  groundTruth[2] = Point<f32>(113.3604f,44.9779f);
  groundTruth[3] = Point<f32>(113.4568f,113.4449f);

  for(s32 i=0; i<4; i++) {
    ASSERT_TRUE(ABS(markers[0].corners[i].x - groundTruth[i].x) < minDifference);
    ASSERT_TRUE(ABS(markers[0].corners[i].y - groundTruth[i].y) < minDifference);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark640)
#endif // #if defined(RUN_HIGH_MEMORY_TESTS)

#if !defined(JUST_FIDUCIAL_DETECTION)
GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)
{
  const s32 numComponents = 60;
  const s32 minQuadArea = 100;
  const s32 quadSymmetryThreshold = 384;
  const s32 imageHeight = 480;
  const s32 imageWidth = 640;
  const s32 minDistanceFromImageEdge = 2;
  const s32 minLaplacianPeakRatio = 5;
  const CornerMethod cornerMethod = CORNER_METHOD_LAPLACIAN_PEAKS; // {CORNER_METHOD_LAPLACIAN_PEAKS, CORNER_METHOD_LINE_FITS};

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  FixedLengthList<BlockMarker> markers(50, scratchOnchip);

  // TODO: check these by hand
  const Quadrilateral<s16> quads_groundTruth[] = {
    Quadrilateral<s16>(Point<s16>(24,4), Point<s16>(10,4), Point<s16>(24,18), Point<s16>(10,18)) ,
    Quadrilateral<s16>(Point<s16>(129,50),  Point<s16>(100,50), Point<s16>(129,79), Point<s16>(100,79)) };

  ConnectedComponents components(numComponents, imageWidth, true, scratchOnchip);

  // Small square
  for(s32 y=0; y<15; y++) {
    components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(10,24,y+4,1));
  }

  // Big square
  for(s32 y=0; y<30; y++) {
    components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(100,129,y+50,2));
  }

  // Skewed quad
  components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(100,300,100,3));
  for(s32 y=0; y<10; y++) {
    components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(100,110,y+100,3));
  }

  // Tiny square
  for(s32 y=0; y<5; y++) {
    components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(10,14,y,4));
  }

  FixedLengthList<Quadrilateral<s16> > extractedQuads(2, scratchOnchip);

  components.SortConnectedComponentSegments();

  const Result result =  ComputeQuadrilateralsFromConnectedComponents(components, minQuadArea, quadSymmetryThreshold, minDistanceFromImageEdge, minLaplacianPeakRatio, imageHeight, imageWidth, cornerMethod, extractedQuads, scratchOnchip);
  ASSERT_TRUE(result == RESULT_OK);

  // extractedQuads.Print("extractedQuads");

  for(s32 i=0; i<extractedQuads.get_size(); i++) {
    ASSERT_TRUE(extractedQuads[i] == quads_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)

GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  FixedPointArray<s32> image(1,15,2,scratchOnchip);
  FixedPointArray<s32> filter(1,5,2,scratchOnchip);
  FixedPointArray<s32> out(1,15,4,scratchOnchip);

  for(s32 i=0; i<image.get_size(1); i++) {
    *image.Pointer(0,i) = 1 + i;
  }

  for(s32 i=0; i<filter.get_size(1); i++) {
    *filter.Pointer(0,i) = 2*(1 + i);
  }

  const s32 out_groundTruth[] = {140, 110, 110, 140, 170, 200, 230, 260, 290, 320, 350, 380, 410, 290, 200};

  const Result result = ImageProcessing::Correlate1dCircularAndSameSizeOutput<s32,s32,s32>(image, filter, out, scratchOnchip);
  ASSERT_TRUE(result == RESULT_OK);

  for(s32 i=0; i<out.get_size(1); i++) {
    ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)

GTEST_TEST(CoreTech_Vision, LaplacianPeaks)
{
  const s32 minLaplacianPeakRatio = 5;
  
#define LaplacianPeaks_BOUNDARY_LENGTH 65
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  FixedLengthList<Point<s16> > boundary(LaplacianPeaks_BOUNDARY_LENGTH, scratchOnchip);

  const s32 componentsX_groundTruth[66] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 componentsY_groundTruth[66] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<LaplacianPeaks_BOUNDARY_LENGTH; i++) {
    boundary.PushBack(Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  FixedLengthList<Point<s16> > peaks(4, scratchOnchip);

  const Result result = ExtractLaplacianPeaks(boundary, minLaplacianPeakRatio, peaks, scratchOnchip);
  ASSERT_TRUE(result == RESULT_OK);

  //boundary.Print("boundary");
  //peaks.Print("peaks");

  ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(109,201));
  ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(109,205));
  ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(104,210));
  ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(100,210));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LaplacianPeaks)

GTEST_TEST(CoreTech_Vision, LineFitsPeaks)
{
  const s32 imageHeight = 480;
  const s32 imageWidth = 640;
  
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Test 1 is extracted from a real quad
#define LineFitsPeaks_test1_BOUNDARY_LENGTH 935
  const s32 componentsX_groundTruth1[LineFitsPeaks_test1_BOUNDARY_LENGTH] = {163, 163, 162, 161, 160, 159, 158, 158, 157, 156, 155, 154, 153, 152, 151, 150, 150, 149, 148, 147, 146, 146, 145, 144, 143, 142, 142, 141, 140, 139, 138, 137, 136, 135, 135, 134, 133, 132, 131, 130, 129, 129, 128, 128, 127, 126, 126, 125, 125, 124, 124, 124, 123, 123, 122, 122, 121, 120, 120, 120, 119, 118, 119, 119, 119, 119, 119, 118, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 118, 119, 119, 119, 119, 119, 120, 121, 121, 120, 120, 121, 122, 123, 123, 124, 124, 124, 124, 124, 125, 126, 126, 127, 128, 128, 129, 129, 129, 129, 130, 131, 132, 133, 134, 134, 134, 134, 135, 136, 137, 138, 139, 140, 141, 141, 142, 143, 144, 145, 146, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 349, 350, 351, 351, 351, 351, 352, 353, 354, 355, 354, 354, 355, 356, 357, 357, 358, 358, 359, 360, 359, 360, 360, 361, 362, 362, 362, 363, 363, 363, 364, 365, 365, 365, 365, 365, 365, 365, 366, 366, 366, 367, 367, 366, 366, 367, 367, 366, 366, 367, 366, 366, 366, 367, 368, 368, 368, 367, 368, 368, 368, 368, 368, 368, 367, 367, 368, 368, 367, 368, 368, 368, 367, 368, 368, 368, 367, 368, 368, 367, 367, 367, 368, 367, 367, 368, 368, 368, 368, 368, 368, 367, 368, 368, 368, 367, 368, 368, 368, 367, 367, 368, 367, 367, 367, 367, 367, 367, 367, 368, 367, 367, 367, 367, 367, 367, 368, 367, 367, 367, 367, 367, 367, 367, 368, 367, 367, 367, 368, 367, 367, 367, 368, 367, 367, 367, 367, 367, 367, 368, 367, 367, 367, 367, 367, 367, 367, 368, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 367, 366, 366, 365, 365, 365, 366, 366, 365, 366, 366, 366, 365, 365, 365, 365, 364, 363, 364, 364, 364, 364, 364, 364, 363, 362, 361, 361, 360, 360, 360, 360, 360, 359, 358, 357, 357, 356, 356, 357, 357, 356, 355, 354, 354, 353, 353, 352, 351, 350, 349, 348, 348, 347, 347, 346, 345, 344, 343, 342, 341, 340, 339, 338, 337, 336, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324, 323, 322, 321, 320, 319, 318, 317, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 307, 306, 305, 304, 303, 302, 301, 300, 299, 298, 297, 296, 295, 294, 293, 292, 291, 290, 289, 288, 287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 262, 262, 261, 260, 259, 258, 257, 256, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163};
  
  const s32 componentsY_groundTruth1[LineFitsPeaks_test1_BOUNDARY_LENGTH] = {167, 168, 169, 169, 169, 169, 169, 170, 171, 171, 171, 171, 171, 172, 172, 172, 173, 174, 174, 174, 174, 175, 176, 175, 176, 177, 178, 179, 180, 180, 181, 181, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 414, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 423, 423, 424, 425, 426, 427, 428, 429, 429, 430, 431, 432, 433, 433, 433, 434, 435, 436, 437, 438, 438, 437, 438, 439, 440, 440, 440, 441, 442, 443, 443, 442, 443, 443, 443, 443, 443, 443, 443, 443, 443, 443, 444, 445, 444, 444, 445, 445, 445, 445, 445, 445, 445, 446, 445, 445, 446, 446, 446, 446, 446, 446, 446, 447, 447, 447, 447, 447, 447, 447, 447, 447, 448, 448, 448, 447, 448, 449, 448, 448, 449, 449, 449, 449, 449, 449, 449, 450, 449, 449, 449, 450, 450, 450, 450, 450, 450, 451, 450, 450, 451, 451, 450, 451, 451, 451, 451, 452, 451, 451, 452, 452, 452, 452, 453, 452, 452, 453, 453, 453, 453, 453, 453, 453, 454, 454, 454, 454, 454, 454, 454, 454, 454, 454, 455, 454, 454, 455, 455, 455, 455, 456, 455, 455, 455, 456, 456, 456, 456, 456, 456, 456, 457, 457, 456, 457, 457, 457, 457, 457, 457, 458, 458, 458, 457, 458, 458, 458, 458, 459, 458, 458, 458, 458, 458, 458, 458, 458, 458, 459, 459, 459, 459, 459, 458, 458, 458, 458, 458, 457, 457, 456, 456, 456, 456, 455, 455, 455, 454, 453, 453, 453, 453, 452, 451, 450, 450, 449, 448, 447, 447, 447, 447, 446, 445, 444, 443, 442, 442, 441, 440, 439, 438, 438, 438, 438, 437, 436, 435, 434, 434, 433, 432, 431, 430, 429, 428, 427, 426, 425, 425, 424, 423, 422, 421, 420, 419, 418, 417, 416, 415, 414, 413, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, 399, 398, 397, 396, 395, 394, 393, 392, 391, 390, 389, 388, 387, 386, 385, 384, 383, 382, 381, 380, 379, 378, 377, 376, 375, 374, 373, 372, 371, 370, 369, 368, 367, 366, 365, 364, 363, 362, 361, 360, 359, 358, 357, 356, 355, 354, 353, 352, 351, 350, 349, 348, 347, 346, 345, 344, 343, 342, 341, 340, 339, 338, 337, 336, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324, 323, 322, 321, 320, 319, 318, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 306, 305, 304, 303, 302, 301, 300, 299, 298, 297, 296, 295, 294, 293, 292, 291, 290, 289, 288, 287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257, 256, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 212, 211, 210, 209, 208, 207, 206, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 191, 190, 189, 188, 187, 186, 186, 186, 186, 185, 184, 183, 183, 183, 182, 182, 181, 181, 182, 181, 180, 179, 179, 179, 179, 178, 179, 179, 178, 178, 177, 176, 177, 177, 177, 177, 177, 177, 176, 176, 177, 176, 176, 177, 177, 176, 176, 176, 176, 176, 176, 176, 175, 176, 176, 175, 175, 176, 176, 175, 175, 174, 174, 175, 175, 175, 174, 174, 175, 174, 174, 174, 174, 174, 174, 174, 173, 172, 173, 174, 174, 173, 173, 174, 173, 173, 174, 174, 174, 174, 173, 172, 172, 173, 173, 173, 173, 172, 171, 172, 172, 172, 172, 172, 172, 172, 172, 172, 171, 172, 172, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 171, 170, 171, 171, 170, 170, 171, 171, 170, 170, 171, 170, 170, 170, 170, 170, 170, 170, 169, 170, 170, 169, 169, 169, 169, 168, 169, 169, 169, 169, 169, 169, 169, 169, 169, 168, 168, 168, 168, 168, 168, 167, 167, 167, 167, 167, 168, 168, 168, 168, 168, 168, 168, 168, 168, 167, 167};
  
  // Test 2 is a perfect, axis aligned rectangle from (50,50) to (100,60)
  #define LineFitsPeaks_test2_BOUNDARY_LENGTH 180
  const s32 componentsX_groundTruth2[LineFitsPeaks_test2_BOUNDARY_LENGTH] = {50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
  
  const s32 componentsY_groundTruth2[LineFitsPeaks_test2_BOUNDARY_LENGTH] = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51};
  
  // Test 1
  /*{
    PUSH_MEMORY_STACK(scratchOnchip);
    
    FixedLengthList<Point<s16> > boundary(LineFitsPeaks_test1_BOUNDARY_LENGTH, scratchOnchip);
    for(s32 i=0; i<LineFitsPeaks_test1_BOUNDARY_LENGTH; i++) {
      boundary.PushBack(Point<s16>(componentsX_groundTruth1[i], componentsY_groundTruth1[i]));
    }

    FixedLengthList<Point<s16> > peaks(4, scratchOnchip);

    const Result result = ExtractLineFitsPeaks(boundary, peaks, imageHeight, imageWidth, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);

    //boundary.Print("boundary");
    peaks.Print("peaks");

    ASSERT_TRUE(peaks.get_size() == 4);
    ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(121,163));
    ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(361,182));
    ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(375,467));
    ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(111,437));
  }*/
  
  // Test 2
  {
    FixedLengthList<Point<s16> > boundary(LineFitsPeaks_test2_BOUNDARY_LENGTH, scratchOnchip);
    
    for(s32 i=0; i<LineFitsPeaks_test2_BOUNDARY_LENGTH; i++) {
      boundary.PushBack(Point<s16>(componentsX_groundTruth2[i], componentsY_groundTruth2[i]));
    }

    FixedLengthList<Point<s16> > peaks(4, scratchOnchip);

    const Result result = ExtractLineFitsPeaks(boundary, peaks, imageHeight, imageWidth, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);

    //boundary.Print("boundary");
    peaks.Print("peaks");

    ASSERT_TRUE(peaks.get_size() == 4);
    ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(50,50));
    ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(90,50));
    ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(90,100));
    ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(50,100));
  }
  
  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LineFitsPeaks)

GTEST_TEST(CoreTech_Vision, Correlate1d)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    FixedPointArray<s32> in1(1,1,2,scratchOnchip);
    FixedPointArray<s32> in2(1,4,2,scratchOnchip);
    FixedPointArray<s32> out(1,4,4,scratchOnchip);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {2, 4, 6, 8};

    const Result result = ImageProcessing::Correlate1d<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    FixedPointArray<s32> in1(1,3,5,scratchOnchip);
    FixedPointArray<s32> in2(1,6,1,scratchOnchip);
    FixedPointArray<s32> out(1,8,3,scratchOnchip);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {0, 2, 3, 5, 6, 8, 4, 1};

    const Result result = ImageProcessing::Correlate1d<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    FixedPointArray<s32> in1(1,4,2,scratchOnchip);
    FixedPointArray<s32> in2(1,4,2,scratchOnchip);
    FixedPointArray<s32> out(1,7,3,scratchOnchip);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {4, 11, 20, 30, 20, 11, 4};

    const Result result = ImageProcessing::Correlate1d<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(scratchOnchip);

    FixedPointArray<s32> in1(1,4,1,scratchOnchip);
    FixedPointArray<s32> in2(1,5,5,scratchOnchip);
    FixedPointArray<s32> out(1,8,8,scratchOnchip);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {32, 88, 160, 240, 320, 208, 112, 40};

    const Result result = ImageProcessing::Correlate1d<s32,s32,s32>(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1d)

GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)
{
  const s32 numComponents = 17;
  const s32 boundaryLength = 65;
  const s32 startComponentIndex = 0;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const s32 yValues[128] = {200, 200, 201, 201, 202, 203, 204, 205, 206, 207, 207, 208, 208, 209, 209, 210, 210};
  const s32 xStartValues[128] = {102, 104, 102, 108, 102, 100, 100, 104, 100, 100, 103, 100, 103, 100, 103, 100, 103};
  const s32 xEndValues[128] = { 102, 105, 105, 109, 109, 105, 105, 109, 105, 101, 104, 101, 104, 101, 104, 101, 104};

  const s32 extractedBoundaryX_groundTruth[128] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 extractedBoundaryY_groundTruth[128] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<numComponents; i++) {
    components.get_componentsU16()->PushBack(ConnectedComponentSegment<u16>(xStartValues[i],xEndValues[i],yValues[i],1));
  }

  components.SortConnectedComponentSegments();

  //#define DRAW_TraceNextExteriorBoundary
#ifdef DRAW_TraceNextExteriorBoundary
  {
    Array<u8> drawnComponents(480, 640, scratchOffchip);
    DrawComponents<u8>(drawnComponents, components, 64, 255);

    matlab.PutArray(drawnComponents, "drawnComponents");
    //drawnComponents.Show("drawnComponents", true, false);
  }
#endif // DRAW_TraceNextExteriorBoundary

  FixedLengthList<Point<s16> > extractedBoundary(boundaryLength, scratchOnchip);

  {
    s32 endComponentIndex = -1;
    const Result result = TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);
  }

  //extractedBoundary.Print();

  for(s32 i=0; i<boundaryLength; i++) {
    ASSERT_TRUE(*extractedBoundary.Pointer(i) == Point<s16>(extractedBoundaryX_groundTruth[i], extractedBoundaryY_groundTruth[i]));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)

GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)
{
  const s32 numComponents = 10;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 10, 0, 1);
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(12, 12, 1, 1);
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(16, 1004, 2, 1);
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(0, 4, 3, 2);
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 2, 4, 3);
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(4, 6, 5, 3);
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(8, 10, 6, 3);
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 4, 7, 4);
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(6, 6, 8, 4);
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(5, 1000, 9, 5);

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  FixedLengthList<Anki::Embedded::Rectangle<s16> > componentBoundingBoxes(numComponents, scratchOnchip);
  {
    const Result result = components.ComputeComponentBoundingBoxes(componentBoundingBoxes);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(*componentBoundingBoxes.Pointer(1) == Anki::Embedded::Rectangle<s16>(0,1005,0,3));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(2) == Anki::Embedded::Rectangle<s16>(0,5,3,4));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(3) == Anki::Embedded::Rectangle<s16>(0,11,4,7));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(4) == Anki::Embedded::Rectangle<s16>(0,7,7,9));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(5) == Anki::Embedded::Rectangle<s16>(5,1001,9,10));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)

GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)
{
  const s32 numComponents = 10;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 10, 0, 1);
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(12, 12, 1, 1);
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(16, 1004, 2, 1);
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(0, 4, 3, 2);
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 2, 4, 3);
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(4, 6, 5, 3);
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(8, 10, 6, 3);
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 4, 7, 4);
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(6, 6, 8, 4);
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 1000, 9, 5);

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  FixedLengthList<Point<s16> > componentCentroids(numComponents, scratchOnchip);
  {
    const Result result = components.ComputeComponentCentroids(componentCentroids, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(*componentCentroids.Pointer(1) == Point<s16>(503,1));
  ASSERT_TRUE(*componentCentroids.Pointer(2) == Point<s16>(2,3));
  ASSERT_TRUE(*componentCentroids.Pointer(3) == Point<s16>(5,5));
  ASSERT_TRUE(*componentCentroids.Pointer(4) == Point<s16>(2,7));
  ASSERT_TRUE(*componentCentroids.Pointer(5) == Point<s16>(500,9));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)

GTEST_TEST(CoreTech_Vision, InvalidateFilledCenterComponents_hollowRows)
{
  const s32 numComponents = 10;
  const f32 minHollowRatio = 0.7f;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 2, 5, 1);
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(4, 6, 5, 1);
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(0, 0, 6, 1);
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(6, 6, 6, 1);
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 1, 7, 2);
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(3, 3, 7, 2);
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(5, 7, 7, 2);
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 1, 8, 2);
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(5, 12, 8, 2);
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 10, 12, 3);

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  {
    const Result result = components.InvalidateFilledCenterComponents_hollowRows(minHollowRatio, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);
  }

  //components.Print();

  ASSERT_TRUE(components.get_componentsU16()->Pointer(0)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(1)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(2)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(3)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(4)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(5)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(6)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(7)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(8)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateFilledCenterComponents_hollowRows)

GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)
{
  const s32 numComponents = 10;
  const s32 sparseMultiplyThreshold = 10 << 5;
  const s32 solidMultiplyThreshold = 2 << 5;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 10, 0, 1); // Ok
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(0, 10, 3, 1);
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(0, 10, 5, 2); // Too solid
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(0, 10, 6, 2);
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 10, 8, 2);
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(0, 10, 10, 3); // Too sparse
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(0, 10, 100, 3);
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 0, 105, 4); // Ok
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(0, 0, 108, 4);
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 10, 110, 5); // Too solid

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  {
    const Result result = components.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.get_componentsU16()->Pointer(0)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(1)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(2)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(3)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(4)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(5)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(6)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(7)->id == 4);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(8)->id == 4);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)

GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)
{
  const s32 numComponents = 10;
  const s32 minimumNumPixels = 6;
  const s32 maximumNumPixels = 1000;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 10, 0, 1);
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(12, 12, 1, 1);
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(16, 1004, 2, 1);
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(0, 4, 3, 2);
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 2, 4, 3);
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(4, 6, 5, 3);
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(8, 10, 6, 3);
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 4, 7, 4);
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(6, 6, 8, 4);
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 1000, 9, 5);

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  {
    const Result result = components.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.get_componentsU16()->Pointer(0)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(1)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(2)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(3)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(4)->id == 3);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(5)->id == 3);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(6)->id == 3);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(7)->id == 4);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(8)->id == 4);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(9)->id == 0);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 2);
  }

  ASSERT_TRUE(components.get_componentsU16()->Pointer(0)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(1)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(2)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(3)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(4)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(5)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(6)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(7)->id == 2);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(8)->id == 2);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)

GTEST_TEST(CoreTech_Vision, CompressComponentIds)
{
  const s32 numComponents = 10;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(0, 0, 0, 5);  // 3
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(0, 0, 0, 10); // 4
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(0, 0, 0, 0);  // 0
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(0, 0, 0, 101);// 6
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(0, 0, 0, 4);  // 2
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(0, 0, 0, 11); // 5
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 0, 0, 5);  // 3

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(scratchOnchip);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 6);
  }

  ASSERT_TRUE(components.get_componentsU16()->Pointer(0)->id == 3);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(1)->id == 4);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(2)->id == 0);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(3)->id == 6);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(4)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(5)->id == 2);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(6)->id == 5);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(7)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(8)->id == 1);
  ASSERT_TRUE(components.get_componentsU16()->Pointer(9)->id == 3);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, CompressComponentIds)

// Not really a test, but computes the size of a list of ComponentSegments, to ensure that c++ isn't adding junk
GTEST_TEST(CoreTech_Vision, ComponentsSize)
{
  const s32 numComponents = 500;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  const s32 usedBytes0 = scratchOnchip.get_usedBytes();

#ifdef PRINTF_SIZE_RESULTS
  CoreTechPrint("Original size: %d\n", usedBytes0);
#endif

  FixedLengthList<ConnectedComponentSegment<u16> > segmentList(numComponents, scratchOnchip);

  const s32 usedBytes1 = scratchOnchip.get_usedBytes();
  const double actualSizePlusOverhead = double(usedBytes1 - usedBytes0) / double(numComponents);

#ifdef PRINTF_SIZE_RESULTS
  CoreTechPrint("Final size: %d\n"
    "Difference: %d\n"
    "Expected size of a components: %d\n"
    "Actual size (includes overhead): %d\n",
    usedBytes1,
    usedBytes1 - usedBytes0,
    sizeof(ConnectedComponentSegment<u16>),
    actualSizePlusOverhead);
#endif // #ifdef PRINTF_SIZE_RESULTS

  const double difference = actualSizePlusOverhead - double(sizeof(ConnectedComponentSegment<u16>));
  ASSERT_TRUE(difference > -0.0001 && difference < 1.0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComponentsSize)

GTEST_TEST(CoreTech_Vision, SortComponents)
{
  const s32 numComponents = 10;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(50, 100, 50, u16_MAX); // 2
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(s16_MAX, s16_MAX, s16_MAX, 0); // 9
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(s16_MAX, s16_MAX, 0, 0); // 7
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(s16_MAX, s16_MAX, s16_MAX, u16_MAX); // 4
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(0, s16_MAX, 0, 0); // 5
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(0, s16_MAX, s16_MAX, 0); // 8
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(0, s16_MAX, s16_MAX, u16_MAX); // 3
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(s16_MAX, s16_MAX, 0, u16_MAX); // 1
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(0, s16_MAX, 0, 0); // 6
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(42, 42, 42, 42); // 0

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  const Result result = components.SortConnectedComponentSegments();
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(*components.get_componentsU16()->Pointer(0) == component9);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(1) == component7);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(2) == component0);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(3) == component6);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(4) == component3);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(5) == component4);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(6) == component8);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(7) == component2);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(8) == component5);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(9) == component1);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, SortComponentsById)
{
  const s32 numComponents = 10;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  ConnectedComponents components(numComponents, 640, true, scratchOnchip);

  const ConnectedComponentSegment<u16> component0 = ConnectedComponentSegment<u16>(1, 1, 1, 3); // 6
  const ConnectedComponentSegment<u16> component1 = ConnectedComponentSegment<u16>(2, 2, 2, 1); // 0
  const ConnectedComponentSegment<u16> component2 = ConnectedComponentSegment<u16>(3, 3, 3, 1); // 1
  const ConnectedComponentSegment<u16> component3 = ConnectedComponentSegment<u16>(4, 4, 4, 0); // X
  const ConnectedComponentSegment<u16> component4 = ConnectedComponentSegment<u16>(5, 5, 5, 1); // 2
  const ConnectedComponentSegment<u16> component5 = ConnectedComponentSegment<u16>(6, 6, 6, 1); // 3
  const ConnectedComponentSegment<u16> component6 = ConnectedComponentSegment<u16>(7, 7, 7, 1); // 4
  const ConnectedComponentSegment<u16> component7 = ConnectedComponentSegment<u16>(8, 8, 8, 4); // 7
  const ConnectedComponentSegment<u16> component8 = ConnectedComponentSegment<u16>(9, 9, 9, 5); // 8
  const ConnectedComponentSegment<u16> component9 = ConnectedComponentSegment<u16>(0, 0, 0, 1); // 5

  components.get_componentsU16()->PushBack(component0);
  components.get_componentsU16()->PushBack(component1);
  components.get_componentsU16()->PushBack(component2);
  components.get_componentsU16()->PushBack(component3);
  components.get_componentsU16()->PushBack(component4);
  components.get_componentsU16()->PushBack(component5);
  components.get_componentsU16()->PushBack(component6);
  components.get_componentsU16()->PushBack(component7);
  components.get_componentsU16()->PushBack(component8);
  components.get_componentsU16()->PushBack(component9);

  const Result result = components.SortConnectedComponentSegmentsById(scratchOnchip);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 9);

  ASSERT_TRUE(*components.get_componentsU16()->Pointer(0) == component1);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(1) == component2);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(2) == component4);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(3) == component5);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(4) == component6);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(5) == component9);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(6) == component0);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(7) == component7);
  ASSERT_TRUE(*components.get_componentsU16()->Pointer(8) == component8);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)
{
  const s32 imageWidth = 18;
  const s32 imageHeight = 5;

  const s32 minComponentWidth = 2;
  const s32 maxSkipDistance = 0;
  const s32 maxComponentSegments = 100;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

#define ApproximateConnectedComponents2d_binaryImageDataLength (18*5)
  const s32 binaryImageData[] = {
    0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0};

  const s32 numComponents_groundTruth = 13;

  //#define UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
#define SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
#ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s32 xStart_groundTruth[] = {4,  3, 10, 13, 5, 9,  13, 6, 12, 16, 7, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 15, 6, 11, 14, 9, 14, 17, 8, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  1,  2, 2,  2,  3, 3,  3,  4, 4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  2,  1, 1,  2,  1, 2,  2,  1, 2,  2};
#else // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d
#ifdef SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
  const s32 xStart_groundTruth[] = {4,  3, 10, 5, 9,  6, 7, 13, 13, 12, 16, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 6, 11, 9, 8, 15, 14, 14, 17, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  2, 2,  3, 4, 1,  2,  3,  3,  4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  1, 1,  1, 1, 2,  2,  2,  2,  2,  2};
#else // Sorted by id, y, and xStart
  const s32 xStart_groundTruth[] = {4,  3, 10, 5, 9,  6, 7, 13, 13, 12, 16, 11, 14};
  const s32 xEnd_groundTruth[]   = {11, 5, 11, 6, 11, 9, 8, 15, 14, 14, 17, 12, 16};
  const s32 y_groundTruth[]      = {0,  1, 1,  2, 2,  3, 4, 1,  2,  3,  3,  4,  4};
  const s32 id_groundTruth[]     = {1,  1, 1,  1, 1,  1, 1, 2,  2,  2,  2,  2,  2};
#endif // #ifdef SORTED_BY_ID_GROUND_TRUTH_ApproximateConnectedComponents2d
#endif // #ifdef UNSORTED_GROUND_TRUTH_ApproximateConnectedComponents2d ... #else

  Array<u8> binaryImage(imageHeight, imageWidth, scratchOnchip);
  ASSERT_TRUE(binaryImage.IsValid());

  //binaryImage.Print("binaryImage");

  ASSERT_TRUE(binaryImage.SetCast<s32>(binaryImageData, ApproximateConnectedComponents2d_binaryImageDataLength) == imageWidth*imageHeight);

  //binaryImage.Print("binaryImage");

  //FixedLengthList<ConnectedComponentSegment<u16> > extractedComponents(maxComponentSegments, scratchOnchip);
  ConnectedComponents components(maxComponentSegments, imageWidth, true, scratchOnchip);
  ASSERT_TRUE(components.IsValid());

  const Result result = components.Extract2dComponents_FullImage(binaryImage, minComponentWidth, maxSkipDistance, scratchOnchip);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.SortConnectedComponentSegmentsById(scratchOnchip) == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 13);

  //components.Print();

  for(s32 i=0; i<numComponents_groundTruth; i++) {
    ASSERT_TRUE(components.get_componentsU16()->Pointer(i)->xStart == xStart_groundTruth[i]);
    ASSERT_TRUE(components.get_componentsU16()->Pointer(i)->xEnd == xEnd_groundTruth[i]);
    ASSERT_TRUE(components.get_componentsU16()->Pointer(i)->y == y_groundTruth[i]);
    ASSERT_TRUE(components.get_componentsU16()->Pointer(i)->id == id_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)
{
  const s32 imageWidth = 50;
  const s32 minComponentWidth = 3;
  const s32 maxComponents = 10;
  const s32 maxSkipDistance = 1;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  u8 * binaryImageRow = reinterpret_cast<u8*>(scratchOnchip.Allocate(imageWidth));
  memset(binaryImageRow, 0, imageWidth);

  FixedLengthList<ConnectedComponentSegment<u16> > extractedComponentSegments(maxComponents, scratchOnchip);

  for(s32 i=10; i<=15; i++) binaryImageRow[i] = 1;
  for(s32 i=25; i<=35; i++) binaryImageRow[i] = 1;
  for(s32 i=38; i<=38; i++) binaryImageRow[i] = 1;
  for(s32 i=43; i<=45; i++) binaryImageRow[i] = 1;
  for(s32 i=47; i<=49; i++) binaryImageRow[i] = 1;

  const Result result = ConnectedComponentsTemplate<u16>::Extract1dComponents(binaryImageRow, imageWidth, minComponentWidth, maxSkipDistance, extractedComponentSegments);

  ASSERT_TRUE(result == RESULT_OK);

  //extractedComponentSegments.Print("extractedComponentSegments");

  ASSERT_TRUE(extractedComponentSegments.get_size() == 3);

  ASSERT_TRUE(extractedComponentSegments.Pointer(0)->xStart == 10 && extractedComponentSegments.Pointer(0)->xEnd == 15);
  ASSERT_TRUE(extractedComponentSegments.Pointer(1)->xStart == 25 && extractedComponentSegments.Pointer(1)->xEnd == 35);
  ASSERT_TRUE(extractedComponentSegments.Pointer(2)->xStart == 43 && extractedComponentSegments.Pointer(2)->xEnd == 49);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)

GTEST_TEST(CoreTech_Vision, BinomialFilter)
{
  const s32 imageWidth = 10;
  const s32 imageHeight = 5;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(imageHeight, imageWidth, scratchOnchip);
  image.SetZero();

  Array<u8> imageFiltered(imageHeight, imageWidth, scratchOnchip);

  ASSERT_TRUE(image.get_buffer()!= NULL);
  ASSERT_TRUE(imageFiltered.get_buffer()!= NULL);

  *image.Pointer(0,0) = 100;
  *image.Pointer(1,0) = 255;
  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = 100 + 10 * static_cast<u8>(x);
  }
  *image.Pointer(imageHeight-2,imageWidth-1) = 100;
  *image.Pointer(imageHeight-1,imageWidth-1) = 255;

  const Result result = ImageProcessing::BinomialFilter<u8,u8,u8>(image, imageFiltered, scratchOnchip);

  CoreTechPrint("image:\n");
  image.Print();

  CoreTechPrint("imageFiltered:\n");
  imageFiltered.Print();

  ASSERT_TRUE(result == RESULT_OK);

  // Compute in Matlab for array "a" with: "a2 = floor(imfilter(floor(imfilter(a, [1,4,6,4,1], 'replicate') / (2^4)), [1,4,6,4,1]', 'replicate')  / (2^4) )"

  const s32 correctResults[imageHeight][imageWidth] = {
    {96, 47, 15, 8, 8, 9, 10, 10, 11, 11},
    {112, 66, 37, 32, 35, 37, 40, 42, 46, 50},
    {86, 62, 49, 48, 52, 56, 60, 66, 79, 97},
    {36, 32, 30, 32, 35, 37, 40, 49, 81, 126},
    {6, 6, 7, 8, 8, 9, 10, 22, 73, 148}};

  for(s32 y=0; y<imageHeight; y++) {
    for(s32 x=0; x<imageWidth; x++) {
      //CoreTechPrint("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageFiltered.Pointer(y,x));
    }
  }

  Array<u8> bigImage(480, 640, scratchOffchip);
  Array<u8> bigImageFiltered(480, 640, scratchOffchip);

  ASSERT_TRUE(AreValid(bigImage, bigImageFiltered));

  f64 totalTime = 0;

  const s32 numIterations = 1;
  for(s32 i=0; i<numIterations; i++) {
    const f64 t0 = GetTimeF64();
    const Result result2 = ImageProcessing::BinomialFilter<u8,u8,u8>(bigImage, bigImageFiltered, scratchOffchip);
    const f64 t1 = GetTimeF64();

    ASSERT_TRUE(result2 == RESULT_OK);

    totalTime += (t1 - t0);
    bigImage.Set(bigImageFiltered);
  }

  printf("Binomial filtered 640x480 image in %fms.\n", 1000*totalTime/numIterations);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BinomialFilter)

GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 imageWidth = 10;
  const s32 imageHeight = 4;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  Array<u8> image(imageHeight, imageWidth, scratchOnchip);
  Array<u8> imageDownsampled(imageHeight/2, imageWidth/2, scratchOnchip);

  ASSERT_TRUE(image.get_buffer()!= NULL);
  ASSERT_TRUE(imageDownsampled.get_buffer()!= NULL);

  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = ImageProcessing::DownsampleByTwo<u8,u32,u8>(image, imageDownsampled);

  ASSERT_TRUE(result == RESULT_OK);

  const s32 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imageDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imageDownsampled.get_size(1); x++) {
      //CoreTechPrint("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageDownsampled.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DownsampleByFactor)

GTEST_TEST(CoreTech_Vision, SolveQuartic)
{
#define PRECISION f32

  const PRECISION factors[5] = {
    -3593989.0f, -33048.973667f, 316991.744900f, 33048.734165f, -235.623396f
  };

  const PRECISION roots_groundTruth[4] = {
    0.334683441970975f, 0.006699578943935f, -0.136720934135068f, -0.213857711381642f
  };

  PRECISION roots_computed[4];
  ASSERT_TRUE(P3P::solveQuartic(factors, roots_computed) == RESULT_OK);

  for(s32 i=0; i<4; ++i) {
    ASSERT_NEAR(roots_groundTruth[i], roots_computed[i], 1e-6f);
  }

#undef PRECISION

  GTEST_RETURN_HERE;
} // GTEST_TEST(PoseEstimation, SolveQuartic)

GTEST_TEST(CoreTech_Vision, P3P_PerspectivePoseEstimation)
{
#define PRECISION f64

  InitBenchmarking();

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(AreValid(scratchCcm, scratchOnchip, scratchOffchip));

  // Parameters
  Array<PRECISION> Rtrue = Array<PRECISION>(3,3,scratchOffchip);
  // rodrigues(3*pi/180*[0 0 1])*rodrigues(4*pi/180*[0 1 0])*rodrigues(-10*pi/180*[1 0 0])
  Rtrue[0][0] =  0.9962;  Rtrue[0][1] =   -0.0636;  Rtrue[0][2] =    0.0595;
  Rtrue[1][0] =  0.0522;  Rtrue[1][1] =    0.9828;  Rtrue[1][2] =    0.1770;
  Rtrue[2][0] = -0.0698;  Rtrue[2][1] =   -0.1732;  Rtrue[2][2] =    0.9824;
  const Point3<PRECISION> Ttrue(10.f, 15.f, 100.f);

  const f32 markerSize = 26.f;

  const f32 focalLength_x = 317.2f;
  const f32 focalLength_y = 318.4f;
  const f32 camCenter_x   = 151.9f;
  const f32 camCenter_y   = 129.0f;
  const u16 camNumRows    = 240;
  const u16 camNumCols    = 320;

  const Quadrilateral<PRECISION> projNoise(Point<PRECISION>(0.1740,    0.0116),
    Point<PRECISION>(0.0041,    0.0073),
    Point<PRECISION>(0.0381,    0.1436),
    Point<PRECISION>(0.2249,    0.0851));

  const f32 distThreshold      = 3.f;
  const f32 angleThreshold     = static_cast<f32>(DEG_TO_RAD(2));
  const f32 pixelErrThreshold  = 1.f;

  // Create the 3D marker and put it in the specified pose relative to the camera
  Point3<PRECISION> marker3d[4];
  marker3d[0] = Point3<PRECISION>(-markerSize/2.f, -markerSize/2.f, 0.f);
  marker3d[1] = Point3<PRECISION>(-markerSize/2.f,  markerSize/2.f, 0.f);
  marker3d[2] = Point3<PRECISION>( markerSize/2.f, -markerSize/2.f, 0.f);
  marker3d[3] = Point3<PRECISION>( markerSize/2.f,  markerSize/2.f, 0.f);

  // Compute the ground truth projection of the marker in the image
  // NOTE: No radial distortion!
  Quadrilateral<PRECISION> proj;
  for(s32 i=0; i<4; ++i) {
    Point3<PRECISION> proj3 = Rtrue*marker3d[i] + Ttrue;
    proj3.x = focalLength_x*proj3.x + camCenter_x*proj3.z;
    proj3.y = focalLength_y*proj3.y + camCenter_y*proj3.z;
    proj[i].x = proj3.x / proj3.z;
    proj[i].y = proj3.y / proj3.z;

    // Add noise
    proj[i] += projNoise[i];
  }

  // Make sure all the corners projected within the image
  for(s32 i_corner=0; i_corner<4; ++i_corner)
  {
    ASSERT_TRUE(! isnan(proj[i_corner].x));
    ASSERT_TRUE(! isnan(proj[i_corner].y));
    ASSERT_GE(proj[i_corner].x, 0.f);
    ASSERT_LT(proj[i_corner].x, camNumCols);
    ASSERT_GE(proj[i_corner].y, 0.f);
    ASSERT_LT(proj[i_corner].y, camNumRows);
  }

  // Compute the pose of the marker w.r.t. camera from the noisy projection
  Array<PRECISION> R = Array<PRECISION>(3,3,scratchOffchip);
  Point3<PRECISION> T;

  BeginBenchmark("P3P::computePose");

  ASSERT_TRUE(P3P::computePose(proj,
    marker3d[0], marker3d[1], marker3d[2], marker3d[3],
    focalLength_x, focalLength_y,
    camCenter_x, camCenter_y,
    R, T) == RESULT_OK);

  EndBenchmark("P3P::computePose");

  ComputeAndPrintBenchmarkResults(true, true, scratchOffchip);

  //
  // Check if the estimated pose matches the true pose
  //

  // 1. Compute angular difference between the two rotation matrices
  // TODO: make this a utility function somewhere?
  // R = R_this * R_other^T
  Array<PRECISION> Rdiff = Array<PRECISION>(3,3,scratchOffchip);
  Point3<PRECISION> Tdiff;
  ComputePoseDiff(R, T, Rtrue, Ttrue, Rdiff, Tdiff, scratchOffchip);

  // This is computing angular rotation vs. identity matrix
  const f32 trace =static_cast<f32>( Rdiff[0][0] + Rdiff[1][1] + Rdiff[2][2] );
  const f32 angleDiff = std::acos(0.5f*(trace - 1.f));

  ASSERT_LE(angleDiff, angleThreshold);

  // 2. Check the translational difference between the two poses
  ASSERT_LE(Tdiff.Length(), distThreshold);

  // Check if the reprojected points match the originals
  for(s32 i_corner=0; i_corner<4; ++i_corner)
  {
    Point3<PRECISION> proj3 = R*marker3d[i_corner] + T;
    proj3.x = focalLength_x*proj3.x + camCenter_x*proj3.z;
    proj3.y = focalLength_y*proj3.y + camCenter_y*proj3.z;

    Point<PRECISION> reproj(proj3.x / proj3.z, proj3.y / proj3.z);

    ASSERT_NEAR(reproj.x, proj[i_corner].x, pixelErrThreshold);
    ASSERT_NEAR(reproj.y, proj[i_corner].y, pixelErrThreshold);
  }

#undef PRECISION

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, P3P_PerspectivePoseEstimation)

GTEST_TEST(CoreTech_Vision, BoxFilterNormalize)
{
  // Input image:
  const s32 testImageHeight = 20;
  const s32 testImageWidth  = 30;
  const u8 testImageData[testImageHeight*testImageWidth] = {
    191, 192, 193, 194, 195, 196, 196, 195, 194, 193, 193, 193, 193, 193, 192, 192, 191, 189, 189, 188, 187, 186, 185, 184, 183, 182, 180,
    178, 176, 174, 193, 193, 192, 184, 169, 159, 164, 179, 191, 194, 194, 194, 193, 193, 193, 193, 194, 194, 193, 192, 191, 189, 188, 186,
    186, 185, 184, 182, 179, 177, 194, 192, 183, 149, 97, 45, 55, 112, 161, 189, 193, 194, 193, 193, 193, 190, 184, 176, 170, 167, 169, 175,
    182, 186, 186, 185, 184, 182, 181, 179, 196, 194, 188, 157, 100, 34, 6, 47, 116, 167, 189, 192, 192, 184, 162, 128, 92, 66, 51, 47, 53,
    71, 102, 136, 163, 177, 181, 180, 180, 179, 198, 197, 195, 185, 148, 88, 26, 8, 58, 125, 170, 187, 175, 136, 78, 26, 3, 6, 16, 21, 14, 4,
    7, 37, 93, 144, 174, 181, 181, 181, 199, 198, 198, 196, 182, 140, 77, 17, 11, 68, 132, 162, 140, 78, 14, 2, 37, 86, 125, 137, 118, 73, 23,
    0, 30, 99, 153, 177, 179, 169, 199, 200, 200, 199, 197, 179, 132, 66, 10, 17, 78, 120, 110, 54, 2, 10, 68, 133, 175, 185, 168, 116, 48, 2,
    15, 79, 141, 172, 165, 126, 203, 203, 204, 203, 202, 198, 175, 124, 56, 8, 25, 79, 96, 71, 22, 0, 22, 65, 98, 109, 92, 54, 13, 4, 46, 110,
    158, 169, 134, 72, 204, 204, 204, 204, 203, 203, 198, 171, 119, 58, 30, 74, 123, 129, 98, 49, 17, 6, 5, 7, 6, 8, 27, 70, 122, 164, 180,
    159, 103, 33, 204, 204, 204, 204, 204, 203, 202, 197, 180, 156, 145, 157, 179, 189, 180, 158, 130, 104, 88, 84, 91, 111, 139, 166, 185,
    192, 184, 145, 77, 13, 204, 204, 204, 204, 203, 202, 200, 195, 181, 165, 161, 175, 190, 196, 194, 186, 173, 160, 152, 154, 164, 178, 190,
    196, 196, 194, 182, 135, 62, 6, 203, 204, 204, 204, 203, 201, 189, 156, 102, 51, 55, 111, 149, 155, 128, 91, 62, 44, 38, 44, 62, 94, 135,
    169, 188, 191, 176, 126, 51, 3, 202, 203, 203, 202, 199, 185, 145, 84, 25, 15, 54, 95, 102, 69, 25, 6, 13, 26, 32, 24, 9, 6, 34, 93, 148,
    179, 173, 126, 54, 5, 200, 200, 199, 196, 179, 134, 71, 19, 20, 68, 113, 113, 68, 13, 8, 46, 99, 138, 150, 131, 83, 28, 0, 29, 101, 157,
    174, 140, 74, 14, 197, 198, 193, 170, 122, 58, 13, 25, 80, 137, 159, 125, 57, 7, 16, 73, 136, 174, 181, 163, 110, 42, 2, 22, 92, 152, 179,
    164, 113, 45, 194, 190, 166, 117, 56, 20, 39, 95, 150, 183, 185, 153, 96, 38, 14, 32, 71, 98, 104, 85, 47, 14, 18, 68, 131, 175, 190, 188,
    166, 116, 189, 165, 116, 56, 32, 62, 119, 169, 194, 200, 200, 193, 169, 129, 87, 55, 41, 38, 36, 37, 46, 71, 110, 151, 183, 197, 198, 198,
    195, 182, 188, 158, 114, 74, 86, 135, 180, 202, 206, 206, 206, 207, 205, 197, 180, 158, 138, 125, 124, 133, 152, 173, 191, 199, 200, 200,
    200, 199, 197, 196, 194, 187, 175, 172, 179, 193, 201, 203, 204, 204, 205, 205, 205, 206, 206, 204, 202, 199, 199, 199, 201, 202, 202,
    200, 199, 199, 198, 196, 194, 192, 191, 192, 193, 195, 198, 198, 198, 199, 200, 201, 202, 202, 202, 202, 201, 201, 201, 201, 200, 199,
    198, 198, 197, 196, 195, 195, 193, 191, 189, 186};

  // Ground truth result for filterWidth = 5, 10, 20, 40, 80
  const s32 NUM_FILTER_WIDTHS = 5;
  const u8 groundTruthData[NUM_FILTER_WIDTHS][testImageHeight*testImageWidth] = {
    // FilterWidth = 5:
    {217, 222, 234, 218, 233, 241, 240, 228, 213, 200, 194, 192, 192, 192, 192, 193, 193, 193, 194, 194, 193, 191, 189, 188, 186, 186, 184, 183, 180, 178, 220, 221, 232, 204, 214, 221, 230, 232, 218, 196, 183, 177, 176, 180, 186, 194, 205, 215, 222, 223, 219, 210, 200, 189, 183, 178, 175, 173, 172, 172, 220, 215, 215, 156, 122, 68, 89, 168, 202, 197, 178, 170, 171, 182, 199, 218, 234, 243, 247, 246, 245, 241, 231, 213, 194, 178, 168, 162, 164, 166, 195, 180, 172, 126, 96, 39, 8, 61, 130, 154, 151, 145, 148, 155, 154, 136, 106, 78, 60, 55, 63, 85, 118, 144, 154, 150, 140, 132, 137, 144, 196, 181, 175, 146, 141, 106, 39, 13, 83, 143, 162, 164, 159, 139, 94, 36, 4, 8, 20, 26, 18, 5, 10, 50, 109, 141, 147, 140, 141, 148, 195, 178, 169, 143, 152, 145, 102, 27, 18, 97, 157, 175, 157, 103, 23, 4, 79, 161, 201, 210, 196, 142, 49, 0, 44, 112, 140, 146, 147, 145, 193, 176, 163, 134, 145, 153, 141, 90, 16, 27, 111, 157, 147, 84, 4, 26, 187, 255, 255, 255, 255, 255, 129, 4, 24, 94, 135, 152, 144, 115, 196, 177, 162, 131, 137, 148, 153, 133, 73, 11, 34, 102, 123, 98, 35, 0, 41, 109, 145, 155, 140, 93, 24, 6, 62, 120, 151, 161, 128, 72, 197, 177, 161, 129, 131, 139, 150, 152, 124, 67, 35, 85, 136, 145, 118, 64, 23, 8, 6, 8, 7, 10, 35, 85, 130, 155, 166, 161, 107, 37, 196, 176, 160, 128, 129, 133, 144, 162, 174, 170, 164, 171, 184, 190, 192, 192, 183, 162, 142, 133, 135, 150, 164, 169, 165, 161, 162, 153, 87, 16, 196, 176, 160, 128, 130, 135, 147, 164, 179, 181, 180, 187, 192, 195, 207, 229, 255, 255, 255, 255, 255, 251, 218, 185, 161, 153, 157, 148, 75, 8, 196, 177, 162, 131, 136, 146, 155, 149, 110, 58, 62, 120, 157, 166, 145, 112, 81, 60, 53, 64, 89, 124, 154, 163, 159, 154, 156, 142, 63, 4, 196, 179, 166, 138, 148, 157, 145, 98, 32, 19, 67, 116, 128, 92, 35, 8, 17, 33, 40, 32, 13, 8, 45, 105, 141, 155, 156, 140, 65, 6, 197, 183, 176, 149, 156, 139, 88, 26, 28, 91, 146, 152, 103, 23, 16, 93, 175, 214, 225, 217, 162, 59, 0, 40, 108, 142, 154, 143, 81, 16, 202, 197, 195, 150, 127, 72, 18, 34, 99, 153, 172, 145, 79, 12, 34, 160, 255, 255, 255, 255, 225, 94, 3, 31, 99, 134, 148, 147, 108, 47, 208, 205, 190, 117, 66, 26, 50, 109, 148, 162, 160, 142, 104, 49, 21, 48, 97, 122, 126, 109, 67, 21, 25, 80, 126, 144, 146, 150, 140, 107, 205, 182, 136, 56, 35, 71, 125, 155, 156, 148, 147, 151, 148, 128, 95, 62, 45, 40, 37, 40, 51, 80, 116, 142, 151, 147, 140, 143, 150, 152, 204, 173, 129, 69, 84, 127, 154, 155, 144, 138, 137, 143, 152, 159, 158, 147, 133, 121, 121, 129, 145, 158, 163, 156, 145, 137, 134, 134, 143, 154, 209, 202, 190, 156, 162, 167, 160, 150, 143, 140, 140, 143, 147, 154, 163, 171, 177, 180, 181, 178, 173, 165, 157, 148, 143, 140, 139, 138, 145, 154, 199, 196, 192, 167, 168, 162, 155, 151, 148, 148, 148, 149, 149, 151, 153, 156, 160, 162, 161, 159, 156, 153, 150, 148, 147, 147, 146, 145, 151, 157},
    // FilterWidth = 10:
    {199, 206, 213, 218, 219, 219, 205, 205, 206, 209, 211, 210, 207, 205, 203, 206, 210, 215, 220, 221, 217, 211, 204, 198, 192, 188, 184, 178, 173, 169, 198, 204, 210, 206, 191, 179, 171, 189, 207, 216, 222, 223, 219, 214, 211, 211, 218, 226, 233, 235, 229, 219, 210, 202, 198, 195, 191, 186, 179, 173, 196, 199, 196, 164, 109, 50, 57, 119, 176, 216, 230, 236, 233, 228, 224, 220, 218, 217, 220, 219, 216, 213, 211, 208, 204, 201, 196, 190, 183, 176, 195, 197, 196, 169, 110, 37, 6, 49, 126, 191, 228, 240, 242, 230, 200, 158, 116, 88, 71, 67, 73, 92, 124, 158, 186, 198, 197, 190, 183, 176, 194, 196, 198, 193, 157, 94, 25, 8, 60, 137, 197, 225, 214, 166, 95, 31, 3, 7, 22, 30, 19, 5, 8, 43, 106, 160, 189, 190, 183, 177, 192, 193, 196, 199, 188, 145, 72, 16, 11, 71, 146, 186, 164, 91, 16, 2, 44, 109, 167, 187, 155, 90, 26, 0, 33, 108, 164, 184, 180, 165, 180, 181, 182, 184, 184, 168, 112, 57, 9, 16, 78, 126, 119, 59, 2, 11, 76, 156, 215, 231, 202, 131, 51, 2, 15, 80, 140, 167, 157, 118, 183, 183, 186, 190, 193, 192, 154, 113, 53, 8, 27, 92, 116, 88, 27, 0, 28, 88, 140, 157, 125, 67, 15, 4, 51, 119, 166, 171, 131, 69, 181, 181, 184, 190, 195, 199, 177, 159, 117, 62, 35, 95, 166, 177, 133, 67, 24, 9, 8, 11, 9, 10, 33, 83, 147, 191, 202, 170, 105, 32, 180, 180, 184, 190, 197, 199, 182, 186, 183, 175, 180, 213, 254, 255, 250, 219, 187, 161, 145, 137, 135, 148, 171, 200, 231, 233, 215, 161, 80, 13, 183, 183, 186, 191, 196, 198, 181, 186, 186, 187, 202, 236, 255, 255, 255, 255, 247, 245, 245, 241, 234, 228, 226, 231, 241, 233, 211, 149, 65, 6, 188, 188, 189, 192, 195, 196, 169, 147, 102, 55, 65, 140, 196, 208, 173, 125, 89, 67, 60, 67, 86, 118, 158, 195, 224, 221, 196, 133, 51, 2, 191, 191, 191, 191, 190, 177, 127, 76, 24, 15, 58, 107, 120, 84, 31, 7, 17, 37, 47, 34, 11, 7, 37, 102, 166, 194, 181, 126, 52, 4, 191, 189, 187, 183, 167, 124, 60, 16, 18, 64, 111, 114, 71, 13, 8, 52, 118, 172, 190, 160, 95, 30, 0, 29, 104, 158, 170, 133, 68, 12, 188, 188, 181, 158, 112, 52, 10, 21, 69, 123, 147, 118, 54, 6, 15, 74, 144, 191, 200, 175, 112, 40, 1, 20, 87, 143, 166, 149, 102, 40, 188, 184, 160, 112, 53, 18, 33, 83, 135, 170, 177, 148, 94, 37, 14, 32, 75, 107, 115, 91, 48, 13, 16, 63, 124, 165, 177, 172, 150, 105, 186, 163, 114, 55, 31, 59, 105, 154, 182, 194, 199, 195, 172, 132, 90, 58, 45, 43, 41, 41, 48, 71, 106, 144, 176, 188, 186, 183, 177, 165, 188, 159, 115, 74, 85, 131, 162, 186, 195, 201, 205, 207, 204, 196, 180, 162, 147, 138, 139, 145, 158, 171, 183, 189, 191, 190, 188, 184, 179, 177, 197, 191, 179, 174, 177, 187, 181, 186, 191, 194, 196, 195, 193, 193, 194, 197, 202, 205, 207, 203, 197, 192, 187, 183, 183, 184, 182, 179, 175, 173, 196, 197, 197, 195, 194, 189, 177, 180, 183, 184, 185, 183, 182, 183, 184, 189, 194, 199, 199, 195, 189, 184, 179, 176, 175, 175, 173, 171, 168, 166},
    // FilterWidth = 20:
    {195, 196, 197, 197, 199, 201, 204, 205, 206, 207, 202, 206, 211, 216, 219, 220, 217, 212, 208, 205, 204, 204, 203, 202, 199, 195, 191, 187, 183, 180, 195, 195, 193, 185, 169, 160, 167, 184, 199, 203, 198, 203, 206, 211, 215, 216, 215, 212, 208, 206, 205, 204, 204, 202, 200, 197, 194, 190, 185, 182, 195, 193, 184, 149, 97, 45, 56, 115, 167, 198, 198, 203, 208, 213, 217, 215, 206, 194, 185, 180, 184, 191, 199, 204, 202, 199, 195, 191, 188, 184, 197, 196, 189, 157, 100, 34, 6, 49, 122, 178, 197, 206, 212, 209, 188, 150, 106, 75, 57, 52, 59, 79, 115, 153, 182, 194, 196, 192, 190, 186, 200, 200, 197, 187, 150, 90, 27, 8, 62, 135, 179, 203, 197, 158, 93, 31, 3, 7, 18, 23, 16, 4, 8, 42, 105, 160, 190, 195, 193, 191, 202, 202, 202, 199, 186, 145, 81, 18, 11, 74, 140, 178, 160, 92, 17, 2, 44, 101, 144, 157, 137, 84, 26, 0, 34, 110, 168, 192, 192, 179, 203, 205, 204, 203, 203, 187, 141, 71, 11, 18, 83, 134, 128, 65, 2, 12, 84, 159, 205, 215, 198, 137, 56, 2, 17, 89, 156, 187, 178, 134, 208, 208, 209, 207, 208, 207, 187, 135, 62, 9, 27, 89, 113, 87, 27, 0, 27, 78, 115, 127, 108, 64, 15, 4, 53, 124, 175, 184, 144, 76, 209, 209, 208, 207, 207, 210, 210, 184, 130, 64, 32, 82, 143, 155, 121, 60, 20, 7, 5, 7, 6, 9, 31, 81, 138, 182, 196, 170, 109, 34, 207, 206, 205, 204, 205, 207, 209, 208, 192, 169, 150, 169, 201, 220, 214, 188, 151, 117, 97, 92, 101, 124, 157, 186, 205, 208, 196, 152, 80, 13, 199, 198, 196, 194, 193, 194, 194, 192, 179, 165, 153, 173, 195, 208, 210, 201, 184, 166, 154, 155, 168, 184, 198, 205, 203, 198, 183, 135, 61, 5, 199, 199, 198, 196, 195, 195, 186, 156, 103, 52, 53, 112, 157, 169, 142, 101, 67, 46, 39, 45, 65, 99, 144, 180, 198, 198, 180, 128, 51, 2, 198, 198, 197, 195, 192, 181, 144, 84, 25, 15, 53, 97, 109, 76, 28, 6, 14, 28, 34, 25, 9, 6, 37, 101, 159, 189, 180, 129, 55, 5, 195, 194, 192, 188, 173, 131, 70, 19, 20, 69, 111, 115, 72, 14, 8, 52, 110, 150, 160, 139, 89, 30, 0, 31, 108, 165, 180, 144, 75, 14, 191, 192, 186, 163, 117, 56, 12, 25, 80, 138, 155, 125, 59, 7, 17, 80, 148, 186, 190, 170, 116, 44, 2, 23, 96, 157, 183, 167, 114, 45, 187, 183, 159, 112, 53, 19, 38, 94, 149, 183, 179, 152, 98, 39, 14, 34, 75, 102, 107, 87, 49, 14, 18, 71, 136, 180, 193, 190, 167, 116, 182, 159, 111, 53, 30, 59, 115, 165, 191, 198, 192, 190, 171, 133, 91, 57, 42, 38, 36, 37, 47, 73, 114, 157, 189, 201, 201, 199, 195, 181, 181, 152, 109, 70, 82, 129, 173, 195, 200, 201, 196, 201, 204, 199, 183, 160, 139, 124, 122, 132, 153, 175, 194, 203, 203, 201, 200, 198, 195, 193, 188, 180, 168, 164, 171, 185, 193, 196, 197, 198, 193, 197, 201, 204, 205, 202, 198, 193, 192, 193, 198, 200, 201, 199, 197, 197, 195, 193, 190, 188, 187, 187, 187, 188, 191, 192, 193, 195, 196, 197, 194, 197, 200, 202, 202, 201, 199, 197, 194, 194, 196, 197, 196, 195, 193, 192, 190, 187, 185, 182},
    // FilterWidth = 40:
    {195, 196, 197, 197, 199, 201, 204, 205, 206, 207, 202, 206, 211, 216, 219, 220, 217, 212, 208, 205, 204, 204, 203, 202, 199, 195, 191, 187, 183, 180, 195, 195, 193, 185, 169, 160, 167, 184, 199, 203, 198, 203, 206, 211, 215, 216, 215, 212, 208, 206, 205, 204, 204, 202, 200, 197, 194, 190, 185, 182, 195, 193, 184, 149, 97, 45, 56, 115, 167, 198, 198, 203, 208, 213, 217, 215, 206, 194, 185, 180, 184, 191, 199, 204, 202, 199, 195, 191, 188, 184, 197, 196, 189, 157, 100, 34, 6, 49, 122, 178, 197, 206, 212, 209, 188, 150, 106, 75, 57, 52, 59, 79, 115, 153, 182, 194, 196, 192, 190, 186, 200, 200, 197, 187, 150, 90, 27, 8, 62, 135, 179, 203, 197, 158, 93, 31, 3, 7, 18, 23, 16, 4, 8, 42, 105, 160, 190, 195, 193, 191, 202, 202, 202, 199, 186, 145, 81, 18, 11, 74, 140, 178, 160, 92, 17, 2, 44, 101, 144, 157, 137, 84, 26, 0, 34, 110, 168, 192, 192, 179, 203, 205, 204, 203, 203, 187, 141, 71, 11, 18, 83, 134, 128, 65, 2, 12, 84, 159, 205, 215, 198, 137, 56, 2, 17, 89, 156, 187, 178, 134, 208, 208, 209, 207, 208, 207, 187, 135, 62, 9, 27, 89, 113, 87, 27, 0, 27, 78, 115, 127, 108, 64, 15, 4, 53, 124, 175, 184, 144, 76, 209, 209, 208, 207, 207, 210, 210, 184, 130, 64, 32, 82, 143, 155, 121, 60, 20, 7, 5, 7, 6, 9, 31, 81, 138, 182, 196, 170, 109, 34, 207, 206, 205, 204, 205, 207, 209, 208, 192, 169, 150, 169, 201, 220, 214, 188, 151, 117, 97, 92, 101, 124, 157, 186, 205, 208, 196, 152, 80, 13, 199, 198, 196, 194, 193, 194, 194, 192, 179, 165, 153, 173, 195, 208, 210, 201, 184, 166, 154, 155, 168, 184, 198, 205, 203, 198, 183, 135, 61, 5, 199, 199, 198, 196, 195, 195, 186, 156, 103, 52, 53, 112, 157, 169, 142, 101, 67, 46, 39, 45, 65, 99, 144, 180, 198, 198, 180, 128, 51, 2, 198, 198, 197, 195, 192, 181, 144, 84, 25, 15, 53, 97, 109, 76, 28, 6, 14, 28, 34, 25, 9, 6, 37, 101, 159, 189, 180, 129, 55, 5, 195, 194, 192, 188, 173, 131, 70, 19, 20, 69, 111, 115, 72, 14, 8, 52, 110, 150, 160, 139, 89, 30, 0, 31, 108, 165, 180, 144, 75, 14, 191, 192, 186, 163, 117, 56, 12, 25, 80, 138, 155, 125, 59, 7, 17, 80, 148, 186, 190, 170, 116, 44, 2, 23, 96, 157, 183, 167, 114, 45, 187, 183, 159, 112, 53, 19, 38, 94, 149, 183, 179, 152, 98, 39, 14, 34, 75, 102, 107, 87, 49, 14, 18, 71, 136, 180, 193, 190, 167, 116, 182, 159, 111, 53, 30, 59, 115, 165, 191, 198, 192, 190, 171, 133, 91, 57, 42, 38, 36, 37, 47, 73, 114, 157, 189, 201, 201, 199, 195, 181, 181, 152, 109, 70, 82, 129, 173, 195, 200, 201, 196, 201, 204, 199, 183, 160, 139, 124, 122, 132, 153, 175, 194, 203, 203, 201, 200, 198, 195, 193, 188, 180, 168, 164, 171, 185, 193, 196, 197, 198, 193, 197, 201, 204, 205, 202, 198, 193, 192, 193, 198, 200, 201, 199, 197, 197, 195, 193, 190, 188, 187, 187, 187, 188, 191, 192, 193, 195, 196, 197, 194, 197, 200, 202, 202, 201, 199, 197, 194, 194, 196, 197, 196, 195, 193, 192, 190, 187, 185, 182},
    // FilterWidth = 80:
    {195, 196, 197, 197, 199, 201, 204, 205, 206, 207, 202, 206, 211, 216, 219, 220, 217, 212, 208, 205, 204, 204, 203, 202, 199, 195, 191, 187, 183, 180, 195, 195, 193, 185, 169, 160, 167, 184, 199, 203, 198, 203, 206, 211, 215, 216, 215, 212, 208, 206, 205, 204, 204, 202, 200, 197, 194, 190, 185, 182, 195, 193, 184, 149, 97, 45, 56, 115, 167, 198, 198, 203, 208, 213, 217, 215, 206, 194, 185, 180, 184, 191, 199, 204, 202, 199, 195, 191, 188, 184, 197, 196, 189, 157, 100, 34, 6, 49, 122, 178, 197, 206, 212, 209, 188, 150, 106, 75, 57, 52, 59, 79, 115, 153, 182, 194, 196, 192, 190, 186, 200, 200, 197, 187, 150, 90, 27, 8, 62, 135, 179, 203, 197, 158, 93, 31, 3, 7, 18, 23, 16, 4, 8, 42, 105, 160, 190, 195, 193, 191, 202, 202, 202, 199, 186, 145, 81, 18, 11, 74, 140, 178, 160, 92, 17, 2, 44, 101, 144, 157, 137, 84, 26, 0, 34, 110, 168, 192, 192, 179, 203, 205, 204, 203, 203, 187, 141, 71, 11, 18, 83, 134, 128, 65, 2, 12, 84, 159, 205, 215, 198, 137, 56, 2, 17, 89, 156, 187, 178, 134, 208, 208, 209, 207, 208, 207, 187, 135, 62, 9, 27, 89, 113, 87, 27, 0, 27, 78, 115, 127, 108, 64, 15, 4, 53, 124, 175, 184, 144, 76, 209, 209, 208, 207, 207, 210, 210, 184, 130, 64, 32, 82, 143, 155, 121, 60, 20, 7, 5, 7, 6, 9, 31, 81, 138, 182, 196, 170, 109, 34, 207, 206, 205, 204, 205, 207, 209, 208, 192, 169, 150, 169, 201, 220, 214, 188, 151, 117, 97, 92, 101, 124, 157, 186, 205, 208, 196, 152, 80, 13, 199, 198, 196, 194, 193, 194, 194, 192, 179, 165, 153, 173, 195, 208, 210, 201, 184, 166, 154, 155, 168, 184, 198, 205, 203, 198, 183, 135, 61, 5, 199, 199, 198, 196, 195, 195, 186, 156, 103, 52, 53, 112, 157, 169, 142, 101, 67, 46, 39, 45, 65, 99, 144, 180, 198, 198, 180, 128, 51, 2, 198, 198, 197, 195, 192, 181, 144, 84, 25, 15, 53, 97, 109, 76, 28, 6, 14, 28, 34, 25, 9, 6, 37, 101, 159, 189, 180, 129, 55, 5, 195, 194, 192, 188, 173, 131, 70, 19, 20, 69, 111, 115, 72, 14, 8, 52, 110, 150, 160, 139, 89, 30, 0, 31, 108, 165, 180, 144, 75, 14, 191, 192, 186, 163, 117, 56, 12, 25, 80, 138, 155, 125, 59, 7, 17, 80, 148, 186, 190, 170, 116, 44, 2, 23, 96, 157, 183, 167, 114, 45, 187, 183, 159, 112, 53, 19, 38, 94, 149, 183, 179, 152, 98, 39, 14, 34, 75, 102, 107, 87, 49, 14, 18, 71, 136, 180, 193, 190, 167, 116, 182, 159, 111, 53, 30, 59, 115, 165, 191, 198, 192, 190, 171, 133, 91, 57, 42, 38, 36, 37, 47, 73, 114, 157, 189, 201, 201, 199, 195, 181, 181, 152, 109, 70, 82, 129, 173, 195, 200, 201, 196, 201, 204, 199, 183, 160, 139, 124, 122, 132, 153, 175, 194, 203, 203, 201, 200, 198, 195, 193, 188, 180, 168, 164, 171, 185, 193, 196, 197, 198, 193, 197, 201, 204, 205, 202, 198, 193, 192, 193, 198, 200, 201, 199, 197, 197, 195, 193, 190, 188, 187, 187, 187, 188, 191, 192, 193, 195, 196, 197, 194, 197, 200, 202, 202, 201, 199, 197, 194, 194, 196, 197, 196, 195, 193, 192, 190, 187, 185, 182}
  };
  const s32 filterWidths[NUM_FILTER_WIDTHS] = {5, 10, 20, 40, 80};

  // Need space for input, output, and ground truth, plus the f32 array used
  // inside BoxFilterNormalize for the integral image, plus some "extra"
  const s32 BUFFER_SIZE = 9*testImageWidth*testImageHeight;
  u8 buffer[BUFFER_SIZE];
  MemoryStack scratch(buffer, BUFFER_SIZE);

  Array<u8> testImage(testImageHeight, testImageWidth, scratch);
  ASSERT_TRUE(testImage.IsValid());

  Array<u8> groundTruthResult(testImageHeight, testImageWidth, scratch);
  ASSERT_TRUE(groundTruthResult.IsValid());

  Array<u8> testImageNorm(testImageHeight, testImageWidth, scratch);
  ASSERT_TRUE(testImageNorm.IsValid());

  ASSERT_TRUE(testImage.Set(testImageData, testImageHeight*testImageWidth) == testImageHeight*testImageWidth);

  for(s32 iWidth=0; iWidth<NUM_FILTER_WIDTHS; ++iWidth) {
    ASSERT_TRUE(groundTruthResult.Set(groundTruthData[iWidth], testImageHeight*testImageWidth) == testImageHeight*testImageWidth);

    Result lastResult = ImageProcessing::BoxFilterNormalize(testImage, filterWidths[iWidth],
      static_cast<u8>(128), testImageNorm, scratch);

    ASSERT_TRUE(lastResult == RESULT_OK);

    ASSERT_TRUE(testImageNorm.IsNearlyEqualTo(groundTruthResult, 1));

    //testImageNorm.Show("testImageNorm", false);
    //groundTruthResult.Show("groundTruth", true);
  } // for each filter width

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BoxFilterNormalize)

#endif // #if !defined(JUST_FIDUCIAL_DETECTION)

#if !ANKICORETECH_EMBEDDED_USE_GTEST
s32 RUN_ALL_VISION_TESTS(s32 &numPassedTests, s32 &numFailedTests)
{
  numPassedTests = 0;
  numFailedTests = 0;

#ifdef RUN_PC_ONLY_TESTS
  //CALL_GTEST_TEST(CoreTech_Vision, TrainFaceRecognizer);
  CALL_GTEST_TEST(CoreTech_Vision, TestFaceRecognizer);
#endif

#if !defined(JUST_FIDUCIAL_DETECTION)

#ifdef RUN_HIGH_MEMORY_TESTS
  CALL_GTEST_TEST(CoreTech_Vision, KLT);
  CALL_GTEST_TEST(CoreTech_Vision, Harris);
#endif

  CALL_GTEST_TEST(CoreTech_Vision, DistanceTransform);
  CALL_GTEST_TEST(CoreTech_Vision, FastGradient);
  CALL_GTEST_TEST(CoreTech_Vision, Canny);
  CALL_GTEST_TEST(CoreTech_Vision, BoxFilterU8U16);
  CALL_GTEST_TEST(CoreTech_Vision, Vignetting);
  CALL_GTEST_TEST(CoreTech_Vision, FaceDetection);
  CALL_GTEST_TEST(CoreTech_Vision, ResizeImage);
  CALL_GTEST_TEST(CoreTech_Vision, DecisionTreeVision);
  //CALL_GTEST_TEST(CoreTech_Vision, BinaryTrackerHeaderTemplate);
  CALL_GTEST_TEST(CoreTech_Vision, BinaryTracker);
  CALL_GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_DerivativeThreshold);
  CALL_GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_GrayvalueThreshold);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo);
  //CALL_GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledProjective);
  //CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Projective);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Affine);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Slow);
  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering);
  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration);
#endif // #if !defined(JUST_FIDUCIAL_DETECTION)

  CALL_GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers);
  //CALL_GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark);

#if defined(RUN_HIGH_MEMORY_TESTS)
  CALL_GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers_benchmark640);
#endif

#if !defined(JUST_FIDUCIAL_DETECTION)
  CALL_GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents);
  CALL_GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput);
  CALL_GTEST_TEST(CoreTech_Vision, LaplacianPeaks);
  CALL_GTEST_TEST(CoreTech_Vision, LineFitsPeaks);
  CALL_GTEST_TEST(CoreTech_Vision, Correlate1d);
  CALL_GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateFilledCenterComponents_hollowRows);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents);
  CALL_GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents);
  CALL_GTEST_TEST(CoreTech_Vision, CompressComponentIds);
  CALL_GTEST_TEST(CoreTech_Vision, ComponentsSize);
  CALL_GTEST_TEST(CoreTech_Vision, SortComponents);
  CALL_GTEST_TEST(CoreTech_Vision, SortComponentsById);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d);
  CALL_GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d);
  CALL_GTEST_TEST(CoreTech_Vision, BinomialFilter);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByFactor);
  CALL_GTEST_TEST(CoreTech_Vision, SolveQuartic);
  CALL_GTEST_TEST(CoreTech_Vision, P3P_PerspectivePoseEstimation);
  CALL_GTEST_TEST(CoreTech_Vision, BoxFilterNormalize);
#endif // #if !defined(JUST_FIDUCIAL_DETECTION)

  return numFailedTests;
} // int RUN_ALL_VISION_TESTS()
#endif // #if !ANKICORETECH_EMBEDDED_USE_GTEST
