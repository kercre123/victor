using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotSettingsPane : MonoBehaviour {

  [SerializeField]
  private Slider _VolumeSlider;

  [SerializeField]
  private Button _ToggleDebugString;

  [SerializeField]
  private GameObject _RobotStateTextFieldPrefab;

  private void Start() {
    _VolumeSlider.minValue = 0;
    _VolumeSlider.maxValue = 1;
    _VolumeSlider.value = 0.5f;
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      _VolumeSlider.value = robot.GetRobotVolume();
    }
    _VolumeSlider.onValueChanged.AddListener(OnVolumeSliderChanged);

    _ToggleDebugString.onClick.AddListener(OnToggleDebugString);
    
  }

  private void OnVolumeSliderChanged(float newValue) {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      robot.SetRobotVolume(_VolumeSlider.value);
    }
  }

  private void OnToggleDebugString() {
    GameObject debug_canvas = GameObject.Find("DebugMenuCanvas");
    if (debug_canvas != null) {
      RobotStateTextField old_instance = debug_canvas.GetComponentInChildren<RobotStateTextField>();
      if (old_instance == null) {
        UIManager.CreateUIElement(_RobotStateTextFieldPrefab, debug_canvas.transform);
      }
      else {
        Destroy(old_instance.gameObject);
      }
    }
    
  }

}
