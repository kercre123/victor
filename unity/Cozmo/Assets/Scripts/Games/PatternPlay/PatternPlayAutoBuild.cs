using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternPlayAutoBuild {
  public PatternPlayController controller;
  public BlockPattern targetPattern;
  public float lastIdeaTime;
  ObservedObject objectHeld = null;

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

  public int AvailableBlocks() {
    return 0;
  }

  public ObservedObject GetClosestAvailableBlock() {
    Robot robot = controller.GetRobot();

    float minDist = float.MaxValue;
    ObservedObject closest = null;

    for (int i = 0; i < robot.observedObjects.Count; ++i) {
      if (neatList.Contains(robot.observedObjects[i])) {
        continue;
      }

      float d = Vector3.Distance(robot.observedObjects[i].WorldPosition, robot.WorldPosition);
      if (d < minDist) {
        minDist = d;
        closest = robot.observedObjects[i];
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
}
