using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  public class WhackAMoleGame : GameBase {
    const float kCloseUpDistance = 50f;
    const float kMinFlashFreq = 5000f;
    const float kMaxFlashFreq = 500f;

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
    public float MoleTimeout;

    [SerializeField]
    private Color _InActiveColor = Color.clear;
    [SerializeField]
    private Color _ActiveColor = Color.red;

    private int _CubeAID;
    private int _CubeBID;
    private Dictionary<KeyValuePair<int,int>,bool> _FaceActive;
    public List<KeyValuePair<int,int>> ActivatedFaces;
    private Dictionary<KeyValuePair<int,int>,float> _ActivatedTimestamp;

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
      MoleTimeout = config.MoleTimeout;
      CurrentTargetKvP = new KeyValuePair<int, int>();
      ActivatedFaces = new List<KeyValuePair<int, int>>();
      _FaceActive = new Dictionary<KeyValuePair<int, int>, bool>();
      _ActivatedTimestamp = new Dictionary<KeyValuePair<int, int>, float>();
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

    protected override void Update() {
      base.Update();
      RefreshCubeTimers();
    }

    private void RefreshCubeTimers() {
      for (int i = 0; i < ActivatedFaces.Count; i++) {
        KeyValuePair<int,int> KvP = ActivatedFaces[i];
        float origTimestamp = _ActivatedTimestamp[KvP];
        float remainingTime = ((Time.time - origTimestamp)/MoleTimeout);
        if (remainingTime >= 1.0f) {
          ToggleCubeFace(KvP);
          continue;
        }
        uint freq = (uint)Mathf.Lerp(kMinFlashFreq,kMaxFlashFreq,remainingTime);
        //CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OffPeriodMs = freq;
        CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OnPeriodMs = freq;
      }
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

      // If Set to active, add the timestamp and activated face to the appropriate dict/list.
      // If Set to inactive, remove timestamp and activated face from dict/list and find new target.
      if (isActive) {
        toSet = _ActiveColor;
        CurrentTargetKvP = KvP;
        if (!ActivatedFaces.Contains(CurrentTargetKvP)) {
          ActivatedFaces.Add(CurrentTargetKvP);
        }
        if (!_ActivatedTimestamp.ContainsKey(CurrentTargetKvP)) {
          _ActivatedTimestamp.Add(CurrentTargetKvP, Time.time);
        }
      }
      else {
        toSet = _InActiveColor;
        if (ActivatedFaces.Contains(KvP)) {
          ActivatedFaces.Remove(KvP);
        }
        if (_ActivatedTimestamp.ContainsKey(KvP)) {
          _ActivatedTimestamp.Remove(KvP);
        }
        if ((CurrentTargetKvP.Key == cubeID) && (CurrentTargetKvP.Value == faceID)) {
          CurrentTargetKvP = new KeyValuePair<int, int>(-1,-1);
          for (int i = 0; i < ActivatedFaces.Count; i++) {
            CurrentTargetKvP = ActivatedFaces[i];
          }
        }
      }
      CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OffPeriodMs = (uint)kMinFlashFreq;
      CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OnPeriodMs = 0;
      CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OnColor = CozmoPalette.ColorToUInt(toSet);
      CurrentRobot.LightCubes[KvP.Key].Lights[KvP.Value].OffColor = CozmoPalette.ColorToUInt(_InActiveColor);
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
      float deg = (90.0f * (KvP.Value+1));
      float result = Mathf.Deg2Rad * deg;
      Debug.Log(string.Format("Cube{2}:Face{3} - Relative Rad - {0} Degrees - {1}", result,deg, KvP.Key,KvP.Value));
      return result;
    }

    public Vector3 GetRelativePos(KeyValuePair<int,int> KvP) {
      Vector3 point = CurrentRobot.LightCubes[KvP.Key].WorldPosition;
      float offset = kCloseUpDistance;
      Vector3 angle = Vector3.zero;
      switch (KvP.Value) 
      {
      case 0:
        angle = Vector3.down;
        break;
      case 1:
        angle = Vector3.right;
        break;
      case 2:
        angle = Vector3.up;
        break;
      case 3:
        angle = Vector3.left;
        break;
      }
      angle *= offset;
      return (point + angle);
    }

    // Cozmo hates keeping his head at an angle where he can see cubes, this is probably problematic for a lot of reasons.
    // This hack exists to try and override whatever is causing it.
    public void FixCozmoAngles() {

      if (CurrentRobot.HeadAngle != -0.8f) {
        CurrentRobot.SetHeadAngle(-0.8f, null, Anki.Cozmo.QueueActionPosition.NEXT);
      }
      if (CurrentRobot.LiftHeight != 1.0f) { 
        CurrentRobot.SetLiftHeight(1.0f, null, Anki.Cozmo.QueueActionPosition.NEXT);
      }
    }

    protected override void CleanUpOnDestroy() { 
    
    }
  }
}
