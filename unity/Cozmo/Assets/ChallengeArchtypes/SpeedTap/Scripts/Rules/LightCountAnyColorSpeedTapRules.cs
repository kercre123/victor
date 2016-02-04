using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountAnyColorSpeedTapRules : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta, Color.yellow };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);
        game.PlayerWinColor = Color.black;
        game.CozmoWinColor = Color.black;

        for (int i = 0; i < lightCount; ++i) {
          int randColor = UnityEngine.Random.Range(0, _Colors.Length);
          game.CozmoBlock.Lights[i].OnColor = _Colors[randColor].ToUInt();
          game.PlayerBlock.Lights[i].OnColor = _Colors[randColor].ToUInt();

          game.PlayerWinColors[i] = _Colors[randColor];
          game.CozmoWinColors[i] = _Colors[randColor];
        }

      }
      else {
        // Do non-match

        // first possibility, match red
        if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.3f) {
          int lightCount = UnityEngine.Random.Range(1, 5);

          for (int i = 0; i < lightCount; ++i) {
            game.PlayerBlock.Lights[i].OnColor = Color.red.ToUInt();
            game.CozmoBlock.Lights[i].OnColor = Color.red.ToUInt();
          }
        }
        // second posibility, match count but not colors
        else if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.4f) {
          int lightCount = UnityEngine.Random.Range(1, 5);

          for (int i = 0; i < lightCount; ++i) {
            int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
            // because the order doesn't matter, force the second color to be the first + 1, (guarantees the two don't match
            int randColorIndex2 = (randColorIndex + 1) % _Colors.Length;
            game.PlayerBlock.Lights[i].OnColor = _Colors[randColorIndex].ToUInt();
            game.CozmoBlock.Lights[i].OnColor = _Colors[randColorIndex2].ToUInt();
          }
        }
        // third posibility, match color but not count.
        else {


          int lightCountPlayer = UnityEngine.Random.Range(1, 5);
          int lightCountCozmo = 1 + ((lightCountPlayer + UnityEngine.Random.Range(0, 3)) % 4);

          for (int i = 0; i < 4; ++i) {
            int colorIndex = UnityEngine.Random.Range(1, _Colors.Length);

            if (i < lightCountPlayer) {
              game.PlayerBlock.Lights[i].OnColor = _Colors[colorIndex].ToUInt();
            }
            if (i < lightCountCozmo) {
              game.CozmoBlock.Lights[i].OnColor = _Colors[colorIndex].ToUInt();
            }
          }
        }
      }
    }
  }
}
