using UnityEngine;
using System.Collections;

namespace SpeedTap {
  /// <summary>
  /// Show one-three colors on each cube.
  /// Colors on the same cube are allowed to match.
  /// When player's cube and cozmo's are meant to NOT match, colors between
  /// the two cubes are allowed to be the same as long as at least one
  /// is exclusive.
  /// </summary>
  public class LightCountThreeColorSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldMatch, SpeedTapGame game) {
      if (shouldMatch) {
        // Pick two base colors; they can be the same.
        // By design / Sean: randColorIndex and randColorIndex2 are allowed to match
        int[] randColors = new int[3];
        for (int i = 0; i < randColors.Length; i++) {
          randColors[i] = PickRandomColor();
        }

        SetLightsRandomly(game.PlayerWinColors, randColors);
        CopyLights(game.CozmoWinColors, game.PlayerWinColors);

        game.SetLEDs(game.PlayerBlockID, game.PlayerWinColors);
        game.SetLEDs(game.CozmoBlockID, game.CozmoWinColors);
      }
      else {
        // Do non-match
        if (!TrySetCubesRed(game)) {
          // Pick different base colors for the player and Cozmo
          int playerExclusiveColor, cozmoExclusiveColor;
          PickTwoDifferentColors(out playerExclusiveColor, out cozmoExclusiveColor);

          // The player's color should not match cozmo's exclusive color; can match their own
          int playerColorIdx2 = PickRandomColor();
          while (playerColorIdx2 == cozmoExclusiveColor) {
            playerColorIdx2 = PickRandomColor();
          }

          // Cozmo's color should not match the player's exclusive color; can match their own
          int cozmoColorIdx2 = PickRandomColor();
          while (cozmoColorIdx2 == playerExclusiveColor) {
            cozmoColorIdx2 = PickRandomColor();
          }

          int randIndex = UnityEngine.Random.Range(0, game.PlayerWinColors.Length);
          game.PlayerWinColors[randIndex] = _Colors[playerExclusiveColor];
          SetLightsRandomly(game.PlayerWinColors, new int[] { playerExclusiveColor, playerColorIdx2 }, randIndex);

          randIndex = UnityEngine.Random.Range(0, game.CozmoWinColors.Length);
          game.CozmoWinColors[randIndex] = _Colors[cozmoExclusiveColor];
          SetLightsRandomly(game.CozmoWinColors, new int[] { cozmoExclusiveColor, cozmoColorIdx2 }, randIndex);

          game.SetLEDs(game.PlayerBlockID, game.PlayerWinColors);
          game.SetLEDs(game.CozmoBlockID, game.CozmoWinColors);
        }
      }
    }
  }
}
