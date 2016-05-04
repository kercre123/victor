using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class NoRedSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldMatch, SpeedTapGame game) {

      if (shouldMatch) {
        // Do match
        Color matchColor = _Colors[PickRandomColor()];
        game.CozmoWinColor = game.PlayerWinColor = matchColor;
        game.CozmoBlock.SetLEDs(matchColor);
        game.PlayerBlock.SetLEDs(matchColor);
      }
      else {
        // Do non-match
        if (!TrySetCubesRed(game)) {
          int playerColorIdx, cozmoColorIdx;
          PickTwoDifferentColors(out playerColorIdx, out cozmoColorIdx);

          Color playerColor = _Colors[playerColorIdx];
          game.PlayerWinColor = playerColor;
          game.PlayerBlock.SetLEDs(playerColor);

          Color cozmoColor = _Colors[cozmoColorIdx];
          game.CozmoWinColor = cozmoColor;
          game.CozmoBlock.SetLEDs(cozmoColor);
        }
      }
    }
  }
}