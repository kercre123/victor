using UnityEngine;
using System.Collections;
using DataPersistence;
using Cozmo.UI;
using Anki.Cozmo;

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

  [SerializeField]
  UnityEngine.UI.Button _GenerateFreshGoalsButton;

  [SerializeField]
  UnityEngine.UI.Button _ClearGoalsButton;

  [SerializeField]
  UnityEngine.UI.Dropdown _GenGoalsDropdown;

  [SerializeField]
  UnityEngine.UI.Button _GenerateSpecificGoalButton;

  private TimelineEntryData _CurrentSession;

  private DailyGoal _CurrentGoal;
  private DailyGoalGenerationData.GoalEntry _CurrentGenData;

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

    _GenerateFreshGoalsButton.onClick.AddListener(HandleGenerateFreshGoalsClicked);
    _ClearGoalsButton.onClick.AddListener(HandleClearGoalsClicked);
    _GenerateSpecificGoalButton.onClick.AddListener(HandleGenSpecificGoalClicked);

    _GoalListDropdown.ClearOptions();
    _GoalListDropdown.AddOptions(DailyGoalManager.Instance.GetCurrentDailyGoalNames());
    UpdateCurrentGoal(0);
    _GoalListDropdown.onValueChanged.AddListener(UpdateCurrentGoal);

    _GenGoalsDropdown.ClearOptions();
    _GenGoalsDropdown.AddOptions(DailyGoalManager.Instance.GetGeneratableGoalNames());
    UpdateCurrentGen(0);
    _GenGoalsDropdown.onValueChanged.AddListener(UpdateCurrentGen);
    DailyGoalManager.Instance.OnRefreshDailyGoals += RefreshOptions;
  }

  void Destroy() {
    DailyGoalManager.Instance.OnRefreshDailyGoals -= RefreshOptions;
  }

  void RefreshOptions() {

    _GoalListDropdown.ClearOptions();
    _GoalListDropdown.AddOptions(DailyGoalManager.Instance.GetCurrentDailyGoalNames());
    UpdateCurrentGoal(0);
    _GoalListDropdown.onValueChanged.AddListener(UpdateCurrentGoal);

    _GenGoalsDropdown.ClearOptions();
    _GenGoalsDropdown.AddOptions(DailyGoalManager.Instance.GetGeneratableGoalNames());
    UpdateCurrentGen(0);
    _GenGoalsDropdown.onValueChanged.AddListener(UpdateCurrentGen);
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
    _CurrentGoal.DebugAddGoalProgress();
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

  private void HandleGenerateFreshGoalsClicked() {
    DataPersistenceManager.Instance.CurrentSession.DailyGoals.Clear();
    DataPersistenceManager.Instance.CurrentSession.DailyGoals.AddRange(DailyGoalManager.Instance.GenerateDailyGoals());
    RefreshGoals();
  }

  private void HandleClearGoalsClicked() {
    DataPersistenceManager.Instance.CurrentSession.DailyGoals.Clear();
    RefreshGoals();
  }

  private void HandleGenSpecificGoalClicked() {
    DataPersistenceManager.Instance.CurrentSession.DailyGoals.Add(new DailyGoal(_CurrentGenData));
    RefreshGoals();
  }

  private void RefreshGoals() {
    if (DailyGoalManager.Instance.OnRefreshDailyGoals != null) {
      DailyGoalManager.Instance.OnRefreshDailyGoals.Invoke();
    }
  }

  private void UpdateCurrentGoal(int eventInt) {
    _CurrentGoal = GetDailyGoalByName(_GoalListDropdown.captionText.text);
  }

  private void UpdateCurrentGen(int eventInt) {
    _CurrentGenData = GetGoalEntryByLocalizedName(_GenGoalsDropdown.captionText.text);
  }

  private DailyGoal GetDailyGoalByName(string gname) {
    if (_CurrentSession == null) {
      return null;
    }
    return _CurrentSession.DailyGoals.Find(x => x.Title == gname);
  }

  private DailyGoalGenerationData.GoalEntry GetGoalEntryByLocalizedName(string gname) {
    return DailyGoalManager.Instance.GetGeneratableGoalEntries().Find(x => Localization.Get(x.TitleKey) == gname);
  }

}
