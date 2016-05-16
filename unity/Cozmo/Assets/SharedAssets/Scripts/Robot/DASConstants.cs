using UnityEngine;
using System.Collections;

namespace DASConstants {
  public static class Game {
    public const string kStart = "game.start";
    public const string kType = "game.type";
    public const string kEnd = "game.end";
    public const string kQuit = "game.quit";
    public const string kQuitGameScore = "game.quit.score";
    public const string kQuitGameRoundsWon = "game.quit.rounds_won";
    public const string kEndWithRank = "game.end.player_rank";
    public const string kRankPlayerLose = "1";
    public const string kRankPlayerWon = "0";
    public const string kEndPointsEarned = "game.end.goal_points_earned.{0}";
    public const string kGlobal = "$game";
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