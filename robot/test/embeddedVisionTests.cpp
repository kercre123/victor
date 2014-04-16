/**
File: embeddedVisionTests.cpp
Author: Peter Barnum
Created: 2013

Various tests of the coretech vision embedded library.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/gtestLight.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/benchmarking_c.h"
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

#include "anki/vision/MarkerCodeDefinitions.h"

#include "../../coretech/vision/blockImages/blockImage50_320x240.h"
#include "../../coretech/vision/blockImages/blockImages00189_80x60.h"
#include "../../coretech/vision/blockImages/blockImages00190_80x60.h"
#include "../../coretech/vision/blockImages/newFiducials_320x240.h"
#include "../../../systemTestImages/cozmo_2014_01_29_11_41_05_10_320x240.h"
#include "../../../systemTestImages/cozmo_2014_01_29_11_41_05_12_320x240.h"
#include "../../../systemTestImages/cozmo_date2014_04_04_time17_40_08_frame0.h"
#include "../../../systemTestImages/cozmo_date2014_04_10_time16_15_40_frame0.h"

#include "anki/vision/robot/lbpcascade_frontalface.h"

#include "embeddedTests.h"

#include <cmath>

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
#include <iostream>
#include <fstream>
#endif

using namespace Anki::Embedded;

//#define RUN_FACE_DETECTION_GUI

#if defined(RUN_FACE_DETECTION_GUI) && defined(ANKICORETECH_EMBEDDED_USE_OPENCV)
GTEST_TEST(CoreTech_Vision, FaceDetection_All)
{
  using namespace std;

  cv::CascadeClassifier face_cascade;

  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt.xml");
  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt2.xml");
  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt_tree.xml");
  const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml");

  //MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  //ASSERT_TRUE(scratchOffchip.IsValid());

  //Classifier::CascadeClassifier cc(face_cascade_name.data(), scratchOffchip);
  //cc.SaveAsHeader("c:/tmp/cascade.h", "lbpcascade_frontalface");

  if( !face_cascade.load( face_cascade_name ) ) {
    printf("Could not load %s\n", face_cascade_name.c_str());
    return;
  }

  std::ifstream faceFilenames("C:/datasets/faces/lfw/allFiles.txt");

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const cv::Size minSize = cv::Size(30,30);

  const s32 MAX_CANDIDATES = 5000;

  FixedLengthList<Rectangle<s32> > detectedFaces_anki(MAX_CANDIDATES, scratchOffchip);

  Classifier::CascadeClassifier_LBP cc(face_cascade_name.data(), scratchOffchip);

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

      const f32 t0 = GetTime();

      face_cascade.detectMultiScale(
        grayImage,
        detectedFaces_opencv,
        1.1, // double scaleFactor=1.1,
        2, // int minNeighbors=3,
        0|CV_HAAR_SCALE_IMAGE, // int flags=0,
        cv::Size(30, 30), // Size minSize=Size(),
        cv::Size() // Size maxSize=Size()
        );

      const f32 t1 = GetTime();

      const cv::Size maxSize = cv::Size(imageArray.get_size(1), imageArray.get_size(0));

      cc.DetectMultiScale(
        imageArray,
        static_cast<f32>(scaleFactor),
        minNeighbors,
        minSize.height, minSize.width,
        maxSize.height, maxSize.width,
        detectedFaces_anki,
        scratchOffchip);

      const f32 t2 = GetTime();

      printf("OpenCV took %f seconds and Anki took %f seconds\n", t1-t0, t2-t1);

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
        cv::Point center( RoundS32((detectedFaces_anki[i].left + detectedFaces_anki[i].right)*0.5), RoundS32((detectedFaces_anki[i].top + detectedFaces_anki[i].bottom)*0.5) );
        cv::ellipse( toShow, center, cv::Size( RoundS32((detectedFaces_anki[i].right-detectedFaces_anki[i].left)*0.5), RoundS32((detectedFaces_anki[i].bottom-detectedFaces_anki[i].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );
      }

      for( size_t i = 0; i < detectedFaces_opencv.size(); i++ )
      {
        cv::Point center( RoundS32(detectedFaces_opencv[i].x + detectedFaces_opencv[i].width*0.5), RoundS32(detectedFaces_opencv[i].y + detectedFaces_opencv[i].height*0.5) );
        cv::ellipse( toShow, center, cv::Size( RoundS32(detectedFaces_opencv[i].width*0.5), RoundS32(detectedFaces_opencv[i].height*0.5)), 0, 0, 360, cv::Scalar( 0, 0, 255 ), 1, 8, 0 );
      }

      const s32 maxChars = 1024;
      char outname[maxChars];
      snprintf(outname, "Detected faces %d", in);

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
#endif // #if defined(RUN_FACE_DETECTION_GUI) && defined(ANKICORETECH_EMBEDDED_USE_OPENCV)

GTEST_TEST(CoreTech_Vision, FaceDetection)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  ASSERT_TRUE(scratchCcm.IsValid());

  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

  cv::CascadeClassifier face_cascade;

  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt.xml");
  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt2.xml");
  //const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_alt_tree.xml");
  const std::string face_cascade_name = std::string("C:/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml");

  const s32 imageHeight = 240;
  const s32 imageWidth = 320;

  const double scaleFactor = 1.1;
  const int minNeighbors = 2;
  const cv::Size minSize = cv::Size(30,30);
  const cv::Size maxSize = cv::Size(imageWidth, imageHeight);

  const s32 MAX_CANDIDATES = 5000;

  Array<u8> image(imageHeight, imageWidth, scratchOnchip);
  image.Set(&cozmo_date2014_04_10_time16_15_40_frame0[0], imageHeight*imageWidth);

  FixedLengthList<Rectangle<s32> > detectedFaces_anki(MAX_CANDIDATES, scratchOnchip);

  //Classifier::CascadeClassifier_LBP cc(face_cascade_name.data(), scratchOffchip);
  //cc.SaveAsHeader("c:/tmp/lbpcascade_frontalface.h", "lbpcascade_frontalface");

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

  f32 t0 = GetTime();

  const Result result = cc.DetectMultiScale(
    image,
    static_cast<f32>(scaleFactor),
    minNeighbors,
    minSize.height, minSize.width,
    maxSize.height, maxSize.width,
    detectedFaces_anki,
    scratchOffchip);

  ASSERT_TRUE(result == RESULT_OK);

  f32 t1 = GetTime();

  printf("Detection took %f seconds\n", t1-t0);

  ASSERT_TRUE(detectedFaces_anki.get_size() == 1);
  ASSERT_TRUE(detectedFaces_anki[0] == Rectangle<s32>(103,218,40,155));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, FaceDetection)

GTEST_TEST(CoreTech_Vision, ResizeImage)
{
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

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
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

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

  FiducialMarkerDecisionTree tree(reinterpret_cast<u8*>(&treeData[0]), treeDataLength, treeDataNumFractionalBits, treeMaxDepth, probeXOffsets, probeYOffsets, numProbeOffsets);

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

GTEST_TEST(CoreTech_Vision, BinaryTrackerHeaderTemplate)
{
  const s32 imageHeight = 240;
  const s32 imageWidth = 320;

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  ASSERT_TRUE(scratchCcm.IsValid());

  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

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

  TemplateTracker::BinaryTracker::EdgeDetectionParameters edgeDetectionParams_update = edgeDetectionParams_template;
  edgeDetectionParams_update.maxDetectionsPerType = 2500;

  const s32 updateEdgeDetection_maxDetectionsPerType = 2500;

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

  templateImage.Set(&cozmo_date2014_04_04_time17_40_08_frame0[0], imageHeight*imageWidth);

  // Skip zero rows/columns (non-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("Skip 0 nonlist\n");
    edgeDetectionParams_template.everyNLines = 1;

    InitBenchmarking();

    BeginBenchmark("BinaryTracker init");

    TemplateTracker::BinaryTracker tracker(
      Anki::Vision::MARKER_BATTERIES,
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

    PrintBenchmarkResults_OnlyTotals();
  } // Skip zero rows/columns (non-list)

  GTEST_RETURN_HERE;
}

GTEST_TEST(CoreTech_Vision, BinaryTracker)
{
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  ASSERT_TRUE(scratchCcm.IsValid());

  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

  Array<u8> templateImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOnchip);
  Array<u8> nextImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOnchip);

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

  templateImage.Set(&cozmo_2014_01_29_11_41_05_10_320x240[0], cozmo_2014_01_29_11_41_05_10_320x240_WIDTH*cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT);
  nextImage.Set(&cozmo_2014_01_29_11_41_05_12_320x240[0], cozmo_2014_01_29_11_41_05_12_320x240_WIDTH*cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT);

  // Skip zero rows/columns (non-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("Skip 0 nonlist\n");
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

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
  } // Skip zero rows/columns (non-list)

  // Skip one row/column (non-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("\nSkip 1 nonlist\n");
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

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
  } // Skip one row/column (non-list)

  // Skip zero rows/columns (with-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("\nSkip 0 list\n");
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

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.068f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
  } // Skip zero rows/columns (with-list)

  // Skip one row/column (with-list)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("\nSkip 1 list\n");
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

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.060f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
  } // Skip one row/column (with-list)

  // Skip zero rows/columns (with-ransac)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("\nSkip 0 ransac\n");
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

    printf("numMatches = %d / %d\n", verify_numMatches, numTemplatePixels);

    // TODO: verify this number manually
    //ASSERT_TRUE(verify_numMatches == );

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.068f; transform_groundTruth[0][1] = -0.001f;   transform_groundTruth[0][2] = 2.376f;
      transform_groundTruth[1][0] = 0.003f; transform_groundTruth[1][1] = 1.061f; transform_groundTruth[1][2] = -4.109f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 1");

      //ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
  } // Skip zero rows/columns (with-ransac)

  // Skip one row/column (with-ransac)
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    printf("\nSkip 1 ransac\n");
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

    printf("numMatches = %d / %d\n", verify_numMatches, numTemplatePixels);

    // TODO: verify this number manually
    //ASSERT_TRUE(verify_numMatches == );

    //Array<u8> warpedTemplateImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOffchip);

    if(edgeDetectionParams_template.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
      Array<f32> transform_groundTruth = Eye<f32>(3,3,scratchOffchip);
      transform_groundTruth[0][0] = 1.069f; transform_groundTruth[0][1] = -0.001f; transform_groundTruth[0][2] = 2.440f;
      transform_groundTruth[1][0] = 0.005f; transform_groundTruth[1][1] = 1.060f; transform_groundTruth[1][2] = -4.100f;
      transform_groundTruth[2][0] = 0.0f;   transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

      //tracker.get_transformation().get_homography().Print("fixed-float 2");

      //ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
    }

    PrintBenchmarkResults_OnlyTotals();
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

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

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

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

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
  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);

  ASSERT_TRUE(scratchOffchip.IsValid());

  ASSERT_TRUE(blockImage50_320x240_WIDTH % MEMORY_ALIGNMENT == 0);
  ASSERT_TRUE(reinterpret_cast<size_t>(&blockImage50_320x240[0]) % MEMORY_ALIGNMENT == 0);

  Array<u8> in(blockImage50_320x240_HEIGHT, blockImage50_320x240_WIDTH, const_cast<u8*>(&blockImage50_320x240[0]), blockImage50_320x240_WIDTH*blockImage50_320x240_HEIGHT, Flags::Buffer(false,false,false));

  Array<u8> out(60, 80, scratchOffchip);
  //in.Print("in");

  const Result result = ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in, 2, out, scratchOffchip);
  ASSERT_TRUE(result == RESULT_OK);

  printf("%d %d %d %d", out[0][0], out[0][17], out[40][80-1], out[59][80-3]);

  ASSERT_TRUE(out[0][0] == 155);
  ASSERT_TRUE(out[0][17] == 157);
  ASSERT_TRUE(out[40][80-1] == 143);
  ASSERT_TRUE(out[59][80-3] == 127);

  //{
  //  Matlab matlab(false);
  //  matlab.PutArray(in, "in");
  //  matlab.PutArray(out, "out");
  //}

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo)

bool IsBlockImage50_320x240Valid(const u8 * const imageBuffer, const bool isBigEndian)
{
  //printf("%d %d %d %d\n", imageBuffer[0], imageBuffer[1000], imageBuffer[320*120], imageBuffer[320*240-1]);

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

/* TODO: Update this to test FillDockErrMsg() in visionSystem.cpp
This will require exposing more of the internal state of the vision system
or changing the function's API to pass these things in.

GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine)
{
// TODO: make these the real values
const s32 horizontalTrackingResolution = 80;
const f32 blockMarkerWidthInMM = 50.0f;
const f32 horizontalFocalLengthInMM = 5.0f;

MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
ASSERT_TRUE(ms.IsValid());

const Quadrilateral<f32> initialCorners(Point<f32>(5.0f,5.0f), Point<f32>(100.0f,100.0f), Point<f32>(50.0f,20.0f), Point<f32>(10.0f,80.0f));
const Transformations::PlanarTransformation_f32 transform(Transformations::TRANSFORM_AFFINE, initialCorners, ms);

f32 rel_x, rel_y, rel_rad;
ASSERT_TRUE(Docking::ComputeDockingErrorSignal(transform,
horizontalTrackingResolution, blockMarkerWidthInMM, horizontalFocalLengthInMM,
rel_x, rel_y, rel_rad, ms) == RESULT_OK);

// TODO: manually compute the correct output
ASSERT_TRUE(FLT_NEAR(rel_x,2.7116308f));
ASSERT_TRUE(NEAR(rel_y,-8.1348925f, 0.1f)); // The Myriad inexact floating point mode is imprecise here
ASSERT_TRUE(FLT_NEAR(rel_rad,-0.87467581f));

GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine)
*/

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledProjective)
{
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOnchip.IsValid());

  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  ASSERT_TRUE(scratchCcm.IsValid());

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(scratchOffchip.IsValid());

  Array<u8> templateImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
  Array<u8> nextImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOnchip);

  const Quadrilateral<f32> templateQuad(Point<f32>(128,78), Point<f32>(220,74), Point<f32>(229,167), Point<f32>(127,171));

  const s32 numPyramidLevels = 4;

  const s32 maxIterations = 25;
  const f32 convergenceTolerance = .05f;

  const f32 scaleTemplateRegionPercent = 1.05f;

  const u8 verify_maxPixelDifference = 30;

  const s32 maxSamplesAtBaseLevel = 2000;

  // TODO: add check that images were loaded correctly

  templateImage.Set(&cozmo_2014_01_29_11_41_05_10_320x240[0], cozmo_2014_01_29_11_41_05_10_320x240_WIDTH*cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT);
  nextImage.Set(&cozmo_2014_01_29_11_41_05_12_320x240[0], cozmo_2014_01_29_11_41_05_12_320x240_WIDTH*cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT);

  // Translation-only LK_Projective
  {
    PUSH_MEMORY_STACK(scratchCcm);
    PUSH_MEMORY_STACK(scratchOnchip);
    PUSH_MEMORY_STACK(scratchOffchip);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    //tracker.ShowTemplate("tracker", true, true);

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 10);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 115);*/

    const f64 time2 = GetTime();

    printf("Translation-only LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Translation-only LK_SampledProjective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3, 3, scratchOffchip);
    transform_groundTruth[0][2] = 3.143f;;
    transform_groundTruth[1][2] = -4.952f;

    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 119);*/

    const f64 time2 = GetTime();

    printf("Affine LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Affine LK_SampledProjective");

    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(nextImage, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratchCcm) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    /*ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 121);
    ASSERT_TRUE(verify_numSimilarPixels == 119);*/

    const f64 time2 = GetTime();

    printf("Projective LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Projective LK_SampledProjective");

    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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

GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof)
{
  //  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  //  ASSERT_TRUE(scratchOnchip.IsValid());
  //
  //  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);
  //  ASSERT_TRUE(scratchCcm.IsValid());
  //
  //  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  //  ASSERT_TRUE(scratchOffchip.IsValid());
  //
  //  Array<u8> templateImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
  //  Array<u8> nextImage(cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_12_320x240_WIDTH, scratchOnchip);
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
  //  templateImage.Set(&cozmo_2014_01_29_11_41_05_10_320x240[0], cozmo_2014_01_29_11_41_05_10_320x240_WIDTH*cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT);
  //  nextImage.Set(&cozmo_2014_01_29_11_41_05_12_320x240[0], cozmo_2014_01_29_11_41_05_12_320x240_WIDTH*cozmo_2014_01_29_11_41_05_12_320x240_HEIGHT);
  //
  //  // Translation-only LK_Projective
  //  {
  //    PUSH_MEMORY_STACK(scratchCcm);
  //    PUSH_MEMORY_STACK(scratchOnchip);
  //    PUSH_MEMORY_STACK(scratchOffchip);
  //
  //    InitBenchmarking();
  //
  //    const f64 time0 = GetTime();
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
  //    const f64 time1 = GetTime();
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
  //    const f64 time2 = GetTime();
  //
  //    printf("Translation-only LK_SampledPlanar6dof totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
  //    PrintBenchmarkResults_All();
  //
  //    tracker.get_transformation().Print("Translation-only LK_SampledPlanar6dof");
  //
  //    // This ground truth is from the PC c++ version
  //    Array<f32> transform_groundTruth = Eye<f32>(3, 3, scratchOffchip);
  //    transform_groundTruth[0][2] = 3.143f;;
  //    transform_groundTruth[1][2] = -4.952f;
  //
  //    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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
  //    const f64 time0 = GetTime();
  //
  //    TemplateTracker::LucasKanadeTracker_SampledProjective tracker(templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, maxSamplesAtBaseLevel, scratchCcm, scratchOnchip, scratchOffchip);
  //
  //    ASSERT_TRUE(tracker.IsValid());
  //
  //    const f64 time1 = GetTime();
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
  //    const f64 time2 = GetTime();
  //
  //    printf("Affine LK_SampledProjective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
  //    PrintBenchmarkResults_All();
  //
  //    tracker.get_transformation().Print("Affine LK_SampledProjective");
  //
  //    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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
  //    const f64 time0 = GetTime();
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
  //    const f64 time1 = GetTime();
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
  //    const f64 time2 = GetTime();
  //
  //    printf("Projective LK_SampledPlanar6dof totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
  //    PrintBenchmarkResults_All();
  //
  //    tracker.get_transformation().Print("Projective LK_SampledPlanar6dof");
  //
  //    Array<u8> warpedImage(cozmo_2014_01_29_11_41_05_10_320x240_HEIGHT, cozmo_2014_01_29_11_41_05_10_320x240_WIDTH, scratchOffchip);
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

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof)

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

  //MemoryStack scratch0(&ddrBuffer[0], 80*60*2 + 256);
  MemoryStack scratch1(&offchipBuffer[0], 600000);

  //ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //printf("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK_Projective
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 13);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 474);

    const f64 time2 = GetTime();

    printf("Translation-only LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Translation-only LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK_Projective
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTime();

    printf("Affine LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Affine LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.301f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.101f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK_Projective
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Projective tracker(image1, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTime();

    printf("Projective LK_Projective totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Projective LK_Projective");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
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

  MemoryStack scratch1(&offchipBuffer[0], 600000);

  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //printf("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Affine tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;

    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 13);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 474);

    const f64 time2 = GetTime();

    printf("Translation-only FAST-LK totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Translation-only LK_Affine");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Affine tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    s32 verify_meanAbsoluteDifference;
    s32 verify_numInBounds;
    s32 verify_numSimilarPixels;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, verify_maxPixelDifference, verify_converged, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged);
    ASSERT_TRUE(verify_meanAbsoluteDifference == 8);
    ASSERT_TRUE(verify_numInBounds == 529);
    ASSERT_TRUE(verify_numSimilarPixels == 521);

    const f64 time2 = GetTime();

    printf("Affine FAST-LK totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Affine LK_Affine");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
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

  //MemoryStack scratch0(&ddrBuffer[0], 80*60*2 + 256);
  MemoryStack scratch1(&offchipBuffer[0], 600000);

  //ASSERT_TRUE(scratch0.IsValid());
  ASSERT_TRUE(scratch1.IsValid());

  ASSERT_TRUE(blockImages00189_80x60_HEIGHT == imageHeight && blockImages00190_80x60_HEIGHT == imageHeight);
  ASSERT_TRUE(blockImages00189_80x60_WIDTH == imageWidth && blockImages00190_80x60_WIDTH == imageWidth);

  Array<u8> image1(imageHeight, imageWidth, scratch1);
  image1.Set(blockImages00189_80x60, imageWidth*imageHeight);

  Array<u8> image2(imageHeight, imageWidth, scratch1);
  image2.Set(blockImages00190_80x60, imageWidth*imageHeight);

  ASSERT_TRUE(*image1.Pointer(0,0) == 45);
  //printf("%d %d %d %d\n", *image1.Pointer(0,0), *image1.Pointer(0,1), *image1.Pointer(0,2), *image1.Pointer(0,3));
  /*image1.Print("image1");
  image2.Print("image2");*/

  // Translation-only LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, verify_converged, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTime();

    printf("Translation-only LK totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Translation-only LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][2] = -1.368f;
    transform_groundTruth[1][2] = -1.041f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Affine LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_AFFINE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, verify_converged, scratch1) == RESULT_OK);
    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTime();

    printf("Affine LK totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Affine LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.299f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.104f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  // Projective LK
  {
    PUSH_MEMORY_STACK(scratch1);

    InitBenchmarking();

    const f64 time0 = GetTime();

    TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, scaleTemplateRegionPercent, numPyramidLevels, Transformations::TRANSFORM_PROJECTIVE, ridgeWeight, scratch1);

    ASSERT_TRUE(tracker.IsValid());

    const f64 time1 = GetTime();

    bool verify_converged = false;
    ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, true, verify_converged, scratch1) == RESULT_OK);

    ASSERT_TRUE(verify_converged == true);

    const f64 time2 = GetTime();

    printf("Projective LK totalTime:%dms initTime:%dms updateTrack:%dms\n", (s32)Round(1000*(time2-time0)), (s32)Round(1000*(time1-time0)), (s32)Round(1000*(time2-time1)));
    PrintBenchmarkResults_All();

    tracker.get_transformation().Print("Projective LK");

    // This ground truth is from the PC c++ version
    Array<f32> transform_groundTruth = Eye<f32>(3,3,scratch1);
    transform_groundTruth[0][0] = 1.013f;  transform_groundTruth[0][1] = 0.032f; transform_groundTruth[0][2] = -1.339f;
    transform_groundTruth[1][0] = -0.036f; transform_groundTruth[1][1] = 1.0f;   transform_groundTruth[1][2] = -1.042f;
    transform_groundTruth[2][0] = 0.0f;    transform_groundTruth[2][1] = 0.0f;   transform_groundTruth[2][2] = 1.0f;

    ASSERT_TRUE(AreElementwiseEqual_PercentThreshold<f32>(tracker.get_transformation().get_homography(), transform_groundTruth, .01, .01));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Slow)

//GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerTmp)
//{
//  const s32 imageTmpHeight = 240;
//  const s32 imageTmpWidth = 320;
//
//  const s32 imageHeight = 60;
//  const s32 imageWidth = 80;
//
//  const s32 numPyramidLevels = 2;
//
//  const f32 ridgeWeight = 0.0f;
//
//  //const Rectangle<f32> templateRegion(59, 149, 35, 125);
//  const Rectangle<f32> templateRegion(59.0f/4.0f, 149.0f/4.0f, 35.0f/4.0f, 125.0f/4.0f);
//
//  const s32 maxIterations = 25;
//  const f32 convergenceTolerance = .05f;
//
//  // TODO: add check that images were loaded correctly
//
//  const s32 bigBufferSize = 10000000;
//  char * bigBuffer = reinterpret_cast<char*>(malloc(bigBufferSize));
//
//  MemoryStack scratch1(bigBuffer, bigBufferSize);
//
//  ASSERT_TRUE(scratch1.IsValid());
//
//  Array<u8> image1Tmp(imageTmpHeight, imageTmpWidth, scratch1);
//  image1Tmp.Set(templateImage, imageTmpWidth*imageTmpHeight);
//
//  Array<u8> image2Tmp(imageTmpHeight, imageTmpWidth, scratch1);
//  image2Tmp.Set(nextImage, imageTmpWidth*imageTmpHeight);
//
//  Array<u8> image1(imageHeight, imageWidth, scratch1);
//  Array<u8> image2(imageHeight, imageWidth, scratch1);
//
//  ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(image1Tmp, 2, image1, scratch1);
//  ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(image2Tmp, 2, image2, scratch1);
//
//  TemplateTracker::LucasKanadeTracker_Slow tracker(image1, templateRegion, numPyramidLevels, Transformations::TRANSFORM_TRANSLATION, ridgeWeight, scratch1);
//
//  ASSERT_TRUE(tracker.IsValid());
//
//  bool verify_converged = false;
//  ASSERT_TRUE(tracker.UpdateTrack(image2, maxIterations, convergenceTolerance, false, verify_converged, scratch1) == RESULT_OK);
//
//  tracker.get_transformation().Print("Translation-only LK");
//
//  free(bigBuffer);
//
//  GTEST_RETURN_HERE;
//} // GTEST_TEST(CoreTech_Vision, LucasKanadeTrackerTmp)

GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering)
{
  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(3,16,ms);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  //image.Print("image");

  Array<u8> image2(4,16,ms);
  ASSERT_TRUE(image2.IsValid());

  image2[0][0] = 1; image2[0][1] = 2; image2[0][2] = 3;
  image2[1][0] = 9; image2[1][1] = 9; image2[1][2] = 9;
  image2[2][0] = 0; image2[2][1] = 0; image2[2][2] = 1;
  image2[3][0] = 5; image2[3][1] = 5; image2[3][2] = 5;

  //image2.Print("image2");

  Array<s32> filteredOutput(1, 16, ms);

  //
  // Test with border of 2
  //

  ScrollingIntegralImage_u8_s32 ii_border2(4, 16, 2, ms);
  ASSERT_TRUE(ii_border2.ScrollDown(image, 4, ms) == RESULT_OK);

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

  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

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

  ScrollingIntegralImage_u8_s32 ii_border1(4, 16, 1, ms);

  ASSERT_TRUE(ii_border1.ScrollDown(image, 4, ms) == RESULT_OK);

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

  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

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

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, ms);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, ms) == RESULT_OK);

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

  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, ms) == RESULT_OK);

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
  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(3,16,ms);
  ASSERT_TRUE(image.IsValid());

  image[0][0] = 1; image[0][1] = 2; image[0][2] = 3;
  image[1][0] = 9; image[1][1] = 9; image[1][2] = 9;
  image[2][0] = 0; image[2][1] = 0; image[2][2] = 1;

  Array<u8> image2(4,16,ms);
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

  ScrollingIntegralImage_u8_s32 ii_border2(3, 16, 2, ms);
  ASSERT_TRUE(ii_border2.get_rowOffset() == -2);

  //Result ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+4,'int32'), 1, 3, 2)
  ASSERT_TRUE(ii_border2.ScrollDown(image, 3, ms) == RESULT_OK);

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
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

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
  ASSERT_TRUE(ii_border2.ScrollDown(image, 2, ms) == RESULT_OK);

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

  ScrollingIntegralImage_u8_s32 ii_border1(3, 16, 1, ms);

  ASSERT_TRUE(ii_border1.get_rowOffset() == -1);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+2,'int32'), 1, 3, 1)
  ASSERT_TRUE(ii_border1.ScrollDown(image, 3, ms) == RESULT_OK);

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
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

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
  ASSERT_TRUE(ii_border1.ScrollDown(image, 1, ms) == RESULT_OK);

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

  ScrollingIntegralImage_u8_s32 ii_border0(3, 16, 0, ms);

  ASSERT_TRUE(ii_border0.get_rowOffset() == 0);

  // initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2),'int32'), 1, 3, 0)
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 3, ms) == RESULT_OK);

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
  ASSERT_TRUE(ii_border0.ScrollDown(image2, 1, ms) == RESULT_OK);

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

GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers)
{
  const s32 scaleImage_thresholdMultiplier = 65536; // 1.0*(2^16)=65536
  //const s32 scaleImage_thresholdMultiplier = 49152; // .75*(2^16)=49152
  const s32 scaleImage_numPyramidLevels = 3;

  const s32 component1d_minComponentWidth = 0;
  const s32 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(newFiducials_320x240_HEIGHT,newFiducials_320x240_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(newFiducials_320x240_HEIGHT,newFiducials_320x240_WIDTH);

  const s32 component_minimumNumPixels = RoundS32(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength));
  const s32 component_maximumNumPixels = RoundS32(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  //const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  //const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8
  const f32 component_minHollowRatio = 1.0f;

  const s32 maxExtractedQuads = 1000/2;
  const s32 quads_minQuadArea = 100/4;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;
  //const s32 maxConnectedComponentSegments = 5000; // 25000/4 = 6250
  const s32 maxConnectedComponentSegments = 39000; // 322*240/2 = 38640

  MemoryStack scratchOffchip(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  MemoryStack scratchOnchip(&onchipBuffer[0], ONCHIP_BUFFER_SIZE);
  MemoryStack scratchCcm(&ccmBuffer[0], CCM_BUFFER_SIZE);

  ASSERT_TRUE(scratchOffchip.IsValid());
  ASSERT_TRUE(scratchOnchip.IsValid());
  ASSERT_TRUE(scratchCcm.IsValid());

  Array<u8> image(newFiducials_320x240_HEIGHT, newFiducials_320x240_WIDTH, scratchOffchip);
  image.Set(newFiducials_320x240, newFiducials_320x240_WIDTH*newFiducials_320x240_HEIGHT);

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
    const f64 time0 = GetTime();
    const Result result = DetectFiducialMarkers(
      image,
      markers,
      homographies,
      scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_minHollowRatio,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      maxConnectedComponentSegments,
      maxExtractedQuads,
      //true, //< TODO: change back to false
      false,
      scratchCcm,
      scratchOnchip,
      scratchOffchip);
    const f64 time1 = GetTime();

    printf("totalTime: %dms\n", (s32)Round(1000*(time1-time0)));

    // TODO: add back
    //PrintBenchmarkResults_All();

    ASSERT_TRUE(result == RESULT_OK);
  }

  markers.Print("markers");

  //[Type 11-MARKER_ANKILOGO]: (139,9) (127,88) (217,21) (205,100)]
  //[Type 10-MARKER_ANGRYFACE]: (8,41) (37,97) (64,12) (92,68)]
  //[Type 19-MARKER_FIRE]: (247,12) (247,52) (287,12) (287,52)]
  //[Type 13-MARKER_BATTERIES]: (238,71) (238,148) (314,71) (314,148)]
  //[Type 11-MARKER_ANKILOGO]: (83,116) (83,155) (122,116) (122,155)]
  //[Type 13-MARKER_BATTERIES]: (17,123) (17,161) (54,123) (54,161)]
  //[Type 14-MARKER_BULLSEYE]: (222,137) (151,137) (222,209) (151,210)]
  //[Type 10-MARKER_ANGRYFACE]: (245,161) (245,224) (307,161) (307,224)]
  //[Type 30-MARKER_SQUAREPLUSCORNERS]: (127,166) (89,166) (128,205) (88,205)]
  //[Type 30-MARKER_SQUAREPLUSCORNERS]: (44,174) (15,201) (70,204) (41,230)]

  if(scaleImage_thresholdMultiplier == 65536) {
    const s32 numMarkers_groundTruth = 10;

    ASSERT_TRUE(markers.get_size() == numMarkers_groundTruth);

    const s16 corners_groundTruth[numMarkers_groundTruth][4][2] = {
      {{139,9},{127,88},{217,21},{205,100}},
      {{8,41},{37,97},{64,12},{92,68}},
      {{247,12},{247,52},{287,12},{287,52}},
      {{238,71},{238,148},{314,71},{314,148}},
      {{83,116},{83,155},{122,116},{122,155}},
      {{17,123},{17,161},{54,123},{54,161}},
      {{222,137},{151,137},{222,209},{151,210}},
      {{245,161},{245,224},{307,161},{307,224}},
      {{127,166},{89,166},{128,205},{88,205}},
      {{44,174},{15,201},{70,204},{41,230}}
    };

    const Anki::Vision::MarkerType markerTypes_groundTruth[numMarkers_groundTruth] = {
      Anki::Vision::MARKER_ANKILOGO,
      Anki::Vision::MARKER_ANGRYFACE,
      Anki::Vision::MARKER_FIRE,
      Anki::Vision::MARKER_BATTERIES,
      Anki::Vision::MARKER_ANKILOGO,
      Anki::Vision::MARKER_BATTERIES,
      Anki::Vision::MARKER_BULLSEYE,
      Anki::Vision::MARKER_ANGRYFACE,
      Anki::Vision::MARKER_SQUAREPLUSCORNERS,
      Anki::Vision::MARKER_SQUAREPLUSCORNERS
    };

    for(s32 iMarker=0; iMarker<numMarkers_groundTruth; iMarker++) {
      ASSERT_TRUE(markers[iMarker].isValid);
      ASSERT_TRUE(markers[iMarker].markerType == markerTypes_groundTruth[iMarker]);
      for(s32 iCorner=0; iCorner<4; iCorner++) {
        ASSERT_TRUE(markers[iMarker].corners[iCorner] == Point<s16>(corners_groundTruth[iMarker][iCorner][0], corners_groundTruth[iMarker][iCorner][1]));
      }
    }
  } else {
    ASSERT_TRUE(false);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers)

GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)
{
  const s32 numComponents = 60;
  const s32 minQuadArea = 100;
  const s32 quadSymmetryThreshold = 384;
  const s32 imageHeight = 480;
  const s32 imageWidth = 640;
  const s32 minDistanceFromImageEdge = 2;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  FixedLengthList<BlockMarker> markers(50, ms);

  // TODO: check these by hand
  const Quadrilateral<s16> quads_groundTruth[] = {
    Quadrilateral<s16>(Point<s16>(24,4), Point<s16>(10,4), Point<s16>(24,18), Point<s16>(10,18)) ,
    Quadrilateral<s16>(Point<s16>(129,50),  Point<s16>(100,50), Point<s16>(129,79), Point<s16>(100,79)) };

  ConnectedComponents components(numComponents, imageWidth, ms);

  // Small square
  for(s32 y=0; y<15; y++) {
    components.PushBack(ConnectedComponentSegment(10,24,y+4,1));
  }

  // Big square
  for(s32 y=0; y<30; y++) {
    components.PushBack(ConnectedComponentSegment(100,129,y+50,2));
  }

  // Skewed quad
  components.PushBack(ConnectedComponentSegment(100,300,100,3));
  for(s32 y=0; y<10; y++) {
    components.PushBack(ConnectedComponentSegment(100,110,y+100,3));
  }

  // Tiny square
  for(s32 y=0; y<5; y++) {
    components.PushBack(ConnectedComponentSegment(10,14,y,4));
  }

  FixedLengthList<Quadrilateral<s16> > extractedQuads(2, ms);

  components.SortConnectedComponentSegments();

  const Result result =  ComputeQuadrilateralsFromConnectedComponents(components, minQuadArea, quadSymmetryThreshold, minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, ms);
  ASSERT_TRUE(result == RESULT_OK);

  // extractedQuads.Print("extractedQuads");

  for(s32 i=0; i<extractedQuads.get_size(); i++) {
    ASSERT_TRUE(extractedQuads[i] == quads_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents)

GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);
  MemoryStack ms(&offchipBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  FixedPointArray<s32> image(1,15,2,ms);
  FixedPointArray<s32> filter(1,5,2,ms);
  FixedPointArray<s32> out(1,15,4,ms);

  for(s32 i=0; i<image.get_size(1); i++) {
    *image.Pointer(0,i) = 1 + i;
  }

  for(s32 i=0; i<filter.get_size(1); i++) {
    *filter.Pointer(0,i) = 2*(1 + i);
  }

  const s32 out_groundTruth[] = {140, 110, 110, 140, 170, 200, 230, 260, 290, 320, 350, 380, 410, 290, 200};

  const Result result = ImageProcessing::Correlate1dCircularAndSameSizeOutput(image, filter, out);
  ASSERT_TRUE(result == RESULT_OK);

  for(s32 i=0; i<out.get_size(1); i++) {
    ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput)

GTEST_TEST(CoreTech_Vision, LaplacianPeaks)
{
#define LaplacianPeaks_BOUNDARY_LENGTH 65
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);

  MemoryStack ms(&offchipBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  FixedLengthList<Point<s16> > boundary(LaplacianPeaks_BOUNDARY_LENGTH, ms);

  const s32 componentsX_groundTruth[66] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 componentsY_groundTruth[66] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<LaplacianPeaks_BOUNDARY_LENGTH; i++) {
    boundary.PushBack(Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  FixedLengthList<Point<s16> > peaks(4, ms);

  const Result result = ExtractLaplacianPeaks(boundary, peaks, ms);
  ASSERT_TRUE(result == RESULT_OK);

  //boundary.Print("boundary");
  //peaks.Print("peaks");

  ASSERT_TRUE(*peaks.Pointer(0) == Point<s16>(109,201));
  ASSERT_TRUE(*peaks.Pointer(1) == Point<s16>(109,205));
  ASSERT_TRUE(*peaks.Pointer(2) == Point<s16>(104,210));
  ASSERT_TRUE(*peaks.Pointer(3) == Point<s16>(100,210));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, LaplacianPeaks)

GTEST_TEST(CoreTech_Vision, Correlate1d)
{
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);
  MemoryStack ms(&offchipBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,1,2,ms);
    FixedPointArray<s32> in2(1,4,2,ms);
    FixedPointArray<s32> out(1,4,4,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {2, 4, 6, 8};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,3,5,ms);
    FixedPointArray<s32> in2(1,6,1,ms);
    FixedPointArray<s32> out(1,8,3,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {0, 2, 3, 5, 6, 8, 4, 1};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,4,2,ms);
    FixedPointArray<s32> in2(1,4,2,ms);
    FixedPointArray<s32> out(1,7,3,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {4, 11, 20, 30, 20, 11, 4};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
    ASSERT_TRUE(result == RESULT_OK);

    //out.Print();

    for(s32 i=0; i<out.get_size(1); i++) {
      ASSERT_TRUE(*out.Pointer(0,i) == out_groundTruth[i]);
    }
  }

  {
    PUSH_MEMORY_STACK(ms);

    FixedPointArray<s32> in1(1,4,1,ms);
    FixedPointArray<s32> in2(1,5,5,ms);
    FixedPointArray<s32> out(1,8,8,ms);

    for(s32 i=0; i<in1.get_size(1); i++) {
      *in1.Pointer(0,i) = 1 + i;
    }

    for(s32 i=0; i<in2.get_size(1); i++) {
      *in2.Pointer(0,i) = 2*(1 + i);
    }

    const s32 out_groundTruth[] = {32, 88, 160, 240, 320, 208, 112, 40};

    const Result result = ImageProcessing::Correlate1d(in1, in2, out);
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

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const s32 yValues[128] = {200, 200, 201, 201, 202, 203, 204, 205, 206, 207, 207, 208, 208, 209, 209, 210, 210};
  const s32 xStartValues[128] = {102, 104, 102, 108, 102, 100, 100, 104, 100, 100, 103, 100, 103, 100, 103, 100, 103};
  const s32 xEndValues[128] = { 102, 105, 105, 109, 109, 105, 105, 109, 105, 101, 104, 101, 104, 101, 104, 101, 104};

  const s32 componentsX_groundTruth[128] = {105, 105, 106, 107, 108, 109, 109, 108, 107, 106, 105, 105, 105, 105, 106, 107, 108, 109, 108, 107, 106, 105, 105, 104, 104, 104, 104, 104, 103, 103, 103, 103, 103, 102, 101, 101, 101, 101, 101, 100, 100, 100, 100, 100, 101, 102, 103, 104, 104, 104, 103, 102, 101, 100, 100, 101, 102, 102, 102, 102, 102, 103, 104, 104, 105};
  const s32 componentsY_groundTruth[128] = {200, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 203, 204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 207, 208, 209, 210, 210, 209, 208, 207, 206, 206, 206, 206, 206, 205, 204, 204, 204, 204, 204, 203, 203, 203, 202, 201, 200, 201, 201, 201, 200, 200};

  for(s32 i=0; i<numComponents; i++) {
    components.PushBack(ConnectedComponentSegment(xStartValues[i],xEndValues[i],yValues[i],1));
  }

  components.SortConnectedComponentSegments();

  //#define DRAW_TraceNextExteriorBoundary
#ifdef DRAW_TraceNextExteriorBoundary
  {
    MemoryStack scratch0(&ddrBuffer[0], OFFCHIP_BUFFER_SIZE);
    ASSERT_TRUE(scratch0.IsValid());

    Array<u8> drawnComponents(480, 640, scratch0);
    DrawComponents<u8>(drawnComponents, components, 64, 255);

    matlab.PutArray(drawnComponents, "drawnComponents");
    //drawnComponents.Show("drawnComponents", true, false);
  }
#endif // DRAW_TraceNextExteriorBoundary

  FixedLengthList<Point<s16> > extractedBoundary(boundaryLength, ms);

  {
    s32 endComponentIndex = -1;
    const Result result = TraceNextExteriorBoundary(components, startComponentIndex, extractedBoundary, endComponentIndex, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  //extractedBoundary.Print();

  for(s32 i=0; i<boundaryLength; i++) {
    ASSERT_TRUE(*extractedBoundary.Pointer(i) == Point<s16>(componentsX_groundTruth[i], componentsY_groundTruth[i]));
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, TraceNextExteriorBoundary)

GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)
{
  const s32 numComponents = 10;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(5, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  FixedLengthList<Anki::Embedded::Rectangle<s16> > componentBoundingBoxes(numComponents, ms);
  {
    const Result result = components.ComputeComponentBoundingBoxes(componentBoundingBoxes);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(*componentBoundingBoxes.Pointer(1) == Anki::Embedded::Rectangle<s16>(0,1004,0,2));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(2) == Anki::Embedded::Rectangle<s16>(0,4,3,3));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(3) == Anki::Embedded::Rectangle<s16>(0,10,4,6));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(4) == Anki::Embedded::Rectangle<s16>(0,6,7,8));
  ASSERT_TRUE(*componentBoundingBoxes.Pointer(5) == Anki::Embedded::Rectangle<s16>(5,1000,9,9));

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComputeComponentBoundingBoxes)

GTEST_TEST(CoreTech_Vision, ComputeComponentCentroids)
{
  const s32 numComponents = 10;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  FixedLengthList<Point<s16> > componentCentroids(numComponents, ms);
  {
    const Result result = components.ComputeComponentCentroids(componentCentroids, ms);
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

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 2, 5, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(4, 6, 5, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(0, 0, 6, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(6, 6, 6, 1);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 1, 7, 2);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(3, 3, 7, 2);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(5, 7, 7, 2);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 1, 8, 2);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(5, 12, 8, 2);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 10, 12, 3);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.InvalidateFilledCenterComponents_hollowRows(minHollowRatio, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  //components.Print();

  ASSERT_TRUE(components.Pointer(0)->id == 1);
  ASSERT_TRUE(components.Pointer(1)->id == 1);
  ASSERT_TRUE(components.Pointer(2)->id == 1);
  ASSERT_TRUE(components.Pointer(3)->id == 1);
  ASSERT_TRUE(components.Pointer(4)->id == 0);
  ASSERT_TRUE(components.Pointer(5)->id == 0);
  ASSERT_TRUE(components.Pointer(6)->id == 0);
  ASSERT_TRUE(components.Pointer(7)->id == 0);
  ASSERT_TRUE(components.Pointer(8)->id == 0);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateFilledCenterComponents_hollowRows)

GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)
{
  const s32 numComponents = 10;
  const s32 sparseMultiplyThreshold = 10 << 5;
  const s32 solidMultiplyThreshold = 2 << 5;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1); // Ok
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(0, 10, 3, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(0, 10, 5, 2); // Too solid
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 10, 6, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 10, 8, 2);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, 10, 10, 3); // Too sparse
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, 10, 100, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 0, 105, 4); // Ok
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, 0, 108, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 10, 110, 5); // Too solid

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 1);
  ASSERT_TRUE(components.Pointer(1)->id == 1);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 0);
  ASSERT_TRUE(components.Pointer(5)->id == 0);
  ASSERT_TRUE(components.Pointer(6)->id == 0);
  ASSERT_TRUE(components.Pointer(7)->id == 4);
  ASSERT_TRUE(components.Pointer(8)->id == 4);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSolidOrSparseComponents)

GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)
{
  const s32 numComponents = 10;
  const s32 minimumNumPixels = 6;
  const s32 maximumNumPixels = 1000;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 10, 0, 1);
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(12, 12, 1, 1);
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(16, 1004, 2, 1);
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 4, 3, 2);
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 2, 4, 3);
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(4, 6, 5, 3);
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(8, 10, 6, 3);
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 4, 7, 4);
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(6, 6, 8, 4);
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 1000, 9, 5);

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, ms);
    ASSERT_TRUE(result == RESULT_OK);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 0);
  ASSERT_TRUE(components.Pointer(1)->id == 0);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 3);
  ASSERT_TRUE(components.Pointer(5)->id == 3);
  ASSERT_TRUE(components.Pointer(6)->id == 3);
  ASSERT_TRUE(components.Pointer(7)->id == 4);
  ASSERT_TRUE(components.Pointer(8)->id == 4);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(ms);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 2);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 0);
  ASSERT_TRUE(components.Pointer(1)->id == 0);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 0);
  ASSERT_TRUE(components.Pointer(4)->id == 1);
  ASSERT_TRUE(components.Pointer(5)->id == 1);
  ASSERT_TRUE(components.Pointer(6)->id == 1);
  ASSERT_TRUE(components.Pointer(7)->id == 2);
  ASSERT_TRUE(components.Pointer(8)->id == 2);
  ASSERT_TRUE(components.Pointer(9)->id == 0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, InvalidateSmallOrLargeComponents)

