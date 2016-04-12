using UnityEngine;
using System.Collections;

public class CalibrationPane : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Button _SetCalibrationButton;

  [SerializeField]
  private UnityEngine.UI.InputField _FocalLengthX;

  [SerializeField]
  private UnityEngine.UI.InputField _FocalLengthY;

  [SerializeField]
  private UnityEngine.UI.InputField _CenterX;

  [SerializeField]
  private UnityEngine.UI.InputField _CenterY;

  // Use this for initialization
  void Start() {
    _SetCalibrationButton.onClick.AddListener(OnSetCalibration);
    _FocalLengthX.contentType = UnityEngine.UI.InputField.ContentType.DecimalNumber;
    _FocalLengthY.contentType = UnityEngine.UI.InputField.ContentType.DecimalNumber;
    _CenterX.contentType = UnityEngine.UI.InputField.ContentType.DecimalNumber;
    _CenterY.contentType = UnityEngine.UI.InputField.ContentType.DecimalNumber;
  }

  private void OnSetCalibration() {
    float focalLengthX = float.Parse(_FocalLengthX.text);
    float focalLengthY = float.Parse(_FocalLengthY.text);
    float centerX = float.Parse(_CenterX.text);
    float centerY = float.Parse(_CenterY.text);

    RobotEngineManager.Instance.CurrentRobot.SetCalibrationData(focalLengthX, focalLengthY, centerX, centerY);

  }
}
