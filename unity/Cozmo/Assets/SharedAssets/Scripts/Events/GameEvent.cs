// Used for GameLogic, NOT animationEvents
namespace Anki {

  namespace Cozmo {

    public enum GameEvent : byte {
      OnChallengeStarted,
      OnChallengePointScored,
      OnChallengeRoundEnd,
      OnChallengeComplete,
      OnChallengeInterrupted,
      OnLearnedPlayerName,
      OnSawNewUnnamedFace,
      OnSawNewNamedFace,
      OnSawOldUnnamedFace,
      OnSawOldNamedFace,
      OnDailyGoalCompleted,
      OnDailyGoalProgress,
      OnUnlockableEarned,
      OnMeetNewPerson,
      OnChallengeDifficultyUnlock,
      OnNewDayStarted,
      OnUnlockableSparked,
      OnFreeplayInterval,
      OnFreeplayBehaviorSuccess,
      OnChallengeInterval,
      OnMemoryMatchVsComplete,
      OnNewHighScore,
      Count
    }
    // namespace Cozmo
  }
  // namespace Anki

}