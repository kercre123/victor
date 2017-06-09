using UnityEngine;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Data class for saving touch information for playback.
/// </summary>
public class FakeTouch {
  public Vector3 TouchPos;
  public float TimeStamp;

  public FakeTouch(Vector3 pos, float time) {
    TouchPos = pos;
    TimeStamp = time;
  }

}