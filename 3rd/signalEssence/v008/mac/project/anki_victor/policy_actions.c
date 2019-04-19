/* (C) Copyright 2017 Signal Essence for Robot; All Rights Reserved

   Module Name  - policy_actions.c

   Author: Hugh McLaughlin

   Description:

   Policy Actions.

   Anki Specific:
   The Anki robot has the microphones on the body and the loudspeaker is !!!
   The body can spin in place, but not terribly fast.
   The head only moves up and down independently of the body.
   
   The Anki robot has 4 microphones closely spaced. Direction-finding operates mostly on 
   frequencies above 2 kHz and works best on frequencies above 4 kHz.

   The confidence results are reported for each of 13 look directions.  There 
   are 12 look directions that look about the horizontal angles and one direction
   that looks straight up.

   For each search direction there is a spatial filter assigned.
   The search beams are different from the spatial filter beams.
   The search beams are delay and sum beams while the spatial filter beams
   use differential beamforming to get more directionality at low frequencies.

   In this application, automatic mode can be used, but performance will be best
   if the robot uses other known information such as the direction where the key phrase
   was detected or where a face is detected.

   The 0 index faces in the same direction that the robots face looks and the rest of
   the indexes are clockwise.

   The spatial filter assignment can be either automatic or controlled by the callback function.
   The callback function provides the means to the application to use other side information
   to override the immediate search information. For example, if a face is in view of the camera
   the application can be quite confident that the face will be the source of the speech.
   Also, if the key phrase was recently detected then it is better to aim in the direction that
   the key phrase was sensed.

   The calling program presumably keeps a circular buffer of the fdsearch_best_beam_index
   (and possibly fdsearch_best_beam_confidence) and checks these results when a key phrase is
   detected. The calling program determines the span of time to check. In the simplest version, it
   would simply vote by tallying the number of occurrences of each index.  The one with the most
   votes would win and this would mark the direction index to associate with the key phrase.
   The usual key phrase is about 750 milliseconds in length.

   The resulting search index corresponds to an angle.
   The angle is defined to be 0 radians (0 degrees) at the forward direction of the robot, i.e. 
   North.

   The indexes start with 0 which also points to forward direction of the robot
   (following right hand rule convention).
   The Anki angles are defined as radians.

   The beam index corresponds to a clock dial position for indexes 0 through 11
   and index 12 corresponds to a location directly overhead.
   The fdsearch_best_beam_confidence is a 16 bit signed number with values 0 to 32767

       convert the index to radians by the following formula:

       Phi(radians) = -index*pi/6

    The right hand rule convention goes counterclockwise, hence the negative sign.
    Stating the obviouus, the clock dial positions go clockwise.

    In order to utilize the information from the key phrase detection and the
    face tracking, the host program can simply tell the policy algorithm the
    angle in radians.

    It then maps this to the spatial filter index, of which there are 12 spatial filters.
    The formula that is applied is the inverse of the one above.

        index = Modulo( -6*(Phi(radians)/pi), 12 )

    checking this assertion.  
        Phi = 0   -->   index = 0
        Phi = pi/6  ->   index = 11

    Obviously, one has to check for signs.  Since the index can only be positive,
    then if a negative number is sensed, then need to add 12.

    Based on the request (from the angle) the spatial filter index is set.
    This is accomplished by poking the SeDiag's below:
       search_result_best_beam_index
       search_result_best_beam_confidence

    In general, the confidence should be set to near 32767, i.e. max confidence.

    Depending on how the spatial filters are configured there can be fallback beams
    based on the confidence metric.  If the confidence is low then it might be that
    only a single microphone is selected for all frequency bands. This would be the 
    case if there is simply no confidence about where sound might be coming from.
    However, if that is the case then it should be in automatic mode.

    Automatic mode is commanded by either 
       calling KeyPhraseDetected( -1 );
       or calling
       SetBodyRelativeAimingLocationAutomatic();     -- no arguments

   Much of this module deals with the fact that the body can spin.

   Other functions can be called asynchronously to set modes of operation. The point of the callback function is to make a 
   decision on noise reduction and the spatial filter direction.

   Modes:
     Auto mode -- accepts search result as-is, effectively bypassing the policy algorithm
     Policy mode -- 
         1. by Key phrase location -- until other information, the last known key prhase is the 
            direction to point.
         2. Other means such as face detection by the camera.  In this case the angle of the 
            location relative to the body is provided and this is interpreted to a spatial filter direction.

   inputs
     from MMFx
        CurrentConfidenceFlt -- added to sediags recently -- a vector 

 	outputs
        Spatial Filter Index
        3D cartesian coordinates
        Noise Reference Fullband Weight (future)
        Noise Reference per Bin Weight (future)

   History:    hjm - Hugh McLaughlin

   10/10/17    hjm  created with models from other projects

   Machine/Compiler: ANSI C 
   ---------------------------------------------------------------------------------*/