GTEST_TEST(CoreTech_Vision, CompressComponentIds)
{
  const s32 numComponents = 10;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(0, 0, 0, 5);  // 3
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(0, 0, 0, 10); // 4
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(0, 0, 0, 0);  // 0
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(0, 0, 0, 101);// 6
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, 0, 0, 4);  // 2
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, 0, 0, 11); // 5
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, 0, 0, 3);  // 1
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 0, 0, 5);  // 3

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  {
    const Result result = components.CompressConnectedComponentSegmentIds(ms);
    ASSERT_TRUE(result == RESULT_OK);

    const s32 maximumId = components.get_maximumId();
    ASSERT_TRUE(maximumId == 6);
  }

  ASSERT_TRUE(components.Pointer(0)->id == 3);
  ASSERT_TRUE(components.Pointer(1)->id == 4);
  ASSERT_TRUE(components.Pointer(2)->id == 0);
  ASSERT_TRUE(components.Pointer(3)->id == 6);
  ASSERT_TRUE(components.Pointer(4)->id == 1);
  ASSERT_TRUE(components.Pointer(5)->id == 2);
  ASSERT_TRUE(components.Pointer(6)->id == 5);
  ASSERT_TRUE(components.Pointer(7)->id == 1);
  ASSERT_TRUE(components.Pointer(8)->id == 1);
  ASSERT_TRUE(components.Pointer(9)->id == 3);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, CompressComponentIds)

// Not really a test, but computes the size of a list of ComponentSegments, to ensure that c++ isn't adding junk
GTEST_TEST(CoreTech_Vision, ComponentsSize)
{
  const s32 numComponents = 500;
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 10000);

  MemoryStack ms(&offchipBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  const s32 usedBytes0 = ms.get_usedBytes();

#ifdef PRINTF_SIZE_RESULTS
  printf("Original size: %d\n", usedBytes0);
#endif

  FixedLengthList<ConnectedComponentSegment> segmentList(numComponents, ms);

  const s32 usedBytes1 = ms.get_usedBytes();
  const double actualSizePlusOverhead = double(usedBytes1 - usedBytes0) / double(numComponents);

#ifdef PRINTF_SIZE_RESULTS
  printf("Final size: %d\n"
    "Difference: %d\n"
    "Expected size of a components: %d\n"
    "Actual size (includes overhead): %d\n",
    usedBytes1,
    usedBytes1 - usedBytes0,
    sizeof(ConnectedComponentSegment),
    actualSizePlusOverhead);
#endif // #ifdef PRINTF_SIZE_RESULTS

  const double difference = actualSizePlusOverhead - double(sizeof(ConnectedComponentSegment));
  ASSERT_TRUE(difference > -0.0001 && difference < 1.0);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ComponentsSize)

GTEST_TEST(CoreTech_Vision, SortComponents)
{
  const s32 numComponents = 10;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(50, 100, 50, u16_MAX); // 2
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(s16_MAX, s16_MAX, s16_MAX, 0); // 9
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(s16_MAX, s16_MAX, 0, 0); // 7
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(s16_MAX, s16_MAX, s16_MAX, u16_MAX); // 4
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(0, s16_MAX, 0, 0); // 5
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(0, s16_MAX, s16_MAX, 0); // 8
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(0, s16_MAX, s16_MAX, u16_MAX); // 3
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(s16_MAX, s16_MAX, 0, u16_MAX); // 1
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(0, s16_MAX, 0, 0); // 6
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(42, 42, 42, 42); // 0

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  const Result result = components.SortConnectedComponentSegments();
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(*components.Pointer(0) == component9);
  ASSERT_TRUE(*components.Pointer(1) == component7);
  ASSERT_TRUE(*components.Pointer(2) == component0);
  ASSERT_TRUE(*components.Pointer(3) == component6);
  ASSERT_TRUE(*components.Pointer(4) == component3);
  ASSERT_TRUE(*components.Pointer(5) == component4);
  ASSERT_TRUE(*components.Pointer(6) == component8);
  ASSERT_TRUE(*components.Pointer(7) == component2);
  ASSERT_TRUE(*components.Pointer(8) == component5);
  ASSERT_TRUE(*components.Pointer(9) == component1);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, SortComponentsById)
{
  const s32 numComponents = 10;

  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

  ConnectedComponents components(numComponents, 640, ms);

  const ConnectedComponentSegment component0 = ConnectedComponentSegment(1, 1, 1, 3); // 6
  const ConnectedComponentSegment component1 = ConnectedComponentSegment(2, 2, 2, 1); // 0
  const ConnectedComponentSegment component2 = ConnectedComponentSegment(3, 3, 3, 1); // 1
  const ConnectedComponentSegment component3 = ConnectedComponentSegment(4, 4, 4, 0); // X
  const ConnectedComponentSegment component4 = ConnectedComponentSegment(5, 5, 5, 1); // 2
  const ConnectedComponentSegment component5 = ConnectedComponentSegment(6, 6, 6, 1); // 3
  const ConnectedComponentSegment component6 = ConnectedComponentSegment(7, 7, 7, 1); // 4
  const ConnectedComponentSegment component7 = ConnectedComponentSegment(8, 8, 8, 4); // 7
  const ConnectedComponentSegment component8 = ConnectedComponentSegment(9, 9, 9, 5); // 8
  const ConnectedComponentSegment component9 = ConnectedComponentSegment(0, 0, 0, 1); // 5

  components.PushBack(component0);
  components.PushBack(component1);
  components.PushBack(component2);
  components.PushBack(component3);
  components.PushBack(component4);
  components.PushBack(component5);
  components.PushBack(component6);
  components.PushBack(component7);
  components.PushBack(component8);
  components.PushBack(component9);

  const Result result = components.SortConnectedComponentSegmentsById(ms);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 9);

  ASSERT_TRUE(*components.Pointer(0) == component1);
  ASSERT_TRUE(*components.Pointer(1) == component2);
  ASSERT_TRUE(*components.Pointer(2) == component4);
  ASSERT_TRUE(*components.Pointer(3) == component5);
  ASSERT_TRUE(*components.Pointer(4) == component6);
  ASSERT_TRUE(*components.Pointer(5) == component9);
  ASSERT_TRUE(*components.Pointer(6) == component0);
  ASSERT_TRUE(*components.Pointer(7) == component7);
  ASSERT_TRUE(*components.Pointer(8) == component8);

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, SortComponents)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)
{
  const s32 imageWidth = 18;
  const s32 imageHeight = 5;

  const s32 minComponentWidth = 2;
  const s32 maxSkipDistance = 0;
  const s32 maxComponentSegments = 100;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&offchipBuffer[0], OFFCHIP_BUFFER_SIZE);
  ASSERT_TRUE(ms.IsValid());

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

  Array<u8> binaryImage(imageHeight, imageWidth, ms);
  ASSERT_TRUE(binaryImage.IsValid());

  //binaryImage.Print("binaryImage");

  ASSERT_TRUE(binaryImage.SetCast<s32>(binaryImageData, ApproximateConnectedComponents2d_binaryImageDataLength) == imageWidth*imageHeight);

  //binaryImage.Print("binaryImage");

  //FixedLengthList<ConnectedComponentSegment> extractedComponents(maxComponentSegments, ms);
  ConnectedComponents components(maxComponentSegments, imageWidth, ms);
  ASSERT_TRUE(components.IsValid());

  const Result result = components.Extract2dComponents_FullImage(binaryImage, minComponentWidth, maxSkipDistance, ms);
  ASSERT_TRUE(result == RESULT_OK);

  ASSERT_TRUE(components.SortConnectedComponentSegmentsById(ms) == RESULT_OK);

  ASSERT_TRUE(components.get_size() == 13);

  //components.Print();

  for(s32 i=0; i<numComponents_groundTruth; i++) {
    ASSERT_TRUE(components.Pointer(i)->xStart == xStart_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->xEnd == xEnd_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->y == y_groundTruth[i]);
    ASSERT_TRUE(components.Pointer(i)->id == id_groundTruth[i]);
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents2d)

