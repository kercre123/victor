using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class AnimationPane : MonoBehaviour {

  [SerializeField]
  private Dropdown _Dropdown;

  [SerializeField]
  private Button _PlayButton;

  // Use this for initialization
  void Start() {
    List<string> robotAnimationNames = RobotEngineManager.Instance.GetRobotAnimationNames();
    robotAnimationNames.Sort();
    for (int i = 0; i < robotAnimationNames.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = robotAnimationNames[i];
      _Dropdown.options.Add(optionData);
    }
    _Dropdown.value = 0;
    _PlayButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.SendAnimation(robotAnimationNames[_Dropdown.value]));
  }
}
