namespace DataPersistence {
  public class GameSkillData {
    public int WinPointsTotal = 0;
    public int LossPointsTotal = 0;
    public int HighestLevel = 0;
    public int LastLevel = 0;

    public void ChangeLevel(int new_level) {
      if (new_level > HighestLevel) {
        HighestLevel = new_level;
      }
      if (new_level >= 0) {
        LastLevel = new_level;
      }
      WinPointsTotal = 0;
      LossPointsTotal = 0;
    }
  }
}