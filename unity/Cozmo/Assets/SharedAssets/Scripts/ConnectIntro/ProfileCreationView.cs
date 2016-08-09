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

  private const int kNameFieldCharacterLimit = 12;

  private void Awake() {
    _NameDoneButton.Initialize(HandleNameDoneButton, "name_done_button", this.DASEventViewName);
    _ContinueButton.Initialize(HandleBirthdateEntryDone, "continue_button", this.DASEventViewName);
    _NameField.onValidateInput += ValidateNameField;
    _NameField.onValueChanged.AddListener(HandleNameFieldChange);
    _NameField.keyboardType = TouchScreenKeyboardType.Default;
    _NameField.ActivateInputField();
    _NameField.shouldHideMobileInput = true;
    _BirthDatePicker.maxYear = System.DateTime.Today.Year + 1;
    _NameDoneButton.Interactable = false;
  }

  private void Start() {
    _BirthDatePicker.date = System.DateTime.Now;
    ShowDOBEntry(false);
  }

  private char ValidateNameField(string input, int charIndex, char charToValidate) {
    if (charIndex >= kNameFieldCharacterLimit) {
      return '\0';
    }
    if (charToValidate >= 'a' && charToValidate <= 'z' || charToValidate >= 'A' && charToValidate <= 'Z') {
      return charToValidate;
    }
    return '\0';
  }

  private void HandleNameFieldChange(string input) {
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
    // TODO: Handle minimum name length.
    HideNameInput();
    ShowBirthdateEntry();
  }

  private void ShowDOBEntry(bool show) {
    _BirthDatePicker.gameObject.SetActive(show);
    _ContinueButton.gameObject.SetActive(show);
    _BirthdateLabel.gameObject.SetActive(show);
  }

  private void HideNameInput() {
    _NameLabel.gameObject.SetActive(false);
    _NameField.gameObject.SetActive(false);
    _NameDoneButton.gameObject.SetActive(false);
  }

  private void ShowBirthdateEntry() {
    ShowDOBEntry(true);
  }

  private void HandleBirthdateEntryDone() {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Birthdate = _BirthDatePicker.date;
    ProfileCreationDone();
  }

  private void ProfileCreationDone() {
    UIManager.CloseView(this);
  }

  protected override void CleanUp() {

  }
}
