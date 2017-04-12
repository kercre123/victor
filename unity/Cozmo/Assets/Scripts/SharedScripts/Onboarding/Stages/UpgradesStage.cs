using Cozmo.UI;
using UnityEngine;
using Cozmo.Upgrades;

namespace Onboarding {

  public class UpgradesStage : OnboardingBaseStage {

    [SerializeField]
    private GameObject _OverviewInstructions;

    [SerializeField]
    private GameObject _SoftSparkInstructions;

    [SerializeField]
    private CozmoButtonLegacy _ContinueButtonInstance;

    private CoreUpgradeDetailsModal _UpgradeDetailsModal = null;

    private float _StartTime;

    protected virtual void Awake() {
      _ContinueButtonInstance.Initialize(HandleSoftSparkContinueButtonTapped, "Onboarding." + name, "Onboarding");
    }

    public override void Start() {
      base.Start();

      _SoftSparkInstructions.SetActive(false);

      GameEventManager.Instance.OnGameEvent += HandleGameEvent;
      BaseModal.BaseModalOpened += HandleModalOpened;
      BaseModal.BaseModalClosed += HandleModalClosed;
      // Highlight region is set by CozmoUnlocksPanel before this phase starts
      OnboardingManager.Instance.ShowOutlineRegion(true, true);
      _StartTime = Time.time;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Onboarding__Core_Upgrades);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      GameEventManager.Instance.OnGameEvent -= HandleGameEvent;
      BaseModal.BaseModalOpened -= HandleModalOpened;
      BaseModal.BaseModalClosed -= HandleModalClosed;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);
    }

    private void HandleModalOpened(BaseModal modal) {
      if (modal is CoreUpgradeDetailsModal) {
        _UpgradeDetailsModal = (CoreUpgradeDetailsModal)modal;
        if (_OverviewInstructions != null) {
          _OverviewInstructions.SetActive(false);
        }
      }
    }
    // QA could get here if they didn't clear their robot save, but cleared their app save.
    // so onboarding needs to get cleaned up properly.
    private void HandleModalClosed(BaseModal modal) {
      if (modal is CoreUpgradeDetailsModal) {
        ShowSoftSparkInfo();
      }
    }

    public override void SkipPressed() {
      // Since onboarding hides the close button really the only way to get out
      // Under the unlocks view, closing happens sooner though, so this is just for debug and sparks
      if (_UpgradeDetailsModal) {
        _UpgradeDetailsModal.CloseDialog();
      }
      GoToNextState();
    }

    private void HandleGameEvent(GameEventWrapper gameEvent) {
      // Unlocking already closes window
      if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnUnlockableEarned) {
        float timeToUpgrade = Time.time - _StartTime;
        DAS.Event("onboarding.upgrade", timeToUpgrade.ToString());
        // What you wanted was more text
        ShowSoftSparkInfo();
      }
    }

    private void ShowSoftSparkInfo() {
      if (_SoftSparkInstructions != null) {
        _SoftSparkInstructions.SetActive(true);
      }
    }

    private void HandleSoftSparkContinueButtonTapped() {
      GoToNextState();
    }

    private void GoToNextState() {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
