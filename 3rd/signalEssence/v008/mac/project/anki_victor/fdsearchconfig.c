
/*----------------------------------------------------------------------- 
 (C) Copyright 2018 Signal Essence; All Rights Reserved
  fdsearchconfig_autogen.h
  Description: Automatically generated configuration from the fdsearchconfig_writer.h
  This file is Auto-generated.  Do not edit.
  Generated on 2018-11-02 10:41:19.899544 by hugh
----------------------------------------------------------------------- */
#include "fdsearchconfig.h"
int MMIfNumMics = 4;        // # of microphones
int MMIfNumSearchBeams = 14; // total number of beams to search

// Beam delays in milliseconds
#define BEAM0 { 0.0428331266f, 0.0428331266f, 0.1050245552f, 0.1050245552f }
#define BEAM1 { 0.0600319598f, 0.0325975520f, 0.0875583176f, 0.1155187833f }
#define BEAM2 { 0.0796610786f, 0.0319888296f, 0.0678759947f, 0.1161452508f }
#define BEAM3 { 0.0964571678f, 0.0411645752f, 0.0512437262f, 0.1067301714f }
#define BEAM4 { 0.1058386127f, 0.0577395065f, 0.0420365178f, 0.0898744107f }
#define BEAM5 { 0.1052143900f, 0.0773531306f, 0.0426473213f, 0.0701764297f }
#define BEAM6 { 0.0947576214f, 0.0947576214f, 0.0529180201f, 0.0529180201f }
#define BEAM7 { 0.0773531306f, 0.1052143900f, 0.0701764297f, 0.0426473213f }
#define BEAM8 { 0.0577395065f, 0.1058386127f, 0.0898744107f, 0.0420365178f }
#define BEAM9 { 0.0411645752f, 0.0964571678f, 0.1067301714f, 0.0512437262f }
#define BEAM10 { 0.0319888296f, 0.0796610786f, 0.1161452508f, 0.0678759947f }
#define BEAM11 { 0.0325975520f, 0.0600319598f, 0.1155187833f, 0.0875583176f }
#define BEAM12 { 0.0635548134f, 0.0635548134f, 0.0839035199f, 0.0839035199f }
#define BEAM13 { 0.0635548134f, 0.0635548134f, 0.0839035199f, 0.0839035199f }

// search beams, but at spatial filter interpolation rate
#define BEAMS0 {     22,     22,     54,     54 }
#define BEAMS1 {     31,     17,     45,     59 }
#define BEAMS2 {     41,     16,     35,     59 }
#define BEAMS3 {     49,     21,     26,     55 }
#define BEAMS4 {     54,     30,     22,     46 }
#define BEAMS5 {     54,     40,     22,     36 }
#define BEAMS6 {     49,     49,     27,     27 }
#define BEAMS7 {     40,     54,     36,     22 }
#define BEAMS8 {     30,     54,     46,     22 }
#define BEAMS9 {     21,     49,     55,     26 }
#define BEAMS10 {     16,     41,     59,     35 }
#define BEAMS11 {     17,     31,     59,     45 }
#define BEAMS12 {     33,     33,     43,     43 }
#define BEAMS13 {     33,     33,     43,     43 }

// distance from beam N to the closest microphone
// This is useful for large arrays only, and is used for AGC in ComputeSoutGain 
// Distances are floating point, and written in centimeters.
#define MIN_MIC_DISTANCE_CM_BEAM0 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM1 98.6f
#define MIN_MIC_DISTANCE_CM_BEAM2 98.6f
#define MIN_MIC_DISTANCE_CM_BEAM3 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM4 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM5 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM6 99.3f
#define MIN_MIC_DISTANCE_CM_BEAM7 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM8 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM9 98.9f
#define MIN_MIC_DISTANCE_CM_BEAM10 98.6f
#define MIN_MIC_DISTANCE_CM_BEAM11 98.6f
#define MIN_MIC_DISTANCE_CM_BEAM12 86.3f
#define MIN_MIC_DISTANCE_CM_BEAM13 86.3f

// XYZ coordinates of a search beam target location.  measured in centimeters
#define XYZ_LOC_COORD_BEAM0 {   86,    0,   49 }
#define XYZ_LOC_COORD_BEAM1 {   75,  -43,   49 }
#define XYZ_LOC_COORD_BEAM2 {   43,  -75,   49 }
#define XYZ_LOC_COORD_BEAM3 {    0,  -86,   49 }
#define XYZ_LOC_COORD_BEAM4 {  -43,  -75,   49 }
#define XYZ_LOC_COORD_BEAM5 {  -75,  -43,   49 }
#define XYZ_LOC_COORD_BEAM6 {  -86,    0,   49 }
#define XYZ_LOC_COORD_BEAM7 {  -75,   43,   49 }
#define XYZ_LOC_COORD_BEAM8 {  -43,   74,   49 }
#define XYZ_LOC_COORD_BEAM9 {    0,   86,   49 }
#define XYZ_LOC_COORD_BEAM10 {   43,   75,   49 }
#define XYZ_LOC_COORD_BEAM11 {   74,   43,   49 }
#define XYZ_LOC_COORD_BEAM12 {    0,    0,   86 }
#define XYZ_LOC_COORD_BEAM13 {    0,    0,   86 }

