using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class MultiContinueStage : OnboardingBaseStage {

    [SerializeField]
    protected CozmoText[] _TextFields;

    [SerializeField]
    protected CozmoButton[] _ContinueButtons;

    private int _CurrStage = 0;

    protected virtual void Awake() {
      for (int i = 0; i < _ContinueButtons.Length; ++i) {
        _ContinueButtons[i].Initialize(HandleContinueClicked, "Onboarding." + name + i, "Onboarding");
      }
      UpdateButtonState();
    }

    protected virtual void HandleContinueClicked() {
      _CurrStage++;
      if (_CurrStage >= _TextFields.Length || _CurrStage >= _ContinueButtons.Length) {
        SkipPressed();
      }
      else {
        UpdateButtonState();
      }
    }
    protected void UpdateButtonState() {
      for (int i = 0; i < _ContinueButtons.Length; ++i) {
        _ContinueButtons[i].gameObject.SetActive(i == _CurrStage);
      }
      for (int i = 0; i < _TextFields.Length; ++i) {
        _TextFields[i].gameObject.SetActive(i == _CurrStage);
      }
    }
  }

}