#include <math.h>
#ifdef SE_PLTFRM_WIN32
#include <memory.h>   // for memset
#else
#include <string.h>
#endif

#include "fdsearchconfig.h"
#include "floatmath.h"
#include "fuzzymapper.h"
#include "sfilters.h"
#include "policy_actions.h"
#include "mmif.h"
#include "mmif_proj.h"
//#include "frmath.h"
#include "vfixlib.h"
#include "vfloatlib.h"
#include "se_diag.h"
#include "se_error.h"
#include "se_snprintf.h"
#include <string.h>       // defines "NULL"
#include "tap_point.h"

SEDiagEnumDescriptor_t FallbackFlagDescriptor[] = {
    DIAG_ENUM_DECLARE(    FBF_FIRST_DONT_USE,    "do not use"),
    DIAG_ENUM_DECLARE(    FBF_AUTO_SELECT,       "auto select spatial filter beam"),
    DIAG_ENUM_DECLARE(    FBF_FORCE_FALLBACK,    "force fallback beam"),
    DIAG_ENUM_DECLARE(    FBF_FORCE_ECHO_CANCEL, "force echo-cancelling beam"),
    DIAG_ENUM_DECLARE(    FBF_LAST,              "do not use"),
    DIAG_ENUM_END
};

// get the maximum value weights for perbin and fullband Noise Reference weights
void GetNoiseRefWeights(float *perBinWeightPtr, float *fullbandWeightPtr);

RobotAudioPolicy_t RobotAudioPolicyObj;
extern FdBeamSpecObj_t FdBeamArray[];

