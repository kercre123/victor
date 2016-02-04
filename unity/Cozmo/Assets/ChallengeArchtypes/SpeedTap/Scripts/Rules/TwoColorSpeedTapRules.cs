using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class TwoColorSpeedTapRules : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
        // they are allowed to match
        int randColorIndex2 = UnityEngine.Random.Range(0, _Colors.Length);

        for (int i = 0; i < 4; i++) {
          game.PlayerWinColors[i] = game.CozmoWinColors[i] = _Colors[i % 2 == 0 ? randColorIndex : randColorIndex2];
          game.CozmoBlock.SetLEDs(game.CozmoWinColors[i].ToUInt(), 0, 0xFF);
          game.PlayerBlock.SetLEDs(game.PlayerWinColors[i].ToUInt(), 0, 0xFF);
        }
      }
      else {
        // Do non-match
        int playerColorIdx = UnityEngine.Random.Range(0, _Colors.Length);
        int cozmoColorIdx = (playerColorIdx + Random.Range(1, _Colors.Length)) % _Colors.Length;

        int playerColorIdx2 = UnityEngine.Random.Range(0, _Colors.Length);
        // intentionally playerColorIdx instead of playerColorIdx2, so at most cozmo
        // and player can match 1 color
        int cozmoColorIdx2 = (playerColorIdx + Random.Range(1, _Colors.Length)) % _Colors.Length;

        Color playerColor = _Colors[playerColorIdx];
        Color cozmoColor = _Colors[cozmoColorIdx];
        Color playerColor2 = _Colors[playerColorIdx2];
        Color cozmoColor2 = _Colors[cozmoColorIdx2];

        if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.38f) {
          playerColor = Color.red;
          cozmoColor = Color.red;
          playerColor2 = Color.red;
          cozmoColor2 = Color.red;
        }

        for (int i = 0; i < 4; i++) {
          game.PlayerBlock.SetLEDs((i % 2 == 0 ? playerColor : playerColor2).ToUInt(), 0, 0xFF);
          game.CozmoBlock.SetLEDs((i % 2 == 0 ? cozmoColor : cozmoColor2).ToUInt(), 0, 0xFF);
        }
      }
    }
  }
}