using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class UpgradesStage : OnboardingBaseStage {

    [SerializeField]
    private GameObject _OverviewInstructions;

    private BaseView _UpgradeDetailsView = null;

    private float _StartTime;
    public override void Start() {
      base.Start();

      BaseView.BaseViewOpened += HandleViewOpened;
      GameEventManager.Instance.OnGameEvent += HandleGameEvent;
      BaseView.BaseViewClosed += HandleViewClosed;
      // Highlight region is set by CozmoUnlocksPanel before this phase starts
      OnboardingManager.Instance.ShowOutlineRegion(true, true);
      _StartTime = Time.time;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding__Core_Upgrades);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseView.BaseViewOpened -= HandleViewOpened;
      GameEventManager.Instance.OnGameEvent -= HandleGameEvent;
      BaseView.BaseViewClosed -= HandleViewClosed;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
    }

    private void HandleViewOpened(BaseView view) {
      if (view is CoreUpgradeDetailsDialog) {
        _UpgradeDetailsView = view;
        _OverviewInstructions.SetActive(false);
      }
    }
    // QA could get here if they didn't clear their robot save, but cleared their app save.
    // so onboarding needs to get cleaned up properly.
    private void HandleViewClosed(BaseView view) {
      if (view is CoreUpgradeDetailsDialog) {
        GoToNextState();
      }
    }

    public override void SkipPressed() {
      // Since onboarding hides the close button really the only way to get out
      // Under the unlocks view, closing happens sooner though, so this is just for debug and sparks
      if (_UpgradeDetailsView) {
        _UpgradeDetailsView.CloseView();
      }
      GoToNextState();
    }

    private void HandleGameEvent(GameEventWrapper gameEvent) {
      // Unlocking already closes window
      if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnUnlockableEarned) {
        float timeToUpgrade = Time.time - _StartTime;
        DAS.Event("onboarding.upgrade", timeToUpgrade.ToString());
        GoToNextState();
      }
    }

    private void GoToNextState() {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
