using UnityEngine;
using System.Collections;

namespace DASConstants {
  public static class Game {
    public const string kStart = "game.start";
    public const string kType = "game.type";
    public const string kEnd = "game.end";
    public const string kQuit = "game.quit";
    public const string kQuitGameStateSpeedTap = "game.quit.player_rounds_won.{0}.cozmo_rounds_won.{1}.player_score.{2}.cozmo_score.{3}";
    public const string kQuitGameStateSimon = "game.quit.sequence_length.{0}";
    public const string kQuitGameStateCubePounce = "game.quit.player_rounds_won.{0}.cozmo_rounds_won.{1}.player_score.{2}.cozmo_score.{3}";
    public const string kEndWithRank = "game.end.player_rank";
    public const string kRankPlayerLose = "1";
    public const string kRankPlayerWon = "0";
    public const string kEndPointsEarned = "game.end.goal_points_earned.{0}";
  }

  public static class Friendship {
    public const string kAddPoints = "world.friendship.add_points";
    public const string kLevelUp = "world.friendship.level_up";
  }

  public static class Goal {
    public const string kComplete = "world.goal_complete";
    public const string kGeneration = "world.daily_goals";
    public const string kProgressSummary = "world.daily_report";
  }
}