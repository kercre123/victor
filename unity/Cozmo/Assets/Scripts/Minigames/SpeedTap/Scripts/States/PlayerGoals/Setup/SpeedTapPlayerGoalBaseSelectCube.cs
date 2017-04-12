using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public abstract class SpeedTapPlayerGoalBaseSelectCube : PlayerGoalBase {

    protected System.Action<PlayerInfo, int> OnCubeSelected;

    protected int _BlockID;

    public static SpeedTapPlayerGoalBaseSelectCube Factory(PlayerInfo playerInfo, int blockID,
                                    System.Action<PlayerInfo, int> CompleteCallback,
                                    System.Action<PlayerInfo, int> OnCubeSelected) {
      SpeedTapPlayerGoalBaseSelectCube created = null;
      if (playerInfo.playerType == PlayerType.Cozmo) {
        created = new SpeedTapPlayerGoalCozmoSelectCube();
      }
      else if (playerInfo.playerType == PlayerType.Human) {
        created = new SpeedTapPlayerGoalHumanSelectCube();
      }
      else if (playerInfo.playerType == PlayerType.Automation) {
        created = new SpeedTapPlayerGoalAutoSelectCube();
      }
      if (created != null) {
        created.OnPlayerGoalCompleted = CompleteCallback;
        created.OnCubeSelected = OnCubeSelected;
        created._BlockID = blockID;
        created._ParentPlayer = playerInfo;
      }
      return created;
    }
  }
}