void PolicyInit(void)
{
    RobotAudioPolicy_t *sp;
    int id;
    int32 numMics;
    int i;

    sp = &RobotAudioPolicyObj;

    // temporary test to see if any missing initializations, so not rely on OS zeroing everything
    memset( (void *)sp, 0xfb, sizeof( RobotAudioPolicy_t ) );

    // get number of microphones
    id = SEDiagGetIndex("mmfx_num_microphones");
    SeAssert(id >= 0);
    numMics = SEDiagGetInt32(id);
    sp->NumMics = (uint16)numMics;

    // get the id for the AlphaConfidenceDown parameter

    id = SEDiagGetIndex( "fdsearch_alpha_confidence_down" );
    SeAssert(id >= 0);
    sp->SEDiagId_AlphaConfidenceDown = id;

    id = SEDiagGetIndex( "fdsearch_alpha_confidence_up" );
    SeAssert(id >= 0);
    sp->SEDiagId_AlphaConfidenceUp = id;

    // This sets the spatial filter index
    id = SEDiagGetIndex("search_result_best_beam_index");
    SeAssert(id >= 0);
    sp->SEDiagId_SearchResultBestBeamIndex = id;

    // This sets the confidence level to be assigned with this spatial filter
    // index.  Normally, set this to 32767.  Setting this to 0 would invoke
    // the fallback beam, which is just a single omni-directional microphone.
    // This could be valid if there is literally no known direction to point.
    id = SEDiagGetIndex("search_result_best_beam_confidence");
    SeAssert(id >= 0);
    sp->SEDiagId_SearchResultBestBeamConfidence = id;

    // init sediags for the policy actions.  This allows
    // for easier testing by poking sediags

    SEDiagNewInt32("policy_new_phi_update_mailbox_flag", &sp->NewPhiMailboxFlag, SE_DIAG_RW, NULL, "new angle flag 1=new, 0=read");
    SEDiagNewFloat32("policy_phi_mailbox",&sp->NewPhiMailbox, SE_DIAG_RW, NULL, "Phi coordinate mailbox");

    SEDiagNewInt32("policy_key_phrase_detected_mailbox_flag", &sp->KeyPhraseDetectedMailboxFlag, SE_DIAG_RW, NULL, "key phrase detected flag 1=new, 0=read");
    SEDiagNewInt32("policy_key_phrase_index_from_host", &sp->KeyPhraseIndexMailbox, SE_DIAG_RW, NULL, "key phrase index reported from host, -1 if not host-determined" );

    SEDiagNewEnum("policy_fallback_flag", (enum_t *)&sp->FallbackFlag, SE_DIAG_RW, NULL, "select fallback mode", FallbackFlagDescriptor);

    id = SEDiagGetIndex("policy_phi_mailbox");
    SeAssert(id >= 0);
    sp->SEDiagId_PhiUpdateMailbox = id;

    id = SEDiagGetIndex("policy_key_phrase_detected_mailbox_flag");
    SeAssert(id >= 0);
    sp->SEDiagId_KeyPhraseDetectedMailboxFlag = id;

    id = SEDiagGetIndex("policy_key_phrase_index_from_host");
    SeAssert(id >= 0);
    sp->SEDiagId_KeyPhraseIndexFromHost = id;

    id = SEDiagGetIndex("senr_nr_weight_factor");
    SeAssert(id >= 0);
    sp->SEDiagId_NR_PerBinWeight = id;

    id = SEDiagGetIndex("senr_nr_fullband_weight_factor");
    SeAssert(id >= 0);
    sp->SEDiagId_NR_FullbandWeight = id;

    id = SEDiagGetIndex("fdsearch_confidence_state");
    SeAssert(id >= 0);
    sp->SEDiagId_FdSearchConfidenceState = id;

    id = SEDiagGetIndex("fdsearch_best_beam_confidence");
    SeAssert(id >= 0);
    sp->SEDiagId_FdSearchBestBeamConfidence = id;

    id = SEDiagGetIndex("fdsearch_best_beam_index");
    SeAssert(id >= 0);
    sp->SEDiagId_FdSearchBestBeamIndex = id; 

    // variables that are useful for validating functionality

    SEDiagNewFloat32("policy_new_phi",&sp->NewPhi, SE_DIAG_RW, NULL, "phi coordinate body orientation");
    SEDiagNewInt32("policy_key_phrase_index", &sp->KeyPhraseIndex, SE_DIAG_RW, NULL, "key phrase index that is utilized" );

    sp->MaxCountNoMovement = 500;        // approximaely 10 seconds if reporting interval is 20 ms
    sp->CountThresholdNoMovement = 75;   // about 1.5 seconds, if no movement

    // note these will vary with the search window
    // check the initialization in mmif_proj.c where the TimeResolution_usec is checked to 
    // set various alpha's
    // this set was for 2.5 ms time window
//#define SEARCH_INTERVAL_2P5_MS 1
#define SEARCH_INTERVAL_1P25_MS 1
#ifdef SEARCH_INTERVAL_2P5_MS
    sp->AlphaDownNoMovement      = 0.0025f;  // slow, 0.5 second time constant  -- was 0.01 for full block
    sp->AlphaDownLessThan1Degree = 0.0025f;  // slow, but not nothing, 200 ms time constant
    sp->AlphaDown2Degrees        = 0.1f;   // 100 ms time constant   !!! hjm divide these and other below by 4 or 5 depending on subblock size
    sp->AlphaDown4Degrees        = 0.15f;  // 60 ms time constant
    sp->AlphaDown8Degrees        = 0.25f;  // 40 ms time constant
    sp->AlphaDown16Degrees       = 0.5f;   // 20 ms time constant -- very quick
    sp->AlphaDownFast            = 0.75f;  // faster than 20 ms
#else
    sp->AlphaDownNoMovement      = 0.01;   // see email from HM 2017.12.12 re decay bug
    sp->AlphaDownLessThan1Degree = 0.00125f;  // slow, but not nothing, 200 ms time constant
    sp->AlphaDown2Degrees        = 0.05f;   // 100 ms time constant   !!! hjm divide these and other below by 4 or 5 depending on subblock size
    sp->AlphaDown4Degrees        = 0.07f;  // 60 ms time constant
    sp->AlphaDown8Degrees        = 0.12f;  // 40 ms time constant
    sp->AlphaDown16Degrees       = 0.25f;   // 20 ms time constant -- very quick
    sp->AlphaDownFast            = 0.50f;  // faster than 20 ms
#endif

    sp->KeyPhraseCountDown       = 0;      // start out in automatic mode
    sp->OverrideWithPointingLocationFlag = 0;
    sp->KeyPhraseTimeOutCount    = 1000;   // 10 seconds, 100 blocks per second
    sp->BlockCounter             = 0;
    sp->CountNoMovement          = 0;
    sp->FallbackFlag = FBF_AUTO_SELECT;

    // kldugy weighting
    sp->NumReadings = MAX_POSITION_READINGS;
    for( i=0; i<sp->NumReadings; i++ )
    {
        sp->DiffWeight[i] = 1.21f * ((float)sp->NumReadings/(3.0f+(float)i));
        sp->WeightedDiffPitch[i] = 0.0f;
        sp->WeightedDiffRotate[i] = 0.0f;
    }

    sp->NewPhiMailbox = 0;
    sp->KeyPhraseDetectedMailboxFlag = 0;

    // set the new downward decay, and upward averaging
    SEDiagSetFloat32( sp->SEDiagId_AlphaConfidenceDown, sp->AlphaDownNoMovement );  
    SEDiagSetFloat32( sp->SEDiagId_AlphaConfidenceUp, 0.99f );    // more of a 2 block average than 3 block default

    // initialize stuff that might be okay without initialization, but good hygiene
    sp->NewPhi = 0.0f;
    sp->KeyPhraseIndex = 0;
    sp->NewPhiMailboxFlag = 0;;  // 1 when new, 0 when read
    sp->NewPhiMailbox = 0.0f;  // new x, y, z coordinates
    sp->KeyPhraseDetectedMailboxFlag = 0;
    sp->KeyPhraseIndexMailbox = 0;
    sp->RelativePhiMailboxFlag = 0;
    sp->RelativePhiMailbox = 0.0f;
    sp->AlphaDown = sp->AlphaDownNoMovement;
    sp->TotalAngleMaxDiff = 0.0f;

    VFill_flt32( sp->PreviousPhi, 0.0f, MAX_POSITION_READINGS );

    sp->DiffPitchMax = 0.0f;
    sp->DiffRotateMax = 0.0f;
    sp->BodyRelativePhi = 0.0f;
    sp->BodyRelativePhiDegrees = 0.0f;  
    sp->KeyPhraseAbsolutePhi = 0.0f;
    sp->KeyPhraseAbsolutePhiDegrees = 0.0f;
    sp->RelativeOffsetPhi = 0.0f;

}

