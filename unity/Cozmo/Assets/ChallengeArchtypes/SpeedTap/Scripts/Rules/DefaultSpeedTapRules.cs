using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class DefaultSpeedTapRules : ISpeedTapRules {
  
    private Color[] _Colors = { Color.white, Color.green, Color.blue, Color.magenta };

    public virtual void SetLights(bool shouldTap, SpeedTapGame game) {
      
      if (shouldTap) {
        // Do match
        int randColorIndex = UnityEngine.Random.Range(0, _Colors.Length);
        game.PlayerWinColor = game.CozmoWinColor = _Colors[randColorIndex];
        game.CozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(game.CozmoWinColor), 0, 0xFF);
        game.PlayerBlock.SetLEDs(CozmoPalette.ColorToUInt(game.PlayerWinColor), 0, 0xFF);
      }
      else {
        // Do non-match
        int playerColorIdx = UnityEngine.Random.Range(0, _Colors.Length);
        int cozmoColorIdx = (playerColorIdx + UnityEngine.Random.Range(1, _Colors.Length)) % _Colors.Length;

        Color playerColor = _Colors[playerColorIdx];
        Color cozmoColor = _Colors[cozmoColorIdx];
        game.PlayerBlock.SetLEDs(CozmoPalette.ColorToUInt(playerColor), 0, 0xFF);
        game.CozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(cozmoColor), 0, 0xFF);
      }
    }
  }
}