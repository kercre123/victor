using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.Cozmo;
using System.Linq;
using System.Text;

public class CozmoFacePane : MonoBehaviour {

  [SerializeField] Slider _FaceCenterX;
  [SerializeField] Slider _FaceCenterY;
  [SerializeField] Slider _FaceScaleX;
  [SerializeField] Slider _FaceScaleY;
  [SerializeField] Slider _FaceAngle;

  [SerializeField] Slider _LeftEyeCenterX;
  [SerializeField] Slider _LeftEyeCenterY;
  [SerializeField] Slider _LeftEyeScaleX;
  [SerializeField] Slider _LeftEyeScaleY;
  [SerializeField] Slider _LeftEyeAngle;
  [SerializeField] Slider _LeftLowerInnerRadiusX;
  [SerializeField] Slider _LeftLowerInnerRadiusY;
  [SerializeField] Slider _LeftUpperInnerRadiusX;
  [SerializeField] Slider _LeftUpperInnerRadiusY;
  [SerializeField] Slider _LeftUpperOuterRadiusX;
  [SerializeField] Slider _LeftUpperOuterRadiusY;
  [SerializeField] Slider _LeftLowerOuterRadiusX;
  [SerializeField] Slider _LeftLowerOuterRadiusY;
  [SerializeField] Slider _LeftUpperLidY;
  [SerializeField] Slider _LeftUpperLidAngle;
  [SerializeField] Slider _LeftUpperLidBend;
  [SerializeField] Slider _LeftLowerLidY;
  [SerializeField] Slider _LeftLowerLidAngle;
  [SerializeField] Slider _LeftLowerLidBend;

  [SerializeField] Slider _RightEyeCenterX;
  [SerializeField] Slider _RightEyeCenterY;
  [SerializeField] Slider _RightEyeScaleX;
  [SerializeField] Slider _RightEyeScaleY;
  [SerializeField] Slider _RightEyeAngle;
  [SerializeField] Slider _RightLowerInnerRadiusX;
  [SerializeField] Slider _RightLowerInnerRadiusY;
  [SerializeField] Slider _RightUpperInnerRadiusX;
  [SerializeField] Slider _RightUpperInnerRadiusY;
  [SerializeField] Slider _RightUpperOuterRadiusX;
  [SerializeField] Slider _RightUpperOuterRadiusY;
  [SerializeField] Slider _RightLowerOuterRadiusX;
  [SerializeField] Slider _RightLowerOuterRadiusY;
  [SerializeField] Slider _RightUpperLidY;
  [SerializeField] Slider _RightUpperLidAngle;
  [SerializeField] Slider _RightUpperLidBend;
  [SerializeField] Slider _RightLowerLidY;
  [SerializeField] Slider _RightLowerLidAngle;
  [SerializeField] Slider _RightLowerLidBend;

  [SerializeField] Text _FaceCenterXLabel;
  [SerializeField] Text _FaceCenterYLabel;
  [SerializeField] Text _FaceScaleXLabel;
  [SerializeField] Text _FaceScaleYLabel;
  [SerializeField] Text _FaceAngleLabel;

  [SerializeField] Text _LeftEyeCenterXLabel;
  [SerializeField] Text _LeftEyeCenterYLabel;
  [SerializeField] Text _LeftEyeScaleXLabel;
  [SerializeField] Text _LeftEyeScaleYLabel;
  [SerializeField] Text _LeftEyeAngleLabel;
  [SerializeField] Text _LeftLowerInnerRadiusXLabel;
  [SerializeField] Text _LeftLowerInnerRadiusYLabel;
  [SerializeField] Text _LeftUpperInnerRadiusXLabel;
  [SerializeField] Text _LeftUpperInnerRadiusYLabel;
  [SerializeField] Text _LeftUpperOuterRadiusXLabel;
  [SerializeField] Text _LeftUpperOuterRadiusYLabel;
  [SerializeField] Text _LeftLowerOuterRadiusXLabel;
  [SerializeField] Text _LeftLowerOuterRadiusYLabel;
  [SerializeField] Text _LeftUpperLidYLabel;
  [SerializeField] Text _LeftUpperLidAngleLabel;
  [SerializeField] Text _LeftUpperLidBendLabel;
  [SerializeField] Text _LeftLowerLidYLabel;
  [SerializeField] Text _LeftLowerLidAngleLabel;
  [SerializeField] Text _LeftLowerLidBendLabel;

