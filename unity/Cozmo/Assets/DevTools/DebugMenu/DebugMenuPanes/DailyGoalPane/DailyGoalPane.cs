using UnityEngine;
using System.Collections;
using DataPersistence;
using Cozmo.UI;

public class DailyGoalPane : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.Button _ResetGoalButton;

  [SerializeField]
  UnityEngine.UI.Button _ProgressGoalButton;

  [SerializeField]
  UnityEngine.UI.Button _UndoProgressGoalButton;

  [SerializeField]
  UnityEngine.UI.Dropdown _GoalListDropdown;

  [SerializeField]
  UnityEngine.UI.InputField _SetProgressInputField;

  [SerializeField]
  UnityEngine.UI.Button _SetProgressGoalButton;

  private TimelineEntryData _CurrentSession;

  private DailyGoal _CurrentGoal;

  void Start() {
    _CurrentSession = DataPersistenceManager.Instance.CurrentSession;
    if (_CurrentSession == null) {
      Debug.LogError("No Current Session for DailyGoal Debug");
      _CurrentSession = null;
    }

    _ResetGoalButton.onClick.AddListener(HandleResetGoalClicked);
    _ProgressGoalButton.onClick.AddListener(HandleProgressGoalClicked);
    _UndoProgressGoalButton.onClick.AddListener(HandleUndoProgressGoalClicked);

    _SetProgressInputField.text = "0";
    _SetProgressInputField.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _SetProgressGoalButton.onClick.AddListener(HandleSetProgressGoalClicked);

    _GoalListDropdown.ClearOptions();
    _GoalListDropdown.AddOptions(DailyGoalManager.Instance.GetCurrentDailyGoalNames());
    UpdateCurrentGoal(0);
    _GoalListDropdown.onValueChanged.AddListener(UpdateCurrentGoal);
  }

  private void HandleResetGoalClicked() {
    if (_CurrentSession == null) {
      return;
    }
    _CurrentGoal.DebugResetGoalProgress();
  }

  private void HandleProgressGoalClicked() {
    if (_CurrentSession == null) {
      return;
    }
    _CurrentGoal.ProgressGoal(_CurrentGoal.GoalEvent);
  }

  private void HandleUndoProgressGoalClicked() {
    if (_CurrentSession == null) {
      return;
    }
    _CurrentGoal.DebugUndoGoalProgress();
  }

  private void HandleSetProgressGoalClicked() {
    if (_CurrentSession == null) {
      return;
    }
    int validInt = int.Parse(_SetProgressInputField.text);
    if (validInt < 0) {
      validInt = 0;
      _SetProgressInputField.text = "0";
    }
    _CurrentGoal.DebugSetGoalProgress(validInt);
  }

  private void UpdateCurrentGoal(int eventInt) {
    _CurrentGoal = GetDailyGoalByName(_GoalListDropdown.captionText.text);
  }

  private DailyGoal GetDailyGoalByName(string name) {
    if (_CurrentSession == null) {
      return null;
    }
    return _CurrentSession.DailyGoals.Find(x => x.Title == name);
  }

}
