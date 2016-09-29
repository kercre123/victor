using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class NoRedSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldMatch, SpeedTapGame game) {

      if (shouldMatch) {
        // Do match
        Color matchColor = _Colors[PickRandomColor()];
        game.CozmoWinColor = game.PlayerWinColor = matchColor;

        game.SetLEDs(game.PlayerBlockID, matchColor);
        game.SetLEDs(game.CozmoBlockID, matchColor);
      }
      else {
        // Do non-match
        if (!TrySetCubesRed(game)) {
          int playerColorIdx, cozmoColorIdx;
          PickTwoDifferentColors(out playerColorIdx, out cozmoColorIdx);

          Color playerColor = _Colors[playerColorIdx];
          game.PlayerWinColor = playerColor;
          game.SetLEDs(game.PlayerBlockID, playerColor);

          Color cozmoColor = _Colors[cozmoColorIdx];
          game.CozmoWinColor = cozmoColor;
          game.SetLEDs(game.CozmoBlockID, cozmoColor);
        }
      }
    }
  }
}