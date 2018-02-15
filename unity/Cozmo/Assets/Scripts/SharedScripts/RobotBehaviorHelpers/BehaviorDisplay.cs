using UnityEngine;
using DG.Tweening;
using Anki.UI;
using Anki.Cozmo;
using Cozmo.UI;

public class BehaviorDisplay : MonoBehaviour {

  private Color _BehaviorDefaultColor;
  private Color _BehaviorRewardColor;

  [SerializeField]
  private CozmoText _BehaviorLabel;
  // So we don't flicker in a one state frame, must be in state for a bit before we want to change to it.
  [SerializeField]
  private float _MinTimeInStateBeforeChange_Sec = 0.5f;
  // So we make sure rewards earned stay up long enough to be noticed/read
  [SerializeField]
  private float _MinTimeShowingRewardBeforeChange_Sec = 2.5f;

  [SerializeField]
  private float _FadeTime_Sec = 0.3f;

  [SerializeField]
  private GameObject _ConnectedIcon;

  [SerializeField]
  private GameObject _DisconnectedIcon;

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

  private bool _IsDisconnected;

  private void Awake() {
    _BehaviorLabel.text = "";
    _BehaviorDefaultColor = UIColorPalette.FreeplayBehaviorDefaultColor;
    _BehaviorRewardColor = UIColorPalette.FreeplayBehaviorRewardColor;
    SetOverrideString(null);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FreeplaySparksAwarded>(HandleFreeplaySparksGiven);

    OnboardingManager.Instance.OnOverrideTickerString += HandleOnboardingStringOverride;
  }

  private void OnDestroy() {
    CleanupTween();
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FreeplaySparksAwarded>(HandleFreeplaySparksGiven);
    OnboardingManager.Instance.OnOverrideTickerString -= HandleOnboardingStringOverride;
  }
  private void CleanupTween() {
    if (_FadeTween != null) {
      _FadeTween.Kill();
    }
  }

  private void Update() {
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot != null) {
      if (_OverrideString == null) {
        string currBehaviorString = string.Empty;
        // If string is empty the design is that cozmo should stay in the previous state
        if ((currBehaviorString != string.Empty && currBehaviorString != _CurrString) || _IsDisconnected) {
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
      _IsDisconnected = false;
      SetConnectedIcon(true);
    }
    else {
      if (!_IsDisconnected) {
        // fade in connected text
        StartFadeTo(Localization.Get(LocalizationKeys.kConnectivityBehaviorDisplayDisconnected));

        _RewardedActionTimeStamp = -1.0f;
        _StateChangeTimeStamp = 0.0f; // fade out immediately when we become connected
        _IsDisconnected = true;
      }
      SetConnectedIcon(false);
    }
  }

  private void SetConnectedIcon(bool connected) {
    if (_ConnectedIcon != null && _ConnectedIcon.activeSelf != connected) {
      _ConnectedIcon.SetActive(connected);
    }
    if (_DisconnectedIcon != null && _DisconnectedIcon.activeSelf != !connected) {
      _DisconnectedIcon.SetActive(!connected);
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
    _BehaviorLabel.text = _CurrString;
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

  private void HandleOnboardingStringOverride(string str) {
    SetOverrideString(str);
  }

  private void HandleFreeplaySparksGiven(Anki.Cozmo.ExternalInterface.FreeplaySparksAwarded msg) {
    string baseString = Localization.Get(msg.sparksAwardedDisplayKey);
    string toShow = Localization.GetWithArgs(baseString, msg.sparksAwarded);
    _RewardedActionTimeStamp = Time.time;
    SetOverrideString(toShow);
  }

}
