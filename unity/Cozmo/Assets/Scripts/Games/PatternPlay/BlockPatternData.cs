using UnityEngine;
using System.Collections;

public class BlockPatternData {

  public BlockPatternData(BlockLights lights, float frameZAccel, float timeTapped) {
    blockLightsLocalSpace = lights;
    lastFrameZAccel = frameZAccel;
    lastTimeTapped = timeTapped;
  }

  public BlockLights blockLightsLocalSpace;
  public float lastFrameZAccel;
  public float lastTimeTapped;
  // for phone input
  public float lastTimeTouched;
  public bool moving = false;

  public bool BlockActiveTimeTouched() {
    return Time.time - lastTimeTouched < 1.2f;
  }
}

