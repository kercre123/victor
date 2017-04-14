using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class EnterNameSlide : MonoBehaviour {

  public System.Action<string> OnNameEntered;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _SubmitName;

  [SerializeField]
  private UnityEngine.UI.InputField _NameInputField;

  private IEnumerator _ActivateInputFieldCoroutine;

  private void Awake() {
    _SubmitName.Initialize(HandleSubmitNameButton, "enter_name_done", "enter_name_slide");
    _SubmitName.Interactable = false;
  }

  private void Start() {
    if (string.IsNullOrEmpty(_NameInputField.text) == false) {
      _SubmitName.Interactable = true;
    }
  }

  private void OnDestroy() {
    if (_ActivateInputFieldCoroutine != null) {
      StopCoroutine(_ActivateInputFieldCoroutine);
    }
  }

  public void SetNameInputField(string existing) {
    if (string.IsNullOrEmpty(existing)) {
      return;
    }
    // there is a bug with unity's InputField for Name types that cuts off the last character of a programmatically
    // set string. This is a workaround.
    // https://fogbugz.unity3d.com/default.asp?824198_olip6sa7g9joavuc
    _NameInputField.characterLimit = _NameInputField.characterLimit + 1;
    _NameInputField.text = existing;
    _NameInputField.characterLimit = _NameInputField.characterLimit - 1;
  }

  public void RegisterInputFocus() {
    _ActivateInputFieldCoroutine = DelayRegisterInputFocus();
    StartCoroutine(_ActivateInputFieldCoroutine);
  }

  private IEnumerator DelayRegisterInputFocus() {
    // COZMO-10748: We have to ensure that InputField.Start gets called before InputField.ActivateInputField,
    // otherwise there will be a null ref exception in Unity's internal logic. Therefore, wait a frame.
    yield return new WaitForEndOfFrame();
    _NameInputField.Select();
    _NameInputField.ActivateInputField();
    _NameInputField.onValueChanged.AddListener(HandleInputFieldChange);
  }

  private void HandleInputFieldChange(string input) {
    if (string.IsNullOrEmpty(input)) {
      _SubmitName.Interactable = false;
    }
    else {
      _SubmitName.Interactable = true;
    }
  }

  private void HandleSubmitNameButton() {
    _NameInputField.onValueChanged.RemoveListener(HandleInputFieldChange);
    _SubmitName.Interactable = false;
    if (OnNameEntered != null) {
      OnNameEntered(_NameInputField.text);
    }
  }
}
