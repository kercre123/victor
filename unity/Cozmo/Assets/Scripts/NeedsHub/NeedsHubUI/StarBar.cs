using Cozmo.UI;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Cozmo.Needs.UI {
  public class StarBar : MonoBehaviour {
    [SerializeField]
    private SegmentedBar _SegmentedBar;

    [SerializeField]
    private NeedsRewardModal _RewardModalPrefab;

    // Some events will go here on clicking, etc...
    public void Start() {

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.StarUnlocked>(HandleStarUnlocked);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.NeedsState>(HandleGotNeedsState);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.StarLevelCompleted>(HandleStarLevelCompleted);

      // Make really sure we have up to date info
      RobotEngineManager.Instance.Message.GetNeedsState = Singleton<Anki.Cozmo.ExternalInterface.GetNeedsState>.Instance;
      RobotEngineManager.Instance.SendMessage();
      // Hide until we get the first need state update
      _SegmentedBar.gameObject.SetActive(false);
    }

    public void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.StarUnlocked>(HandleStarUnlocked);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.NeedsState>(HandleGotNeedsState);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.StarLevelCompleted>(HandleStarLevelCompleted);
    }

    private void HandleStarUnlocked(Anki.Cozmo.ExternalInterface.StarUnlocked message) {
      UpdateBar(message.currentStars, message.maxStarsForLevel);

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.RewardBox)) {
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.RewardBox);
      }
    }

    private void HandleGotNeedsState(Anki.Cozmo.ExternalInterface.NeedsState message) {
      _SegmentedBar.gameObject.SetActive(true);
      UpdateBar(message.numStarsAwarded, message.numStarsForNextUnlock);
    }

    private void HandleStarLevelCompleted(Anki.Cozmo.ExternalInterface.StarLevelCompleted message) {
      UIManager.OpenModal(_RewardModalPrefab, new ModalPriorityData(),
                          (BaseModal newModal) => {
                            NeedsRewardModal needsModal = (NeedsRewardModal)newModal;
                            needsModal.Init(message.rewards);
                          });
      // TODO: some animation growing the bar exploding and resetting to next level.
      UpdateBar(0, message.starsRequiredForNextUnlock);
    }

    private void UpdateBar(int currSegments, int maxSegments) {
      _SegmentedBar.SetMaximumSegments(maxSegments);
      _SegmentedBar.SetCurrentNumSegments(currSegments);
    }


  }
}