using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using UnityEngine;
namespace CodeLab {

  public class ValueSanitizer {
    public static float SanitizeFloat(float inValue) {
      if (float.IsNaN(inValue) || float.IsInfinity(inValue)) {
        return 0.0f;
      }
      else {
        return inValue;
      }
    }
  }

  public class CodeLabVector2 {
    public float x = 0.0f;
    public float y = 0.0f;

    public void Set(float inX, float inY) {
      x = ValueSanitizer.SanitizeFloat(inX);
      y = ValueSanitizer.SanitizeFloat(inY);
    }

    public void Set(Vector2 inVector) {
      Set(inVector.x, inVector.y);
    }
  }

  public class CodeLabVector3 {
    public float x = 0.0f;
    public float y = 0.0f;
    public float z = 0.0f;

    public void Set(float inX, float inY, float inZ) {
      x = ValueSanitizer.SanitizeFloat(inX);
      y = ValueSanitizer.SanitizeFloat(inY);
      z = ValueSanitizer.SanitizeFloat(inZ);
    }

    public void Set(Vector3 inVector) {
      Set(inVector.x, inVector.y, inVector.z);
    }
  }

  public class CubeStateForCodeLab {
    public CodeLabVector3 pos = new CodeLabVector3();
    public CodeLabVector2 camPos = new CodeLabVector2();
    public bool isValid;
    public bool isVisible;
    public bool wasJustTapped;
    public int framesSinceTapped = CodeLabGame.kMaxFramesSinceCubeTapped;
    public float pitch_d;
    public float roll_d;
    public float yaw_d;
  }

  public class FaceStateForCodeLab {
    public CodeLabVector3 pos = new CodeLabVector3();
    public CodeLabVector2 camPos = new CodeLabVector2();
    public string name;
    public bool isVisible;
    public string expression;
  }

  public class DeviceStateForCodeLab {
    public float pitch_d;
    public float roll_d;
    public float yaw_d;
  }

  public class CozmoProjectOpenInWorkspaceRequest {
    public Guid projectUUID;
    public string projectName;
    [JsonConverter(typeof(RawJsonConverter))]
    public string projectJSON;
    public string isSampleStr;
  }

  public class RawJsonConverter : JsonConverter {
    public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
      writer.WriteRawValue(value.ToString());
    }

