using UnityEngine;
using System.Collections.Generic;

public class ProfileTab : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _PlayerName;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _TimeWithCozmoCountValueLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _GamesPlayCountValueLabel;

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

    // force compute / update the time
    PlayTimeManager.Instance.ComputeAndSetPlayTime();
    System.TimeSpan timeSpan = System.TimeSpan.FromSeconds(GetTotalPlayTimeInSeconds());

    _TimeWithCozmoCountValueLabel.text = string.Format(Localization.GetCultureInfo(), "{0:D2}h:{1:D2}m:{2:D2}s",
                timeSpan.Hours,
                timeSpan.Minutes,
                timeSpan.Seconds);

    _GamesPlayCountValueLabel.text = GetGamesPlayedCount().ToString();
    _StreaksCountValueLabel.text = DataPersistence.DataPersistenceManager.Instance.CurrentStreak.ToString();

    _EditNameButton.Initialize(HandleEditNameButton, "edit_name_button", "profile_tab");
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

  private int GetGamesPlayedCount() {
    int playCount = 0;
    foreach (KeyValuePair<string, int> kvp in DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed) {
      playCount += kvp.Value;
    }
    return playCount;
  }

  private float GetTotalPlayTimeInSeconds() {
    float totalTime = 0.0f;
    foreach (DataPersistence.TimelineEntryData sessionEntry in DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Sessions) {
      totalTime += sessionEntry.PlayTime;
    }
    return totalTime;
  }
}