// Microphone coordinates, int16, millimeters.
// format is { x, y, z, MIC_TYPE_CARDIOID|MIC_TYPE_OMNI, direction_azimuth, direction_elevation }
#define XYZ_LOC_COORD_MIC0 {  -10,  -11,   -3, MIC_TYPE_OMNI,   0,   0 }
#define XYZ_LOC_COORD_MIC1 {  -10,   11,   -3, MIC_TYPE_OMNI,   0,   0 }
#define XYZ_LOC_COORD_MIC2 {   10,   11,    3, MIC_TYPE_OMNI,   0,   0 }
#define XYZ_LOC_COORD_MIC3 {   10,  -11,    3, MIC_TYPE_OMNI,   0,   0 }

MicrophoneLocation_mm microphoneLocations_mm[] = {
    XYZ_LOC_COORD_MIC0,
    XYZ_LOC_COORD_MIC1,
    XYZ_LOC_COORD_MIC2,
    XYZ_LOC_COORD_MIC3,
};

// Subband weigts are in q10 format
// { 0.000, 0.100, 0.500, 2.000, 12.000, 31.000 }
int16 SubbandWeightsQ10[MAX_NUM_FD_SUBBANDS] = { 0, 102, 512, 2048, 12288, 31744 };

// Beam Weights. These are in q12 format.
// { 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.976562, 0.002441, 0.002441 }
int16 BeamWeights[] = { 4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000, 4000, 10, 10 };
float DirectivityComp[MAX_SEARCH_BEAMS][MAX_NUM_FD_SUBBANDS] = {
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },
	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f }
};

FdBeamSpecObj_t FdBeamArray[] =
{
    // beam 0
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 0, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 0, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 0, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 0, contributor 3, subbands 0 to 6
        },
        BEAM0,
        XYZ_LOC_COORD_BEAM0,
        MIN_MIC_DISTANCE_CM_BEAM0
    },
    // beam 1
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 1, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 1, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 1, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 1, contributor 3, subbands 0 to 6
        },
        BEAM1,
        XYZ_LOC_COORD_BEAM1,
        MIN_MIC_DISTANCE_CM_BEAM1
    },
    // beam 2
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 2, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 2, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 2, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 2, contributor 3, subbands 0 to 6
        },
        BEAM2,
        XYZ_LOC_COORD_BEAM2,
        MIN_MIC_DISTANCE_CM_BEAM2
    },
    // beam 3
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 3, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 3, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 3, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 3, contributor 3, subbands 0 to 6
        },
        BEAM3,
        XYZ_LOC_COORD_BEAM3,
        MIN_MIC_DISTANCE_CM_BEAM3
    },
    // beam 4
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 4, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 4, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 4, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 4, contributor 3, subbands 0 to 6
        },
        BEAM4,
        XYZ_LOC_COORD_BEAM4,
        MIN_MIC_DISTANCE_CM_BEAM4
    },
    // beam 5
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 5, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 5, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 5, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 5, contributor 3, subbands 0 to 6
        },
        BEAM5,
        XYZ_LOC_COORD_BEAM5,
        MIN_MIC_DISTANCE_CM_BEAM5
    },
    // beam 6
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 6, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 6, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 6, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 6, contributor 3, subbands 0 to 6
        },
        BEAM6,
        XYZ_LOC_COORD_BEAM6,
        MIN_MIC_DISTANCE_CM_BEAM6
    },
    // beam 7
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 7, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 7, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 7, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 7, contributor 3, subbands 0 to 6
        },
        BEAM7,
        XYZ_LOC_COORD_BEAM7,
        MIN_MIC_DISTANCE_CM_BEAM7
    },
    // beam 8
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 8, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 8, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 8, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 8, contributor 3, subbands 0 to 6
        },
        BEAM8,
        XYZ_LOC_COORD_BEAM8,
        MIN_MIC_DISTANCE_CM_BEAM8
    },
    // beam 9
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 9, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 9, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 9, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 9, contributor 3, subbands 0 to 6
        },
        BEAM9,
        XYZ_LOC_COORD_BEAM9,
        MIN_MIC_DISTANCE_CM_BEAM9
    },
    // beam 10
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 10, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 10, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 10, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 10, contributor 3, subbands 0 to 6
        },
        BEAM10,
        XYZ_LOC_COORD_BEAM10,
        MIN_MIC_DISTANCE_CM_BEAM10
    },
    // beam 11
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 11, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 11, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 11, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 11, contributor 3, subbands 0 to 6
        },
        BEAM11,
        XYZ_LOC_COORD_BEAM11,
        MIN_MIC_DISTANCE_CM_BEAM11
    },
    // beam 12
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 12, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 12, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 12, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 12, contributor 3, subbands 0 to 6
        },
        BEAM12,
        XYZ_LOC_COORD_BEAM12,
        MIN_MIC_DISTANCE_CM_BEAM12
    },
    // beam 13
    { 4,      // NumContributors
        { 0, 1, 2, 3 },    // MicIndex for each contributor
        {
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 13, contributor 0, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 13, contributor 1, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 13, contributor 2, subbands 0 to 6
            {  8191,  8191,  8191,  8191,  8191,  8191 }, // Beam 13, contributor 3, subbands 0 to 6
        },
        BEAM13,
        XYZ_LOC_COORD_BEAM13,
        MIN_MIC_DISTANCE_CM_BEAM13
    },
};
