using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;

public class RemoteControlPane : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoButton _Forward;

  [SerializeField]
  private Cozmo.UI.CozmoButton _Backwards;

  [SerializeField]
  private Cozmo.UI.CozmoButton _Left;

  [SerializeField]
  private Cozmo.UI.CozmoButton _Right;

  [SerializeField]
  private Slider _HeadAngle;

  [SerializeField]
  private Slider _LiftHeight;

  private float _Speed = 40.0f;

  // Use this for initialization
  void Start() {
    _Forward.onPress.AddListener(OnForward);
    _Forward.onRelease.AddListener(OnStop);

    _Backwards.onPress.AddListener(OnBackwards);
    _Backwards.onRelease.AddListener(OnStop);

    _Left.onPress.AddListener(OnLeft);
    _Left.onRelease.AddListener(OnStop);

    _Right.onPress.AddListener(OnRight);
    _Right.onRelease.AddListener(OnStop);

    _HeadAngle.minValue = CozmoUtil.kMinHeadAngle;
    _HeadAngle.maxValue = CozmoUtil.kMaxHeadAngle;

    _LiftHeight.minValue = 0.0f;
    _LiftHeight.maxValue = 1.0f;

    _HeadAngle.onValueChanged.AddListener(OnHeadValueChanged);
    _LiftHeight.onValueChanged.AddListener(OnLiftHeightChanged);
  }

  private void OnHeadValueChanged(float value) {
    RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(value * Mathf.Deg2Rad, useExactAngle: true);
  }

  private void OnLiftHeightChanged(float value) {
    RobotEngineManager.Instance.CurrentRobot.SetLiftHeight(value);
  }

  private void OnForward() {
    RobotEngineManager.Instance.CurrentRobot.DriveWheels(_Speed, _Speed);
  }

  private void OnBackwards() {
    RobotEngineManager.Instance.CurrentRobot.DriveWheels(-_Speed, -_Speed);
  }

  private void OnLeft() {
    RobotEngineManager.Instance.CurrentRobot.DriveWheels(-_Speed, _Speed);
  }

  private void OnRight() {
    RobotEngineManager.Instance.CurrentRobot.DriveWheels(_Speed, -_Speed);
  }

  private void OnStop() {
    RobotEngineManager.Instance.CurrentRobot.DriveWheels(0.0f, 0.0f);
  }
 
}
