using UnityEngine;
using System.Collections;

public class ProfileCreationModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private UnityEngine.UI.InputField _NameField;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _NameDoneButton;

  [SerializeField]
  private WheelDatePicker _BirthDatePicker;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _ContinueButton;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _NameLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _BirthdateLabel;

  [SerializeField]
  private GameObject _SkipBirthdateEntryButtonContainer;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _SkipBirthdateEntryButton;

  private IEnumerator _ActivateInputFieldCoroutine;

  private void Awake() {
    DAS.Debug("ProfileCreationView.Awake", "Enter Awake");
    _NameDoneButton.Initialize(HandleNameDoneButton, "name_done_button", this.DASEventDialogName);
    _ContinueButton.Initialize(HandleBirthdateEntryDone, "continue_button", this.DASEventDialogName);
    _SkipBirthdateEntryButton.Initialize(HandleBirthdateEntrySkip, "skip_button", this.DASEventDialogName);

    _BirthDatePicker.maxYear = System.DateTime.Today.Year + 1;
    _NameDoneButton.Interactable = false;

    DialogOpenAnimationFinished += HandleViewOpenFinished;
  }

  private void Start() {
    _BirthDatePicker.date = System.DateTime.Now;
    ShowDOBEntry(false);
  }

  private void OnDestroy() {
    if (_ActivateInputFieldCoroutine != null) {
      StopCoroutine(_ActivateInputFieldCoroutine);
    }
  }

  private void HandleViewOpenFinished() {
    _ActivateInputFieldCoroutine = DelayRegisterInputFocus();
    StartCoroutine(_ActivateInputFieldCoroutine);
  }

  private IEnumerator DelayRegisterInputFocus() {
    // COZMO-10748: We have to ensure that InputField.Start gets called before InputField.ActivateInputField,
    // otherwise there will be a null ref exception in Unity's internal logic. Therefore, wait a frame.
    yield return new WaitForEndOfFrame();
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

#if UNITY_IOS || UNITY_EDITOR
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
