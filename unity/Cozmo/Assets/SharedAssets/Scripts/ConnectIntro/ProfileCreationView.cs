using UnityEngine;
using System.Collections;

public class ProfileCreationView : Cozmo.UI.BaseView {

  [SerializeField]
  private UnityEngine.UI.InputField _NameField;

  [SerializeField]
  private Cozmo.UI.CozmoButton _NameDoneButton;

  [SerializeField]
  private WheelDatePicker _BirthDatePicker;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ContinueButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _NameLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _BirthdateLabel;

  private void Awake() {
    DAS.Debug("ProfileCreationView.Awake", "Enter Awake");
    _NameDoneButton.Initialize(HandleNameDoneButton, "name_done_button", this.DASEventViewName);
    _ContinueButton.Initialize(HandleBirthdateEntryDone, "continue_button", this.DASEventViewName);

    _BirthDatePicker.maxYear = System.DateTime.Today.Year + 1;
    _NameDoneButton.Interactable = false;

    ViewOpenAnimationFinished += HandleViewOpenFinished;
  }

  private void Start() {
    _BirthDatePicker.date = System.DateTime.Now;
    ShowDOBEntry(false);
  }

  private void HandleViewOpenFinished() {
    _NameField.Select();
    _NameField.ActivateInputField();
    _NameField.onValueChanged.AddListener(HandleNameFieldChange);
  }

  private void HandleNameFieldChange(string input) {
    DAS.Debug("ProfileCreationView.HandleNameFieldChange", input);
    if (input.Length > 0) {
      _NameDoneButton.Interactable = true;
    }
    else {
      _NameDoneButton.Interactable = false;
    }
  }

  private void HandleNameDoneButton() {
    DAS.Debug("ProfileCreationView.HandleNameDoneButton", "name done button pressed");
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName = _NameField.text;
    HideNameInput();
    ShowBirthdateEntry();
  }

  private void ShowDOBEntry(bool show) {
    _BirthDatePicker.gameObject.SetActive(show);
    _ContinueButton.gameObject.SetActive(show);
    _BirthdateLabel.gameObject.SetActive(show);
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

  private void HideNameInput() {
    _NameLabel.gameObject.SetActive(false);
    _NameField.gameObject.SetActive(false);
    _NameDoneButton.gameObject.SetActive(false);
  }

  private void ShowBirthdateEntry() {
    ShowDOBEntry(true);
    _BirthDatePicker.ValueChange += HandleDatePicked;
  }

  private void HandleBirthdateEntryDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Birthdate = _BirthDatePicker.date;
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileCreated = true;
    DataPersistence.DataPersistenceManager.Instance.Save();
    DasTracker.Instance.TrackBirthDateEntered(_BirthDatePicker.date);
    ProfileCreationDone();
  }

  private void ProfileCreationDone() {
    UIManager.CloseView(this);
  }

  protected override void CleanUp() {

  }
}
