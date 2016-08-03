using Cozmo.UI;
using DataPersistence;
using UnityEngine;

namespace Onboarding {

  public class MultilineContinueStage : OnboardingBaseStage {

    [SerializeField]
    private UnityEngine.UI.Text[] _TextFields;
    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    [SerializeField]
    private float _RevealTime = 2.5f;

    private int _CurrReveal = 0;
    private float _NextRevealTime = -1.0f;
    public override void Start() {
      base.Start();
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      if (_TextFields != null) {
        for (int i = 1; i < _TextFields.Length; ++i) {
          _TextFields[i].gameObject.SetActive(false);
        }
        _NextRevealTime = Time.time + _RevealTime;
        _ContinueButtonInstance.gameObject.SetActive(false);
      }

    }

    public void Update() {
      if (_NextRevealTime < Time.time) {
        _CurrReveal++;
        _NextRevealTime = Time.time + _RevealTime;
        if (_CurrReveal < _TextFields.Length) {
          _TextFields[_CurrReveal].gameObject.SetActive(true);
        }
        else {
          _ContinueButtonInstance.gameObject.SetActive(true);
        }
      }
    }

    public override void SkipPressed() {
      if (_CurrReveal <= _TextFields.Length) {
        for (int i = 0; i < _TextFields.Length; ++i) {
          _TextFields[i].gameObject.SetActive(true);
        }
        _ContinueButtonInstance.gameObject.SetActive(true);
        _CurrReveal = _TextFields.Length;
      }
      else {
        HandleContinueClicked();
      }
    }

    protected virtual void HandleContinueClicked() {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
