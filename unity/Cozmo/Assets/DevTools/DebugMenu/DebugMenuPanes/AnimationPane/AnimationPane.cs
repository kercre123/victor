using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class AnimationPane : MonoBehaviour {

  [SerializeField]
  private Dropdown _AnimationDropdown;

  [SerializeField]
  private Button _PlayAnimationButton;

  [SerializeField]
  private Dropdown _SequenceDropdown;

  [SerializeField]
  private Button _PlaySequenceButton;

  // Use this for initialization
  void Start() {
    List<string> robotAnimationNames = RobotEngineManager.Instance.GetRobotAnimationNames();
    robotAnimationNames.Sort();
    for (int i = 0; i < robotAnimationNames.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = robotAnimationNames[i];
      _AnimationDropdown.options.Add(optionData);
    }
    _AnimationDropdown.value = 0;
    _AnimationDropdown.RefreshShownValue();
    _PlayAnimationButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.SendAnimation(robotAnimationNames[_AnimationDropdown.value]));


    List<string> sequenceNames = ScriptedSequences.ScriptedSequenceManager.Instance.GetSequenceNames();
    sequenceNames.Sort();
    for (int i = 0; i < sequenceNames.Count; ++i) {
      Dropdown.OptionData sequenceOptionData = new Dropdown.OptionData();
      sequenceOptionData.text = sequenceNames[i];
      _SequenceDropdown.options.Add(sequenceOptionData);
    }
    _SequenceDropdown.value = 0;
    _SequenceDropdown.RefreshShownValue();
    _PlaySequenceButton.onClick.AddListener(
      () => {
        ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(sequenceNames[_SequenceDropdown.value], true); 
      }
    );
  }
}
