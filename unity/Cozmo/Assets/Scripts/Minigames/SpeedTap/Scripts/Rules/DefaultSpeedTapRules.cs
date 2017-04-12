using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Cozmo.Util; // For Shuffle.. Because extentions
using System.Linq;

namespace SpeedTap {
  public class DefaultSpeedTapRules {

    private const float _kRedPercentChance = 0.33f;

    protected Color[] _Colors;

    // C# is now ready to support N lights!
    private const int _kMaxLights = 4;

    private int _NumLights;

    protected List<Color> _PotentialColors;

    public DefaultSpeedTapRules(int numLights) {

      _Colors = new Color[] { Color.yellow, Color.green, Color.blue, Color.magenta };
      _NumLights = Mathf.Clamp(numLights, 1, _kMaxLights);
      _PotentialColors = new List<Color>();
    }

    public virtual void SetUsableColors(Color[] colors) {
      _Colors = colors;
    }

    private void FillColors(Color[] lightsToSet) {
      // special case 1,2 just make copies. but 3+ lights sets something random as the extra colors

      // put available colors in random order and then grab first NumLights
      _PotentialColors.Shuffle();
      const int kMaxUnique = 2;
      bool FillExtraSlotsArbitrary = _NumLights > kMaxUnique;
      for (int i = 0; i < kMaxUnique; i++) {
        lightsToSet[i] = _PotentialColors[i % _PotentialColors.Count];
      }

      if (FillExtraSlotsArbitrary) {
        // The remainer can be random and repeat.
        for (int i = kMaxUnique; i < _kMaxLights; i++) {
          int maxRange = Mathf.Min(_NumLights, _PotentialColors.Count);
          lightsToSet[i] = _PotentialColors[Random.Range(0, maxRange)];
        }
      }
      else {
        // for the 1,2 case make a copy and repeat ( ABAB pattern )
        for (int i = _NumLights; i < _kMaxLights; i++) {
          lightsToSet[i] = lightsToSet[i - _NumLights];
        }
      }
      // if this function was called, we actively don't want it matching others.
      // as a result remove one of the colors.
      _PotentialColors.RemoveAt(0);
    }

    protected bool TrySetCubesRed(SpeedTapGame game) {
      bool setCubesRed = false;
      if (UnityEngine.Random.Range(0.0f, 1.0f) < _kRedPercentChance) {
        Color[] redLights = new Color[] { Color.red, Color.red, Color.red, Color.red };
        int playerCount = game.GetPlayerCount();
        for (int i = 0; i < playerCount; ++i) {
          SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)game.GetPlayerByIndex(i);
          SetLEDs(playerInfo, redLights, game);
          playerInfo.PlayerWinColor = Color.white;
        }
        setCubesRed = true;
      }
      game.RedMatch = setCubesRed;
      return setCubesRed;
    }

    protected void CopyLights(Color[] targetColors, Color[] sourceColors) {
      for (int i = 0; i < targetColors.Length; i++) {
        targetColors[i] = sourceColors[i % sourceColors.Length];
      }
    }

    protected void SetLEDs(SpeedTapPlayerInfo playerInfo, Color[] color, SpeedTapGame game) {
      if (game.CurrentRobot != null) {
        LightCube cube = null;
        // if they're just watching, don't set the cube to anything...
        if (playerInfo.playerRole == PlayerRole.Player) {
          game.CurrentRobot.LightCubes.TryGetValue(playerInfo.CubeID, out cube);
          if (cube != null) {
            cube.SetLEDs(color);
          }
        }
      }
    }

    public void SetLights(bool shouldMatch, SpeedTapGame game) {
      // Do match
      _PotentialColors = _Colors.ToList();
      if (shouldMatch) {
        Color[] MatchingColors = new Color[4];
        FillColors(MatchingColors);
        int playerCount = game.GetPlayerCount();
        for (int i = 0; i < playerCount; ++i) {
          SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)game.GetPlayerByIndex(i);
          CopyLights(playerInfo.PlayerWinColors, MatchingColors);
          SetLEDs(playerInfo, playerInfo.PlayerWinColors, game);
        }
      }
      else {
        // Do non-match
        if (!TrySetCubesRed(game)) {
          int playerCount = game.GetPlayerCount();
          for (int i = 0; i < playerCount; ++i) {
            SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)game.GetPlayerByIndex(i);

            FillColors(playerInfo.PlayerWinColors);
            SetLEDs(playerInfo, playerInfo.PlayerWinColors, game);
          }
        }
      }
    }

  }
}