    public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer) {
      throw new NotImplementedException();
    }

    public override bool CanConvert(Type objectType) {
      return typeof(string).IsAssignableFrom(objectType);
    }

    public override bool CanRead {
      get { return false; }

    }
  }

  public class CozmoStateForCodeLab {
    public float posePitch_d;
    public float poseRoll_d;
    public float poseYaw_d;
    public float liftHeightPercentage;
    public float headAngle_d;
    public int lastTappedCube; // 1..3 (0 for none)
    public CodeLabVector3 pos = new CodeLabVector3();
    public CubeStateForCodeLab cube1 = new CubeStateForCodeLab();
    public CubeStateForCodeLab cube2 = new CubeStateForCodeLab();
    public CubeStateForCodeLab cube3 = new CubeStateForCodeLab();
    public FaceStateForCodeLab face = new FaceStateForCodeLab();
    public DeviceStateForCodeLab device = new DeviceStateForCodeLab();
  }

  public class ScratchRequest {
    public int requestId { get; set; }
    public string command { get; set; }
    public string argString { get; set; }
    public string argString2 { get; set; }
    public string argUUID { get; set; }
    public int argInt { get; set; }
    public uint argUInt { get; set; }
    public uint argUInt2 { get; set; }
    public uint argUInt3 { get; set; }
    public uint argUInt4 { get; set; }
    public uint argUInt5 { get; set; }
    public float argFloat { get; set; }
    public float argFloat2 { get; set; }
    public float argFloat3 { get; set; }
    public float argFloat4 { get; set; }
    public bool argBool { get; set; }
    public bool argBool2 { get; set; }
    public bool argBool3 { get; set; }
  }

  public class ScratchRequests {
    public List<ScratchRequest> messages;
  }

  public delegate void ScratchBlockUpdate();

  public class InProgressScratchBlock {
    private int _RequestId;
    private CodeLabGame _CodeLabGame;
    private ActionType _ActionType;
    private ActionType _WaitingOnActionType;
    private uint _ActionIdTag;
    private ScratchBlockUpdate _UpdateMethod;
    private const ushort kInvalidAudioPlayID = 0;
    private ushort _AudioPlayID = kInvalidAudioPlayID;

    public void Init(int requestId = -1, CodeLabGame codeLabGame = null, ActionType actionType = ActionType.Count, uint actionIdTag = (uint)Anki.Cozmo.ActionConstants.INVALID_TAG) {
      _RequestId = requestId;
      _CodeLabGame = codeLabGame;
      _ActionType = actionType;
      _WaitingOnActionType = ActionType.Count;
      _ActionIdTag = actionIdTag;
      _UpdateMethod = null;
      _AudioPlayID = kInvalidAudioPlayID;
    }

    public void DoUpdate() {
      if (_UpdateMethod != null) {
        _UpdateMethod();
      }
    }

    public void SetAudioPlayID(ushort audioPlayID) {
      _AudioPlayID = audioPlayID;
    }

    public void SetActionData(ActionType actionType, uint actionIdTag) {
      _ActionType = actionType;
      _ActionIdTag = actionIdTag;
    }

    public bool MatchesActionType(ActionType actionType) {
      return ((actionType == _ActionType) || ((actionType == ActionType.All) && (_ActionType != ActionType.Count)));
    }

    public void WaitOnActions() {
      if (!InProgressScratchBlockPool.AreAnyActionsInProgress(_WaitingOnActionType)) {
        AdvanceToNextBlock(true);
      }
    }

    public void SetWaitOnAction(ActionType actionType) {
      DAS.Info("CodeLab.SetWaitOnAction", "actionType = " + actionType.ToString());
      _UpdateMethod = WaitOnActions;
      _WaitingOnActionType = actionType;
    }

    public void CancelAction() {
      DAS.Info("CodeLab.CancelAction", "actionType = " + _ActionType.ToString());
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelActionByIdTag(_ActionIdTag);
      }
      ReleaseFromPool();
    }

    public void AudioPlaySoundCompleted(Anki.Cozmo.Audio.CallbackInfo callbackInfo) {
      AdvanceToNextBlock(true);
    }

    public void VerticalOnAnimationComplete(bool success) {
      // Failure is usually because another action (e.g. animation) was requested by user clicking in Scratch
      // Therefore don't queue the neutral face animation in that case, as it will clobber the just requested animation
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (success && (robot != null)) {
        // TODO: Confirm if we still need this for vertical (or do we force user to trigger the neutral face if they need it?)
        _ActionIdTag = robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.NeutralFace, this.AdvanceToNextBlock, Anki.Cozmo.QueueActionPosition.IN_PARALLEL);
      }
      else {
        AdvanceToNextBlock(success);
      }
    }

    public void NeutralFaceThenAdvanceToNextBlock(bool success) {
      // Failure is usually because another action (e.g. animation) was requested by user clicking in Scratch
      // Therefore don't queue the neutral face animation in that case, as it will clobber the just requested animation
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (success && (robot != null)) {
        _ActionIdTag = robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.NeutralFace, this.AdvanceToNextBlock);
      }
      else {
        AdvanceToNextBlock(success);
      }
    }

    public bool HasActiveRequestPromise() {
      return (this._RequestId >= 0);
    }

    public void OnReleased() {
      ResolveRequestPromise();
      RemoveAllCallbacks();
      Init();
    }

    public void CompletedTurn(bool success) {
      AdvanceToNextBlock(success);
    }

    public void ReleaseFromPool() {
      InProgressScratchBlockPool.ReleaseInProgressScratchBlock(this);
    }

    public void ResolveRequestPromise() {
      if (this._RequestId >= 0) {
        // Calls the JavaScript function resolving the Promise on the block
        _CodeLabGame.EvaluateJS(@"window.resolveCommands[" + this._RequestId + "]();");
        this._RequestId = -1;
      }
    }

    public void AdvanceToNextBlock(bool success) {
      ResolveRequestPromise();
      ReleaseFromPool();
    }

    public void CubeTapped(int id, int tappedTimes, float timeStamp) {
      LightCube.TappedAction -= CubeTapped;
      AdvanceToNextBlock(true);
    }

    public void RemoveAllCallbacks() {
      // Just attempt to remove all callbacks rather than track the ones added
      var rEM = RobotEngineManager.Instance;
      rEM.RemoveCallback<RobotObservedFace>(RobotObservedFace);
      rEM.RemoveCallback<RobotObservedFace>(RobotObservedHappyFace);
      rEM.RemoveCallback<RobotObservedFace>(RobotObservedSadFace);
      rEM.RemoveCallback<RobotObservedObject>(RobotObservedObject);

      if (_AudioPlayID != kInvalidAudioPlayID) {
        Anki.Cozmo.Audio.GameAudioClient.UnregisterCallbackHandler(_AudioPlayID);
        _AudioPlayID = kInvalidAudioPlayID;
      }

      var robot = rEM.CurrentRobot;
      if (robot != null) {
        robot.CancelCallback(VerticalOnAnimationComplete);
        robot.CancelCallback(NeutralFaceThenAdvanceToNextBlock);
        robot.CancelCallback(CompletedTurn);
        robot.CancelCallback(AdvanceToNextBlock);
        robot.CancelCallback(DockWithCube);
        robot.CancelCallback(FinishDockWithCube);
      }
    }

    public void RobotObservedFace(RobotObservedFace message) {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedFace);
      AdvanceToNextBlock(true);
    }

    public void RobotObservedHappyFace(RobotObservedFace message) {
      if (message.expression == Anki.Vision.FacialExpression.Happiness) {
        RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedHappyFace);
        AdvanceToNextBlock(true);
      }
    }

    public void RobotObservedSadFace(RobotObservedFace message) {
      if (CodeLabGame.FaceIsSad(message)) {
        RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedSadFace);
        AdvanceToNextBlock(true);
      }
    }

    public void RobotObservedObject(RobotObservedObject message) {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedObject>(RobotObservedObject);
      AdvanceToNextBlock(true);
    }

    public static Face GetMostRecentlySeenFace() {
      Face lastFaceSeen = null;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        for (int i = 0; i < robot.Faces.Count; ++i) {
          Face face = robot.Faces[i];
          if (face.IsInFieldOfView) {
            return face;
          }
          else {
            if ((lastFaceSeen == null) || (face.NumVisionFramesSinceLastSeen < lastFaceSeen.NumVisionFramesSinceLastSeen)) {
              lastFaceSeen = face;
            }
          }
        }
      }

      return lastFaceSeen;
    }

    private static LightCube GetMostRecentlySeenCube() {
      LightCube lastCubeSeen = null;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
          LightCube cube = kvp.Value;
          if (cube.IsInFieldOfView) {
            return cube;
          }
          else {
            if ((lastCubeSeen == null) || (cube.NumVisionFramesSinceLastSeen < lastCubeSeen.NumVisionFramesSinceLastSeen)) {
              lastCubeSeen = cube;
            }
          }
        }
      }

      return lastCubeSeen;
    }

    // If robot currently sees a cube, try to dock with it.
    public void DockWithCube(bool headAngleActionSucceeded) {
      bool success = false;

      if (headAngleActionSucceeded) {
        LightCube cube = GetMostRecentlySeenCube();
        const int kMaxVisionFramesSinceSeeingCube = 30;
        if ((cube != null) && (cube.NumVisionFramesSinceLastSeen < kMaxVisionFramesSinceSeeingCube)) {
          var robot = RobotEngineManager.Instance.CurrentRobot;
          if (robot != null) {
            success = true;
            _ActionIdTag = robot.AlignWithObject(cube, 0.0f, callback: FinishDockWithCube, usePreDockPose: true, alignmentType: Anki.Cozmo.AlignmentType.LIFT_PLATE, numRetries: 2);
          }
        }
        else {
          DAS.Warn("DockWithCube.NoVisibleCube", "NumVisionFramesSinceLastSeen = " + ((cube != null) ? cube.NumVisionFramesSinceLastSeen : -1));
        }
      }
      else {
        DAS.Warn("DockWithCube.HeadAngleActionFailed", "");
      }

      if (!success) {
        FinishDockWithCube(false);
      }
    }

    private void FinishDockWithCube(bool success) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (!success && (robot != null)) {
        // Play angry animation since Cozmo wasn't able to complete the task
        // As this is on failure, queue anim in parallel - failure here could be because another block was clicked, and we don't want to interrupt that one
        Anki.Cozmo.QueueActionPosition queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
        _ActionIdTag = robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.FrustratedByFailureMajor, callback: AdvanceToNextBlock, queueActionPosition: queuePos);
      }
      else {
        AdvanceToNextBlock(true);
      }
    }
  }

  public static class InProgressScratchBlockPool {
    private static List<InProgressScratchBlock> _available = new List<InProgressScratchBlock>();
    private static List<InProgressScratchBlock> _inUse = new List<InProgressScratchBlock>();

    public static InProgressScratchBlock GetInProgressScratchBlock() {
      // TODO Verify the lock is required and if we can assert on threadId here
      // to make sure we're not opening ourselves to threading issues.
      lock (_available) {
        if (_available.Count != 0) {
          InProgressScratchBlock po = _available[0];
          _inUse.Add(po);
          _available.RemoveAt(0);

          return po;
        }
        else {
          InProgressScratchBlock po = new InProgressScratchBlock();
          po.Init();
          _inUse.Add(po);

          return po;
        }
      }
    }

    public static void ReleaseAllInUse() {
      lock (_available) {
        if (_inUse.Count > 0) {
          for (int i = 0; i < _inUse.Count; ++i) {
            InProgressScratchBlock po = _inUse[i];
            po.OnReleased();
            _available.Add(po);
          }
          _inUse.Clear();
        }
      }
    }

    public static void ReleaseInProgressScratchBlock(InProgressScratchBlock po) {
      po.OnReleased();

      lock (_available) {
        _available.Add(po);
        _inUse.Remove(po);
      }
    }

    public static void UpdateBlocks() {
      lock (_available) {
        for (int i = 0; i < _inUse.Count; /* don't increment here */) {
          InProgressScratchBlock scratchBlock = _inUse[i];
          // Update for a block can complete itself (and remove itself from this list)
          // So track if the block does that, and iterate accordingly
          int previousCount = _inUse.Count;
          scratchBlock.DoUpdate();
          if (_inUse.Count == previousCount) {
            // No change - increment to next block
            ++i;
          }
          else if (_inUse.Count == (previousCount - 1)) {
            // 1 less item - the block completed and removed itself - don't increment i - it's already the next block
          }
          else {
            DAS.Error("CodeLab.UpdateBlocks.BadCountChange", "From " + previousCount + " to " + _inUse.Count);
          }
        }
      }
    }

    public static bool AreAnyActionsInProgress(ActionType actionType) {
      lock (_available) {
        for (int i = 0; i < _inUse.Count; ++i) {
          InProgressScratchBlock scratchBlock = _inUse[i];
          if (scratchBlock.MatchesActionType(actionType)) {
            return true;
          }
        }
      }
      return false;
    }

    public static void CancelActionsOfType(ActionType actionType) {
      lock (_available) {
        for (int i = 0; i < _inUse.Count; /* don't increment here */ ) {
          InProgressScratchBlock scratchBlock = _inUse[i];
          if (scratchBlock.MatchesActionType(actionType)) {
            // Cancellation also removes the block from the list, so don't increment i
            scratchBlock.CancelAction();
          }
          else {
            ++i;
          }
        }
      }
    }
  }

}