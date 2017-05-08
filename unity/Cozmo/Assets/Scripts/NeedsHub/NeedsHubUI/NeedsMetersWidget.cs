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
      _RepairMeter.Initialize(allowInput, RaiseRepairPressed, "repair_need_meter_button", dasParentDialogName);
      _RepairMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Repair));

      _EnergyMeter.Initialize(allowInput, RaiseEnergyPressed, "energy_need_meter_button", dasParentDialogName);
      _EnergyMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Energy));

      _PlayMeter.Initialize(allowInput, RaisePlayPressed, "play_need_meter_button", dasParentDialogName);
      _PlayMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Play));

      baseDialog.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;

      _EnableButtonsBasedOnNeeds = enableButtonBasedOnNeeds;
      if (_EnableButtonsBasedOnNeeds) {
        NeedBracketId repairBracket = nsm.GetCurrentDisplayBracket(NeedId.Repair);
        NeedBracketId energyBracket = nsm.GetCurrentDisplayBracket(NeedId.Energy);
        EnableButtonsBasedOnBrackets(repairBracket, energyBracket);
      }
    }

    private void OnDestroy() {
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;
    }

    private void HandleDialogFinishedOpenAnimation() {
      UpdateMetersToLatestFromEngine();
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;

      if (_EnableButtonsBasedOnNeeds) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedBracketId repairBracket = nsm.PopLatestEngineBracket(NeedId.Repair);
        NeedBracketId energyBracket = nsm.PopLatestEngineBracket(NeedId.Energy);
        EnableButtonsBasedOnBrackets(repairBracket, energyBracket);
        NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;
      }
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      UpdateMetersToLatestFromEngine();
    }

    private void UpdateMetersToLatestFromEngine() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _RepairMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Repair));
      _EnergyMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Energy));
      _PlayMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Play));
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      if (needId == NeedId.Repair || needId == NeedId.Energy) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedBracketId repairBracket = nsm.PopLatestEngineBracket(NeedId.Repair);
        NeedBracketId energyBracket = nsm.PopLatestEngineBracket(NeedId.Energy);
        EnableButtonsBasedOnBrackets(repairBracket, energyBracket);
      }
    }

    private void EnableButtonsBasedOnBrackets(NeedBracketId repairBracket, NeedBracketId energyBracket) {
      if (repairBracket == NeedBracketId.Critical || repairBracket == NeedBracketId.Warning) {
        _EnergyMeter.ButtonEnabled = false;
        _PlayMeter.ButtonEnabled = false;
      }
      else if (energyBracket == NeedBracketId.Critical || energyBracket == NeedBracketId.Warning) {
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
  }
}