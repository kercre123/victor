#ifndef TRACE_H_
#define TRACE_H_

#ifndef MATLAB_COMPILER
// Set this define to enable SD-card trace mode
//#define SD_TRACING

// Set this define to being SD-card logging immediately upon power up
#define SD_TRACE_IMMEDIATELY
#endif

// Masks defining trace categories.
// A trace var can belong to more than one.
typedef enum {
  TRACE_MASK_ALWAYS_ON        = 0x0001,
  TRACE_MASK_MOTOR_CONTROLLER = 0x0002,
  TRACE_MASK_POWER            = 0x0004,
  TRACE_MASK_LOCALIZATION     = 0x0008,
  TRACE_MASK_DELOCALIZATION   = 0x0010,
  TRACE_MASK_HMT              = 0x0020,
  TRACE_MASK_LANE_CHANGE      = 0x0040,
  TRACE_MASK_COMMS            = 0x0080,
  TRACE_MASK_SCAN             = 0x0100,
  TRACE_MASK_PROFILING        = 0x0200,
  TRACE_MASK_PLAYBACK         = 0x0400,
  TRACE_MASK_SCAN2            = 0x0800,
  TRACE_MASK_SELF_TEST        = 0x1000,
} TraceCategoryMask;

// DO NOT REORDER AND KEEP SYNC'D WITH SD IMPORTER
// ALL ADDITIONS MUST GO AT THE END OF THE ENUM
typedef enum
{
  TRACE_VAR_LOC_ID = 2,
  TRACE_VAR_PIECE_ID,
  TRACE_VAR_CURR_BARCODE_SECTION,
  TRACE_VAR_CURR_ACCUMULATING_LOCATION_ID,
  TRACE_VAR_CURR_ACCUMULATING_PIECE_ID,
  
  TRACE_VAR_IN_MSG,
  
  TRACE_VAR_IS_DELOC,
  TRACE_VAR_DELOC_MODE,
  TRACE_VAR_LOCALIZED_CYCLES,
  TRACE_VAR_DELOCALIZED_CYCLES,
  TRACE_VAR_SAW_FULL_BARCODE,
  TRACE_VAR_DEFINITELY_LOCALIZED_LAST_CYCLE,
  
  TRACE_VAR_NUM_LANES_TRAVERSED,
  TRACE_VAR_GET_CURR_PIXEL_SHIFT_RATE,
  
  TRACE_VAR_LANE_CHANGE_MODE,
  TRACE_VAR_STARTED_DECELERATING_FOR_LANE_APPROACH,
  TRACE_VAR_MERGING_INTO_BORDER_LANE,
  TRACE_VAR_BORDER_LANE_OVERSHOOT_ACCELERATION_PIXELS_PER_CYCLE,
  TRACE_VAR_LAST_BORDER_LANE_STATUS,
  TRACE_VAR_DESIRED_PIXELS_PER_CYCLE,
  TRACE_VAR_CURRENT_PIXELS_PER_CYCLE,
  TRACE_VAR_LANE_CHANGE_PIXELS_REQUIRED,
  TRACE_VAR_CURR_LANE_CHANGE_PIXELS_DESIRED,
  TRACE_VAR_LANE_CHANGE_TRACKING_PROGRESS,
  TRACE_VAR_LANE_CHANGE_PIXELS_DECELERATION_START,
  TRACE_VAR_CHANGING_RIGHT,
  TRACE_VAR_DOING_INFINITE_LANE_CHANGE,
  TRACE_VAR_CUMULATIVE_INVALID_CYCLES_EFFECT,
  TRACE_VAR_OFFSET_FROM_ROAD_CENTER_MM,
  TRACE_VAR_PRE_LANE_CHANGE_OFFSET_FROM_ROAD_CENTER_MM,
  TRACE_VAR_TARGET_LANE_OFFSET_FROM_ROAD_CENTER_MM,
  TRACE_VAR_INTENT_TO_HOP,
  TRACE_VAR_HAS_QUEUED_LANE_CHANGE,
  TRACE_VAR_QUEUED_LANE_CHANGE_RATE_MM_PER_SEC,
  TRACE_VAR_QUEUED_DEST_OFFSET_FROM_ROAD_CENTER_MM,
  TRACE_VAR_QUEUED_HOP_INTENT,
  
  TRACE_VAR_RESET_FULL_SCAN_PARSING,
  TRACE_VAR_ROAD_PIECE_IDX,
  TRACE_VAR_ROAD_OFFSET_MM,
  TRACE_VAR_OFFSET_SINCE_POSITION_UPDATE,
  TRACE_VAR_OFFSET_SINCE_RELATIVE_POSITION_UPDATE,
  TRACE_VAR_DEFINITELY_LOCALIZED,
  
  TRACE_VAR_PROF_CYCLE_TIME,
  TRACE_VAR_SCAN,
  TRACE_VAR_SYSTEM_STATE,
  TRACE_VAR_FIDX,
  TRACE_VAR_PARSE_MODE,
  TRACE_VAR_SAW_TRANS,
  TRACE_VAR_BORDER_LANE_STATUS,
  TRACE_VAR_CURR_TRACKED_MOTION_PIXELS,
  
  TRACE_VAR_PLAYBACK_ACTION_START_TIME,
  TRACE_VAR_CURRENT_SEQUENCE,
  TRACE_VAR_IS_PLAYING_MOTOR_ACTION,
  
  TRACE_VAR_AVG_WHITE_PIXEL_VAL,
  TRACE_VAR_AVG_BLACK_PIXEL_VAL,
  TRACE_VAR_CONTRAST_VAL,
  
  TRACE_VAR_ATTITUDE,
  TRACE_VAR_XTRACK_ERROR,
  TRACE_VAR_XTRACK_SPEED,
  TRACE_VAR_SPEEDMPS,
  TRACE_VAR_CURVATURE,
  
  TRACE_VAR_VSC_DESIRED_SPEED,
  TRACE_VAR_VSC_ERROR_SUM,
  
  TRACE_VAR_SPD_L,
  TRACE_VAR_SPD_R,
  TRACE_VAR_SPD_L_FILT,
  TRACE_VAR_SPD_R_FILT,
  TRACE_VAR_SPD_USER_DES,
  TRACE_VAR_SPD_USER_CUR,
  TRACE_VAR_SPD_CTRL,
  TRACE_VAR_SPD_MEAS,
  
  TRACE_VAR_DESIRED_SPD_L,
  TRACE_VAR_DESIRED_SPD_R,
  TRACE_VAR_WSPD_FILT_L,
  TRACE_VAR_WSPD_FILT_R,
  TRACE_VAR_ERROR_L,
  TRACE_VAR_ERROR_R,
  TRACE_VAR_SPIN_COUNT,
  
  TRACE_VAR_PWM_L,
  TRACE_VAR_PWM_R,
  
  TRACE_VAR_BTNHELD,
  
  TRACE_VAR_MV_BAT,
  TRACE_VAR_MV_BAT_FILT,
  TRACE_VAR_MV_EXT,
  TRACE_VAR_MV_REF,
  TRACE_VAR_PWR_OFF,
  
  TRACE_VAR_OUT_MSG,
  
  TRACE_VAR_profHAL,
  TRACE_VAR_profCamWait,
  TRACE_VAR_profCamera,
  TRACE_VAR_profLights,
  TRACE_VAR_profMessaging,
  TRACE_VAR_profLocalization,
  TRACE_VAR_profHighLevelScan,
  TRACE_VAR_profDelocalization,
  TRACE_VAR_profLaneChange,
  TRACE_VAR_profSpeedControl,
  TRACE_VAR_profSteerControl,
  TRACE_VAR_profWheelControl,
  TRACE_VAR_profPlayback,
  TRACE_VAR_profLogging,
  TRACE_VAR_profOverCycle,
  TRACE_VAR_profSDWrite,
  
  TRACE_VAR_FIDX_LC,
  TRACE_VAR_NUM_NEW_INVALID_TRACKING_CYCLES,
  TRACE_VAR_DID_TRACKING_CORRECTION,
  TRACE_VAR_PREV_FIDX,
  TRACE_VAR_NEW_TRACKED_BAR_CENTER,
  TRACE_VAR_OLD_TRACKED_BAR_CENTER,
  TRACE_VAR_HMT_PROGRESS_PRE_CORRECTION,
  TRACE_VAR_HMT_FIRST_CYCLE,

  TRACE_VAR_LW_ODO,
  TRACE_VAR_RW_ODO,
  
  TRACE_VAR_TRANS_PIXEL_COMP,
  TRACE_VAR_LANE_TO_MERGE_INTO_OFFSET,
  TRACE_VAR_SPEED_DAMPING,
  TRACE_VAR_LANE_CHANGE_LOC_MODE,
  TRACE_VAR_2DSCAN1,
  TRACE_VAR_2DSCAN2,
  TRACE_VAR_2DSCAN3,
  TRACE_VAR_2DLINEBIN,

  TRACE_VAR_DO_180_TURN,
  TRACE_VAR_REVERSE_DRIVING,
  TRACE_VAR_QUEUED_LC_OFFSET,

  TRACE_VAR_IS_IN_PLAYBACK_MODE,
  TRACE_VAR_IS_IN_MOTOR_DRIVING_PLAYBACK_MODE,

  TRACE_VAR_SKIP_AUTO_DRIVE,
  TRACE_VAR_REVERSE_DRIVING_COUNT,
  TRACE_VAR_CLASSIFICATION_MARGIN_SUM,

  TRACE_VAR_SELF_TEST_STATE,
  TRACE_VAR_SELF_TEST_CODE_COUNT,
  TRACE_VAR_SELF_TEST_SPEED_MEAS,
  TRACE_VAR_SELF_TEST_AVG_SPEED,
  TRACE_VAR_SELF_TEST_ERR,
  TRACE_VAR_NUM_CONSECUTIVE_SAME_PARSINGS, 
  TRACE_VAR_NUM_DONE_SCANNING_CURR_CODE,
  TRACE_VAR_SELF_TEST_NUM_BAD_CODES,

  // Add additional variables here and sync with SD importer
  
} TraceVariable;

