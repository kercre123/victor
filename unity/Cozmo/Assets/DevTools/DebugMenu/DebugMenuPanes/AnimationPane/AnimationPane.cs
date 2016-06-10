using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class AnimationPane : MonoBehaviour {

  [SerializeField]
  private Dropdown _AnimationDropdown;

  [SerializeField]
  private Button _PlayAnimationButton;


  // Use this for initialization
  void Start() {
    List<string> robotGroupNames = new List<string>();
    foreach (string aGroup in AnimationManager.Instance.AnimationGroupDict.Values) {
      robotGroupNames.Add(aGroup);
    }
    robotGroupNames.Sort();
    for (int i = 0; i < robotGroupNames.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = robotGroupNames[i];
      _AnimationDropdown.options.Add(optionData);
    }
    _AnimationDropdown.value = 0;
    _AnimationDropdown.RefreshShownValue();
    _PlayAnimationButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.SendAnimationGroup(robotGroupNames[_AnimationDropdown.value]));
  }
}
