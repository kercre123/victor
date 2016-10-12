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
    if (RobotEngineManager.Instance.CurrentRobot != null && _BehaviorLabel != null) {
      if (_OverrideString == null) {
        string currBehaviorString = GetBehaviorString(RobotEngineManager.Instance.CurrentRobot.CurrentBehaviorType);
        // If string is empty the design is that cozmo should stay in the previous state
        if (currBehaviorString != "" && currBehaviorString != _CurrString) {
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

  private string GetBehaviorString(BehaviorType behaviorType) {
    string ret = "";
    // Might share some of these in the future, so not a dictionary
    switch (behaviorType) {
    case BehaviorType.BringCubeToBeacon:
      ret = Localization.Get(LocalizationKeys.kBehaviorBringCubeToBeacon);
      break;
    case BehaviorType.LookAround:
      ret = Localization.Get(LocalizationKeys.kBehaviorLookAround);
      break;
    case BehaviorType.ExploreVisitPossibleMarker:
      ret = Localization.Get(LocalizationKeys.kBehaviorExploreVisitPossibleMarker);
      break;
    case BehaviorType.LookInPlaceMemoryMap:
      ret = Localization.Get(LocalizationKeys.kBehaviorLookInPlaceMemoryMap);
      break;
    case BehaviorType.ExploreLookAroundInPlace:
      ret = Localization.Get(LocalizationKeys.kBehaviorExploreLookAroundInPlace);
      break;
    case BehaviorType.ThinkAboutBeacons:
      ret = Localization.Get(LocalizationKeys.kBehaviorThinkAboutBeacons);
      break;
    case BehaviorType.VisitInterestingEdge:
      ret = Localization.Get(LocalizationKeys.kBehaviorVisitInterestingEdge);
      break;
    case BehaviorType.BuildPyramid:
      ret = Localization.Get(LocalizationKeys.kBehaviorBuildPyramid);
      break;
    case BehaviorType.KnockOverCubes:
      ret = Localization.Get(LocalizationKeys.kBehaviorKnockOverCubes);
      break;
    case BehaviorType.PopAWheelie:
      ret = Localization.Get(LocalizationKeys.kBehaviorPopAWheelie);
      break;
    case BehaviorType.PounceOnMotion:
      ret = Localization.Get(LocalizationKeys.kBehaviorPounceOnMotion);
      break;
    case BehaviorType.RollBlock:
      ret = Localization.Get(LocalizationKeys.kBehaviorRollBlock);
      break;
    case BehaviorType.StackBlocks:
      ret = Localization.Get(LocalizationKeys.kBehaviorStackCube);
      break;
    case BehaviorType.FindFaces:
      ret = Localization.Get(LocalizationKeys.kBehaviorLookingForFaces);
      break;
    case BehaviorType.AcknowledgeFace:
      ret = Localization.Get(LocalizationKeys.kBehaviorAcknowledgeObject);
      break;
    case BehaviorType.InteractWithFaces:
      ret = Localization.Get(LocalizationKeys.kBehaviorInteractWithFaces);
      break;
    case BehaviorType.PlayAnim:
      ret = Localization.Get(LocalizationKeys.kBehaviorBored);
      break;
    case BehaviorType.ReactToCliff:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToCliff);
      break;
    case BehaviorType.AcknowledgeObject:
      ret = Localization.Get(LocalizationKeys.kBehaviorAcknowledgeObject);
      break;
    case BehaviorType.ReactToRobotOnBack:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToOnBack);
      break;
    case BehaviorType.ReactToRobotOnFace:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToOnFace);
      break;
    case BehaviorType.ReactToReturnedToTreads:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToOnTreads);
      break;
    case BehaviorType.ReactToUnexpectedMovement:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToUnexpectedMovement);
      break;
    case BehaviorType.ReactToFrustration:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToFrustration);
      break;
    case BehaviorType.ReactToOnCharger:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToOnCharger);
      break;
    case BehaviorType.ReactToPickup:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToPickup);
      break;
    case BehaviorType.ReactToRobotOnSide:
      ret = Localization.Get(LocalizationKeys.kBehaviorReactToOnSide);
      break;
    case BehaviorType.DriveOffCharger:
      ret = Localization.Get(LocalizationKeys.kBehaviorDriveOffCharger);
      break;
    case BehaviorType.RequestGameSimple:
      ret = Localization.Get(LocalizationKeys.kBehaviorRequestToPlay);
      break;
    default:
      break;
    }
    return ret;
  }

  private void HandleFreeplayRewardedAction(RewardedActionData reward) {
    string rewardDesc = Localization.Get(reward.Reward.DescriptionKey);
    string toShow = Localization.GetWithArgs(LocalizationKeys.kRewardFreeplayBehaviorDisplay, reward.Reward.Amount, rewardDesc);
    _RewardedActionTimeStamp = Time.time;
    SetOverrideString(toShow);
  }

}