#define TRACE_ENABLE_MASK TRACE_MASK_SCAN | TRACE_MASK_LOCALIZATION | TRACE_MASK_DELOCALIZATION | TRACE_MASK_HMT | TRACE_MASK_MOTOR_CONTROLLER | TRACE_MASK_COMMS


/*
// Defines which trace categories are enabled.
#define TRACE_ENABLE_MASK   0 \
    | TRACE_MASK_MOTOR_CONTROLLER \
    | TRACE_MASK_POWER \
    | TRACE_MASK_LOCALIZATION \
    | TRACE_MASK_DELOCALIZATION \
    | TRACE_MASK_HMT \
    | TRACE_MASK_LANE_CHANGE \
    | TRACE_MASK_COMMS \
    | TRACE_MASK_SCAN \
    | TRACE_MASK_PROFILING
*/

#define TracePrint(text, tcm) //_IfTraceEnabled(_TraceVal("t0" text, 0, 0), tcm)

#define Traceu8(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 1, val), tcm)
#define Traces8(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 1, val), tcm)
#define Traceu16(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 2, val), tcm)
#define Traces16(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 2, val), tcm)
#define Traceu32(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 4, val), tcm)
#define Traces32(name, val, tcm)  _IfTraceEnabled(_TraceVal(name, 4, val), tcm)
#define Tracefloat(name, val, tcm)  _IfTraceEnabled(_TraceFloat(name, 4, val), tcm)

