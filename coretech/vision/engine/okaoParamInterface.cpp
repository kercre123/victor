/**
 * File:   okaoParamInterface.cpp
 * 
 * Author: Humphrey Hu
 * Date:   6/6/2018
 * 
 * Description: Interfaces for the OKAO library parameters
 * 
 * Copyright: Anki, Inc. 2018
 **/

#include "DetectorComDef.h"
// #include "OkaoPtAPI.h" // Face parts detection // TODO
// #include "OkaoExAPI.h" // Expression recognition // TODO
// #include "OkaoSmAPI.h" // Smile estimation // TODO
// #include "OkaoGbAPI.h" // Gaze & blink estimation // TODO

#include "coretech/vision/engine/okaoParamInterface.h"
#include <algorithm>
#include <vector>


namespace Anki {
namespace Vision {
namespace Okao {

// Specializations for PoseAngle
template <>
typename ParamTraits<PoseAngle>::EnumArray ParamTraits<PoseAngle>::GetEnums()
{
  return
  {{
    {PoseAngle::Unspecified, POSE_ANGLE_UNSPECIFIED,  "Unspecified"},
    {PoseAngle::Front,       POSE_ANGLE_FRONT,        "Front"},
    {PoseAngle::HalfProfile, POSE_ANGLE_HALF_PROFILE, "Half Profile"},
    {PoseAngle::Profile,     POSE_ANGLE_PROFILE,      "Profile"},
    {PoseAngle::Rear,        POSE_ANGLE_REAR,         "Rear"}
  }};
}

// Specialization for RollAngle
template <>
typename ParamTraits<RollAngle>::EnumArray ParamTraits<RollAngle>::GetEnums()
{
  return
  {{
    {RollAngle::Upper0,             ROLL_ANGLE_0,     "Upper (0)"},
    {RollAngle::UpperRight30,       ROLL_ANGLE_1,     "Upper Right (30)"},
    {RollAngle::UpperRight60,       ROLL_ANGLE_2,     "Upper Right (60)"},
    {RollAngle::Right90,            ROLL_ANGLE_3,     "Right (90)"},
    {RollAngle::LowerRight120,      ROLL_ANGLE_4,     "Lower Right (120)"},
    {RollAngle::LowerRight150,      ROLL_ANGLE_5,     "Lower Right (150)"},
    {RollAngle::Down180,            ROLL_ANGLE_6,     "Down (180)"},
    {RollAngle::LowerLeft210,       ROLL_ANGLE_7,     "Lower Left (210)"},
    {RollAngle::LowerLeft240,       ROLL_ANGLE_8,     "Lower Left (240)"},
    {RollAngle::Left270,            ROLL_ANGLE_9,     "Left (270)"},
    {RollAngle::UpperLeft300,       ROLL_ANGLE_10,    "Upper Left (300)"},
    {RollAngle::UpperLeft330,       ROLL_ANGLE_11,    "Upper Left (330)"},
    {RollAngle::All,                ROLL_ANGLE_ALL,   "All"},
    {RollAngle::None,               ROLL_ANGLE_NONE,  "None"},
    {RollAngle::UpperPm45,          ROLL_ANGLE_U45,   "Upper +- 45"},
    {RollAngle::UpperLeftRightPm15, ROLL_ANGLE_ULR15, "Upper Left Right +- 15"},
    {RollAngle::UpperLeftRightPm45, ROLL_ANGLE_ULR45, "Upper Left Right +- 45"}
  }};
}

// Specializations for RotationAngle
template <>
typename ParamTraits<RotationAngle>::EnumArray ParamTraits<RotationAngle>::GetEnums()
{
  return
  {{
    {RotationAngle::None,       ROTATION_ANGLE_EXT0,   "None"},
    {RotationAngle::LeftRight1, ROTATION_ANGLE_EXT1,   "LeftRight1"},
    {RotationAngle::LeftRight2, ROTATION_ANGLE_EXT2,   "LeftRight2"},
    {RotationAngle::All,        ROTATION_ANGLE_EXTALL, "All"}
  }};
}

// Specialization for SearchDensity
template <>
typename ParamTraits<SearchDensity>::EnumArray ParamTraits<SearchDensity>::GetEnums()
{
  return
  {{
    {SearchDensity::Highest, DENSITY_HIGHEST, "Highest"},
    {SearchDensity::High,    DENSITY_HIGH,    "High"},
    {SearchDensity::Normal,  DENSITY_NORMAL,  "Normal"},
    {SearchDensity::Low,     DENSITY_LOW,     "Low"},
    {SearchDensity::Lowest,  DENSITY_LOWEST,  "Lowest"}
  }};
}

// Specialization for DetectionMode
template <>
typename ParamTraits<DetectionMode>::EnumArray ParamTraits<DetectionMode>::GetEnums()
{
  return
  {{
    {DetectionMode::Default,    DETECTION_MODE_DEFAULT, "Default"},
    {DetectionMode::Whole,      DETECTION_MODE_MOTION1, "Whole"},
    {DetectionMode::Partition3, DETECTION_MODE_MOTION2, "Partition 3"},
    {DetectionMode::Gradual,    DETECTION_MODE_MOTION3, "Gradual"},
    {DetectionMode::Still,      DETECTION_MODE_STILL,   "Still"},
    {DetectionMode::Movie,      DETECTION_MODE_MOVIE,   "Movie"}
  }};
}

// Specialization for TrackingAccuracy
template <>
typename ParamTraits<TrackingAccuracy>::EnumArray ParamTraits<TrackingAccuracy>::GetEnums()
{
  return
  {{
    {TrackingAccuracy::Normal, TRACKING_ACCURACY_NORMAL, "Normal"},
    {TrackingAccuracy::High,   TRACKING_ACCURACY_HIGH,   "High"}
  }};
}

} // end namespace Okao
} // end namespace Vision
} // end namespace Anki