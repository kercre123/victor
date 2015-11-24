using System.Collections.Generic;
using System;
using Anki.Cozmo;

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
        // NOTE: Initialization done here is used in the tool at runtime.
        // Duplicate initialization is done in FaceForm.cs, so that realistic values
        // are set as defaults when editing the tool.
        public const string NAME = "ProceduralFaceKeyFrame";

        public float faceAngle = 0f;
        public int faceCenterX = 0;
        public int faceCenterY = 0;
        public float faceScaleX = 1.0f;
        public float faceScaleY = 1.0f;

        public float[] leftEye;
        public float[] rightEye;

        public ProceduralFacePointData()
        {
            Name = NAME;

            leftEye = new float[(int)ProceduralEyeParameter.NumParameters];
            rightEye = new float[(int)ProceduralEyeParameter.NumParameters];

            // Init some parameters with nonzero values
            int curIndex = (int)ProceduralEyeParameter.EyeScaleX;
            leftEye[curIndex] = rightEye[curIndex] = 1.0f;

            curIndex = (int)ProceduralEyeParameter.EyeScaleY;
            leftEye[curIndex] = rightEye[curIndex] = 1.0f;

            // Make a quick list to init all the radius params to the same value
            var radiusParamList = new[]
            {
                (int)ProceduralEyeParameter.LowerInnerRadiusX,
                (int)ProceduralEyeParameter.LowerInnerRadiusY,
                (int)ProceduralEyeParameter.UpperInnerRadiusX,
                (int)ProceduralEyeParameter.UpperInnerRadiusY,
                (int)ProceduralEyeParameter.UpperOuterRadiusX,
                (int)ProceduralEyeParameter.UpperOuterRadiusY,
                (int)ProceduralEyeParameter.LowerOuterRadiusX,
                (int)ProceduralEyeParameter.LowerOuterRadiusY
            };

            foreach (var param in radiusParamList)
            {
                leftEye[param] = rightEye[param] = 0.5f;
            }
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
