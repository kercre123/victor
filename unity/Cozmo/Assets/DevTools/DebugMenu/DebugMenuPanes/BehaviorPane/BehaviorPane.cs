using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class BehaviorPane : MonoBehaviour {
  
  [SerializeField]
  private Dropdown _ChooserDropdown;

  [SerializeField]
  private Button _ChooserButton;

  [SerializeField]
  private Dropdown _BehaviorDropdown;

  [SerializeField]
  private Button _BehaviorButton;

  // Use this for initialization
  void Start() {
    _ChooserButton.onClick.AddListener(OnChooserButton);
    _BehaviorButton.onClick.AddListener(OnBehaviorButton);

    for (int i = 0; i < (int)Anki.Cozmo.BehaviorChooserType.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = ((Anki.Cozmo.BehaviorChooserType)i).ToString();
      _ChooserDropdown.options.Add(optionData);
    }

    for (int i = 0; i < (int)Anki.Cozmo.BehaviorType.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = ((Anki.Cozmo.BehaviorType)i).ToString();
      _BehaviorDropdown.options.Add(optionData);
    }
  }

  private void OnChooserButton() {
    Anki.Cozmo.BehaviorChooserType chooserType = (Anki.Cozmo.BehaviorChooserType)System.Enum.Parse(typeof(Anki.Cozmo.BehaviorChooserType), _ChooserDropdown.options[_ChooserDropdown.value].text);
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(chooserType);
  }

  private void OnBehaviorButton() {
    Anki.Cozmo.BehaviorType behaviorType = (Anki.Cozmo.BehaviorType)System.Enum.Parse(typeof(Anki.Cozmo.BehaviorType), _BehaviorDropdown.options[_BehaviorDropdown.value].text);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(behaviorType);
  }
}
