using UnityEngine;
using System.Collections;

public class BlockPatternData {

  public BlockPatternData(BlockLights lights, float frameZAccel, float timeTapped) {
    BlockLightsLocalSpace = lights;
    LastFrameZAccel = frameZAccel;
    LastTimeTapped = timeTapped;
  }

  public BlockLights BlockLightsLocalSpace;
  public float LastFrameZAccel;
  public float LastTimeTapped;
  // for phone input
  public float LastTimeTouched;
  public bool Moving = false;
  public float SeenAccumulator = 0.0f;

  public bool BlockActiveTimeTouched() {
    return Time.time - LastTimeTouched < 1.2f;
  }
}