/*
The host robot reports the newest orientation of the body relative to true north
This is absolute location, for example, as determined by a compass
*/
void PolicySetAbsoluteOrientation( float phiRadians )
{
    RobotAudioPolicy_t *sp;
    int i;

    sp = &RobotAudioPolicyObj;

    // push mini delay line
    // This delay line could be used in the future to determine if the recent movement
    // has been significant enough to change any parameters or variables.
    // If Cozmo had an acoustic echo canceller then this history information would be used
    // to determine if the AEC is likely to be misconverged due to the movement.
    for( i=(sp->NumReadings-1); i>0; i-- )
    {
        sp->PreviousPhi[i] = sp->PreviousPhi[i-1];
    }
    sp->PreviousPhi[0] = sp->NewPhi;  // consume previous Phi

    sp->NewPhi = phiRadians;

    // code would go here to determine if the echo canceller's ERLE variable needs to be lowered
    // so that the AEC is not spoofed into thinking that echo is a near end talker.
}

/*
  Update the policy state variables, modify MMFx behavior -- This is the callback function.
*/
void PolicyDoActions( void *p)
{
    uint16 bestBeamIndex;
    int16  searchConf;
    RobotAudioPolicy_t *sp;

    UNUSED(p);

    sp = &RobotAudioPolicyObj;

    // okay, get the latest direction on where to point.

    if( 0 < sp->NewPhiMailboxFlag )
    {
        PolicySetAbsoluteOrientation( sp->NewPhiMailbox );
        sp->NewPhiMailboxFlag = 0;  // clear mailbox flag
    }

    if( 0 < sp->KeyPhraseDetectedMailboxFlag )
    {
        PolicySetKeyPhraseDirection( sp->KeyPhraseIndexMailbox );
        sp->KeyPhraseDetectedMailboxFlag = 0;    // clear mailbox flag -- read
    }

    if( 0 < sp->RelativePhiMailboxFlag )
    {
        PolicySetAimingDirection(sp->RelativePhiMailbox );
        sp->RelativePhiMailboxFlag = 0;    // clear mailbox flag -- read
    }

    // The pointing direction can be one of:
    //    1. determined automatically
    //    2. using the key phrase detected direction
    //    3. using an alternative means such as face detection
    //    4. nothing using fallback "omni" beam
    //    5. nothing using echo cancelling beam
    // If the key phrase was recently detected with high confidence then use the key phrase direction
    // If no key phrase was detected for a while then go back to using the automatic detection.
    // For now just use a simple timeout.  This is just a count down of blocks at 10 ms intervals.
    // 
    // The key phrase sets the RelativePhi. This can be overridden by calling SetBodyRelativeAimingLocation()

    if( (sp->KeyPhraseCountDown > 0) || sp->OverrideWithPointingLocationFlag || (sp->FallbackFlag!=FBF_AUTO_SELECT) )    
    {
        float x, y, z;
        int xi, yi, zi;
        int minError;
        int i;
        int16 newIndex;

        if( sp->KeyPhraseCountDown > 0 )
        {
            // then compute the RelativeOffsetPhi given the 
            // absolute orientation relative to where the key phrase was last seen

            // get the current body rotational direction
            // subtract the current body direction from the absolute position
            // where the key phrase was detected
            sp->RelativeOffsetPhi = sp->KeyPhraseAbsolutePhi - sp->NewPhi;
            if( sp->RelativeOffsetPhi > M_PI )
                sp->RelativeOffsetPhi = sp->RelativeOffsetPhi - (float)(2*M_PI);
            else if( sp->RelativeOffsetPhi < -M_PI )
                sp->RelativeOffsetPhi = sp->RelativeOffsetPhi + (float) (2*M_PI);
        }

        // look up the index that the relative offset should be
        // convert to x, y, z
        // then look up from search configuration
        // theta is usually higher than lower, not that it matters
        // take sin of 15 degrees for elevation.  cos(10) = 0.173, cos(10) = 

        x = (float)cos( sp->RelativeOffsetPhi );
        y = (float)sin( sp->RelativeOffsetPhi );
        z = 0.0f;  // probably unnecessary

        // convert from float to centimeter target int
        xi = (int)(100.0f * x);
        yi = (int)(100.0f * y);
        zi = (int)(100.0f * z);

        // for every search direction look for the closest matching position

        minError = 30000;      // 3 * 100 * 100
        newIndex = 0;
        for( i=0; i<12; i++ )   // 12 search locations, just the middle level
        {
            long dx2, dy2, dz2;
            long totError;
            dx2 = xi-FdBeamArray[i].Coordinate.x;
            dx2 = dx2*dx2;
            dy2 = yi-FdBeamArray[i].Coordinate.y;
            dy2 = dy2*dy2;
            dz2 = zi-FdBeamArray[i].Coordinate.z;
            dz2 = dz2*dz2;
            totError = dx2+dy2+dz2;
            if( totError < minError )
            {
                newIndex = (uint16)i;  // convert to 16 bit unsigned number
                minError = totError;
            }
        }

        // But, if the fallback flag is set then the bestBeamIndex to the fallback index
        if (sp->FallbackFlag == FBF_FORCE_FALLBACK)
            newIndex = 12;              // where index 12 is the fallback beam
        else if(sp->FallbackFlag == FBF_FORCE_ECHO_CANCEL)   // check if Echo Cancelling Beam
            newIndex = 13;              // where 13 is the echo-cancelling beam
        else
            SeAssert(0);

        bestBeamIndex = newIndex;
        searchConf = 32000;            // set to high confidence
        
        // post the bestBeamIndex and searchConf to the SearchResults data structure.
        // Note, not posted to the FdBeamSearchObject_t, which keeps an internal record.
        // The SearchResults structure is picked up by the spatial filter
        SEDiagSetUInt16( sp->SEDiagId_SearchResultBestBeamIndex, bestBeamIndex );
        SEDiagSetInt16( sp->SEDiagId_SearchResultBestBeamConfidence, searchConf );
    }
    // else
    //   Leave the bestBeamIndex and confidence as-is

    sp->KeyPhraseCountDown -= 1;
    if( sp->KeyPhraseCountDown < 0 )
        sp->KeyPhraseCountDown = 0;    // no reason to go negative

    // increment block counter.  It is unsigned 32-bit so it will just roll over
    sp->BlockCounter += 1;

    // apply noise reference weights depending on mode and pointing direction
    // if mode is speech reco, then apply the noise reference if the sound is coming from the front
    // either 11, 0, or 1 O'clock positions
    {
        float perBinWeight;
        float fullbandWeight;
        uint16 curBestBeamIndex;
        unsigned int curBestBeamModulo;
        MMIfOpMode_t opMode;

        GetNoiseRefWeights( &perBinWeight, &fullbandWeight );
        curBestBeamIndex = SEDiagGetUInt16( sp->SEDiagId_SearchResultBestBeamIndex );
                         
        opMode = MMIfGetOperatingMode();

        // reduce to clock dial from 3 elevations of beams
        curBestBeamModulo = curBestBeamIndex % 12;

        if( MMIF_SPEECH_RECO_MODE == opMode )
        {
            if( (curBestBeamModulo == 11) ||  (curBestBeamModulo == 1) || (curBestBeamModulo == 0) )
            {
                // if off-axis deweight the noise reference weights a little
                if( curBestBeamModulo != 0 )
                {
                    // scale reference weights by 0.8
                    float offAxisWeight = 0.8f;
                    perBinWeight *= offAxisWeight;
                    fullbandWeight *= offAxisWeight;
                }
            }
            else
            {
                perBinWeight   = 0.0f;
                fullbandWeight = 0.0f;
            }
        }
        else if( MMIF_TELECOM_MODE == opMode )
        {
            // currently the same strategy as speech reco mode
            if( (curBestBeamModulo == 11) ||  (curBestBeamModulo == 1) || (curBestBeamModulo == 0) )
            {
                // if off-axis deweight the noise reference weights a little
                if( curBestBeamModulo != 0 )
                {
                    // scale reference weights by 0.8
                    float offAxisWeight = 0.8f;
                    perBinWeight *= offAxisWeight;
                    fullbandWeight *= offAxisWeight;
                }
            }
            else
            {
                perBinWeight   = 0.0f;
                fullbandWeight = 0.0f;
            }
        }
        else
        {
            perBinWeight   = 0.0f;
            fullbandWeight = 0.0f;
        }
        SEDiagSetFloat32( sp->SEDiagId_NR_PerBinWeight,   perBinWeight );  
        SEDiagSetFloat32( sp->SEDiagId_NR_FullbandWeight, fullbandWeight );  
    }
}