  [SerializeField] Text _RightEyeCenterXLabel;
  [SerializeField] Text _RightEyeCenterYLabel;
  [SerializeField] Text _RightEyeScaleXLabel;
  [SerializeField] Text _RightEyeScaleYLabel;
  [SerializeField] Text _RightEyeAngleLabel;
  [SerializeField] Text _RightLowerInnerRadiusXLabel;
  [SerializeField] Text _RightLowerInnerRadiusYLabel;
  [SerializeField] Text _RightUpperInnerRadiusXLabel;
  [SerializeField] Text _RightUpperInnerRadiusYLabel;
  [SerializeField] Text _RightUpperOuterRadiusXLabel;
  [SerializeField] Text _RightUpperOuterRadiusYLabel;
  [SerializeField] Text _RightLowerOuterRadiusXLabel;
  [SerializeField] Text _RightLowerOuterRadiusYLabel;
  [SerializeField] Text _RightUpperLidYLabel;
  [SerializeField] Text _RightUpperLidAngleLabel;
  [SerializeField] Text _RightUpperLidBendLabel;
  [SerializeField] Text _RightLowerLidYLabel;
  [SerializeField] Text _RightLowerLidAngleLabel;
  [SerializeField] Text _RightLowerLidBendLabel;

  [SerializeField] Button _SendToCozmoButton;

  private ProceduralEyeParameters _LeftEyeParameters = ProceduralEyeParameters.MakeDefaultLeftEye();

  private ProceduralEyeParameters _RightEyeParameters = ProceduralEyeParameters.MakeDefaultRightEye();

