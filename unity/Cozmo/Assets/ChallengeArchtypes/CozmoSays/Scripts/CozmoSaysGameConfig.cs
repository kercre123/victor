using UnityEngine;
using System.Collections;

namespace CozmoSays {
  public class CozmoSaysGameConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 0;
    }

    public override int NumPlayersRequired() {
      return 1;
    }
  }
}
