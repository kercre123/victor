// Used for GameLogic, NOT animationEvents
namespace Anki {

  namespace Cozmo {

    public enum GameEvent : byte {
      OnChallengeStarted,
      // 0
      OnChallengePointScored,
      // 1
      OnChallengeRoundEnd,
      // 2
      OnChallengeComplete,
      // 3
      OnChallengeInterrupted,
      // 4
      OnLearnedPlayerName,
      // 5
      OnSawNewUnnamedFace,
      // 6
      OnSawNewNamedFace,
      // 7
      OnSawOldUnnamedFace,
      // 8
      OnSawOldNamedFace,
      // 9
      OnDailyGoalCompleted,
      // 10
      OnDailyGoalProgress,
      // 11
      OnUnlockableEarned,
      // 12
      OnMeetNewPerson,
      // 13
      OnChallengeDifficultyUnlock,
      // 14
      OnNewDayStarted,
      // 15
      Count
      // 16



    }
    // namespace Cozmo

  }
  // namespace Anki

}