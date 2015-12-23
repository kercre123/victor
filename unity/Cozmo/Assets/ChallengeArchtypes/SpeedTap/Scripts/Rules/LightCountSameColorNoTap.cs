using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountSameColorNoTap : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        for (int i = 0; i < lightCount; ++i) {
          game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
          game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.white);
        }

      }
      else {
        // Do non-match

        if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.4f) {
          int lightCount = UnityEngine.Random.Range(1, 5);
          int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);

          for (int i = 0; i < lightCount; ++i) {
            game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[randColorIndex]);
          }
        }
        else {

          int lightCountPlayer = UnityEngine.Random.Range(1, 5);
          int lightCountCozmo = UnityEngine.Random.Range(1, 5);

          if (lightCountPlayer == lightCountCozmo) {
            lightCountPlayer = (lightCountPlayer + 1) % 5;
          }

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
}
