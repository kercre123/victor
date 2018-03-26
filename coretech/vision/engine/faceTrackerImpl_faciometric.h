/**
 * File: faceTrackerImpl_faciometric.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for FacioMetric face detector / tracker and emotion estimator.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"

#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/rotation.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#define ESTIMATE_EMOTION 0
#define ESTIMATE_GAZE    0
#define DO_RECOGNITION   0

#include <intraface/core/core.h>
#include <intraface/core/LocalManager.h>

#if DO_RECOGNITION
#  include <intraface/facerecog/SubjectSerializer.h>
#  include <intraface/facerecog/FaceRecog.h>
#  include <intraface/facerecog/SubjectRecogData.h>
#  include <intraface/facerecog/Macros.h>
#  include <fstream>
#  include <dirent.h>
#endif

#if ESTIMATE_GAZE
#  include <intraface/gaze/gaze.h>
#endif

#if ESTIMATE_EMOTION
#  include <intraface/emo/EmoDet.h>
#endif

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <list>

namespace Anki {
namespace Vision {
  
  class FaceTracker::Impl
  {
  public:
    
    Impl(const Camera&        camera,
         const std::string&   modelPath,
         const Json::Value&   config);
    ~Impl();
    
    Result Update(const Vision::Image& frame);
    
    std::list<TrackedFace> GetFaces() const;
    
  private:
    // License manager:
    static facio::LocalManager _lm;
    
    // OpenCV face detector:
    cv::CascadeClassifier _faceCascade;
    
    using SDM = facio::FaceAlignmentSDM<facio::AlignmentFeature, facio::NO_CONTOUR, facio::LocalManager>;
    using HPE = facio::HPEstimatorProcrustes<facio::LocalManager>;
    
    static SDM* _sdm;
    HPE* _hpe;
    
#   if ESTIMATE_GAZE
    using IE  = facio::IrisEstimator<facio::IrisFeature, facio::LocalManager>;
    using GE  = facio::DMGazeEstimation<facio::LocalManager>;
    IE*  _ie;
    GE*  _ge;
#   endif
    
#   if ESTIMATE_EMOTION
    using ED  = facio::EmoDet<facio::LocalManager>;
    ED*  _ed;
    facio::emoScores _Emo; // Emotions to show
#   endif
    
#   if DO_RECOGNITION
    facio::FaceRecog* _fr;
    std::vector<facio::SubjectRecogData> _recognizableFaces;
    std::string _recognizedFacesPath;
#   endif
    
    //const std::string _modelPath;
    
    static int _faceCtr;
    
    // Store TrackedFaces paired with their landmarks in cv::Mat format so that
    // we can feed the previous landmarks to the FacioMetric tracker. This is
    // not memory efficient since we are also storing the features inside the
    // TrackedFace, but it prevents translated back and forth (and instead just
    // translates _from_ cv::Mat _to_ TrackedFace::Features each udpate.
    std::list<std::pair<TrackedFace,cv::Mat> > _faces;
    
  }; // class FaceTrackerImpl
  
  // Static initializations:
  int FaceTracker::Impl::_faceCtr = 0;
  FaceTracker::Impl::SDM* FaceTracker::Impl::_sdm = nullptr;
  facio::LocalManager FaceTracker::Impl::_lm;
  
  
# if DO_RECOGNITION
  //! This function gives a vector with all the files inside a given folder
  /*!
   \param path  Path of the folder you want to search the files.
   
   \return std::vector<std::string> Vector containing all the files inside the given folder
   */
  std::vector<std::string> getSubjectsPaths(std::string path)
  {
    
    DIR*    dir;
    dirent* pdir;
    std::vector<std::string> files;
    
    dir = opendir(path.c_str());
    
    
    if (dir == NULL) // if pent has not been initialised correctly
    {
      printf ("ERROR! Couldn't get Library");
      exit (0);
    }
    
    
    while ((pdir = readdir(dir))) {
      std::string s(pdir->d_name);
      if(s.substr(s.find_last_of(".") + 1) == "dat")
      {
        files.push_back(path + "/" + s);
      }
    }
    
    return files;
  } // getSubjectsPaths()
  
  
  //! This function loads all the subject files from the given folder
  /*!
   \param folder Path of the folder you want to search the files.
   
   \return std::vector<facio::SubjectRecogData> Vector containing the loaded subjects
   */
  std::vector<facio::SubjectRecogData> loadEnrolledSubjectsFromFolder(std::string folder)
  {
    
    std::vector<facio::SubjectRecogData> vectorSubjectRecogData;
    
    //Init SubjectSerializer
    std::unique_ptr<facio::SubjectSerializer> subjectSerializer(new facio::SubjectSerializer());
    
    //Get the path of the subjects
    std::vector<std::string> subjectsPaths = getSubjectsPaths(folder);
    
    for (int i=0; i< (int) subjectsPaths.size();i++)
    {
      //We load every subject
      std::ifstream infile (subjectsPaths[i],std::ifstream::binary);
      
      if(infile.is_open()){
        
        char* bufferSizeMetaData = new char[sizeof(size_t)];
        char* bufferSizeData = new char[sizeof(size_t)];
        
        //We get the MetaData of the subject
        infile.read(bufferSizeMetaData, (long)sizeof(size_t));
        size_t* sizeMetadata = (size_t *) bufferSizeMetaData;
        
        char *bufferMetaData = new char[*sizeMetadata];
        infile.read(bufferMetaData,(long)*sizeMetadata);
        
        //We get the Data of the subject
        infile.read(bufferSizeData, (long)sizeof(size_t));
        size_t* sizeData = (size_t *) bufferSizeData;
        
        char *bufferData = new char[*sizeData];
        infile.read(bufferData,(long)*sizeData);
        
        //Close file
        infile.close();
        
        facio::SubjectRecogData subjectData;
        
        //Now we deserialize the data (It will return 1 if everything is correct
        int successDeserialize = subjectSerializer->deserialize(bufferMetaData, bufferData, subjectData);
        if( successDeserialize == 0){
          //We add the current Deserialized subject to the vector
          vectorSubjectRecogData.push_back(subjectData);
          
        }else{
          PRINT_STREAM_INFO("FaceTracker.loadEnrolledSubjectsFromFolder",
                            "There was a problem deserializing the subject with error: " << successDeserialize);
        }
      }
    }
    
    if(!vectorSubjectRecogData.empty()) {
      PRINT_NAMED_INFO("FaceTracker.loadEnrolledSubjectsFromFolder.Success",
                       "Loaded %lu subjects", vectorSubjectRecogData.size());
    }
    
    return vectorSubjectRecogData;
  } // loadEnrolledSubjectsFromFolder()
  
  Result saveSubjectData(facio::SubjectRecogData& subjectDataToEnroll, std::string fileToSave)
  {
    //Variables to save the information
    char *subjectMetaData;
    size_t subjectMetaDataSize;
    char *subjectData;
    size_t subjectDataSize;

    //Init SubjectSerializer
    std::unique_ptr<facio::SubjectSerializer> subjectSerializer(new facio::SubjectSerializer());
    
    //We serialize the subject to save it to a file
    if(subjectSerializer->serialize(subjectDataToEnroll,subjectMetaData, subjectMetaDataSize, subjectData, subjectDataSize) == 1){
      
      //Now we save the serialized data to a file for later use
      std::ofstream outfile (fileToSave,std::ofstream::binary);
      
      //If we can open file
      if(outfile.is_open()){
        //Save size of metadata
        outfile.write((char*)&subjectMetaDataSize,(long)sizeof(size_t));
        //Save metadata
        outfile.write(subjectMetaData,subjectMetaDataSize);
        //Save size of data
        outfile.write((char*)&subjectDataSize,(long)sizeof(size_t));
        //Save data
        outfile.write(subjectData,subjectDataSize);
        
        //Close file
        outfile.close();
        
        //Enrollment done
        PRINT_NAMED_INFO("FaceTracker.saveSubjectData.Success",
                         "Successfully enrolled %s to the following path: %s",
                         subjectDataToEnroll.getLabel().c_str(), fileToSave.c_str());
      }
      else{
        PRINT_NAMED_ERROR("FaceTracker.saveSubjectData.Fail",
                          "Could not open file %s", fileToSave.c_str());
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  } // saveSubjectData()
  
  
  //! This function perfom the recognition of the subject
  /*!
   \param faceRecog  Recognizer object to create the subject and perform the recognition.
   \param imRDList  Vector containing the ImRawData necessary to create the subject that will be recognized.
   \param vectorSubjectRecogData  Vector containing all the subject to recognize with.
   */
  int doRecognition(facio::FaceRecog &faceRecog,
                     std::vector<facio::ImRawData> imRDList,
                     std::vector<facio::SubjectRecogData> vectorSubjectRecogData,
                     std::vector<float>& scores)
  {
    // Index of the subject recornized (if it is not recognized it will be set to -1)
    int subject = -1;
    
    bool status = false;
    
    //Perform recognition
    /*
    PRINT_NAMED_INFO("FaceTracker.doRecognition",
                     "Performing recognition with %lu images.",
                     imRDList.size());
     */
    
    //We prepare the subject that we will recognize
    facio::SubjectRecogData subjectDataToRecognize;
    std::string label("ToRecognize");
    
    bool subjectCreated = faceRecog.processSubjectRecogData(imRDList, label, subjectDataToRecognize);
    
    if(subjectCreated){
      
      //Perform recodnition

      status = faceRecog.predict(subjectDataToRecognize, vectorSubjectRecogData, subject, scores);
      
      // Debug:
      //saveSubjectData(subjectDataToRecognize, "/Users/andrew/temp/toRecognize.dat");
      //saveSubjectData(vectorSubjectRecogData[0], "/Users/andrew/temp/enrolledFace.dat");
      
      if(status)
      {/*
        if (subject != -1) {
          PRINT_NAMED_INFO("FaceTracker.doRecognition", "Recognized: %s with score %f",
                           vectorSubjectRecogData[subject].getLabel().c_str(),
                           scores[subject]);
        } else {
          PRINT_NAMED_INFO("FaceTracker.doRecognition", "Subject not found");
        }
        for (int i = 0; i < (int) scores.size(); i++) {
          PRINT_NAMED_INFO("FaceTracker.doRecognition", "Subject %s with score %f",
                           vectorSubjectRecogData[i].getLabel().c_str(), scores[i]);
        }
         */
      }
      else
      {
        PRINT_NAMED_ERROR("FaceTracker.doRecognition.InvalidInputs", "");
      }
      
    }else{
      PRINT_NAMED_ERROR("FaceTracker.doRecognition.CouldNotCreateSubject", "");
    }
    
    return subject;
  } // doRecognition()
  
  //! This function asks for a name and enrolls de subject to the given folder
  /*!
   \param faceRecog  Recognizer that will create the subject that we will enroll.
   \param imRDList  Vector containing the ImRawData necessary to create the subject that we will enroll.
   \param folder Path of the folder we save the enrolled objects.
   */
  bool enrollSubject(facio::FaceRecog &faceRecog,
                     std::string name,
                     std::vector<facio::ImRawData> imRDList,
                     std::string folder,
                     std::vector<facio::SubjectRecogData>& knownFaces)
  {
    facio::SubjectRecogData subjectDataToEnroll;
   
    /*
    //Get the name of the person
    std::cout << "Name: " << std::endl;
    std::cin >> name;
    */
    
    // Fill the subjectDataToEnroll with the information
    bool subjectCreated = faceRecog.processSubjectRecogData(imRDList, name, subjectDataToEnroll);
    
    if(subjectCreated){
      //Store in the given vector
      knownFaces.push_back(subjectDataToEnroll);

      PRINT_NAMED_INFO("FaceTracker.enrollSubject.SubjectAdded",
                       "Added subject with label %s and name %s.",
                       subjectDataToEnroll.getLabel().c_str(),
                       name.c_str());

      // Uncomment to save each subject as they are enrolled (for use on next run)
      //std::string fileToSave = folder+"/"+name+".dat";
      //saveSubjectData(subjectDataToEnroll, fileToSave);
      
    }
    else{
      PRINT_NAMED_ERROR("FaceTracker.enrollSubject.Fail",
                        "The subject could not be created for recognition.");
    }
    
    return subjectCreated;
  } // enrollSubject()
  
# endif // DO_RECOGNITION
  
  FaceTracker::Impl::Impl(const std::string& modelPath, FaceTracker::DetectionMode mode)
  //: _displayEnabled(false)
  {
    const std::string subPath = modelPath + "/faciometric/models/";
    
    const std::string cascadeFilename = subPath + "haarcascade_frontalface_alt2.xml";
    
    const bool loadSuccess = _faceCascade.load(cascadeFilename);
    if(!loadSuccess) {
      PRINT_NAMED_ERROR("VisionSystem.Update.LoadFaceCascade",
                        "Failed to load face cascade from %s",
                        cascadeFilename.c_str());
    }
    
    // Tracker object, we are using a unique pointer(http://en.cppreference.com/w/cpp/memory/unique_ptr)
    // Please notice that the SDM class does not have an explicit constructor.
    // To get an instance of the SDM class, please use the function SDM::getInstance
    if(_sdm == nullptr) { // Only initialize once to avoid LicenseManager complaining about multiple instances
      PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateSDM", "");
      _sdm = SDM::getInstance((subPath + "tracker_model49.bin").c_str(), &_lm);
    }
    
#   if ESTIMATE_GAZE
    // Create an instance of the class IrisEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateIE", "");
    _ie  = new IE((subPath + "iris_model.bin").c_str(), &_lm);
    
    // Create an instance of the class GazeEstimator
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateGE", "");
    _ge  = new GE((subPath + "gaze_model.bin").c_str(), &_lm);
#   endif
    
    // Create an instance of the class HPEstimatorProcrustes
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateHPE", "");
    _hpe = new HPE((subPath + "hp_model.bin").c_str(), &_lm);
    
#   if ESTIMATE_EMOTION
    // Create an instance of the class EmoDet
    PRINT_NAMED_INFO("FaceTrackerImpl.Constructor.InstantiateED", "");
    _ed  = new ED((subPath + "emo_model.bin").c_str(), &_lm);
#   endif
    
#   if DO_RECOGNITION
    
    // Load any faces we already know about
    _recognizedFacesPath = subPath + "subjects";
    _recognizableFaces = loadEnrolledSubjectsFromFolder(_recognizedFacesPath);
    
    // Initialize Face Recognition
    _fr = new facio::FaceRecog(subPath);
#   endif
    
  }
  
  
  
  FaceTracker::Impl::~Impl()
  {
    //if(_sdm) delete _sdm; // points to a singleton, so don't delete, call ~SDM();
    //_sdm->~SDM();

    Util::SafeDelete(_hpe);
    
    Util::SafeDelete(_hpe);
    
#   if ESTIMATE_GAZE
    Util::SafeDelete(_ie);
    Util::SafeDelete(_ge);
#   endif
    
#   if ESTIMATE_EMOTION
    Util::SafeDelete(_ed);
#   endif
    
#   if DO_RECOGNITION
    Util::SafeDelete(_fr);
#   endif
    
  }
  
  // For arbitrary indices
  static inline TrackedFace::Feature GetFeatureHelper(const float* x, const float* y,
                                                      std::vector<int>&& indices)
  {
    TrackedFace::Feature pointVec;
    pointVec.reserve(indices.size());
    
    for(auto index : indices) {
      pointVec.emplace_back(x[index],y[index]);
    }
    
    return pointVec;
  }
  
  // For indices in order
  static inline TrackedFace::Feature GetFeatureHelper(const float* x, const float* y,
                                                      const int fromIndex, const int toIndex,
                                                      bool closed = false)
  {
    TrackedFace::Feature pointVec;
    
    pointVec.reserve(toIndex-fromIndex+1 + (closed ? 1 : 0));
    
    for(int i=fromIndex; i<=toIndex; ++i) {
      pointVec.emplace_back(x[i],y[i]);
    }
    
    if(closed) {
      pointVec.emplace_back(x[fromIndex], y[fromIndex]);
    }
    
    return pointVec;
  }
  
  static inline Point2f GetFeatureAverageHelper(const float* x, const float* y,
                                                const int fromIndex, const int toIndex)
  {
    Point2f avg(0.f,0.f);
    
    for(int i=fromIndex; i<=toIndex; ++i) {
      avg.x() += x[i];
      avg.y() += y[i];
    }
    
    avg /= static_cast<float>(toIndex-fromIndex+1);
    
    return avg;
  }
                                                
  static inline void UpdateFaceFeatures(const cv::Mat& landmarks,
                                        TrackedFace& face)
  {
    // X coordinates on the first row of the cv::Mat, Y on the second
    const float *x = landmarks.ptr<float>(0);
    const float *y = landmarks.ptr<float>(1);
    
    const bool drawFaceContour = landmarks.cols > 49;
    
    face.SetFeature(TrackedFace::LeftEyebrow,  GetFeatureHelper(x, y, 0,  4));
    face.SetFeature(TrackedFace::RightEyebrow, GetFeatureHelper(x, y, 5,  9));
    face.SetFeature(TrackedFace::NoseBridge,   GetFeatureHelper(x, y, 10, 13));
    face.SetFeature(TrackedFace::Nose,         GetFeatureHelper(x, y, 14, 18));
    face.SetFeature(TrackedFace::LeftEye,      GetFeatureHelper(x, y, 19, 24, true));
    face.SetFeature(TrackedFace::RightEye,     GetFeatureHelper(x, y, 25, 30, true));
    
#   if !ESTIMATE_GAZE
    face.SetLeftEyeCenter(GetFeatureAverageHelper(x, y, 19, 24));
    face.SetRightEyeCenter(GetFeatureAverageHelper(x, y, 25, 30));
#   endif
    
    face.SetFeature(TrackedFace::UpperLip, GetFeatureHelper(x, y, {
      31,32,33,34,35,36,37,45,44,43,31}));
    
    face.SetFeature(TrackedFace::LowerLip, GetFeatureHelper(x, y, {
      31,48,47,46,37,38,39,40,41,42,31}));
    
    // contour
    if (drawFaceContour) {
      face.SetFeature(TrackedFace::Contour, GetFeatureHelper(x, y, 49, 65));
    }
  }
  
  
  Result FaceTracker::Impl::Update(const Vision::Image& frameOrig)
  {
    const float minScoreThreshold = 0.1f;
    
    cv::Mat frame;
    cv::Mat landmarksNext;
    float score=0.f;
    frameOrig.get_CvMat_().copyTo(frame);
    
    // Update existing faces
    for(auto faceIter = _faces.begin(); faceIter != _faces.end(); /* increment in loop */)
    {
      TrackedFace& face  = faceIter->first;
      cv::Mat& landmarks = faceIter->second;
      
      // From the landmarks in the previous frame, we predict the landmark in the current iteration
      _sdm->track(frame, landmarks, landmarksNext, score);
      
      if(score < minScoreThreshold) {
        // Delete this face (and update iterator accordingly)
        faceIter = _faces.erase(faceIter);
      } else {
        // We copy the current estimation ('landmarksNext'), to the previous
        // frame ('landmarks') for the next iteration
        landmarksNext.copyTo(landmarks);
        
        // Update the rectangle based on the landmarks, since we aren't using
        // the opencv detector for faces we're already tracking:
        double xmin, xmax, ymin, ymax;
        cv::minMaxLoc(landmarks.row(0), &xmin, &xmax);
        cv::minMaxLoc(landmarks.row(1), &ymin, &ymax);
        face.SetRect(Rectangle<f32>(xmin, ymin, xmax-xmin, ymax-ymin));
        
        // Black out face, so we don't re-detect them as new faces
        // (Make sure the rectangle doesn't extend outside the image)
        cv::Rect_<float> roi = (face.GetRect().get_CvRect_() &
                                cv::Rect_<float>(0,0,frame.cols-1, frame.rows-1));
        frame(roi).setTo(0);
        
        face.SetScore(score);
        face.SetIsBeingTracked(true);
        
        // Move to next known face
        ++faceIter;
      }
    }

    // Detecting new faces in the image
    // TODO: Expose these parameters
    std::vector<cv::Rect> newFaceRects;
    _faceCascade.detectMultiScale(frame, newFaceRects, opencvDetectScaleFactor,
                                  2, 0, opencvDetectMinFaceSize);
    
    // TODO: See if we recognize this face
    
    for(auto & newFaceRect : newFaceRects) {
      // Initializing the tracker, detecting landmarks from the output of the face detector
      cv::Mat landmarks;
      float score=0.f;
      _sdm->detect(frame, newFaceRect, landmarks, score);
      
      if(score > minScoreThreshold)
      {
        TrackedFace newFace;
        
        newFace.SetRect(Rectangle<f32>(newFaceRect));
        
#       if !DO_RECOGNITION
        newFace.SetID(_faceCtr++);
#       endif
        
        _faces.emplace_back(newFace, landmarks);
      }
    }
    
    for(auto & facePair : _faces)
    {
      TrackedFace& face = facePair.first;
      cv::Mat& landmarks = facePair.second;
      
      // Update the timestamp for all new/updated faces
      face.SetTimeStamp(frameOrig.GetTimestamp());
      
      // Update the TrackedFace::Features from the FacioMetric landmarks:
      UpdateFaceFeatures(landmarks, face);
      
#     if ESTIMATE_EMOTION
      // Computing the Emotion information & Predicting the emotion information
      _ed->predict(frame, landmarks, &_Emo);
#     endif
      
      // Estimating the headpose
      facio::HeadPose headPose;
      _hpe->estimateHP(landmarks, headPose);
      
      // Construct a rotation matrix from the cv::Mat from faciometric. First
      // constructing a SmallSquareMatrix and then constructing an Rmat forces
      // a renormalization.
      SmallSquareMatrix<3,f32> Rtemp;
      Rtemp.get_CvMatx_() = headPose.rot;
      RotationMatrix3d Rmat(Rtemp);
      
      // Set the observed head orientation
      face.SetHeadOrientation(Rmat.GetAngleAroundZaxis(), Rmat.GetAngleAroundXaxis(), Rmat.GetAngleAroundYaxis());
      
      // Set the initial head pose with the same orientation. Later, we will hook
      // this up to the robot's (historical) camera and then get w.r.t. origin.
      // We store the head orientation w.r.t. camera above separately so that it
      // is preserved.
      Rmat *= RotationMatrix3d(-M_PI_2, X_AXIS_3D());
      Pose3d pose(Rmat, {});
      face.SetHeadPose(pose);
      // TODO: Hook up pose parent to camera?
     
#     if ESTIMATE_GAZE
      // Estimating the irises
      facio::Irisesf irises;
      _ie->detect(frame, landmarks, irises);
      face.SetLeftEyeCenter(irises.left.center);
      face.SetRightEyeCenter(irises.right.center);
      
      // Estimating the gaze, the gaze information is stored in the struct 'egs'
      facio::EyeGazes gazes;
      _ge->compute(landmarks, irises, headPose, gazes);
#     endif
      
#     if DO_RECOGNITION
      // Save ImRawData to create the subject in the future for several frames
      // and store it to the vector
      facio::ImRawData ird;
      ird.im = frame.clone();
      ird.lmks = landmarks.clone();
      ird.score = score;
      ird.hpData.rot = headPose.rot.clone();
      ird.hpData.angles = headPose.angles;
      ird.hpData.xyz = headPose.xyz;
      std::vector<facio::ImRawData> currentRecogData{ird};

      // Vector of the scores resulting from the recognition
      std::vector<float> scores;
      
      int subject = -1;
      
      if(!_recognizableFaces.empty()) {
        // Index of the subject recornized (if it is not recognized it will be set to -1)
        subject = doRecognition(*_fr, currentRecogData, _recognizableFaces, scores);
      }
      
      const float scoreThreshold = 0.9f;
      if(subject != -1 && scores[subject] > scoreThreshold) {
        face.SetID(subject);
        face.SetName(_recognizableFaces[subject].getLabel());
      } else {
        std::string newName("Person" + std::to_string(_faceCtr++));
        if(false == enrollSubject(*_fr, newName,
                                  currentRecogData, _recognizedFacesPath,
                                  _recognizableFaces))
        {
          PRINT_NAMED_ERROR("FaceTracker.Impl.Update",
                            "Failed to enroll new subject.");
          return RESULT_FAIL;
        }
        face.SetID(_recognizableFaces.size()-1);
        face.SetName(_recognizableFaces.back().getLabel());
      }
#     endif // DO_RECOGNITION
      
    } // for each face
    
    return RESULT_OK;
  } // Update()
 
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    std::list<TrackedFace> retVector;
    //retVector.reserve(_faces.size());
    for(auto & face : _faces) {
      retVector.emplace_back(face.first);
    }
    return retVector;
  }

} // namespace Vision
} // namespace Anki
