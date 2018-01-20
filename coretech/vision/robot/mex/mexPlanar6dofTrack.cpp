#include "mex.h"

#include <string.h>

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/fixedLengthList.h"

#include "coretech/vision/robot/lucasKanade.h"
#include "coretech/vision/robot/imageProcessing_declarations.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

using namespace Anki::Embedded;

// Memory
MemoryStack onchipMemory, offchipMemory, ccmMemory;
static const s32 OFFCHIP_BUFFER_SIZE = 2000000;
static const s32 ONCHIP_BUFFER_SIZE = 170000; // The max here is somewhere between 175000 and 180000 bytes
static const s32 CCM_BUFFER_SIZE = 50000; // The max here is probably 65536 (0x10000) bytes

static char offchipBuffer[OFFCHIP_BUFFER_SIZE];
static char onchipBuffer[ONCHIP_BUFFER_SIZE];
static char ccmBuffer[CCM_BUFFER_SIZE];

// Image
Array<u8> grayscaleImage;

// Tracker
TemplateTracker::LucasKanadeTracker_SampledPlanar6dof tracker;

void NormalizeImage(Array<u8>& grayscaleImage, const Quadrilateral<f32>& currentQuad, const f32 filterWidthFraction, MemoryStack scratch)
{
  // TODO: Add the ability to only normalize within the vicinity of the quad
  // Note that this requires templateQuad to be sorted!
  const s32 filterWidth = static_cast<s32>(filterWidthFraction*((currentQuad[3] - currentQuad[0]).Length()));
  AnkiAssert(filterWidth > 0.f);

  Array<u8> grayscaleImageNormalized(grayscaleImage.get_size(0), grayscaleImage.get_size(1), scratch);

  mxAssert(grayscaleImageNormalized.IsValid(),
    "Out of memory allocating grayscaleImageNormalized.\n");

  Anki::Result lastResult = ImageProcessing::BoxFilterNormalize(grayscaleImage, filterWidth, static_cast<u8>(128),
    grayscaleImageNormalized, scratch);

  mxAssert(lastResult == Anki::RESULT_OK, "BoxFilterNormalize failed.");

  {
    grayscaleImage.Show("grayscaleImage", false);
    grayscaleImageNormalized.Show("grayscaleImageNormalized", false);
  }

  grayscaleImage.Set(grayscaleImageNormalized);
} // NormalizeImage()

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  mexAtExit(cv::destroyAllWindows);

  // TODO: Make this a passed-in parameter
  const f32 filterWidthFraction = 0.5f;

  if(nrhs == 14 && (nlhs ==0 || nlhs == 1 || nlhs == 7)) {
    //
    // Tracker Init
    //

    // Reset memory
    offchipMemory = MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
    onchipMemory  = MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
    ccmMemory     = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);

    mwSize argIndex = 0;

    // Get Initial Image
    mxAssert(mxGetClassID(prhs[0]) == mxUINT8_CLASS,
      "Image should be UINT8.");
    mxAssert(mxGetNumberOfDimensions(prhs[0]) == 2,
      "Image must be grayscale.");
    
    const mwSize nrows = mxGetM(prhs[argIndex]);
    const mwSize ncols = mxGetN(prhs[argIndex]);
    
    mxAssert(nrows < std::numeric_limits<s32>::max() &&
             ncols < std::numeric_limits<s32>::max(),
             "Image too large for conversion to Array.");
    
    grayscaleImage = Array<u8>(static_cast<s32>(nrows), static_cast<s32>(ncols), onchipMemory);
    mxArrayToArray(prhs[argIndex], grayscaleImage);
    ++argIndex;

    // Get Initial Quad
    mxAssert(mxGetM(prhs[argIndex])==4 && mxGetN(prhs[argIndex])==2 &&
      mxGetClassID(prhs[argIndex]) == mxDOUBLE_CLASS,
      "Initial quad must be 4x2 DOUBLE matrix.");
    const double* quadX = mxGetPr(prhs[argIndex]);
    const double* quadY = quadX + 4;
    Quadrilateral<f32> templateQuad(Point2f(quadX[0], quadY[0]),
      Point2f(quadX[1], quadY[1]),
      Point2f(quadX[2], quadY[2]),
      Point2f(quadX[3], quadY[3]));
    ++argIndex;

    // Get Calibration Data
    f32 focalLength_x = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    f32 focalLength_y = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    f32 camCenter_x   = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    f32 camCenter_y   = static_cast<f32>(mxGetScalar(prhs[argIndex++]));

    f32 templateWidth_mm = static_cast<f32>(mxGetScalar(prhs[argIndex++]));

    // Get Parameters
    const f32 scaleTemplateRegionPercent  = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const s32 numPyramidLevels            = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
    const s32 maxSamplesAtBaseLevel       = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
    const s32 numSamplingRegions          = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
    const s32 numFiducialSquareSamples    = static_cast<s32>(mxGetScalar(prhs[argIndex++]));
    const f32 fiducialSquareWidthFraction = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const bool normalizeImage             = static_cast<bool>(mxGetScalar(prhs[argIndex++]));

    if(normalizeImage) {
      NormalizeImage(grayscaleImage, templateQuad, filterWidthFraction, offchipMemory);
    }

    tracker = TemplateTracker::LucasKanadeTracker_SampledPlanar6dof(
      grayscaleImage,
      templateQuad,
      scaleTemplateRegionPercent,
      numPyramidLevels,
      Transformations::TRANSFORM_PROJECTIVE,
      numFiducialSquareSamples,
      fiducialSquareWidthFraction,
      maxSamplesAtBaseLevel,
      numSamplingRegions,
      focalLength_x,
      focalLength_y,
      camCenter_x,
      camCenter_y,
      templateWidth_mm,
      ccmMemory,
      onchipMemory,
      offchipMemory);

    if(!tracker.IsValid()) {
      mexErrMsgTxt("Failed to instantiate valid tracker!");
    }

    /*
    // TODO: Set this elsewhere
    const f32 Kp_min = 0.1f;
    const f32 Kp_max = 0.75f;
    const f32 tz_min = 30.f;
    const f32 tz_max = 150.f;
    tracker.SetGainScheduling(tz_min, tz_max, Kp_min, Kp_max);
    */

    if(nlhs > 0) {
      using namespace TemplateTracker;
      const FixedLengthList<LucasKanadeTracker_SampledPlanar6dof::TemplateSample>& samples = tracker.get_templateSamples(0);
      const s32 numSamples = samples.get_size();

      plhs[0] = mxCreateDoubleMatrix(numSamples, 2, mxREAL);
      double* xSample = mxGetPr(plhs[0]);
      double* ySample = xSample + numSamples;

      for(s32 i=0; i<numSamples; ++i) {
        xSample[i] = samples[i].xCoordinate;
        ySample[i] = samples[i].yCoordinate;
      }

      if(nlhs >= 7) {
        plhs[1] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleX()));
        plhs[2] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleY()));
        plhs[3] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleZ()));

        plhs[4] = mxCreateDoubleScalar(static_cast<double>(tracker.GetTranslation().x));
        plhs[5] = mxCreateDoubleScalar(static_cast<double>(tracker.GetTranslation().y));
        plhs[6] = mxCreateDoubleScalar(static_cast<double>(tracker.GetTranslation().z));
      }
    }

    //mexPrintf("Tracker initialized.\n");
  }
  else if(nrhs == 6 && nlhs == 12) {
    //
    // Track
    //
    s32 argIndex = 0;

    mxAssert(mxGetClassID(prhs[0]) == mxUINT8_CLASS,
      "Image should be UINT8.");
    mxAssert(mxGetNumberOfDimensions(prhs[0]) == 2,
      "Image must be grayscale.");
    mxArrayToArray(prhs[argIndex], grayscaleImage);
    ++argIndex;

    const f32 maxIterations        = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const f32 convergenceTol_angle = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const f32 convergenceTol_dist  = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const f32 verify_maxPixelDiff  = static_cast<f32>(mxGetScalar(prhs[argIndex++]));
    const bool normalizeImage      = static_cast<bool>(mxGetScalar(prhs[argIndex++]));

    // Outputs
    bool converged = false;
    s32 verify_meanAbsDiff, verify_numInBounds, verify_numSimilarPixels;

    Quadrilateral<f32> currentQuad = tracker.get_transformation().get_transformedCorners(onchipMemory);
  
    if(normalizeImage) {
      NormalizeImage(grayscaleImage, currentQuad, filterWidthFraction, offchipMemory);
    }

    // Update tracker
    const Anki::Result trackerResult = tracker.UpdateTrack(grayscaleImage,
      maxIterations,
      convergenceTol_angle,
      convergenceTol_dist,
      verify_maxPixelDiff,
      converged,
      verify_meanAbsDiff,
      verify_numInBounds,
      verify_numSimilarPixels,
      onchipMemory);

    if(trackerResult != Anki::RESULT_OK) {
      char buffer[128];
      snprintf(buffer, 128, "UpdateTrack() failed with result = %x!", trackerResult);
      mexWarnMsgTxt(buffer);
    }

    // Return converged and verification info
    s32 argOutIndex = 0;
    plhs[argOutIndex++] = mxCreateLogicalScalar(converged);

    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(verify_meanAbsDiff));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(verify_numInBounds));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(verify_numSimilarPixels));

    // Return current tracked corners
    plhs[argOutIndex] = mxCreateDoubleMatrix(4,2,mxREAL);
    double* xQuad = mxGetPr(plhs[argOutIndex]);
    double* yQuad = xQuad + 4;
    Quadrilateral<f32> corners = tracker.get_transformation().get_transformedCorners(offchipMemory);
    for(s32 i=0; i<4; ++i) {
      xQuad[i] = corners[i].x;
      yQuad[i] = corners[i].y;
    }
    ++argOutIndex;

    // Return the three rotation angles
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleX()));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleY()));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(tracker.get_angleZ()));

    // Return the three translation values
    const Point3<f32>& translation = tracker.GetTranslation();
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(translation.x));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(translation.y));
    plhs[argOutIndex++] = mxCreateDoubleScalar(static_cast<double>(translation.z));

    // Return the current homography
    const Array<f32>& H = tracker.get_transformation().get_homography();
    plhs[argOutIndex++] = arrayToMxArray(H);
  }
  else {
    mexPrintf(
      "\nInitialize with: \n\n"
      "\t [samples,\n"
      "\t  angleX,\n"
      "\t  angleY,\n"
      "\t  angleZ,\n"
      "\t  tX,\n"
      "\t  tY,\n"
      "\t  tZ] = mexPlanar6dofTrack(\n"
      "\t\t templateImage,\n"
      "\t\t templateQuad, %% (must be 4x2)\n"
      "\t\t focalLength_x,\n"
      "\t\t focalLength_y,\n"
      "\t\t camCenter_x,\n"
      "\t\t camCenter_y,\n"
      "\t\t templateWidth_mm, %% (i.e. trackingMarkerWidth_mm)\n"
      "\t\t scaleTemplateRegionPercent, \n"
      "\t\t numPyramidLevels,\n"
      "\t\t maxSamplesAtBaseLevel,\n"
      "\t\t numSamplingRegions,\n"
      "\t\t numFiducialSquareSamples,\n"
      "\t\t fiducialSquareWidthFraction,\n"
      "\t\t normalizeImage);\n"
      "\n"
      "Track with: \n"
      "\t [verify_converged,\n"
      "\t  verify_meanAbsoluteDifference,\n"
      "\t  verify_numInBounds,\n"
      "\t  verify_numSimilarPixels,\n"
      "\t  currentQuad,\n"
      "\t  angleX,\n"
      "\t  angleY,\n"
      "\t  angleZ,\n"
      "\t  tX,\n"
      "\t  tY,\n"
      "\t  tZ,\n"
      "\t  tform] = mexPlanar6dofTrack(\n"
      "\t\t nextImage,\n"
      "\t\t maxIterations,\n"
      "\t\t convergenceTolerance_angle,\n"
      "\t\t convergenceTolerance_distance,\n"
      "\t\t verify_maxPixelDifference,\n"
      "\t\t normalizeImage);\n\n");

    //mexErrMsgTxt("Unrecognized inputs.");
  }
} // mexFunction
