public class SpeedTapGameConfig : MinigameConfigBase {
  public override int NumCubesRequired() {
    return 2;
  }

  public override int NumPlayersRequired() {
    return 1;
  }

  public int Rounds;
  public int MaxScorePerRound;
  public SpeedTap.SpeedTapRuleSet RuleSet;
}
