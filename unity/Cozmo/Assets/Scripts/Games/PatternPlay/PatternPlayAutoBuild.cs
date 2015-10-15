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
    
  }

  public int AvailableBlocks() {
    return 0;
  }

  public ObservedObject GetClosestAvailableBlock() {
    return null;
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

  public void SetBlockLightsToPattern(int blockID) {
    controller.SetPatternOnBlock(blockID, targetPattern.blocks[0].NumberOfLightsOn());
  }
}
