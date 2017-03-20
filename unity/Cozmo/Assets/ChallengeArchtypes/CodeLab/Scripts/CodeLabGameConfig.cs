using UnityEngine;
using System.Collections;

namespace CodeLab {
  public class CodeLabGameConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 0;
    }

    public override int NumPlayersRequired() {
      return 1;
    }
  }
}
