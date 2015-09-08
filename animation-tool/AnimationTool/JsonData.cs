using System.Collections.Generic;
using System;

namespace AnimationTool
{
    [Serializable]
    public class PointData
    {
        public int triggerTime_ms;
        public int durationTime_ms;
        public string Name;
    }

    [Serializable]
    public class HeadAnglePointData : PointData
    {
        public int angle_deg;
        public int angleVariability_deg;
        public const string NAME = "HeadAngleKeyFrame";

        public HeadAnglePointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class LiftHeightPointData : PointData
    {
        public int height_mm;
        public int heightVariability_mm;
        public const string NAME = "LiftHeightKeyFrame";

        public LiftHeightPointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class AudioRobotPointData : PointData
    {
        public List<string> audioName;
        public string pathFromRoot;
        public double volume;
        public const string NAME = "RobotAudioKeyFrame";

        public AudioRobotPointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class AudioDevicePointData : AudioRobotPointData
    {
        public new const string NAME = "DeviceAudioKeyFrame";

        public AudioDevicePointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class FaceAnimationPointData : PointData
    {
        public string animName;
        public string pathFromRoot;
        public const string NAME = "FaceAnimationKeyFrame";

        public FaceAnimationPointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class ProceduralFacePointData : PointData
    {
        public const string NAME = "ProceduralFaceKeyFrame";

        public float faceAngle_deg;

        public float leftBrowAngle;
        public float rightBrowAngle;
        public float leftBrowCenX;
        public float rightBrowCenX;
        public float leftBrowCenY;
        public float rightBrowCenY;

        public float leftEyeHeight;
        public float rightEyeHeight;
        public float leftEyeWidth;
        public float rightEyeWidth;

        public float leftPupilHeight;
        public float rightPupilHeight;
        public float leftPupilWidth;
        public float rightPupilWidth;
        public float leftPupilCenX;
        public float rightPupilCenX;
        public float leftPupilCenY;
        public float rightPupilCenY;

        public ProceduralFacePointData()
        {
            Name = NAME;
        }
    }

    [Serializable]
    public class BodyMotionPointData : PointData
    {
        public object radius_mm;
        public double speed;
        public const string NAME = "BodyMotionKeyFrame";

        public BodyMotionPointData()
        {
            Name = NAME;
        }
    }
}
