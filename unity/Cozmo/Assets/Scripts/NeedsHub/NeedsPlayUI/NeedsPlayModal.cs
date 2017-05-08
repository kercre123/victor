using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Play.UI {
  public class NeedsPlayModal : BaseModal {

    [SerializeField]
    private NeedsMeter _PlayMeter;

    public void InitializePlayModal() {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      _PlayMeter.Initialize(allowInput: false,
                            buttonClickCallback: null,
                            dasButtonName: "play_need_meter_button",
                            dasParentDialogName: DASEventDialogName);
      _PlayMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Play));
    }

    private void OnDestroy() {
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
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _PlayMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Play));
    }
  }
}