  private void UpdateSliders() {
    _RightEyeCenterX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterX];
    _RightEyeCenterY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterY];
    _RightEyeScaleX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleX];
    _RightEyeScaleY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleY];
    _RightEyeAngle.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeAngle];
    _RightLowerInnerRadiusX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusX];
    _RightLowerInnerRadiusY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusY];
    _RightUpperInnerRadiusX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusX];
    _RightUpperInnerRadiusY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusY];
    _RightUpperOuterRadiusX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusX];
    _RightUpperOuterRadiusY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusY];
    _RightLowerOuterRadiusX.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusX];
    _RightLowerOuterRadiusY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusY];
    _RightUpperLidY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidY];
    _RightUpperLidAngle.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidAngle];
    _RightUpperLidBend.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidBend];
    _RightLowerLidY.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidY];
    _RightLowerLidAngle.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidAngle];
    _RightLowerLidBend.value = _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidBend];

    _LeftEyeCenterX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterX];
    _LeftEyeCenterY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterY];
    _LeftEyeScaleX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleX];
    _LeftEyeScaleY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleY];
    _LeftEyeAngle.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeAngle];
    _LeftLowerInnerRadiusX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusX];
    _LeftLowerInnerRadiusY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusY];
    _LeftUpperInnerRadiusX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusX];
    _LeftUpperInnerRadiusY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusY];
    _LeftUpperOuterRadiusX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusX];
    _LeftUpperOuterRadiusY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusY];
    _LeftLowerOuterRadiusX.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusX];
    _LeftLowerOuterRadiusY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusY];
    _LeftUpperLidY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidY];
    _LeftUpperLidAngle.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidAngle];
    _LeftUpperLidBend.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidBend];
    _LeftLowerLidY.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidY];
    _LeftLowerLidAngle.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidAngle];
    _LeftLowerLidBend.value = _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidBend];

  }

  private void UpdateParameters() {
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterX] = _RightEyeCenterX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterY] = _RightEyeCenterY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleX] = _RightEyeScaleX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleY] = _RightEyeScaleY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.EyeAngle] = _RightEyeAngle.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusX] = _RightLowerInnerRadiusX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusY] = _RightLowerInnerRadiusY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusX] = _RightUpperInnerRadiusX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusY] = _RightUpperInnerRadiusY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusX] = _RightUpperOuterRadiusX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusY] = _RightUpperOuterRadiusY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusX] = _RightLowerOuterRadiusX.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusY] = _RightLowerOuterRadiusY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidY] = _RightUpperLidY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidAngle] = _RightUpperLidAngle.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidBend] = _RightUpperLidBend.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidY] = _RightLowerLidY.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidAngle] = _RightLowerLidAngle.value;
    _RightEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidBend] = _RightLowerLidBend.value;

    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterX] = _LeftEyeCenterX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeCenterY] = _LeftEyeCenterY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleX] = _LeftEyeScaleX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeScaleY] = _LeftEyeScaleY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.EyeAngle] = _LeftEyeAngle.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusX] = _LeftLowerInnerRadiusX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerInnerRadiusY] = _LeftLowerInnerRadiusY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusX] = _LeftUpperInnerRadiusX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperInnerRadiusY] = _LeftUpperInnerRadiusY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusX] = _LeftUpperOuterRadiusX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperOuterRadiusY] = _LeftUpperOuterRadiusY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusX] = _LeftLowerOuterRadiusX.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerOuterRadiusY] = _LeftLowerOuterRadiusY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidY] = _LeftUpperLidY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidAngle] = _LeftUpperLidAngle.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.UpperLidBend] = _LeftUpperLidBend.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidY] = _LeftLowerLidY.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidAngle] = _LeftLowerLidAngle.value;
    _LeftEyeParameters.Array[(int)ProceduralEyeParameter.LowerLidBend] = _LeftLowerLidBend.value;

  }

  private void UpdateLabels() {
    _FaceCenterXLabel.text = _FaceCenterX.value.ToString();
    _FaceCenterYLabel.text = _FaceCenterY.value.ToString();
    _FaceScaleXLabel.text = _FaceScaleX.value.ToString();
    _FaceScaleYLabel.text = _FaceScaleY.value.ToString();
    _FaceAngleLabel.text = _FaceAngle.value.ToString();

    _LeftEyeCenterXLabel.text = _LeftEyeCenterX.value.ToString();
    _LeftEyeCenterYLabel.text = _LeftEyeCenterY.value.ToString();
    _LeftEyeScaleXLabel.text = _LeftEyeScaleX.value.ToString();
    _LeftEyeScaleYLabel.text = _LeftEyeScaleY.value.ToString();
    _LeftEyeAngleLabel.text = _LeftEyeAngle.value.ToString();
    _LeftLowerInnerRadiusXLabel.text = _LeftLowerInnerRadiusX.value.ToString();
    _LeftLowerInnerRadiusYLabel.text = _LeftLowerInnerRadiusY.value.ToString();
    _LeftUpperInnerRadiusXLabel.text = _LeftUpperInnerRadiusX.value.ToString();
    _LeftUpperInnerRadiusYLabel.text = _LeftUpperInnerRadiusY.value.ToString();
    _LeftUpperOuterRadiusXLabel.text = _LeftUpperOuterRadiusX.value.ToString();
    _LeftUpperOuterRadiusYLabel.text = _LeftUpperOuterRadiusY.value.ToString();
    _LeftLowerOuterRadiusXLabel.text = _LeftLowerOuterRadiusX.value.ToString();
    _LeftLowerOuterRadiusYLabel.text = _LeftLowerOuterRadiusY.value.ToString();
    _LeftUpperLidYLabel.text = _LeftUpperLidY.value.ToString();
    _LeftUpperLidAngleLabel.text = _LeftUpperLidAngle.value.ToString();
    _LeftUpperLidBendLabel.text = _LeftUpperLidBend.value.ToString();
    _LeftLowerLidYLabel.text = _LeftLowerLidY.value.ToString();
    _LeftLowerLidAngleLabel.text = _LeftLowerLidAngle.value.ToString();
    _LeftLowerLidBendLabel.text = _LeftLowerLidBend.value.ToString();

    _RightEyeCenterXLabel.text = _RightEyeCenterX.value.ToString();
    _RightEyeCenterYLabel.text = _RightEyeCenterY.value.ToString();
    _RightEyeScaleXLabel.text = _RightEyeScaleX.value.ToString();
    _RightEyeScaleYLabel.text = _RightEyeScaleY.value.ToString();
    _RightEyeAngleLabel.text = _RightEyeAngle.value.ToString();
    _RightLowerInnerRadiusXLabel.text = _RightLowerInnerRadiusX.value.ToString();
    _RightLowerInnerRadiusYLabel.text = _RightLowerInnerRadiusY.value.ToString();
    _RightUpperInnerRadiusXLabel.text = _RightUpperInnerRadiusX.value.ToString();
    _RightUpperInnerRadiusYLabel.text = _RightUpperInnerRadiusY.value.ToString();
    _RightUpperOuterRadiusXLabel.text = _RightUpperOuterRadiusX.value.ToString();
    _RightUpperOuterRadiusYLabel.text = _RightUpperOuterRadiusY.value.ToString();
    _RightLowerOuterRadiusXLabel.text = _RightLowerOuterRadiusX.value.ToString();
    _RightLowerOuterRadiusYLabel.text = _RightLowerOuterRadiusY.value.ToString();
    _RightUpperLidYLabel.text = _RightUpperLidY.value.ToString();
    _RightUpperLidAngleLabel.text = _RightUpperLidAngle.value.ToString();
    _RightUpperLidBendLabel.text = _RightUpperLidBend.value.ToString();
    _RightLowerLidYLabel.text = _RightLowerLidY.value.ToString();
    _RightLowerLidAngleLabel.text = _RightLowerLidAngle.value.ToString();
    _RightLowerLidBendLabel.text = _RightLowerLidBend.value.ToString();
  }

  private void UpdateCozmoFaceMaterial() {
    CozmoFace.DisplayProceduralFace(
      _FaceAngle.value,
      new Vector2(_FaceCenterX.value, _FaceCenterY.value), 
      new Vector2(_FaceScaleX.value, _FaceScaleY.value),
      _LeftEyeParameters,
      _RightEyeParameters);
  }

  void Awake() {

    _SendToCozmoButton.onClick.AddListener(HandleSendFaceToCozmo);

    _FaceCenterX.minValue = -100;
    _FaceCenterX.maxValue = 100;
    _FaceCenterY.minValue = -100;
    _FaceCenterY.maxValue = 100;

    _FaceScaleX.minValue = 0f;
    _FaceScaleX.maxValue = 5f;
    _FaceScaleY.minValue = 0f;
    _FaceScaleY.maxValue = 5f;

    _FaceAngle.minValue = -180f;
    _FaceAngle.maxValue = 180f;

    // Right Eye
    _RightEyeCenterX.minValue = -100;
    _RightEyeCenterX.maxValue = 100;
    _RightEyeCenterY.minValue = -100;
    _RightEyeCenterY.maxValue = 100;

    _RightEyeScaleX.minValue = 0f;
    _RightEyeScaleX.maxValue = 5f;
    _RightEyeScaleY.minValue = 0f;
    _RightEyeScaleY.maxValue = 5f;

    _RightEyeAngle.minValue = -180f;
    _RightEyeAngle.maxValue = 180f;

    _RightLowerInnerRadiusX.minValue = 0;
    _RightLowerInnerRadiusX.maxValue = 1;
    _RightLowerInnerRadiusY.minValue = 0;
    _RightLowerInnerRadiusY.maxValue = 1;

    _RightUpperInnerRadiusX.minValue = 0;
    _RightUpperInnerRadiusX.maxValue = 1;
    _RightUpperInnerRadiusY.minValue = 0;
    _RightUpperInnerRadiusY.maxValue = 1;

    _RightUpperOuterRadiusX.minValue = 0;
    _RightUpperOuterRadiusX.maxValue = 1;
    _RightUpperOuterRadiusY.minValue = 0;
    _RightUpperOuterRadiusY.maxValue = 1;

    _RightLowerOuterRadiusX.minValue = 0;
    _RightLowerOuterRadiusX.maxValue = 1;
    _RightLowerOuterRadiusY.minValue = 0;
    _RightLowerOuterRadiusY.maxValue = 1;

    _RightLowerLidY.minValue = 0;
    _RightLowerLidY.maxValue = 1;

    _RightLowerLidAngle.minValue = -45;
    _RightLowerLidAngle.maxValue = 45;

    _RightLowerLidBend.minValue = 0;
    _RightLowerLidBend.maxValue = 1;

    _RightUpperLidY.minValue = 0;
    _RightUpperLidY.maxValue = 1;

    _RightUpperLidAngle.minValue = -45;
    _RightUpperLidAngle.maxValue = 45;

    _RightUpperLidBend.minValue = 0;
    _RightUpperLidBend.maxValue = 1;

    // Left Eye
    _LeftEyeCenterX.minValue = -100;
    _LeftEyeCenterX.maxValue = 100;
    _LeftEyeCenterY.minValue = -100;
    _LeftEyeCenterY.maxValue = 100;

    _LeftEyeScaleX.minValue = 0f;
    _LeftEyeScaleX.maxValue = 5f;
    _LeftEyeScaleY.minValue = 0f;
    _LeftEyeScaleY.maxValue = 5f;

    _LeftEyeAngle.minValue = -180f;
    _LeftEyeAngle.maxValue = 180f;

    _LeftLowerInnerRadiusX.minValue = 0;
    _LeftLowerInnerRadiusX.maxValue = 1;
    _LeftLowerInnerRadiusY.minValue = 0;
    _LeftLowerInnerRadiusY.maxValue = 1;

    _LeftUpperInnerRadiusX.minValue = 0;
    _LeftUpperInnerRadiusX.maxValue = 1;
    _LeftUpperInnerRadiusY.minValue = 0;
    _LeftUpperInnerRadiusY.maxValue = 1;

    _LeftUpperOuterRadiusX.minValue = 0;
    _LeftUpperOuterRadiusX.maxValue = 1;
    _LeftUpperOuterRadiusY.minValue = 0;
    _LeftUpperOuterRadiusY.maxValue = 1;

    _LeftLowerOuterRadiusX.minValue = 0;
    _LeftLowerOuterRadiusX.maxValue = 1;
    _LeftLowerOuterRadiusY.minValue = 0;
    _LeftLowerOuterRadiusY.maxValue = 1;

    _LeftLowerLidY.minValue = 0;
    _LeftLowerLidY.maxValue = 1;

    _LeftLowerLidAngle.minValue = -45;
    _LeftLowerLidAngle.maxValue = 45;

    _LeftLowerLidBend.minValue = 0;
    _LeftLowerLidBend.maxValue = 1;

    _LeftUpperLidY.minValue = 0;
    _LeftUpperLidY.maxValue = 1;

    _LeftUpperLidAngle.minValue = -45;
    _LeftUpperLidAngle.maxValue = 45;

    _LeftUpperLidBend.minValue = 0;
    _LeftUpperLidBend.maxValue = 1;

    _FaceScaleX.value = 1;
    _FaceScaleY.value = 1;
    UpdateSliders();

  }

  void Update() {
    UpdateParameters();
    UpdateLabels();
    UpdateCozmoFaceMaterial();
  }

  private void HandleSendFaceToCozmo() {
    var currentRobot = RobotEngineManager.Instance.CurrentRobot;

    if (currentRobot != null) {
      currentRobot.DisplayProceduralFace(_FaceAngle.value, 
        new Vector2(_FaceCenterX.value, _FaceCenterY.value), 
        new Vector2(_FaceScaleX.value, _FaceScaleY.value), 
        _LeftEyeParameters, 
        _RightEyeParameters);
    }

    PrintCode();
  }

  private void PrintCode() {
    StringBuilder leftSB = new StringBuilder(), rightSB = new StringBuilder(), fullSB = new StringBuilder();

    fullSB.AppendLine("float faceAngle = " + _FaceAngle.value + "f;");
    fullSB.AppendLine("Vector2 faceCenter = new Vector2(" + _FaceCenterX.value + "f, " + _FaceCenterY.value + "f);");

    var props = typeof(ProceduralEyeParameters).GetProperties();

    leftSB.AppendLine("var leftEye = new ProceduralEyeParameters() {");
    rightSB.AppendLine("var rightEye = new ProceduralEyeParameters() {");

    foreach (var prop in props) {
      string leftLine = null, rightLine = null;
      if (prop.PropertyType == typeof(float)) {
        leftLine = "\t" + prop.Name + " = " + prop.GetValue(_LeftEyeParameters, null) + "f,";
        rightLine = "\t" + prop.Name + " = " + prop.GetValue(_RightEyeParameters, null) + "f,";
      }
      if (prop.PropertyType == typeof(Vector2)) {
        var leftVector = (Vector2)prop.GetValue(_LeftEyeParameters, null);
        var rightVector = (Vector2)prop.GetValue(_RightEyeParameters, null);

        leftLine = "\t" + prop.Name + " = new Vector2(" + leftVector.x + "f, " + leftVector.y + "f),";
        rightLine = "\t" + prop.Name + " = new Vector2(" + rightVector.x + "f, " + rightVector.y + "f),";
      }
      if (!string.IsNullOrEmpty(leftLine)) {
        leftSB.AppendLine(leftLine);
        rightSB.AppendLine(rightLine);
      }
    }

    leftSB.AppendLine("};");
    rightSB.AppendLine("};");

    fullSB.AppendLine(leftSB.ToString());
    fullSB.AppendLine(rightSB.ToString());

    DAS.Debug(this, fullSB.ToString());
  }
}
