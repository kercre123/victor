using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Repair.UI {
  public class NeedsRepairModal : BaseModal {

    [SerializeField]
    private CozmoButton _RepairHeadButton;

    [SerializeField]
    private CozmoButton _RepairLiftButton;

    [SerializeField]
    private CozmoButton _RepairTreadsButton;

    [SerializeField]
    private NeedsMeter _RepairMeter;

    public void InitializeRepairModal() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.PauseDecay();

      _RepairMeter.Initialize(allowInput: false,
                              buttonClickCallback: null,
                              dasButtonName: "repair_need_meter_button",
                              dasParentDialogName: DASEventDialogName);
      _RepairMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Repair));

      _RepairHeadButton.Initialize(HandleRepairHeadButtonPressed, "repair_head_button", DASEventDialogName);

      _RepairLiftButton.Initialize(HandleRepairLiftButtonPressed, "repair_lift_button", DASEventDialogName);

      _RepairTreadsButton.Initialize(HandleRepairTreadsButtonPressed, "repair_treads_button", DASEventDialogName);
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.ResumeDecay();
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
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _RepairHeadButton.Interactable = nsm.PopLatestEnginePartIsBroken(RepairablePartId.Head);
      _RepairLiftButton.Interactable = nsm.PopLatestEnginePartIsBroken(RepairablePartId.Lift);
      _RepairTreadsButton.Interactable = nsm.PopLatestEnginePartIsBroken(RepairablePartId.Treads);
    }

    protected override void RaiseDialogClosed() {
      base.RaiseDialogClosed();
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      if (actionId == NeedsActionId.RepairHead || actionId == NeedsActionId.RepairLift
          || actionId == NeedsActionId.RepairTreads) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        _RepairMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Repair));
      }
    }

    private void HandleRepairHeadButtonPressed() {
      _RepairHeadButton.Interactable = false;
      NeedsStateManager.Instance.RegisterNeedActionCompleted(NeedsActionId.RepairHead);
    }

    private void HandleRepairLiftButtonPressed() {
      _RepairLiftButton.Interactable = false;
      NeedsStateManager.Instance.RegisterNeedActionCompleted(NeedsActionId.RepairLift);
    }

    private void HandleRepairTreadsButtonPressed() {
      _RepairTreadsButton.Interactable = false;
      NeedsStateManager.Instance.RegisterNeedActionCompleted(NeedsActionId.RepairTreads);
    }
  }
}