#define Traceu8s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Traces8s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Traceu16s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Traces16s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Traceu32s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Traces32s(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Tracefloats(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);
#define Tracechars(name, val, tcm) _IfTraceEnabled(_TraceArray(name, sizeof(val), (void*)val), tcm);

// Trace-based profiling
// Make a name starting with pro for each task, for example proLaneChange
// Call PROFILE_START(proLaneChange) at the start, PROFILE_END(proLaneChange) at the end
// In the trace file, you will find a variable named proLaneChange containing the microseconds spent between start/end
// NOTE:  The START and END must be inside the same C block - i.e., same function at same level
#ifdef SD_TRACING
#include "hal/sd.h"
#define PROFILE_START(profName)   int profName = getMicroCounter();
#define PROFILE_END(profName)     Traceu16(TRACE_VAR_##profName, getMicroCounter() - profName, TRACE_MASK_PROFILING);

#define PROFILE_QUOTE(x)      #x
#else
#define PROFILE_START(x)
#define PROFILE_END(x)
#endif

// For internal use only - use the macros above
#ifdef SD_TRACING

#define _TraceVal(name, length, value) SdFileTraceValue((u8)name, value, length);
#define _TraceFloat(name, length, value) SdFileTraceFloat((u8)name, value, length);
#define _TraceArray(name, length, value) SdFileTraceArray((u8)name, value, length);
#define TraceStart() SdFileTraceStart();
#define _IfTraceEnabled(expr, tcm) do { if ((tcm) & (TRACE_ENABLE_MASK | TRACE_MASK_ALWAYS_ON)) expr; } while (0)

#else
// Dummy stubs for no tracing
#define _TraceVal(w, x, y)
#define _TraceFloat(w, x, y)
#define _TraceArray(w, x, y)
#define TraceStart()
#define _IfTraceEnabled(expr, tcm)
#endif

#endif
