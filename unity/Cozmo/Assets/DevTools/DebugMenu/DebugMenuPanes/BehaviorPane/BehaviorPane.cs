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


  [SerializeField]
  private Button _BehaviorByNameButton;

  [SerializeField]
  private InputField _BehaviorNameInput;

  // Use this for initialization
  void Start() {
    _ChooserButton.onClick.AddListener(OnChooserButton);
    _BehaviorButton.onClick.AddListener(OnBehaviorButton);
    _BehaviorByNameButton.onClick.AddListener(OnBehaviorByNameButton);
    Anki.Cozmo.Viz.VizManager.Enabled = true;

    for (int i = 0; i < (int)Anki.Cozmo.BehaviorChooserType.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = ((Anki.Cozmo.BehaviorChooserType)i).ToString();
      _ChooserDropdown.options.Add(optionData);
    }

    for (int i = 0; i < (int)Anki.Cozmo.BehaviorClass.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = ((Anki.Cozmo.BehaviorClass)i).ToString();
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
      Anki.Cozmo.ExecutableBehaviorType behaviorType = (Anki.Cozmo.ExecutableBehaviorType)System.Enum.Parse(typeof(Anki.Cozmo.ExecutableBehaviorType), _BehaviorDropdown.options[_BehaviorDropdown.value].text);
      RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByExecutableType(behaviorType);
    }
  }

  private void OnBehaviorByNameButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByName(_BehaviorNameInput.text);
    }
  }
}
