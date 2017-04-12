using UnityEngine;
using System.Collections.Generic;
using Cozmo.UI;

public class ProfileTab : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _PlayerName;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _TimeWithCozmoCountValueLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _DailyGoalsCompletedCount;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _StreaksCountValueLabel;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _EditNameButton;

  [SerializeField]
  private Cozmo.UI.BaseModal _ProfileEditNameModalPrefab;
  private Cozmo.UI.BaseModal _ProfileEditNameModalInstance;

  // Use this for initialization
  private void Awake() {
    _PlayerName.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName;

    _TimeWithCozmoCountValueLabel.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DaysWithCozmo.ToString();

    _DailyGoalsCompletedCount.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalDailyGoalsCompleted.ToString();
    _StreaksCountValueLabel.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.MaximumStreak.ToString();

    _EditNameButton.Initialize(HandleEditNameButton, "edit_name_button", "profile_tab");
  }

  private void HandleEditNameButton() {
    System.Action<BaseModal> profileEditModalCreated = (newEditNameModal) => {
      _ProfileEditNameModalInstance = newEditNameModal;

      EnterNameSlide enterNameSlideScript = _ProfileEditNameModalInstance.GetComponent<EnterNameSlide>();
      enterNameSlideScript.SetNameInputField(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      enterNameSlideScript.RegisterInputFocus();
      enterNameSlideScript.OnNameEntered += HandleProfileEditNameDone;
    };
    ModalPriorityData profileEditNameData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                                  LowPriorityModalAction.CancelSelf,
                                                                  HighPriorityModalAction.Stack);
    UIManager.OpenModal(_ProfileEditNameModalPrefab, profileEditNameData, profileEditModalCreated);
  }

  private void HandleProfileEditNameDone(string newName) {
    _PlayerName.text = newName;
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName = newName;
    DataPersistence.DataPersistenceManager.Instance.Save();
    UIManager.CloseModal(_ProfileEditNameModalInstance);
  }

  private void OnDestroy() {
    if (_ProfileEditNameModalInstance != null) {
      UIManager.CloseModal(_ProfileEditNameModalInstance);
    }
  }
}
