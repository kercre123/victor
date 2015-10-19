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

  // the index 0 object should always be the anchor object.
  public List<ObservedObject> neatList = new List<ObservedObject>();

  public void PickNewTargetPattern() {

    targetPattern = new BlockPattern();
    targetPattern.facingCozmo = false;
    targetPattern.verticalStack = false;

    // Right now this is hard-coded. Replace with not-seen pattern once
    // pauley gets his stuff in.
    for (int i = 0; i < 3; ++i) {
      BlockLights lightPattern = new BlockLights();
      lightPattern.front = true;
      lightPattern.right = false;
      lightPattern.left = false;
      lightPattern.back = false;
      targetPattern.blocks.Add(lightPattern);
    }
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

  public void PlaceBlockSuccess() {
    neatList.Add(objectHeld);
    if (neatList.Count == 1) {
      ComputeIdealViewPose();
    }
    objectHeld = null;
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

  public void SetBlockLightsToPattern(int blockID) {
    controller.SetPatternOnBlock(blockID, targetPattern.blocks[0].NumberOfLightsOn());
  }

  private void ComputeIdealViewPose() {
    idealViewPosition = neatList[0].WorldPosition + neatList[0].Forward * 130.0f;

  }
}
