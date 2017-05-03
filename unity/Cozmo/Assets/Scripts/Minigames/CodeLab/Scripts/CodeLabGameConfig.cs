using UnityEngine;
using System.Collections;

namespace CodeLab {
  public class CodeLabGameConfig : ChallengeConfigBase {
    public override int NumCubesRequired() {
      return 0;
    }

    public override int NumPlayersRequired() {
      return 1;
    }
  }
}
