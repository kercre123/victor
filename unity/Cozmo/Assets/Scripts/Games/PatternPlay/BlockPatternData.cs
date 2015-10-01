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
}

