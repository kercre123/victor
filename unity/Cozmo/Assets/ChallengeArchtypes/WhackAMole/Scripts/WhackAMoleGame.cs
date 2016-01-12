using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  public class WhackAMoleGame : GameBase {

    public enum MoleState {
      NONE,
      SINGLE,
      BOTH
    }

    public MoleState CubeState {
      get {
        if (ActivatedFaces.Count > 1) {
          return MoleState.BOTH;
        }
        else if (ActivatedFaces.Count == 1) {
          return MoleState.SINGLE;
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
    public float MaxPanicInterval;
    public float MaxConfuseTime;
    public float PanicDecayMin;
    public float PanicDecayMax;


    [SerializeField]
    private Color _InActiveColor = Color.black;
    [SerializeField]
    private Color _ActiveColor = Color.red;

    private int _CubeAID;
    private int _CubeBID;
    private Dictionary<KeyValuePair<int,int>,bool> _FaceActive;
    public List<KeyValuePair<int,int>> ActivatedFaces;

    public KeyValuePair<int,int> CurrentTargetKvP;

    private int _NumCubesRequired;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      WhackAMoleGameConfig config = minigameConfig as WhackAMoleGameConfig;
      MaxPanicTime = config.MaxPanicTime;
      MaxConfuseTime = config.MaxConfusionTime;
      MaxPanicInterval = config.MaxPanicInterval;
      PanicDecayMin = config.PanicDecayMin;
      PanicDecayMax = config.PanicDecayMax;
      _NumCubesRequired = config.NumCubesRequired();
      CurrentTargetKvP = new KeyValuePair<int, int>();
      ActivatedFaces = new List<KeyValuePair<int, int>>();
      _FaceActive = new Dictionary<KeyValuePair<int, int>, bool>();
      _GamePanel = UIManager.OpenView(_WhackAMolePanelPrefab).GetComponent<WhackAMolePanel>();
      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      SetUpCubes();
    }

    public void SetUpCubes() {
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WhackAMoleIdle(), _NumCubesRequired, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
    }

    private void InitialCubesDone() {
      bool aDone = false;
      bool bDone = false;
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
        InitializeButtons();
      }
      else {
        Debug.LogError("Cubes are NOT set up properly. Could not find an A and B cube for whack a mole.");
      }
    }

    // Overload for passing in both ints instead of full KvP
    public void ToggleCubeFace(int cubeID, int faceID) {
      ToggleCubeFace(new KeyValuePair<int,int>(cubeID, faceID));
    }

    public void ToggleCubeFace(KeyValuePair<int,int> KvP) {
      int cubeID = KvP.Key;
      int faceID = KvP.Value;
      Color toSet;
      bool isActive;
      bool isCubeA;
      Button currButt;
      isCubeA = (cubeID == _CubeAID);

      if (isCubeA) {
        isActive = !_FaceActive[KvP];
        currButt = _GamePanel.CubeAFaceButtons[faceID];
        _FaceActive[KvP] = isActive;
      }
      else {
        isActive = !_FaceActive[KvP];
        currButt = _GamePanel.CubeBFaceButtons[faceID];
        _FaceActive[KvP] = isActive;
      }

      if (isActive) {
        toSet = _ActiveColor;
        CurrentTargetKvP = KvP;
        if (!ActivatedFaces.Contains(CurrentTargetKvP)) {
          ActivatedFaces.Add(CurrentTargetKvP);
        }
      }
      else {
        toSet = _InActiveColor;
        if ((CurrentTargetKvP.Key == cubeID) && (CurrentTargetKvP.Value == faceID)) {
          if (ActivatedFaces.Contains(CurrentTargetKvP)) {
            ActivatedFaces.Remove(CurrentTargetKvP);
          }
          CurrentTargetKvP = new KeyValuePair<int, int>(-1,-1);
          for (int i = 0; i < ActivatedFaces.Count; i++) {
            CurrentTargetKvP = ActivatedFaces[i];
          }
        }
      }
      CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OnColor = CozmoPalette.ColorToUInt(toSet);
      currButt.image.color = toSet;
      if (MoleStateChanged != null) {
        MoleStateChanged.Invoke(CubeState);
      }
    }

    public void InitializeButtons() {
      Button curr;
      CurrentTargetKvP = new KeyValuePair<int, int>(-1,-1);
      ActivatedFaces.Clear();
      _FaceActive.Clear();
      LightCube cube;

      // Reset all of Cube A's faces.
      if (CurrentRobot.LightCubes.TryGetValue(_CubeAID, out cube)) {
        cube.SetLEDs(_InActiveColor);
      }
      for (int i = 0; i < _GamePanel.CubeAFaceButtons.Count; i++) {
        curr = _GamePanel.CubeAFaceButtons[i];
        curr.onClick.RemoveAllListeners();
        SetUpButton(curr, _CubeAID, i);
        _FaceActive.Add(new KeyValuePair<int, int>(_CubeAID,i),false);
      }

      // Reset all of Cube B's faces.
      if (CurrentRobot.LightCubes.TryGetValue(_CubeBID, out cube)) {
        cube.SetLEDs(_InActiveColor);
      }

      for (int i = 0; i < _GamePanel.CubeBFaceButtons.Count; i++) {
        curr = _GamePanel.CubeBFaceButtons[i];
        curr.onClick.RemoveAllListeners();
        SetUpButton(curr, _CubeBID, i);
        _FaceActive.Add(new KeyValuePair<int, int>(_CubeBID,i),false);
      }
    }

    void SetUpButton(Button button, int cubeID, int faceID) {
      button.onClick.AddListener(() => {
        ToggleCubeFace(cubeID, faceID);
      });
      button.image.color = _InActiveColor;
    }

    public float GetRelativeRad(KeyValuePair<int,int> KvP) {
      float deg = (90.0f * (3-KvP.Value));
      float result = Mathf.Deg2Rad * deg;
      Debug.Log(string.Format("Relative Rad - {0} Degrees - {1}", result,deg));
      return result;
    }

    protected override void CleanUpOnDestroy() { 
    
    }
  }
}
