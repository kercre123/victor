using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotSettingsPane : MonoBehaviour {

  [SerializeField]
  private Slider volumeSlider_;

	private void Start(){
    volumeSlider_.minValue = 0;
    volumeSlider_.maxValue = 1;
    volumeSlider_.value = 0.5f;
    Robot robot = RobotEngineManager.instance.CurrentRobot;
    if (robot != null) {
      volumeSlider_.value = robot.GetRobotVolume();
    }
    volumeSlider_.onValueChanged.AddListener(OnVolumeSliderChanged);
  }

  private void OnVolumeSliderChanged(float newValue){
    Robot robot = RobotEngineManager.instance.CurrentRobot;
    if (robot != null) {
      robot.SetRobotVolume(volumeSlider_.value);
    }
  }
}
