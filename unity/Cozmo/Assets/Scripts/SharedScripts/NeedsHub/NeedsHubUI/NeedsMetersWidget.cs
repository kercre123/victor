using Anki.Cozmo;
using Cozmo.UI;
using System.Collections;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsMetersWidget : MonoBehaviour {
    public delegate void NeedsMeterButtonPressedHandler();
    public event NeedsMeterButtonPressedHandler OnRepairPressed;
    public event NeedsMeterButtonPressedHandler OnEnergyPressed;
    public event NeedsMeterButtonPressedHandler OnPlayPressed;

    [SerializeField]
    private NeedsMeter _RepairMeter;

    [SerializeField]
    private NeedsMeter _EnergyMeter;

    [SerializeField]
    private NeedsMeter _PlayMeter;

    // Allow for a little breathing room so that players can register the change as a 
    // separate event.
    private const float _kDelayOpenUpdateSeconds = 0.5f;
    private IEnumerator _DelayOpenUpdateCoroutine;

    public void Initialize(bool allowButtonInput, bool enableButtonBasedOnNeeds, string dasParentDialogName, BaseDialog baseDialog) {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      _RepairMeter.Initialize(allowButtonInput, RaiseRepairPressed, "repair_need_meter_button", dasParentDialogName);
      _RepairMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Repair));

      _EnergyMeter.Initialize(allowButtonInput, RaiseEnergyPressed, "energy_need_meter_button", dasParentDialogName);
      _EnergyMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Energy));

      _PlayMeter.Initialize(allowButtonInput, RaisePlayPressed, "play_need_meter_button", dasParentDialogName);
      _PlayMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Play));

      baseDialog.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;

      // IVY TODO: Handle enableButtonBasedOnNeeds
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      if (_DelayOpenUpdateCoroutine != null) {
        StopCoroutine(_DelayOpenUpdateCoroutine);
      }
    }

    private void HandleDialogFinishedOpenAnimation() {
      _DelayOpenUpdateCoroutine = DelayedOpenUpdateValues();
      StartCoroutine(_DelayOpenUpdateCoroutine);
    }

    private IEnumerator DelayedOpenUpdateValues() {
      yield return new WaitForSeconds(_kDelayOpenUpdateSeconds);
      UpdateMetersToLatestFromEngine();
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
    }

    private void HandleLatestNeedsLevelChanged() {
      UpdateMetersToLatestFromEngine();
    }

    private void UpdateMetersToLatestFromEngine() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _RepairMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Repair));

      _EnergyMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Energy));

      _PlayMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Play));
    }

    private void RaiseRepairPressed() {
      if (OnRepairPressed != null) {
        OnRepairPressed();
      }
    }

    private void RaiseEnergyPressed() {
      if (OnEnergyPressed != null) {
        OnEnergyPressed();
      }
    }

    private void RaisePlayPressed() {
      if (OnPlayPressed != null) {
        OnPlayPressed();
      }
    }
  }
}