using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountNoColorSpeedTapRules : ISpeedTapRules {

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);
        for (int i = 0; i < lightCount; ++i) {
          game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
          game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
        }

      }
      else {
        // Do non-match
        int lightCountPlayer = UnityEngine.Random.Range(1, 5);
        int lightCountCozmo = UnityEngine.Random.Range(1, 5);

        if (lightCountPlayer == lightCountCozmo) {
          lightCountPlayer = (lightCountPlayer + 1) % 5;
        }

        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);

        for (int i = 0; i < lightCountPlayer; ++i) {
          game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
        }

        for (int i = 0; i < lightCountCozmo; ++i) {
          game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
        }

      }
    }
  }
}