/*
Call this function to dwell when the keyphrase has been detected.
This will cause the spatial filter to dwell in a specific direction for a 
period of time, after which MMFx will return to automatic search.

Specify index == -1 to return to automatic search.
*/
void PolicySetKeyPhraseDirection( int index )
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    // the fact that this is called means that we are going to point
    // in the direction of the key phrase and not the Override direction
    sp->OverrideWithPointingLocationFlag = 0;

    if( index >= 0 )   // index specified externally
    {
        sp->KeyPhraseIndex = index;
        sp->KeyPhraseCountDown = sp->KeyPhraseTimeOutCount;  // start the time out count down
        sp->OverrideWithPointingLocationFlag = 0;
    }
    else
    {
        // setting the count down to 0 makes searching automatic
        sp->KeyPhraseCountDown = 0;
        // consider adding a net confidence factor from the FDSW software in the future
        // in order to weight the degree of the voting and search confidence
    }

    // retrieve the coordinates of the key phrase search winner
    // index 0 starts at 0 degrees
    // conversion of index to radians is index*2*pi/12 = index * pi /6 = index * 0.523599
    sp->BodyRelativePhiDegrees = -sp->KeyPhraseIndex * 30.0f;
    if( sp->BodyRelativePhiDegrees < -180.0f )
        sp->BodyRelativePhiDegrees += 360.0f;

    sp->BodyRelativePhi = sp->BodyRelativePhiDegrees * (float)M_PI / 180.0f;

    // Compute the absolute angular position.  

    sp->KeyPhraseAbsolutePhi = sp->BodyRelativePhi + sp->NewPhi;
    if( sp->KeyPhraseAbsolutePhi > M_PI )
        sp->KeyPhraseAbsolutePhi = sp->KeyPhraseAbsolutePhi - (2.0f * (float)M_PI);
    else if( sp->KeyPhraseAbsolutePhi < -M_PI )
        sp->KeyPhraseAbsolutePhi = sp->KeyPhraseAbsolutePhi + (2.0f * (float)M_PI);

    sp->KeyPhraseAbsolutePhiDegrees = sp->KeyPhraseAbsolutePhi * 180.0f / (float)M_PI;
}

