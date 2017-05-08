namespace DataPersistence {
  public class GameSkillData {
    public int WinPointsTotal = 0;
    public int LossPointsTotal = 0;
    public int HighestLevel = 0;
    public int LastLevel = 0;

    public void ChangeLevel(int newLevel) {
      if (newLevel > HighestLevel) {
        HighestLevel = newLevel;
      }
      if (newLevel >= 0) {
        LastLevel = newLevel;
      }
      ResetPoints();
    }

    public void ResetPoints() {
      WinPointsTotal = 0;
      LossPointsTotal = 0;
    }
  }
}