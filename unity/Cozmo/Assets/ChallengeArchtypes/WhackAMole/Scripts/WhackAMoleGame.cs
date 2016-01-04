using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  public class WhackAMoleGame : GameBase {

    public enum MoleState {
      NONE,
      SINGLE,
      TOO_MANY
    }

    public MoleState CubeState {
      get {
        if (_CubeActiveA || _CubeActiveB) {
          return MoleState.SINGLE;
        }
        else if (_CubeActiveA && _CubeActiveB) {
          return MoleState.TOO_MANY;
        }
        else {
          return MoleState.NONE;
        }
      }
    }

    public Action<MoleState> MoleStateChanged;

    [SerializeField]
    private WhackAMolePanel _WhackAMolePanelPrefab;
    private WhackAMolePanel _GamePanel;

    public float MaxPanicTime;
    public float MaxConfuseTime;

    [SerializeField]
    private Color _CubeAColor = Color.green;
    [SerializeField]
    private Color _CubeBColor = Color.blue;
    [SerializeField]
    private Color _ActiveColor = Color.red;

    private int _CubeAID;
    private int _CubeBID;
    private bool _CubeActiveB = false;
    private bool _CubeActiveA = false;

    public LightCube CurrentTarget = null;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      WhackAMoleGameConfig config = minigameConfig as WhackAMoleGameConfig;
      MaxPanicTime = config.MaxPanicTime;
      MaxConfuseTime = config.MaxConfusionTime;

      _GamePanel = UIManager.OpenView(_WhackAMolePanelPrefab).GetComponent<WhackAMolePanel>();

      // Set Buttons to Yellow until cubes are set up properly
      _GamePanel.CubeAButton.image.color = Color.yellow;
      _GamePanel.CubeBButton.image.color = Color.yellow;
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WhackAMoleIdle(), 2, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
    }

    private void InitialCubesDone() {
      bool aDone = false;
      bool bDone = false;
      _CubeActiveA = false;
      _CubeActiveB = false;
      // Look through Visible objects to find 2 light cubes, set one to cubeA, the other
      // to cubeB. Track these IDs, if they are ever confirmed as no longer available, return
      // to cube setup state.
      foreach (KeyValuePair<int, LightCube> kvp in CurrentRobot.LightCubes) {
        if (!aDone) {
          _CubeAID = kvp.Key;
          aDone = true;
        }
        else if (!bDone) {
          _CubeBID = kvp.Key;
          bDone = true;
        }
      }

      if (aDone && bDone) {
        _GamePanel.CubeAButton.onClick.RemoveAllListeners();
        _GamePanel.CubeBButton.onClick.RemoveAllListeners();
        _GamePanel.CubeAButton.onClick.AddListener(() => {
          _CubeActiveA = !_CubeActiveA;
          UpdateCubeState();
        });
        _GamePanel.CubeBButton.onClick.AddListener(() => {
          _CubeActiveB = !_CubeActiveB;
          UpdateCubeState();
        });
        UpdateCubeState();
      }
      else {
        Debug.LogError("Cubes are NOT set up properly. Could not find an A and B cube for whack a mole.");
      }
    }

    private void UpdateCubeState() {
      bool cubeLost = false;
      if (CurrentRobot.LightCubes.ContainsKey(_CubeAID)) {
        if (_CubeActiveA) {
          _GamePanel.CubeAButton.image.color = _ActiveColor;
          CurrentRobot.LightCubes[_CubeAID].SetLEDs(_ActiveColor);
        }
        else {
          _GamePanel.CubeAButton.image.color = _CubeAColor;
          CurrentRobot.LightCubes[_CubeAID].SetLEDs(_CubeBColor);
        }
      }
      else {
        Debug.LogError(string.Format("Can NOT find Cube A ID : {0}",_CubeAID));
        cubeLost = true;
      }

      if (CurrentRobot.LightCubes.ContainsKey(_CubeBID)) {
        if (_CubeActiveB) {
          _GamePanel.CubeBButton.image.color = _ActiveColor;
          CurrentRobot.LightCubes[_CubeBID].SetLEDs(_ActiveColor);
        }
        else {
          _GamePanel.CubeBButton.image.color = _CubeBColor;
          CurrentRobot.LightCubes[_CubeBID].SetLEDs(_CubeBColor);
        }
      }
      else {
        Debug.LogError(string.Format("Can NOT find Cube B ID : {0}",_CubeBID));
        cubeLost = true;
      }

      if (cubeLost) {
        // Set Buttons to Yellow when we lose a cube
        _GamePanel.CubeAButton.image.color = Color.yellow;
        _GamePanel.CubeBButton.image.color = Color.yellow;
        // TODO: Use Confusion Animation transition to return to setup state.
        InitialCubesState initCubeState = new InitialCubesState();
        initCubeState.InitialCubeRequirements(new WhackAMoleIdle(), 2, true, InitialCubesDone);
        _StateMachine.SetNextState(initCubeState);
      }
      else {
        // Upon entry, states listen to the MoleStateChanged Action, then change
        // state based on current state.
        if (MoleStateChanged != null) {
          MoleStateChanged.Invoke(CubeState);
        }
      }
    }

    protected override void CleanUpOnDestroy() { 
    
    }
  }
}
