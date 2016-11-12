// Used for GameLogic, NOT animationEvents
namespace Anki {

  namespace Cozmo {

    public enum GameEvent : byte {
      OnChallengeStarted,
      OnChallengePointScored,
      OnChallengeRoundEnd,
      OnChallengeComplete,
      OnChallengeInterrupted,
      OnDailyGoalCompleted,
      OnDailyGoalProgress,
      OnUnlockableEarned,
      OnReEnrollFace,
      OnMeetNewPerson,
      OnChallengeDifficultyUnlock,
      OnNewDayStarted,
      OnUnlockableSparked,
      OnFreeplayInterval,
      OnFreeplayBehaviorSuccess,
      OnChallengeInterval,
      OnConnectedInterval,
      OnMemoryMatchVsComplete,
      OnNewHighScore,
      Count
    }
    // namespace Cozmo
  }
  // namespace Anki

}