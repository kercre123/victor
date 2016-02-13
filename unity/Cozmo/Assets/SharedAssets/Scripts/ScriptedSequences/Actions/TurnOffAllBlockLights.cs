using System;
using System.ComponentModel;

using System.Collections.Generic;

namespace ScriptedSequences.Actions {
  public class TurnOffAllBlockLights : ScriptedSequenceAction {

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken token = new SimpleAsyncToken();
      IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      foreach (KeyValuePair<int, LightCube> kvp in currentRobot.LightCubes) {
        kvp.Value.SetLEDs(0);
      }
      token.Succeed();
      return token;
    }
  }
}