GTEST_TEST(CoreTech_Vision, ApproximateConnectedComponents1d)
{
  const s32 imageWidth = 50;
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);

  const s32 minComponentWidth = 3;
  const s32 maxComponents = 10;
  const s32 maxSkipDistance = 1;

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&offchipBuffer[0], numBytes);
  ASSERT_TRUE(ms.IsValid());

  u8 * binaryImageRow = reinterpret_cast<u8*>(ms.Allocate(imageWidth));
  memset(binaryImageRow, 0, imageWidth);

  FixedLengthList<ConnectedComponentSegment> extractedComponentSegments(maxComponents, ms);

  for(s32 i=10; i<=15; i++) binaryImageRow[i] = 1;
  for(s32 i=25; i<=35; i++) binaryImageRow[i] = 1;
  for(s32 i=38; i<=38; i++) binaryImageRow[i] = 1;
  for(s32 i=43; i<=45; i++) binaryImageRow[i] = 1;
  for(s32 i=47; i<=49; i++) binaryImageRow[i] = 1;

  const Result result = ConnectedComponents::Extract1dComponents(binaryImageRow, imageWidth, minComponentWidth, maxSkipDistance, extractedComponentSegments);

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
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 5000);

  // Allocate memory from the heap, for the memory allocator
  MemoryStack ms(&offchipBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(imageHeight, imageWidth, ms);
  image.SetZero();

  Array<u8> imageFiltered(imageHeight, imageWidth, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageFiltered.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  const Result result = ImageProcessing::BinomialFilter<u8,u32,u8>(image, imageFiltered, ms);

  //printf("image:\n");
  //image.Print();

  //printf("imageFiltered:\n");
  //imageFiltered.Print();

  ASSERT_TRUE(result == RESULT_OK);

  const s32 correctResults[16][16] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  for(s32 y=0; y<imageHeight; y++) {
    for(s32 x=0; x<imageWidth; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageFiltered.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
} // GTEST_TEST(CoreTech_Vision, BinomialFilter)

GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 imageWidth = 10;
  const s32 imageHeight = 4;

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 1000);

  MemoryStack ms(&offchipBuffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Array<u8> image(imageHeight, imageWidth, ms);
  Array<u8> imageDownsampled(imageHeight/2, imageWidth/2, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageDownsampled.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<imageWidth; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = ImageProcessing::DownsampleByTwo<u8,u32,u8>(image, imageDownsampled);

  ASSERT_TRUE(result == RESULT_OK);

  const s32 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imageDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imageDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
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
  ASSERT_TRUE(P3P::solveQuartic(factors, roots_computed) == EXIT_SUCCESS);

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

  // Allocate memory from the heap, for the memory allocator
  // TODO: How much memory do i need here?
  const s32 numBytes = MIN(OFFCHIP_BUFFER_SIZE, 250*sizeof(PRECISION));

  // TODO: is onchipbBuffer the right one to use here?
  MemoryStack memory(&offchipBuffer[0], numBytes);

  // Parameters
  Array<PRECISION> Rtrue = Array<PRECISION>(3,3,memory);
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
  Array<PRECISION> R = Array<PRECISION>(3,3,memory);
  Point3<PRECISION> T;

  BeginBenchmark("P3P::computePose");

  ASSERT_TRUE(P3P::computePose(proj,
    marker3d[0], marker3d[1], marker3d[2], marker3d[3],
    focalLength_x, focalLength_y,
    camCenter_x, camCenter_y,
    R, T, memory) == EXIT_SUCCESS);

  EndBenchmark("P3P::computePose");

  PrintBenchmarkResults_All();

  //
  // Check if the estimated pose matches the true pose
  //

  // 1. Compute angular difference between the two rotation matrices
  // TODO: make this a utility function somewhere?
  // R = R_this * R_other^T
  Array<PRECISION> Rdiff = Array<PRECISION>(3,3,memory);
  Point3<PRECISION> Tdiff;
  ComputePoseDiff(R, T, Rtrue, Ttrue, Rdiff, Tdiff, memory);

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

#if !ANKICORETECH_EMBEDDED_USE_GTEST
s32 RUN_ALL_VISION_TESTS(s32 &numPassedTests, s32 &numFailedTests)
{
  numPassedTests = 0;
  numFailedTests = 0;

  //CALL_GTEST_TEST(CoreTech_Vision, ResizeImage);
  //CALL_GTEST_TEST(CoreTech_Vision, DecisionTreeVision);
  CALL_GTEST_TEST(CoreTech_Vision, BinaryTracker);
  /*CALL_GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_DerivativeThreshold);
  CALL_GTEST_TEST(CoreTech_Vision, DetectBlurredEdge_GrayvalueThreshold);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByPowerOfTwo);
  //CALL_GTEST_TEST(CoreTech_Vision, ComputeDockingErrorSignalAffine);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledProjective);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_SampledPlanar6dof);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Projective);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Affine);
  CALL_GTEST_TEST(CoreTech_Vision, LucasKanadeTracker_Slow);
  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageFiltering);
  CALL_GTEST_TEST(CoreTech_Vision, ScrollingIntegralImageGeneration);
  CALL_GTEST_TEST(CoreTech_Vision, DetectFiducialMarkers);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeQuadrilateralsFromConnectedComponents);
  CALL_GTEST_TEST(CoreTech_Vision, Correlate1dCircularAndSameSizeOutput);
  CALL_GTEST_TEST(CoreTech_Vision, LaplacianPeaks);
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
  CALL_GTEST_TEST(CoreTech_Vision, P3P_PerspectivePoseEstimation);*/

  return numFailedTests;
} // int RUN_ALL_VISION_TESTS()
#endif // #if !ANKICORETECH_EMBEDDED_USE_GTEST
