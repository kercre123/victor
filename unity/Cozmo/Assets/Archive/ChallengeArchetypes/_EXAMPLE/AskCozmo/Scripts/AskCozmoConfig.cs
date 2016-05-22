using System;
using UnityEngine;

namespace AskCozmo {
  public class AskCozmoConfig : MinigameConfigBase {

    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public string Param1;

    public string Param2;

    public string[] ParamList;

  }
}

