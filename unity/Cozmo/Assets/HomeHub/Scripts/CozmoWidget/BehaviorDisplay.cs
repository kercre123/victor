using UnityEngine;
using DG.Tweening;
using Anki.UI;
using Anki.Cozmo;
using Cozmo.UI;

public class BehaviorDisplay : MonoBehaviour {

  private Color _BehaviorDefaultColor;
  private Color _BehaviorRewardColor;

  [SerializeField]
  private AnkiTextLabel _BehaviorLabel;
  // So we don't flicker in a one state frame, must be in state for a bit before we want to change to it.
  [SerializeField]
  private float _MinTimeInStateBeforeChange_Sec = 0.5f;
  // So we make sure rewards earned stay up long enough to be noticed/read
  [SerializeField]
  private float _MinTimeShowingRewardBeforeChange_Sec = 2.5f;

  [SerializeField]
  private float _FadeTime_Sec = 0.3f;

  private Tween _FadeTween = null;
  private string _OverrideString = null;

  private float _StateChangeTimeStamp = -1.0f;
  private float _RewardedActionTimeStamp = -1.0f;
  // Getter to make use case for this timestamp clearer
  // As long as the rewarded action timestamp is greater than 0
  // that means we are displaying a rewarded action and should set
  // color and other values accordingly
  public bool IsRewardedActionDisplayed {
    get { return _RewardedActionTimeStamp > 0.0f; }
  }

  private string _CurrString = "";


  private void Start() {
    _BehaviorLabel.text = "";
    _BehaviorDefaultColor = UIColorPalette.FreeplayBehaviorDefaultColor;
    _BehaviorRewardColor = UIColorPalette.FreeplayBehaviorRewardColor;
    SetOverrideString(null);
    RewardedActionManager.Instance.OnFreeplayRewardEvent += HandleFreeplayRewardedAction;
  }

  private void OnDestroy() {
    CleanupTween();
    RewardedActionManager.Instance.OnFreeplayRewardEvent -= HandleFreeplayRewardedAction;
  }
  private void CleanupTween() {
    if (_FadeTween != null) {
      _FadeTween.Kill();
    }
  }

  private void Update() {
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot != null && _BehaviorLabel != null) {
      if (_OverrideString == null) {
        string currBehaviorString = string.Empty;
        if (CurrentRobot.CurrentBehaviorDisplayNameKey != string.Empty) {
          currBehaviorString = Localization.Get(CurrentRobot.CurrentBehaviorDisplayNameKey);
        }
        // If string is empty the design is that cozmo should stay in the previous state
        if (currBehaviorString != string.Empty && currBehaviorString != _CurrString) {
          // start counting start of a change
          if (_StateChangeTimeStamp < 0.0f) {
            _StateChangeTimeStamp = Time.time;
          }
          else if ((_StateChangeTimeStamp + _MinTimeInStateBeforeChange_Sec) < Time.time) {
            StartFadeTo(currBehaviorString);
          }
        }
        // gone back to previous string so no need to change.
        else {
          _StateChangeTimeStamp = -1.0f;
        }
      }
      else {
        if (IsRewardedActionDisplayed && Time.time - _RewardedActionTimeStamp > _MinTimeShowingRewardBeforeChange_Sec) {
          _RewardedActionTimeStamp = -1.0f;
          _OverrideString = null;
        }
      }
    }
  }

  private void StartFadeTo(string nextString) {
    _CurrString = nextString;
    CleanupTween();
    _FadeTween = _BehaviorLabel.DOFade(0.0F, _FadeTime_Sec).OnComplete(HandleFadeEnd);
  }
  private void HandleFadeEnd() {
    // If we are fading in a rewarded action string, set color to green, otherwise enforce default color
    if (IsRewardedActionDisplayed) {
      _BehaviorLabel.color = _BehaviorRewardColor;
    }
    else {
      _BehaviorLabel.color = _BehaviorDefaultColor;
    }
    // Fade our new string back up
    _BehaviorLabel.text = "> " + _CurrString;
    CleanupTween();
    _FadeTween = _BehaviorLabel.DOFade(1.0F, _FadeTime_Sec);
  }

  // Mostly for onboarding
  public void SetOverrideString(string str) {
    _OverrideString = str;
    if (str == null) {
      str = "";
    }
    StartFadeTo(str);
  }

  private void HandleFreeplayRewardedAction(RewardedActionData reward) {
    string rewardDesc = Localization.Get(reward.Reward.DescriptionKey);
    string toShow = Localization.GetWithArgs(LocalizationKeys.kRewardFreeplayBehaviorDisplay, reward.Reward.Amount, rewardDesc);
    _RewardedActionTimeStamp = Time.time;
    SetOverrideString(toShow);
  }

}
