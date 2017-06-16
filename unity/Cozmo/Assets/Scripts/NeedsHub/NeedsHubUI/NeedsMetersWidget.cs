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

    private bool _EnableButtonsBasedOnNeeds;

    public void Initialize(bool enableButtonBasedOnNeeds, string dasParentDialogName, BaseDialog baseDialog) {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      bool allowInput = true;
      NeedsValue repairValue, energyValue, playValue;
      repairValue = nsm.GetCurrentDisplayValue(NeedId.Repair);
      energyValue = nsm.GetCurrentDisplayValue(NeedId.Energy);
      playValue = nsm.GetCurrentDisplayValue(NeedId.Play);

      _RepairMeter.Initialize(allowInput, RaiseRepairPressed, "repair_need_meter_button", dasParentDialogName);
      _RepairMeter.ProgressBar.SetValueInstant(repairValue.Value);

      _EnergyMeter.Initialize(allowInput, RaiseEnergyPressed, "energy_need_meter_button", dasParentDialogName);
      _EnergyMeter.ProgressBar.SetValueInstant(energyValue.Value);

      _PlayMeter.Initialize(allowInput, RaisePlayPressed, "play_need_meter_button", dasParentDialogName);
      _PlayMeter.ProgressBar.SetValueInstant(playValue.Value);

      baseDialog.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;

      _EnableButtonsBasedOnNeeds = enableButtonBasedOnNeeds;
      if (_EnableButtonsBasedOnNeeds) {
        EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);
      }
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;
    }

    private void HandleDialogFinishedOpenAnimation() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue, playValue;
      repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
      energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
      playValue = nsm.PopLatestEngineValue(NeedId.Play);
      UpdateMeters(repairValue.Value, energyValue.Value, playValue.Value);
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;

      if (_EnableButtonsBasedOnNeeds) {
        EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);
        NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;
      }
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue, playValue;
      repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
      energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
      playValue = nsm.PopLatestEngineValue(NeedId.Play);
      UpdateMeters(repairValue.Value, energyValue.Value, playValue.Value);
    }

    private void UpdateMeters(float repairValue, float energyValue, float playValue) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _RepairMeter.ProgressBar.SetTargetAndAnimate(repairValue);
      _EnergyMeter.ProgressBar.SetTargetAndAnimate(energyValue);
      _PlayMeter.ProgressBar.SetTargetAndAnimate(playValue);
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      if (needId == NeedId.Repair || needId == NeedId.Energy) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedsValue repairValue, energyValue;
        repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
        energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
        EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);
      }
    }

    private void EnableButtonsBasedOnBrackets(NeedBracketId repairBracket, NeedBracketId energyBracket) {
      if (repairBracket == NeedBracketId.Critical) {
        _EnergyMeter.ButtonEnabled = false;
        _PlayMeter.ButtonEnabled = false;
      }
      else if (energyBracket == NeedBracketId.Critical) {
        _EnergyMeter.ButtonEnabled = true;
        _PlayMeter.ButtonEnabled = false;
      }
      else {
        _EnergyMeter.ButtonEnabled = true;
        _PlayMeter.ButtonEnabled = true;
      }
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

    #region Onboarding
    public CozmoButton RepairButton {
      get {
        return _RepairMeter.Button;
      }
    }
    public CozmoButton FeedButton {
      get {
        return _EnergyMeter.Button;
      }
    }
    #endregion
  }
}