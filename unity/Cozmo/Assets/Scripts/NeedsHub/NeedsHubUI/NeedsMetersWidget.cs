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
    private GameObject _DimmedImage;

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

    private bool _EnergyMeterPaused = false;
    public bool EnergyMeterPaused {
      get {
        return _EnergyMeterPaused;
      }
      set {
        bool unpaused = _EnergyMeterPaused && !value;
        _EnergyMeterPaused = value;
        if (unpaused) {
          HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        }
      }
    }

    private bool _PlayMeterPaused = false;
    public bool PlayMeterPaused {
      get {
        return _PlayMeterPaused;
      }
      set {
        bool unpaused = _PlayMeterPaused && !value;
        _PlayMeterPaused = value;
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
      _DimmedImage.SetActive(focusOnMeter != NeedId.Count);
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
        NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
        NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;
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

    //Returns true if it plays the sound, false if not because values are similar.
    private bool PlayMeterMoveSound(float oldVal, float newVal) {
      bool isConnectedToRobot = (RobotEngineManager.Instance.CurrentRobot != null);
      if (newVal - oldVal > float.Epsilon) {
        if (isConnectedToRobot) {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Up);
        }
        else {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Up_Disconnected);
        }
      }
      else if (newVal - oldVal < -float.Epsilon) {
        if (isConnectedToRobot) {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Down_Connected);
        }
        else {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Down);
        }
      }
      else {
        //Difference in values too small, not playing sound.
        return false;
      }
      return true;
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      if (!_RepairMeterPaused) {
        NeedsValue oldRepairValue = nsm.GetCurrentDisplayValue(NeedId.Repair);
        NeedsValue repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
        PlayMeterMoveSound(oldRepairValue.Value, repairValue.Value);
        _RepairMeter.SetTargetAndAnimate(repairValue.Value);
      }

      if (!_EnergyMeterPaused) {
        NeedsValue oldEnergyValue = nsm.GetCurrentDisplayValue(NeedId.Energy);
        NeedsValue energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
        PlayMeterMoveSound(oldEnergyValue.Value, energyValue.Value);
        _EnergyMeter.SetTargetAndAnimate(energyValue.Value);
      }

      if (!_PlayMeterPaused) {
        NeedsValue oldPlayValue = nsm.GetCurrentDisplayValue(NeedId.Play);
        NeedsValue playValue = nsm.PopLatestEngineValue(NeedId.Play);
        PlayMeterMoveSound(oldPlayValue.Value, playValue.Value);
        _PlayMeter.SetTargetAndAnimate(playValue.Value);
      }
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      bool enteredCriticalBracket = false;
      bool enteredFullBracket = false;
      switch (needId) {
      case NeedId.Repair:
        if (!_RepairMeterPaused) {
          NeedsValue repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
          enteredCriticalBracket = repairValue.Bracket == NeedBracketId.Critical;
          enteredFullBracket = repairValue.Bracket == NeedBracketId.Full;
        }
        break;
      case NeedId.Energy:
        if (!_EnergyMeterPaused) {
          NeedsValue energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
          enteredCriticalBracket = energyValue.Bracket == NeedBracketId.Critical;
          enteredFullBracket = energyValue.Bracket == NeedBracketId.Full;
        }
        break;
      case NeedId.Play:
        if (!_PlayMeterPaused) {
          NeedsValue playValue = nsm.PopLatestEngineValue(NeedId.Energy);
          enteredCriticalBracket = playValue.Bracket == NeedBracketId.Critical;
          enteredFullBracket = playValue.Bracket == NeedBracketId.Full;
        }
        break;
      }
      if (enteredCriticalBracket) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Severe_Start);
      }
      if (enteredFullBracket) {
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

      bool isOnboarding = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.NurtureIntro);
      if (isOnboarding) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Appear_Onboarding);
      }
      float oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Repair).Value;
      float newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Repair).Value;
      bool isPlayingSound = PlayMeterMoveSound(oldValue, newValue);
      _RepairMeter.SetTargetAndAnimate(newValue);
      float staggerTime = isOnboarding ? _InitialFillOnboardingStaggerTime : _InitialFillStaggerTime;
      if (isPlayingSound) {
        yield return new WaitForSeconds(staggerTime);
      }
      oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Energy).Value;
      newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Energy).Value;
      if (isOnboarding) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Appear_Onboarding);
      }
      isPlayingSound = PlayMeterMoveSound(oldValue, newValue);
      _EnergyMeter.SetTargetAndAnimate(newValue);
      if (isPlayingSound) {
        yield return new WaitForSeconds(staggerTime);
      }
      oldValue = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Play).Value;
      newValue = NeedsStateManager.Instance.PopLatestEngineValue(NeedId.Play).Value;
      if (isOnboarding) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Appear_Onboarding);
      }
      isPlayingSound = PlayMeterMoveSound(oldValue, newValue);
      _PlayMeter.SetTargetAndAnimate(newValue);

      yield return new WaitForSeconds(_InitialFillStaggerTime);

      HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
      //now that we are done with our initial fills, let's subscribed to further changes
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
      NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;

      _StaggeredFillCoroutine = null;
      yield return 0;
    }

    #region Onboarding

    public void DimNeedMeters(List<NeedId> dimmedMeters) {
      bool dimming = (dimmedMeters != null && dimmedMeters.Count > 0);
      _DimmedImage.SetActive(dimming);

      if (dimming) {
        _DimmedImage.transform.SetAsLastSibling();

        if (!dimmedMeters.Contains(NeedId.Energy)) {
          _EnergyMeter.transform.SetAsLastSibling();
        }

        if (!dimmedMeters.Contains(NeedId.Repair)) {
          _RepairMeter.transform.SetAsLastSibling();
        }

        if (!dimmedMeters.Contains(NeedId.Play)) {
          _PlayMeter.transform.SetAsLastSibling();
        }
      }
      else {
        //reassert standard meter ordering
        _EnergyMeter.transform.SetAsLastSibling();
        _RepairMeter.transform.SetAsLastSibling();
        _PlayMeter.transform.SetAsLastSibling();
      }
    }

    public void OnboardingSkipped() {
      HandleDialogFinishedOpenAnimation();
    }

    #endregion
  }
}