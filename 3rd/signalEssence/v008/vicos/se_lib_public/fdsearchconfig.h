#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2011 Signal Essence; All Rights Reserved
  
  Module Name  - fdsearchconfig.h
  
  Author: Hugh McLaughlin
  
  History
  
  2011-11-28    hjm  created
  
  Description:
  Frequency domain search configuration
**************************************************************************
*/

#ifndef __fdsearchconfig_h
#define __fdsearchconfig_h

#include "mmglobalsizes.h"
#include "mmglobalcodes.h"
#include "se_types.h"

#define MIC_TYPE_UNKNOWN       0
#define MIC_TYPE_OMNI          1
#define MIC_TYPE_CARDIOID      2
#define MIC_TYPE_SUPERCARDIOID 3
#define MIC_TYPE_HYPERCARDIOID 4
#define MIC_TYPE_BIDIR         5

// Beam target coordinates in cm
typedef struct
{
    int16 x, y, z;
} BeamCoordinate_t;

// microphone coordinates in mm
typedef struct MicrophoneLocation_mm {
    int16 x, y, z;
    int16 lookAzimuth, lookElevation;  // Look azimuth and elevation in degrees
    int16 micType;
} MicrophoneLocation_mm;

typedef struct
{
    uint16              NumContributors;
    uint16              MicIndex[MAX_SEARCH_CONTRIBUTORS];
    int16               CompactCoefs[MAX_SEARCH_CONTRIBUTORS][MAX_NUM_FD_SUBBANDS];
    float               DelaysPerBeamPerContrib_ms[MAX_SEARCH_CONTRIBUTORS];
    BeamCoordinate_t    Coordinate;             // centimeters - uint16.  XYZ position of beam location
    float               NearestDistanceCmFlt;   // centimeters - distance from talker position to the closest microphone to aid with AGC on large arrays. (See ComputeSoutGain, 4th parameter)
} FdBeamSpecObj_t;

typedef struct
{
    // configuration parameters
    int16               InputSampleRate_kHz;
    uint16              BlockSizePreDownsampler;
	uint32				BlockSize_usec;
    uint16              NumMics;
    uint16              NumBeamsToSearch;
    float32             NoiseFloorBiasFactor;
    uint16              EnableDownsample2to1;
    uint16              TimeResolution_usec;    // effectively a request, see the main struct for the state
    FdBeamSpecObj_t     *FdBeamSpecObjPtr[MAX_SEARCH_BEAMS];
    float               *DirectivityCompPtr; // [MAX_SEARCH_BEAMS][MAX_NUM_FD_SUBBANDS];

    // decision related parameters
    float32             AlphaSubbandTrk;
    float32             PowerCompareValue;  // sets floor of fuzzy lookup for power level confidence
//    float32             PowerCompareScaleFactor; // correct PowerCmpareValue for sin gain
    float32             AlphaConfidenceUp;
    float32             AlphaConfidenceDown;
    float32             AlphaMaxBeamMerit;
    float32             BetaEdge;
    float32             ReverbFactor;
    float32             AlphaReverb;
    float32             MinBestSearchConfidenceFlt;
    float32             MaxBeamMeritMin;  // keep from using 0 or absurdly low power
    float32             MinBeamMerit;  // keep from using 0 or absurdly low power
    float32             BeamMeritDeductFactor;    // 1.0 or greater, amount to subtract from the rest of the merits for the ratio compare
    float32             EdgeRatioExponent;
    float32             MeritRatioExponent;
    int16               EchoConfidenceThreshQ15;
    float               NominalDistanceCmFlt;
    int16               *SubbandWeightsPtrQ10;
    int16               *BeamWeightsPtr;
    float               PowerConfidenceRatio[FUZZY_NUM_CURVE_PNTS];    
    float               PowerConfidenceFzPt[FUZZY_NUM_CURVE_PNTS];
}  FdBeamSearchConfig_t;

#endif
#ifdef __cplusplus
}
#endif