/*
Call this function to force the spatial filter to dwell in a specific
direction relative to the robot's orientation.
The spatial filter will dwell indefinitely.
*/
void PolicySetAimingDirection( float relativePhiRadians )
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    sp->BodyRelativePhi = relativePhiRadians;
    sp->BodyRelativePhiDegrees = sp->BodyRelativePhi * 180.0f / (float)M_PI;
    sp->OverrideWithPointingLocationFlag = 1;
}

/*
Return to automatic search.
*/
void PolicyDoAutoSearch(void)
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    // disable override
    sp->OverrideWithPointingLocationFlag = 0;

    // setting the count down to 0 makes searching automatic
    sp->KeyPhraseCountDown = 0;
}

// set and clear the fallback beam
// Set the fallback beam when the calling program knows that the direction of sound
// is likely to be unknown or invalid, especially if there was gear noise interference.

void PolicySetFallback(void)
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    // disable override
    sp->FallbackFlag = FBF_FORCE_FALLBACK;
}

void PolicySetFallbackEchoCancelling(void)
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    // disable override
    sp->FallbackFlag = FBF_FORCE_ECHO_CANCEL;
}

void PolicyClearFallback(void)
{
    RobotAudioPolicy_t *sp;
    sp = &RobotAudioPolicyObj;

    // disable override
    sp->FallbackFlag = FBF_AUTO_SELECT;
}


