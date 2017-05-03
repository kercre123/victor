using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
namespace CodeLab {

  public class ScratchRequest {
    public int requestId { get; set; }
    public string command { get; set; }
    public string argString { get; set; }
    public string argUUID { get; set; }
    public int argInt { get; set; }
    public uint argUInt { get; set; }
    public float argFloat { get; set; }
  }

  public class InProgressScratchBlock {
    private int _RequestId;
    private WebViewObject _WebViewObjectComponent;

    public void Init(int requestId = -1, WebViewObject webViewObjectComponent = null) {
      _RequestId = requestId;
      _WebViewObjectComponent = webViewObjectComponent;
    }

    public void NeutralFaceThenAdvanceToNextBlock(bool success) {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.NeutralFace, this.AdvanceToNextBlock);
    }

    public void AdvanceToNextBlock(bool success) {
      if (this._RequestId >= 0) {
        // Calls the JavaScript function resolving the Promise on the block
        _WebViewObjectComponent.EvaluateJS(@"window.resolveCommands[" + this._RequestId + "]();");
      }
    }

    public void CubeTapped(int id, int tappedTimes, float timeStamp) {
      LightCube.TappedAction -= CubeTapped;
      AdvanceToNextBlock(true);
    }

    public void RobotObservedFace(RobotObservedFace message) {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedFace);
      AdvanceToNextBlock(true);
    }

    public void RobotObservedHappyFace(RobotObservedFace message) {
      if (message.expression == Anki.Vision.FacialExpression.Happiness) {
        RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedHappyFace);
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, false);
        AdvanceToNextBlock(true);
      }
    }

    public void RobotObservedSadFace(RobotObservedFace message) {
      // Match on Angry or Sad, and only if score is high (minimize false positives)
      if ((message.expression == Anki.Vision.FacialExpression.Anger) ||
          (message.expression == Anki.Vision.FacialExpression.Sadness)) {
        var expressionScore = message.expressionValues[(int)message.expression];
        const int kMinSadExpressionScore = 75;
        if (expressionScore >= kMinSadExpressionScore) {
          RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedSadFace);
          RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, false);
          AdvanceToNextBlock(true);
        }
      }
    }

    public void RobotObservedObject(RobotObservedObject message) {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedObject>(RobotObservedObject);
      AdvanceToNextBlock(true);
    }

    // If robot currently sees a cube, try to dock with it.
    public void DockWithCube(bool headAngleActionSucceeded) {
      bool success = false;
      LightCube cube = null;

      if (headAngleActionSucceeded) {
        foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
          if (kvp.Value.IsInFieldOfView) {
            success = true;
            cube = kvp.Value;
            RobotEngineManager.Instance.CurrentRobot.AlignWithObject(cube, 0.0f, callback: FinishDockWithCube, usePreDockPose: true, alignmentType: Anki.Cozmo.AlignmentType.LIFT_PLATE, numRetries: 2);
            return;
          }
        }
      }

      if (!success) {
        FinishDockWithCube(false);
      }
    }

    private void FinishDockWithCube(bool success) {
      if (!success) {
        // Play angry animation since Cozmo wasn't able to complete the task
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.FrustratedByFailureMajor, callback: AdvanceToNextBlock);
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

    public static void ReleaseInProgressScratchBlock(InProgressScratchBlock po) {
      po.Init();

      lock (_available) {
        _available.Add(po);
        _inUse.Remove(po);
      }
    }
  }

}