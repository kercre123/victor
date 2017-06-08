using UnityEngine;
using System.Collections;

public class ProfileCreationModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private WheelDatePicker _BirthDatePicker;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _ContinueButton;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _BirthdateLabel;

  [SerializeField]
  private GameObject _SkipBirthdateEntryButtonContainer;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _SkipBirthdateEntryButton;

  private void Awake() {
    DAS.Debug("ProfileCreationView.Awake", "Enter Awake");
    _ContinueButton.Initialize(HandleBirthdateEntryDone, "continue_button", this.DASEventDialogName);
    _SkipBirthdateEntryButton.Initialize(HandleBirthdateEntrySkip, "skip_button", this.DASEventDialogName);

    _BirthDatePicker.maxYear = System.DateTime.Today.Year + 1;
  }

  private void Start() {
    _BirthDatePicker.date = System.DateTime.Now;
    ShowBirthdateEntry();
  }

  private void ShowDOBEntry(bool show) {
    _BirthDatePicker.gameObject.SetActive(show);
    _ContinueButton.gameObject.SetActive(show);
    _BirthdateLabel.gameObject.SetActive(show);

    // See COZMO-10878, COZMO-10973 for history of this feature
#if ENABLE_BIRTHDATE_SKIP
    _SkipBirthdateEntryButtonContainer.SetActive(show);
#else
    _SkipBirthdateEntryButtonContainer.SetActive(false);
#endif

    _ContinueButton.Interactable = false;
  }

  private void HandleDatePicked() {
    if (_BirthDatePicker.date < System.DateTime.Today) {
      _ContinueButton.Interactable = true;
    }
    else {
      _ContinueButton.Interactable = false;
    }
  }

  private void ShowBirthdateEntry() {
    ShowDOBEntry(true);
    _BirthDatePicker.ValueChange += HandleDatePicked;
  }

  private void HandleBirthdateEntryDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Birthdate = _BirthDatePicker.date;
    DasTracker.Instance.TrackBirthDateEntered(_BirthDatePicker.date);
    ProfileCreationDone();
  }

  private void HandleBirthdateEntrySkip() {
    DasTracker.Instance.TrackBirthDateSkipped();
    ProfileCreationDone();
  }

  private void ProfileCreationDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated = true;
    DataPersistence.DataPersistenceManager.Instance.Save();
    UIManager.CloseModal(this);
  }

  protected override void CleanUp() {

  }
}
