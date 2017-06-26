using Cozmo.UI;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Cozmo.Needs.UI {
  public class StarBar : MonoBehaviour {
    [SerializeField]
    private SegmentedBarAnimated _SegmentedBar;

    [SerializeField]
    private NeedsRewardModal _RewardModalPrefab;

    // Some events will go here on clicking, etc...
    public void Start() {

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.StarUnlocked>(HandleStarUnlocked);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.NeedsState>(HandleGotNeedsState);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.StarLevelCompleted>(HandleStarLevelCompleted);

      // Requested GetNeedsState would do this automatically
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        const int kCurrStarsMock = 0;
        const int kMaxStarsMock = 3;
        UpdateBar(kCurrStarsMock, kMaxStarsMock);
      }
      // Make really sure we have up to date info
      RobotEngineManager.Instance.Message.GetNeedsState = Singleton<Anki.Cozmo.ExternalInterface.GetNeedsState>.Instance;
      RobotEngineManager.Instance.SendMessage();

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.RewardBox)) {
        OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingPhaseComplete;
        OnboardingManager.Instance.OnOnboardingPhaseStarted += HandleOnboardingPhaseStarted;
      }
    }

    public void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.StarUnlocked>(HandleStarUnlocked);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.NeedsState>(HandleGotNeedsState);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.StarLevelCompleted>(HandleStarLevelCompleted);

      OnboardingManager.Instance.OnOnboardingPhaseCompleted -= HandleOnboardingPhaseComplete;
      OnboardingManager.Instance.OnOnboardingPhaseStarted -= HandleOnboardingPhaseStarted;
    }

    private void HandleStarUnlocked(Anki.Cozmo.ExternalInterface.StarUnlocked message) {
      UpdateBar(message.currentStars, message.maxStarsForLevel);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Earn_Token);

      if (!OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro) &&
           OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.RewardBox)) {
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.RewardBox);
      }
    }

    private void HandleGotNeedsState(Anki.Cozmo.ExternalInterface.NeedsState message) {
      UpdateBar(message.numStarsAwarded, message.numStarsForNextUnlock);
    }

    private void HandleStarLevelCompleted(Anki.Cozmo.ExternalInterface.StarLevelCompleted message) {
      UIManager.OpenModal(_RewardModalPrefab, new ModalPriorityData(),
                          (BaseModal newModal) => {
                            NeedsRewardModal needsModal = (NeedsRewardModal)newModal;
                            needsModal.Init(message.rewards);
                          });
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Earn_Bonus_Box);
      // TODO: some animation growing the bar exploding and resetting to next level.
      UpdateBar(0, message.starsRequiredForNextUnlock);
    }

    private void UpdateBar(int currSegments, int maxSegments) {
      _SegmentedBar.SetMaximumSegments(maxSegments);
      _SegmentedBar.SetCurrentNumSegments(currSegments);
    }

    #region Onboarding
    private void HandleOnboardingPhaseComplete(OnboardingManager.OnboardingPhases phase) {
      SetHighlight(false);
    }
    private void HandleOnboardingPhaseStarted(OnboardingManager.OnboardingPhases phase) {
      if (phase == OnboardingManager.OnboardingPhases.PlayIntro ||
          phase == OnboardingManager.OnboardingPhases.RewardBox) {
        SetHighlight(true);
      }
    }
    private void SetHighlight(bool highlight) {
      _SegmentedBar.SetBoolAnimParam("Highlighted", highlight);
    }
    #endregion

  }
}