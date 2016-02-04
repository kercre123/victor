using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountTwoColorSpeedTapRules : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);

        int randColor = UnityEngine.Random.Range(0, _Colors.Length);
        int randColor2 = UnityEngine.Random.Range(0, _Colors.Length);

        for (int i = 0; i < lightCount; ++i) {
          game.CozmoBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? randColor : randColor2].ToUInt();
          game.PlayerBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? randColor : randColor2].ToUInt();
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
            game.PlayerBlock.Lights[i].OnColor = Color.red.ToUInt();
            game.CozmoBlock.Lights[i].OnColor = Color.red.ToUInt();
          }
        }
        // second posibility, match count but not colors
        else if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.4f) {
          int lightCount = UnityEngine.Random.Range(1, 5);
          int playerColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
          int playerColorIndex2 = UnityEngine.Random.Range(0, _Colors.Length);
          // pick the second from the remaining colors
          int cozmoColorIndex = (playerColorIndex + UnityEngine.Random.Range(1, _Colors.Length)) % _Colors.Length;
          int cozmoColorIndex2 = (playerColorIndex + UnityEngine.Random.Range(1, _Colors.Length)) % _Colors.Length;

          for (int i = 0; i < lightCount; ++i) {
            game.PlayerBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? playerColorIndex : playerColorIndex2].ToUInt();
            game.CozmoBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? cozmoColorIndex : cozmoColorIndex2].ToUInt();
          }
        }
        // third posibility, match color but not count.
        else {

          int colorIndex = UnityEngine.Random.Range(1, _Colors.Length);
          int colorIndex2 = UnityEngine.Random.Range(1, _Colors.Length);

          int lightCountPlayer = UnityEngine.Random.Range(1, 5);
          int lightCountCozmo = 1 + ((lightCountPlayer + UnityEngine.Random.Range(0, 3)) % 4);

          for (int i = 0; i < lightCountPlayer; ++i) {
            game.PlayerBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? colorIndex : colorIndex2].ToUInt();
          }

          for (int i = 0; i < lightCountCozmo; ++i) {
            game.CozmoBlock.Lights[i].OnColor = _Colors[i % 2 == 0 ? colorIndex : colorIndex2].ToUInt();
          }
        }
      }
    }
  }
}
