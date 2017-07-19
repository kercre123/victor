using Anki.Cozmo;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
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

    [SerializeField]
    private CozmoImage _DimmedImage;

    [SerializeField]
    private float _InitialFillStaggerTime = 1f;

    [SerializeField]
    private float _InitialFillOnboardingStaggerTime = 0.5f;

    private bool _RepairMeterPaused = false;
    public bool RepairMeterPaused {
      get {
        return _RepairMeterPaused;
      }
      set {
        bool unpaused = _RepairMeterPaused && !value;
        _RepairMeterPaused = value;
        if (unpaused) {
          HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        }
      }
    }

    private bool _EnableButtonsBasedOnNeeds;

    private Coroutine _StaggeredFillCoroutine = null;

    public void Initialize(string dasParentDialogName,
                           BaseDialog baseDialog,
                           NeedId focusOnMeter = NeedId.Count) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      _RepairMeter.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Repair).Value);
      _EnergyMeter.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Energy).Value);
      _PlayMeter.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Play).Value);

      //if we are isolating a meter, let's stick a dimmer behind it, covering the rest.
      _DimmedImage.gameObject.SetActive(focusOnMeter != NeedId.Count);
      _DimmedImage.transform.SetAsLastSibling();

      switch (focusOnMeter) {
      case NeedId.Repair:
        _RepairMeter.transform.SetAsLastSibling();
        break;
      case NeedId.Energy:
        _EnergyMeter.transform.SetAsLastSibling();
        break;
      case NeedId.Play:
        _PlayMeter.transform.SetAsLastSibling();
        break;
      }
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.NurtureIntro)) {
        OnboardingManager.Instance.OnOnboardingAnimEvent += HandleOnboardingAnimEvent;
      }
      else {
        baseDialog.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;
      }
    }

    private void OnDestroy() {
      if (_StaggeredFillCoroutine != null) {
        StopCoroutine(_StaggeredFillCoroutine);
      }
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;
      OnboardingManager.Instance.OnOnboardingAnimEvent -= HandleOnboardingAnimEvent;
    }

    private void HandleDialogFinishedOpenAnimation() {
      // Onboarding disables this game object until it's done and starts on it's own.
      if (isActiveAndEnabled) {
        _StaggeredFillCoroutine = StartCoroutine(StaggerMeterFills());
      }
      else {
        NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
        NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;
      }
    }

    private void HandleOnboardingAnimEvent(string eventName) {
      const string _kMechanimEventOnboardingNeedsMeterShow = "NeedsMeterStartEvent";
      if (eventName == _kMechanimEventOnboardingNeedsMeterShow) {
        HandleDialogFinishedOpenAnimation();
      }
    }

    private void PlayMeterMoveSound(float oldVal, float newVal) {
      if (newVal - oldVal > float.Epsilon) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Up);
      }
      else if (newVal - oldVal < -float.Epsilon) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Down);
      }
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue, playValue;
      NeedsValue oldRepairValue = nsm.GetCurrentDisplayValue(NeedId.Repair);
      NeedsValue oldEnergyValue = nsm.GetCurrentDisplayValue(NeedId.Energy);
      NeedsValue oldPlayValue = nsm.GetCurrentDisplayValue(NeedId.Play);
      repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
      energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
      playValue = nsm.PopLatestEngineValue(NeedId.Play);

      PlayMeterMoveSound(repairValue.Value, oldRepairValue.Value);
      PlayMeterMoveSound(energyValue.Value, oldEnergyValue.Value);
      PlayMeterMoveSound(playValue.Value, oldPlayValue.Value);

      UpdateMeters(repairValue.Value, energyValue.Value, playValue.Value);
    }

    private void UpdateMeters(float repairValue, float energyValue, float playValue) {
      if (!_RepairMeterPaused) {
        _RepairMeter.SetTargetAndAnimate(repairValue);
      }
      _EnergyMeter.SetTargetAndAnimate(energyValue);
      _PlayMeter.SetTargetAndAnimate(playValue);
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue;
      repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
      energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
      NeedsValue playValue = nsm.PopLatestEngineValue(NeedId.Play);
      if ((needId == NeedId.Repair && repairValue.Bracket == NeedBracketId.Critical) ||
          (needId == NeedId.Energy && energyValue.Bracket == NeedBracketId.Critical) ||
          (needId == NeedId.Play && playValue.Bracket == NeedBracketId.Critical)) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Severe_Start);
      }
      if ((needId == NeedId.Repair && repairValue.Bracket == NeedBracketId.Full) ||
          (needId == NeedId.Energy && energyValue.Bracket == NeedBracketId.Full) ||
          (needId == NeedId.Play && energyValue.Bracket == NeedBracketId.Full)) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Full);
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

    private IEnumerator StaggerMeterFills() {
      float oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Repair).Value;
      float newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Repair).Value;
      PlayMeterMoveSound(oldValue, newValue);
      _RepairMeter.SetTargetAndAnimate(newValue);

      float staggerTime = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.NurtureIntro) ?
                                     _InitialFillOnboardingStaggerTime : _InitialFillStaggerTime;

      yield return new WaitForSeconds(staggerTime);

      oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Energy).Value;
      newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Energy).Value;
      PlayMeterMoveSound(oldValue, newValue);
      _EnergyMeter.SetTargetAndAnimate(newValue);

      yield return new WaitForSeconds(staggerTime);

      oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Play).Value;
      newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Play).Value;
      PlayMeterMoveSound(oldValue, newValue);
      _PlayMeter.SetTargetAndAnimate(newValue);

      //now that we are done with our initial fills, let's subscribed to further changes
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
      NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;

      _StaggeredFillCoroutine = null;
      yield return 0;
    }

    #region Onboarding

    public void DimNeedMeters(List<NeedId> dimmedMeters) {
      _EnergyMeter.Dim = dimmedMeters.Contains(NeedId.Energy);
      _RepairMeter.Dim = dimmedMeters.Contains(NeedId.Repair);
      _PlayMeter.Dim = dimmedMeters.Contains(NeedId.Play);
    }

    #endregion
  }
}