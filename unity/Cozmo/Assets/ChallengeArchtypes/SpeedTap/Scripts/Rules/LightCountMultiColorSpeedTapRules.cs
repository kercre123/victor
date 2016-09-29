using UnityEngine;
using System.Collections;

namespace SpeedTap {
  /// <summary>
  /// Show one-four colors on each cube. 
  /// Valid patterns: AAAA, ABAB, AABB, ABBB, ABCC, ABCD
  /// Colors on the same cube are allowed to match.
  /// When player's cube and cozmo's are meant to NOT match, colors between
  /// the two cubes are allowed to be the same.
  /// </summary>
  public class LightCountMultiColorSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldMatch, SpeedTapGame game) {
      if (shouldMatch) {
        // Pick base colors; they can be the same.
        // By design / Sean: randColorIndex and randColorIndex2 are allowed to match
        int[] randColors = new int[4];
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

          int[] playerColors = new int[4];
          playerColors[0] = playerExclusiveColor;
          for (int i = 1; i < playerColors.Length; i++) {
            playerColors[i] = PickRandomColor();
            // The player's color should not match cozmo's exclusive color; can match their own
            while (playerColors[i] == cozmoExclusiveColor) {
              playerColors[i] = PickRandomColor();
            }
          }

          int[] cozmoColors = new int[4];
          cozmoColors[0] = cozmoExclusiveColor;
          for (int i = 1; i < cozmoColors.Length; i++) {
            cozmoColors[i] = PickRandomColor();
            // Cozmo's color should not match the player's exclusive color; can match their own
            while (cozmoColors[i] == playerExclusiveColor) {
              cozmoColors[i] = PickRandomColor();
            }
          }

          int randIndex = UnityEngine.Random.Range(0, game.PlayerWinColors.Length);
          game.PlayerWinColors[randIndex] = _Colors[playerExclusiveColor];
          SetLightsRandomly(game.PlayerWinColors, playerColors, randIndex);

          randIndex = UnityEngine.Random.Range(0, game.CozmoWinColors.Length);
          game.CozmoWinColors[randIndex] = _Colors[cozmoExclusiveColor];
          SetLightsRandomly(game.CozmoWinColors, cozmoColors, randIndex);

          game.SetLEDs(game.PlayerBlockID, game.PlayerWinColors);
          game.SetLEDs(game.CozmoBlockID, game.CozmoWinColors);
        }
      }
    }
  }
}
