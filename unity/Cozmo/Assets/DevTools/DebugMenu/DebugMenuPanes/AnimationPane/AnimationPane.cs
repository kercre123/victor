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
    List<Anki.Cozmo.AnimationTrigger> robotGroupNames = new List<Anki.Cozmo.AnimationTrigger>();
    for (int i = 0; i < (int)Anki.Cozmo.AnimationTrigger.Count; ++i) {
      Anki.Cozmo.AnimationTrigger ev = (Anki.Cozmo.AnimationTrigger)(i);
      robotGroupNames.Add(ev);
    }
    robotGroupNames.Sort();
    for (int i = 0; i < robotGroupNames.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = robotGroupNames[i].ToString();
      _AnimationDropdown.options.Add(optionData);
    }
    _AnimationDropdown.value = 0;
    _AnimationDropdown.RefreshShownValue();
    _PlayAnimationButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(robotGroupNames[_AnimationDropdown.value]));
  }
}
