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
    private int _TimeBeforeContinue_Sec = 120;

    private System.DateTime _TimeStartUTC;
    private float _TimerPausedTotal_Sec = 0;

    public override void Start() {
      base.Start();

      _ChargingProgressContainer.gameObject.SetActive(true);
      _ChargingCompleteContainer.gameObject.SetActive(false);
      _TimeStartUTC = System.DateTime.UtcNow;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      // the playspace image needs a white background again, then will transition back.
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
    }

    public void Update() {
      bool isOnCharger = true;

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        isOnCharger = RobotEngineManager.Instance.CurrentRobot.Status(Anki.Cozmo.RobotStatusFlag.IS_ON_CHARGER);
      }
      _KeepOnChargerText.gameObject.SetActive(!isOnCharger);

      // This isn't going to be an exact match, since we don't get Status messages every frame.
      // But should be close enough since we only display seconds.
      if (!isOnCharger) {
        _TimerPausedTotal_Sec += Time.deltaTime;
      }

      System.DateTime now = System.DateTime.UtcNow;
      System.TimeSpan timeInState = now - _TimeStartUTC;
      float timeInStateSec = (float)timeInState.TotalSeconds - _TimerPausedTotal_Sec;

      _CounterLabel.text = Mathf.CeilToInt(_TimeBeforeContinue_Sec - timeInStateSec).ToString();
      _ChargingCompleteContainer.gameObject.SetActive(timeInStateSec > _TimeBeforeContinue_Sec);
      _ChargingProgressContainer.gameObject.SetActive(timeInStateSec <= _TimeBeforeContinue_Sec);
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
