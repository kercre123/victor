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

  [SerializeField]
  private Text _InfoLabel;

  // Use this for initialization
  void Start() {
    _ChooserButton.onClick.AddListener(OnChooserButton);
    _BehaviorButton.onClick.AddListener(OnBehaviorButton);
    Anki.Cozmo.Viz.VizManager.Enabled = true;

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

  void OnDestroy() {
    Anki.Cozmo.Viz.VizManager.Enabled = false;
  }

  private void Update() {
    _InfoLabel.text = string.Format("Current Behavior: {0}\n" +
    "Recent Mood Events: {1}\n",
      Anki.Cozmo.Viz.VizManager.Instance.Behavior,
      Anki.Cozmo.Viz.VizManager.Instance.RecentMoodEvents != null ? string.Join(", ", Anki.Cozmo.Viz.VizManager.Instance.RecentMoodEvents) : "");
  }

  private void OnChooserButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      Anki.Cozmo.BehaviorChooserType chooserType = (Anki.Cozmo.BehaviorChooserType)System.Enum.Parse(typeof(Anki.Cozmo.BehaviorChooserType), _ChooserDropdown.options[_ChooserDropdown.value].text);
      RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(chooserType);
    }
  }

  private void OnBehaviorButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      Anki.Cozmo.BehaviorType behaviorType = (Anki.Cozmo.BehaviorType)System.Enum.Parse(typeof(Anki.Cozmo.BehaviorType), _BehaviorDropdown.options[_BehaviorDropdown.value].text);
      RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(behaviorType);
    }
  }
}
