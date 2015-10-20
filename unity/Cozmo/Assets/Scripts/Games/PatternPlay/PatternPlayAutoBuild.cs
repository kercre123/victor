using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternPlayAutoBuild {
  public PatternPlayController controller;
  public BlockPattern targetPattern;
  public float lastIdeaTime;
  ObservedObject objectHeld = null;
  Vector3 idealViewPosition;
  float idealViewAngle = 0.0f;
  public bool autoBuilding = false;

  // the index 0 object should always be the anchor object.
  List<ObservedObject> neatList = new List<ObservedObject>();

  public bool PickNewTargetPattern() {
    targetPattern = controller.GetPatternMemory().GetAnUnseenPattern(p => (!p.facingCozmo && !p.verticalStack));
    if (targetPattern == null)
      return false;
    else
      return true;
  }

  public Vector3 IdealViewPosition() {
    return idealViewPosition;
  }

  public float IdealViewAngle() {
    return idealViewAngle;
  }

  public int AvailableBlocks() {
    return controller.GetRobot().seenObjects.Count;
  }

  public ObservedObject GetClosestAvailableBlock() {
    Robot robot = controller.GetRobot();

    float minDist = float.MaxValue;
    ObservedObject closest = null;

    for (int i = 0; i < robot.seenObjects.Count; ++i) {
      // object is already part of the pattern so skip it.
      if (neatList.Contains(robot.seenObjects[i])) {
        continue;
      }

      float d = Vector3.Distance(robot.seenObjects[i].WorldPosition, robot.WorldPosition);
      if (d < minDist) {
        minDist = d;
        closest = robot.seenObjects[i];
      }
    }
    return closest;
  }

  public void SetObjectHeld(ObservedObject objectHeld_) {
    objectHeld = objectHeld_;
  }

  public ObservedObject GetHeldObject() {
    return objectHeld;
  }

  public Vector3 FindPlaceTarget() {
    if (neatList.Count == 0) {
      return controller.GetRobot().WorldPosition + controller.GetRobot().Forward * 40.0f;
    }
    else if (neatList.Count == 1) {
      // there is an anchor block. let's put it to the right of the block.
      Debug.Log("anchor block: " + neatList[0].WorldPosition);
      return neatList[0].WorldPosition + neatList[0].Right * CozmoUtil.BLOCK_LENGTH_MM * 2f;
    }
    else {
      // there are two blocks. we need to figure out where to put the third block
      Vector3 relPos = neatList[1].WorldPosition - neatList[0].WorldPosition;
      if (Vector3.Dot(relPos, neatList[0].Right) > 0.0f) {
        // place on the left of anchor block.
        return neatList[0].WorldPosition - neatList[0].Right * CozmoUtil.BLOCK_LENGTH_MM * 2f;
      }
      else {
        // place on right of anchor block.
        return neatList[0].WorldPosition + neatList[0].Right * CozmoUtil.BLOCK_LENGTH_MM * 2f;
      }
    }
  }

  public void PlaceBlockSuccess() {
    neatList.Add(objectHeld);
    if (neatList.Count == 1) {
      ComputeIdealViewPose();
    }
    objectHeld = null;
  }

  public bool NeatListContains(ObservedObject observedObj) {
    return neatList.Contains(observedObj);
  }

  public void ObjectMoved(int blockID) {
    int movedIndex = -1;
    for (int i = 0; i < neatList.Count; ++i) {
      if (neatList[i].ID == blockID) {
        // we found a block in the neat list that was moved.
        movedIndex = i;
      }
    }
    if (movedIndex != -1) {
      neatList.RemoveAt(movedIndex);
    }
  }

  public void ClearNeatList() {
    neatList.Clear();
  }

  public void SetBlockLightsToPattern(int blockID) {
    controller.SetPatternOnBlock(blockID, targetPattern.blocks[0].NumberOfLightsOn());
  }

  private void ComputeIdealViewPose() {
    idealViewPosition = neatList[0].WorldPosition + neatList[0].Forward * 150.0f;
    idealViewAngle = Mathf.PI;
  }
}
