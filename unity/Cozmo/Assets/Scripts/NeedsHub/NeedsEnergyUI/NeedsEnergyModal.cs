using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Energy.UI {
  public class NeedsEnergyModal : BaseModal {

    [SerializeField]
    private CozmoButton _FeedButton;

    [SerializeField]
    private NeedsMeter _EnergyMeter;

    public void InitializeEnergyModal() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.PauseExceptForNeed(NeedId.Energy);

      _EnergyMeter.Initialize(allowInput: false,
                              buttonClickCallback: null,
                              dasButtonName: "energy_need_meter_button",
                              dasParentDialogName: DASEventDialogName);
      _EnergyMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Energy));

      _FeedButton.Initialize(HandleFeedButtonPressed, "feed_button", DASEventDialogName);
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.ResumeAllNeeds();
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      ConstructDefaultFadeOpenAnimation(openAnimation);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      ConstructDefaultFadeCloseAnimation(closeAnimation);
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
    }

    protected override void RaiseDialogClosed() {
      base.RaiseDialogClosed();
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      if (actionId == NeedsActionId.FeedBlue) {
        _FeedButton.Interactable = true;
        NeedsStateManager nsm = NeedsStateManager.Instance;
        _EnergyMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Energy));
      }
    }

    private void HandleFeedButtonPressed() {
      _FeedButton.Interactable = false;
      NeedsStateManager.Instance.RegisterNeedActionCompleted(NeedsActionId.FeedBlue);
    }
  }
}