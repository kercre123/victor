using UnityEngine;
using System.Collections.Generic;
using Cozmo.UI;

public class ProfileTab : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _PlayerName;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _TimeWithCozmoCountValueLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _DailyGoalsCompletedCount;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _StreaksCountValueLabel;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EditNameButton;

  [SerializeField]
  private Cozmo.UI.BaseView _ProfileEditNameViewPrefab;
  private Cozmo.UI.BaseView _ProfileEditNameViewInstance;

  // Use this for initialization
  private void Awake() {
    _PlayerName.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName;

    int daysWithCozmo = GetDaysWithCozmo();
    _TimeWithCozmoCountValueLabel.text = daysWithCozmo.ToString();

    _DailyGoalsCompletedCount.text = TotalDailyGoalsCompleted().ToString();
    _StreaksCountValueLabel.text = GetLongestStreak().ToString();

    _EditNameButton.Initialize(HandleEditNameButton, "edit_name_button", "profile_tab");
  }

  private int GetLongestStreak() {
    int longestStreak = 1;
    int streakCounter = 1;
    for (int i = 1; i < DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Count; ++i) {
      int prevIndex = i - 1;
      int currIndex = i;
      DataPersistence.Date prevDate = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions[prevIndex].Date;
      DataPersistence.Date currDate = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions[currIndex].Date;

      if (prevDate.OffsetDays(1) == currDate) {
        // we have a streak!
        streakCounter++;
        if (streakCounter > longestStreak) {
          longestStreak = streakCounter;
        }
      }
      else {
        // we lost a streak...
        streakCounter = 1;
      }

    }
    return longestStreak;
  }

  private void HandleEditNameButton() {
    _ProfileEditNameViewInstance = UIManager.OpenView(_ProfileEditNameViewPrefab);
    _ProfileEditNameViewInstance.GetComponent<EnterNameSlide>().SetNameInputField(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
    _ProfileEditNameViewInstance.GetComponent<EnterNameSlide>().RegisterInputFocus();
    _ProfileEditNameViewInstance.GetComponent<EnterNameSlide>().OnNameEntered += HandleProfileEditNameDone;
  }

  private void HandleProfileEditNameDone(string newName) {
    _PlayerName.text = newName;
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName = newName;
    DataPersistence.DataPersistenceManager.Instance.Save();
    UIManager.CloseView(_ProfileEditNameViewInstance);
  }

  private void OnDestroy() {
    if (_ProfileEditNameViewInstance != null) {
      UIManager.CloseView(_ProfileEditNameViewInstance);
    }
  }

  private int GetDaysWithCozmo() {
    int daysWithCozmo = 0;
    foreach (DataPersistence.TimelineEntryData sessionEntry in DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions) {
      if (sessionEntry.HasConnectedToCozmo) {
        daysWithCozmo++;
      }
    }
    return daysWithCozmo;
  }

  private int TotalDailyGoalsCompleted() {
    int dailyGoalsCompleted = 0;
    foreach (DataPersistence.TimelineEntryData sessionEntry in DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions) {
      foreach (DailyGoal goal in sessionEntry.DailyGoals) {
        if (goal.GoalComplete) {
          dailyGoalsCompleted++;
        }
      }
    }
    return dailyGoalsCompleted;
  }

}
