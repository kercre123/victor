using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Energy.UI {
  public class NeedsEnergyModal : BaseModal {

    [SerializeField]
    private CozmoText[] _InstructionNumberTexts;

    [SerializeField]
    private NeedsMeter _EnergyMeter;

    [SerializeField]
    private CozmoButton _DoneCozmoButton;

    [SerializeField]
    private MoveTweenSettings _HungryTweenSettings;

    [SerializeField]
    private MoveTweenSettings _FullTweenSettings;

    private Sequence _HungryAnimation = null;
    private Sequence _FullAnimation = null;

    private NeedBracketId _LastNeedBracket = NeedBracketId.Count;

    public void InitializeEnergyModal() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayActivity(false);
      }

      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.PauseExceptForNeed(NeedId.Energy);

      _EnergyMeter.Initialize(allowInput: false,
                              buttonClickCallback: null,
                              dasButtonName: "energy_need_meter_button",
                              dasParentDialogName: DASEventDialogName);

      float displayEnergy = nsm.GetCurrentDisplayValue(NeedId.Energy);
      _EnergyMeter.ProgressBar.SetValueInstant(displayEnergy);

      if (_InstructionNumberTexts != null) {
        for (int i = 0; i < _InstructionNumberTexts.Length; i++) {
          _InstructionNumberTexts[i].SetText(Localization.GetNumber(i + 1));
        }
      }

      if (_DoneCozmoButton != null) {
        _DoneCozmoButton.Initialize(HandleUserClose, "done_button", DASEventDialogName);
      }

      //animate our display energy to the engine energy
      HandleLatestNeedsLevelChanged(NeedsActionId.FeedBlue);

      RefreshForCurrentBracket(nsm);
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.ResumeAllNeeds();
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;

      //RETURN TO FREEPLAY
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayLightStates(true);
        robot.SetEnableFreeplayActivity(true);
      }
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ActivateHighLevelActivity(HighLevelActivity.Feeding);
      }
    }

    protected override void RaiseDialogClosed() {
      base.RaiseDialogClosed();

      if (_LastNeedBracket != NeedBracketId.Count) {
        AnimateElements(fullElements: _LastNeedBracket == NeedBracketId.Full, hide: true);
      }

      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      if (actionId == NeedsActionId.FeedBlue) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        float engineEnergy = nsm.PopLatestEngineValue(NeedId.Energy);
        _EnergyMeter.ProgressBar.SetTargetAndAnimate(engineEnergy);
        RefreshForCurrentBracket(nsm);
      }
    }

    private void RefreshForCurrentBracket(NeedsStateManager nsm) {
      NeedBracketId newNeedBracket = nsm.PopLatestEngineBracket(NeedId.Energy);
      //only hide/show elements if bracket has changed
      if (_LastNeedBracket != newNeedBracket) {
        bool cozmoIsFull = newNeedBracket == NeedBracketId.Full;
        //first hide unwanted elements, snap hidden if this is initial bracket assignment
        AnimateElements(fullElements: !cozmoIsFull, hide: true, snap: _LastNeedBracket == NeedBracketId.Count);
        //then show wanted elements
        AnimateElements(fullElements: cozmoIsFull, hide: false);
        _LastNeedBracket = newNeedBracket;
      }
    }

    private void AnimateElements(bool fullElements = false, bool hide = false, bool snap = false) {
      MoveTweenSettings tweenSettings = _HungryTweenSettings;
      Sequence sequence = _HungryAnimation;
      if (fullElements) {
        tweenSettings = _FullTweenSettings;
        sequence = _FullAnimation;
      }

      if (sequence != null) {
        sequence.Kill();
      }
      sequence = DOTween.Sequence();

      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      if (tweenSettings.targets.Length > 0) {
        if (hide ^ snap) {
          settings.ConstructCloseMoveTween(ref sequence, tweenSettings);
        }
        else {
          settings.ConstructOpenMoveTween(ref sequence, tweenSettings);
        }
      }

      if (snap) {
        sequence.Rewind();
      }
      else {
        sequence.Play();
      }
    }
  }
}