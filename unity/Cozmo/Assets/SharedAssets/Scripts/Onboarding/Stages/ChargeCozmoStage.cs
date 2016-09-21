using Cozmo.UI;
using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;

namespace Onboarding {

  // This screen is either initial
  public class ChargeCozmoStage : ShowContinueStage {

    [SerializeField]
    private AnkiTextLabel _CounterLabel;

    [SerializeField]
    private AnkiTextLabel _KeepOnChargerText;


    [SerializeField]
    private GameObject _ChargingProgressContainer;

    [SerializeField]
    private GameObject _ChargingCompleteContainer;

    [SerializeField]
    private int _TimeBeforeContinue_Sec = 180;

    private System.DateTime _TimeStartUTC;
    private float _TimerPausedTotal_Sec = 0;
    private bool _WasOnCharger = false;

    [SerializeField]
    private GameObject[] _SlideGameObjects;

    [SerializeField]
    private GameObject _FinalChargeSlide;

    [SerializeField]
    private float _SlideInterval_Sec = 15.0f;

    private float _timeOfLastSlide_Sec;
    private int _CurrentSlideIndex;
    private int _NumSlides;

    private bool _timerComplete;

    public override void Start() {
      base.Start();

      _ChargingProgressContainer.gameObject.SetActive(true);
      _ChargingCompleteContainer.gameObject.SetActive(false);
      _TimeStartUTC = System.DateTime.UtcNow;

      _timerComplete = false;
      InitSlides();
    }

    public override void OnDestroy() {
      base.OnDestroy();
      // the playspace image needs a white background again, then will transition back.
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
    }

    private void UpdateStatusText() {
      _KeepOnChargerText.text = _WasOnCharger ? Localization.Get(LocalizationKeys.kOnboardingCozmoNeedsChargeKeepOnCharger) : Localization.Get(LocalizationKeys.kOnboardingCozmoNeedsChargePutBackOnCharger);
    }

    public void Update() {
      bool isOnCharger = true;

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        isOnCharger = RobotEngineManager.Instance.CurrentRobot.Status(Anki.Cozmo.RobotStatusFlag.IS_ON_CHARGER);
      }

      // This isn't going to be an exact match, since we don't get Status messages every frame.
      // But should be close enough since we only display seconds.
      if (!isOnCharger) {
        _TimerPausedTotal_Sec += Time.deltaTime;
      }
      if (_WasOnCharger != isOnCharger) {
        _WasOnCharger = isOnCharger;
        UpdateStatusText();
      }

      System.DateTime now = System.DateTime.UtcNow;
      System.TimeSpan timeInState = now - _TimeStartUTC;
      float timeInStateSec = (float)timeInState.TotalSeconds - _TimerPausedTotal_Sec;
//      float timeInStateSec = (float)timeInState.TotalSeconds;

      float displayTime = _TimeBeforeContinue_Sec - timeInStateSec;

      if (displayTime >= 0.0f) {
        _CounterLabel.text = Mathf.CeilToInt(displayTime).ToString();
      } else {
        _CounterLabel.text = "0";
        _timerComplete = true;
        
      }

      _ChargingCompleteContainer.gameObject.SetActive(timeInStateSec > _TimeBeforeContinue_Sec);
      _ChargingProgressContainer.gameObject.SetActive(timeInStateSec <= _TimeBeforeContinue_Sec);

      UpdateSlides(timeInStateSec);

    }

    private void InitSlides() {
      _CurrentSlideIndex = 0;
      _NumSlides = _SlideGameObjects.Length;
      _timeOfLastSlide_Sec = 0;
      _FinalChargeSlide.SetActive(false);

      for (int i = 0; i < _NumSlides; ++i) {
        _SlideGameObjects[i].SetActive(false);
      }

      _SlideGameObjects[0].SetActive (true);
    }

    private void UpdateSlides(float time) {
      if (_timerComplete) {
        ShowFinalTipSlide();
        return;
      }

      float deltaTime = time - _timeOfLastSlide_Sec;
      if(deltaTime > _SlideInterval_Sec){
        _timeOfLastSlide_Sec = time;
        ShowNextSlide();
      }
    }

    private void ShowNextSlide() {

      _SlideGameObjects[_CurrentSlideIndex].SetActive(false);
      _CurrentSlideIndex++;
      if (_CurrentSlideIndex >= _NumSlides) _CurrentSlideIndex = 0;
      _SlideGameObjects[_CurrentSlideIndex].SetActive(true);

    }

    private void ShowFinalTipSlide() {
      _SlideGameObjects[_CurrentSlideIndex].SetActive(false);
      _FinalChargeSlide.SetActive(true);
    }

    public override void SkipPressed() {
      if (_ChargingCompleteContainer.activeInHierarchy) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        _TimeStartUTC = _TimeStartUTC.AddSeconds(-_TimeBeforeContinue_Sec - 1);
      }
    }

  }

}
