using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class NoRedSpeedTapRules : ISpeedTapRules {

    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
        game.MatchColor = _Colors[randColorIndex];
        game.CozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(game.MatchColor), 0, 0xFF);
        game.PlayerBlock.SetLEDs(CozmoPalette.ColorToUInt(game.MatchColor), 0, 0xFF);
      }
      else {
        // Do non-match
        int playerColorIdx = UnityEngine.Random.Range(0, _Colors.Length);
        int cozmoColorIdx = UnityEngine.Random.Range(0, _Colors.Length);
        if (cozmoColorIdx == playerColorIdx) {
          cozmoColorIdx = (cozmoColorIdx + 1) % _Colors.Length;
        }

        Color playerColor = _Colors[playerColorIdx];
        Color cozmoColor = _Colors[cozmoColorIdx];

        if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.38f) {
          playerColor = Color.red;
          cozmoColor = Color.red;
        }

        game.PlayerBlock.SetLEDs(CozmoPalette.ColorToUInt(playerColor), 0, 0xFF);
        game.CozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(cozmoColor), 0, 0xFF);
      }
    }
  }
}