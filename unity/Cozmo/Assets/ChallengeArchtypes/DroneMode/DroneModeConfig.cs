namespace Cozmo {
  namespace Minigame {
    public class DroneModeConfig : MinigameConfigBase {
      public override int NumCubesRequired() {
        return 0;
      }

      public override int NumPlayersRequired() {
        return 1;
      }
    }
  }
}