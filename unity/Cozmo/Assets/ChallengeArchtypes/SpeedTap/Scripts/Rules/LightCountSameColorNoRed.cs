using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountSameColorNoRed : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);

        int randColor = UnityEngine.Random.Range(0, _Colors.Length);

        for (int i = 0; i < lightCount; ++i) {
          game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[randColor]);
          game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[randColor]);
        }

        game.PlayerWinColor = _Colors[randColor];
        game.CozmoWinColor = _Colors[randColor];
      }
      else {
        // Do non-match

        // first possibility, match red
        if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.3f) {
          int lightCount = UnityEngine.Random.Range(1, 5);

          for (int i = 0; i < lightCount; ++i) {
            game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.red);
            game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(Color.red);
          }
        }
        // second posibility, match count but not colors
        else if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.4f) {
          int lightCount = UnityEngine.Random.Range(1, 5);
          int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
          // pick the second from the remaining colors
          int randColorIndex2 = (randColorIndex + UnityEngine.Random.Range(1, _Colors.Length)) % _Colors.Length;

          for (int i = 0; i < lightCount; ++i) {
            game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[randColorIndex]);
            game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[randColorIndex2]);
          }
        }
        // third posibility, match color but not count.
        else {

          int colorIndex = UnityEngine.Random.Range(1, _Colors.Length);

          int lightCountPlayer = UnityEngine.Random.Range(1, 5);
          int lightCountCozmo = 1 + ((lightCountPlayer + UnityEngine.Random.Range(0, 3)) % 4);

          for (int i = 0; i < lightCountPlayer; ++i) {
            game.PlayerBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[colorIndex]);
          }

          for (int i = 0; i < lightCountCozmo; ++i) {
            game.CozmoBlock.Lights[i].OnColor = CozmoPalette.ColorToUInt(_Colors[colorIndex]);
          }
        }
      }
    }
  }
}
