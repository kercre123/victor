using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using Viz = Anki.Cozmo.Viz;

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

  [SerializeField]
  private Dropdown _ReactionTriggerDropdown;

  [SerializeField]
  private Dropdown _ReactionTriggerBehaviorDropdown;

  [SerializeField]
  private Button _ReactionTriggerButton;

  private List<List<string>> _ReactionTriggerBehaviorMap;
  // Use this for initialization
  void Start() {
    Viz.VizManager.Enabled = true;

    _ChooserDropdown.onValueChanged.AddListener(OnChooserDropdownChanged);
    _ReactionTriggerDropdown.onValueChanged.AddListener(OnReactionTriggerDropdownChanged);
    _ChooserButton.onClick.AddListener(OnChooserButton);
    _BehaviorButton.onClick.AddListener(OnBehaviorButton);
    _BehaviorByNameButton.onClick.AddListener(OnBehaviorByNameButton);
    _ReactionTriggerButton.onClick.AddListener(OnReactionTriggerButton);


    _ChooserDropdown.ClearOptions();
    for (int i = 0; i < (int)HighLevelActivity.Count; ++i) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = ((HighLevelActivity)i).ToString();
      _ChooserDropdown.options.Add(optionData);
    }
    _ChooserDropdown.captionText.text = _ChooserDropdown.options[0].text;

    RobotEngineManager.Instance.AddCallback<G2U.RespondAllBehaviorsList>(HandleRespondAllBehaviorsList);
    RobotEngineManager.Instance.AddCallback<G2U.RespondReactionTriggerMap>(HandleRespondReactionTriggerMap);

    _ReactionTriggerBehaviorDropdown.interactable = false;
    _ReactionTriggerDropdown.interactable = false;
    _ReactionTriggerButton.interactable = false;
    _ReactionTriggerBehaviorMap = new List<List<string>>();

    OnChooserDropdownChanged(0);
  }


  void OnDestroy() {
    Viz.VizManager.Enabled = false;
    RobotEngineManager.Instance.RemoveCallback<G2U.RespondAllBehaviorsList>(HandleRespondAllBehaviorsList);
    RobotEngineManager.Instance.RemoveCallback<G2U.RespondReactionTriggerMap>(HandleRespondReactionTriggerMap);
  }

  private void Update() {
    _InfoLabel.text = string.Format("Current Behavior: {0}\n" +
    "Recent Mood Events: {1}\n" +
    "Last Reaction Trigger: {2}\n",
      Viz.VizManager.Instance.BehaviorID.ToString(),
      Viz.VizManager.Instance.RecentMoodEvents != null ?
                                    string.Join(", ", Viz.VizManager.Instance.RecentMoodEvents) : "",
      Viz.VizManager.Instance.Reaction);
  }

  private void HandleRespondAllBehaviorsList(G2U.RespondAllBehaviorsList message) {
    List<BehaviorID> behaviorList = new List<BehaviorID>(message.behaviors);
    behaviorList.Sort(delegate (BehaviorID s1, BehaviorID s2) {
      if (s1 == BehaviorID.NoneBehavior) {
        if (s2 == BehaviorID.NoneBehavior) {
          return 0;
        }
        return -1;
      }
      if (s2 == BehaviorID.NoneBehavior) {
        return 1;
      }
      return s1.CompareTo(s2);
    });
    foreach (BehaviorID behaviorID in behaviorList) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = behaviorID.ToString();
      _BehaviorDropdown.options.Add(optionData);
    }

    _BehaviorDropdown.value = 0;
    if (message.behaviors.Length > 0) {
      _BehaviorDropdown.captionText.text = _BehaviorDropdown.options[0].text;
      _BehaviorButton.interactable = true;
      _BehaviorDropdown.interactable = true;
      _BehaviorByNameButton.interactable = true;
      _BehaviorNameInput.interactable = true;
    }
    else {
      _BehaviorDropdown.captionText.text = "";
    }
  }

  private void HandleRespondReactionTriggerMap(G2U.RespondReactionTriggerMap message) {
    List<string> reactionTriggers = new List<string>();

    _ReactionTriggerDropdown.ClearOptions();
    _ReactionTriggerBehaviorDropdown.ClearOptions();

    //  allocate trigger mapping and dropdown setup
    _ReactionTriggerBehaviorMap.Clear();
    for (int i = 0; i < ((int)ReactionTrigger.Count); ++i) {
      _ReactionTriggerBehaviorMap.Add(new List<string>());
      reactionTriggers.Add(((ReactionTrigger)i).ToString());
    }

    reactionTriggers.Sort();
    foreach (string s in reactionTriggers) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = s;
      _ReactionTriggerDropdown.options.Add(optionData);
    }

    //  set our trigger behavior map values
    foreach (ReactionTriggerToBehavior reactionTriggerEntry in message.reactionTriggerEntries) {
      _ReactionTriggerBehaviorMap[(int)reactionTriggerEntry.trigger].Add(reactionTriggerEntry.behaviorID.ToString());
    }

    _ReactionTriggerDropdown.value = 0;

    if (message.reactionTriggerEntries.Length > 0) {
      _ReactionTriggerDropdown.captionText.text = _ReactionTriggerDropdown.options[0].text;
      _ReactionTriggerDropdown.interactable = true;
    }
    else {
      _ReactionTriggerDropdown.captionText.text = "";
    }

    OnReactionTriggerDropdownChanged(0);
  }

  private void OnChooserButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      HighLevelActivity activityType  = (HighLevelActivity)_ChooserDropdown.value;
      if (activityType == HighLevelActivity.Selection) {
        RobotEngineManager.Instance.CurrentRobot.ActivateHighLevelActivity(activityType);
      }
      if (_ReactionTriggerBehaviorMap.Count == 0) {
        RobotEngineManager.Instance.CurrentRobot.RequestReactionTriggerMap();
      }
    }
  }

  private void OnChooserDropdownChanged(int new_value) {
    _BehaviorDropdown.ClearOptions();
    _BehaviorButton.interactable = false;
    _BehaviorDropdown.interactable = false;
    _BehaviorByNameButton.interactable = false;
    _BehaviorNameInput.interactable = false;

    if (RobotEngineManager.Instance.CurrentRobot != null) {
      // We only allow unity to set behaviors when in selection activity
      HighLevelActivity activityType  = (HighLevelActivity)_ChooserDropdown.value;
      if (activityType == HighLevelActivity.Selection) {
        RobotEngineManager.Instance.CurrentRobot.RequestAllBehaviorsList();
      }
    }

    OnChooserButton();
  }

  private void OnBehaviorButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      BehaviorID behaviorID = (BehaviorID)Enum.Parse(typeof(BehaviorID), _BehaviorDropdown.captionText.text);
      RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByID(behaviorID);
    }
  }

  private void OnBehaviorByNameButton() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      BehaviorID behaviorID = (BehaviorID)Enum.Parse(typeof(BehaviorID), _BehaviorNameInput.text);
      RobotEngineManager.Instance.CurrentRobot.ExecuteBehaviorByID(behaviorID, 1);
    }
  }

  private void OnReactionTriggerDropdownChanged(int new_value) {
    //  the reaction triggers aren't mapped to dropdown index (since it's sorted for QA ease of use)
    string reactionTriggerText = _ReactionTriggerDropdown.options[_ReactionTriggerDropdown.value].text;
    int currentTriggerIndex = (int)(ReactionTrigger)System.Enum.Parse(typeof(ReactionTrigger), reactionTriggerText);
    _ReactionTriggerBehaviorDropdown.ClearOptions();

    //  repopulate the dropdown w/ the current trigger's behaviors
    foreach (string behaviorName in _ReactionTriggerBehaviorMap[currentTriggerIndex]) {
      Dropdown.OptionData optionData = new Dropdown.OptionData();
      optionData.text = behaviorName;
      _ReactionTriggerBehaviorDropdown.options.Add(optionData);
    }

    //  enable/disable the buttons and initialize the dropdown
    _ReactionTriggerBehaviorDropdown.value = 0;
    _ReactionTriggerBehaviorDropdown.interactable = _ReactionTriggerBehaviorDropdown.options.Count > 1;
    _ReactionTriggerButton.interactable = _ReactionTriggerBehaviorDropdown.options.Count > 0;
    if (_ReactionTriggerButton.interactable) {
      _ReactionTriggerBehaviorDropdown.captionText.text = _ReactionTriggerBehaviorDropdown.options[0].text;
    }
    else {
      _ReactionTriggerDropdown.captionText.text = "";
    }
  }

  private void OnReactionTriggerButton() {
    if (_ReactionTriggerBehaviorDropdown.options.Count > 0) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        int dropdownIndex = _ReactionTriggerBehaviorDropdown.value;
        ReactionTriggerToBehavior toExecuteTrigger = new ReactionTriggerToBehavior();

        BehaviorID behaviorID = (BehaviorID)Enum.Parse(typeof(BehaviorID), 
                          _ReactionTriggerBehaviorDropdown.options[dropdownIndex].text);
        toExecuteTrigger.Initialize((ReactionTrigger)_ReactionTriggerDropdown.value, behaviorID);

        RobotEngineManager.Instance.CurrentRobot.ExecuteReactionTrigger(toExecuteTrigger);
      }
    }
  }
}
