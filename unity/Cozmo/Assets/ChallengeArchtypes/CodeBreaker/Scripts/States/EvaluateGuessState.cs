using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace CodeBreaker {
  public class EvaluateGuessState : State {
    private CubeCode _WinningCode;
    private CubeState[] _TargetCubeStates;
    private int _NumCorrectPosAndColor;
    private int _NumCorrectColor;

    public EvaluateGuessState(CubeCode winningCubeCode, CubeState[] targetCubeState) {
      _WinningCode = winningCubeCode;
      _TargetCubeStates = targetCubeState;

      CubeCode guess = CreateGuessFromCubeState(_TargetCubeStates);

      // Count the number of correct color and right position
      _NumCorrectPosAndColor = 0;
      List<int> usedGuessIndices = new List<int>();
      for (int i = 0; i < guess.cubeColorIndex.Length; i++) { 
        if (guess.cubeColorIndex[i] == _WinningCode.cubeColorIndex[i]) {
          _NumCorrectPosAndColor++;
          usedGuessIndices.Add(i);
        }
      }
      // Get the # of each color in the winning code, but skip the cubes that are perfectly placed
      Dictionary<int, int> winningColorIndexToNum = GetNumberOfEachColor(_WinningCode, usedGuessIndices);
      // Get the # of each color in the guess, but skip the cubes that are perfectly placed
      Dictionary<int, int> guessColorIndexToNum = GetNumberOfEachColor(guess, usedGuessIndices);

      // Count the number of correct color but wrong position
      _NumCorrectColor = 0;
      foreach (var kvp in winningColorIndexToNum) {
        int numGuessesOfWinningColor; 
        if (guessColorIndexToNum.TryGetValue(kvp.Key, out numGuessesOfWinningColor)) {
          _NumCorrectColor += Mathf.Min(kvp.Value, numGuessesOfWinningColor);
        }
      }
    }

    public override void Enter() {
      base.Enter();

      // Set backpack lights
      CodeBreakerGame game = _StateMachine.GetGame() as CodeBreakerGame;
      LEDId ledToChange = LEDId.NUM_BACKPACK_LEDS;
      for (int i = 0; i < _NumCorrectPosAndColor; i++) {
        ledToChange = GetNextLEDId(ledToChange);
        _CurrentRobot.SetBackpackBarLED(ledToChange, game.CorrectPosAndColorBackpackColor);
      }

      for (int i = 0; i < _NumCorrectColor; i++) {
        ledToChange = GetNextLEDId(ledToChange);
        _CurrentRobot.SetBackpackBarLED(ledToChange, game.CorrectColorOnlyBackpackColor);
      }

      for (int i = 0; i < _WinningCode.NumCubes - (_NumCorrectColor + _NumCorrectPosAndColor); i++) {
        ledToChange = GetNextLEDId(ledToChange);
        _CurrentRobot.SetBackpackBarLED(ledToChange, game.NotCorrectColor);
      }

      // Play animation
      // TODO: Play reaction animation and leave current state based on game state
      if (_NumCorrectPosAndColor >= _WinningCode.NumCubes) {
        _CurrentRobot.SendAnimation(AnimationName.kMajorFail, HandleCozmoLoseAnimationFinished);
      }
      else if (game.AnyGuessesLeft()) {
        _CurrentRobot.SendAnimation(AnimationName.kEnjoyLight, HandleTryAgainAnimationFinished);
      }
      else {
        // Player lost
        _CurrentRobot.SendAnimation(AnimationName.kMajorWin, HandlePlayerLostAnimationFinished);
      }
    }

    private void HandleCozmoLoseAnimationFinished(bool success) {
      PickCodeAgain();
    }

    private void HandleTryAgainAnimationFinished(bool success) {
      _StateMachine.SetNextState(new WaitForGuessState(_WinningCode, _TargetCubeStates));
    }

    private void HandlePlayerLostAnimationFinished(bool success) {
      PickCodeAgain();
    }

    private void PickCodeAgain() {
      LightCube[] lightCubes = new LightCube[_TargetCubeStates.Length];
      for (int i = 0; i < lightCubes.Length; i++) {
        lightCubes[i] = _TargetCubeStates[i].cube;
      }
      _StateMachine.SetNextState(new PickCodeState(lightCubes));
    }

    private CubeCode CreateGuessFromCubeState(CubeState[] cubeStates) {
      CubeCode guess = new CubeCode();
      guess.cubeColorIndex = new int[cubeStates.Length];

      // Evaluate guess from cubes in row
      List<CubeState> sortedCubes = new List<CubeState>();
      while (sortedCubes.Count != cubeStates.Length) {
        float minDot = float.MaxValue;
        CubeState leftmostState = null;
        for (int i = 0; i < cubeStates.Length; i++) {
          if (!sortedCubes.Contains(cubeStates[i])) {
            Vector3 cozmoSpacePos = _CurrentRobot.WorldToCozmo(cubeStates[i].cube.WorldPosition);
            float dotRight = Vector3.Dot(cozmoSpacePos, _CurrentRobot.Right);
            if (dotRight < minDot) {
              minDot = dotRight;
              leftmostState = cubeStates[i];
            }
          }
        }
        if (leftmostState != null) {
          sortedCubes.Add(leftmostState);
        }
        else {
          Debug.LogError("null leftmost state!");
          break;
        }
      }

      for (int i = 0; i < sortedCubes.Count; i++) {
        guess.cubeColorIndex[i] = sortedCubes[i].currentColorIndex;
      }

      return guess;
    }

    private Dictionary<int, int> GetNumberOfEachColor(CubeCode code, List<int> usedIndices) {
      Dictionary<int, int> numberOfEachColorInCode = new Dictionary<int, int>();
      for (int i = 0; i < code.cubeColorIndex.Length; i++) {
        if (!usedIndices.Contains(i)) {
          if (numberOfEachColorInCode.ContainsKey(code.cubeColorIndex[i])) {
            numberOfEachColorInCode[code.cubeColorIndex[i]]++;
          }
          else {
            numberOfEachColorInCode.Add(code.cubeColorIndex[i], 1);
          }
        }
      }
      return numberOfEachColorInCode;
    }

    private LEDId GetNextLEDId(LEDId currentLed) {
      LEDId nextLed = LEDId.NUM_BACKPACK_LEDS;
      switch (currentLed) {
      case LEDId.NUM_BACKPACK_LEDS:
        nextLed = LEDId.LED_BACKPACK_FRONT;
        break;
      case LEDId.LED_BACKPACK_FRONT:
        nextLed = LEDId.LED_BACKPACK_MIDDLE;
        break;
      case LEDId.LED_BACKPACK_MIDDLE:
        nextLed = LEDId.LED_BACKPACK_BACK;
        break;
      case LEDId.LED_BACKPACK_BACK:
        // Cozmo only supports 3 full color LEDs on his back.
        throw new System.NotImplementedException();
      default:
        throw new System.NotImplementedException();
      }
      return nextLed;
    }
  }
}