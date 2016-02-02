using System;
using System.Collections.Generic;

namespace CozmoAnimation {
  public enum KeyFrameName {
    LiftHeightKeyFrame,
    HeadAngleKeyFrame,
    ProceduralFaceKeyFrame,
    RobotAudioKeyFrame,
    BodyMotionKeyFrame
  }

  public class KeyFrame {
    public float durationTime_ms;
    public float triggerTime_ms;
    public float faceCenterY;
    public float faceCenterX;
    public KeyFrameName Name;
    public float faceScaleY;
    public float faceScaleX;
    public float faceAngle;
    public float[] leftEye;
    public float[] rightEye;
  }

}
