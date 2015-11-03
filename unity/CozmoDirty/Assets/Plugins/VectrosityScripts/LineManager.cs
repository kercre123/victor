// Version 4.1
// ©2015 Starscene Software. All rights reserved. Redistribution of source code without permission not allowed.

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Vectrosity;

[AddComponentMenu("Vectrosity/LineManager")]
public class LineManager : MonoBehaviour {
  
  static List<VectorLine> lines;
  static List<Transform> transforms;
  static int lineCount = 0;
  bool destroyed = false;

  void Awake() {
    Initialize();
    DontDestroyOnLoad(this);
  }

  void Initialize() {
    lines = new List<VectorLine>();
    transforms = new List<Transform>();
    lineCount = 0;
    enabled = false;
  }

  public void AddLine(VectorLine vectorLine, Transform thisTransform, float time) {
    if (time > 0.0f) {  // Needs to be before the line check, to accommodate re-added lines
      StartCoroutine(DisableLine(vectorLine, time, false));
    }
    for (int i = 0; i < lineCount; i++) {
      if (vectorLine == lines[i]) {
        return;
      }
    }
    lines.Add(vectorLine);
    transforms.Add(thisTransform);
    
    if (++lineCount == 1) {
      enabled = true;
    }
  }

  public void DisableLine(VectorLine vectorLine, float time) {
    StartCoroutine(DisableLine(vectorLine, time, false));
  }

  IEnumerator DisableLine(VectorLine vectorLine, float time, bool remove) {
    yield return new WaitForSeconds(time);
    if (remove) {
      RemoveLine(vectorLine);
    }
    else {
      RemoveLine(vectorLine);
      VectorLine.Destroy(ref vectorLine);
    }
    vectorLine = null;
  }

  void LateUpdate() {
    if (!VectorLine.camTransformExists)
      return;
    
    // Draw3DAuto lines
    for (int i = 0; i < lineCount; i++) {
      if (lines[i].rectTransform != null) {
        lines[i].Draw3D();
      }
      else {
        RemoveLine(i--);
      }
    }
    
    // Only redraw static objects if camera is moving
    if (VectorLine.CameraHasMoved()) {
      VectorManager.DrawArrayLines();
    }
    
    VectorLine.UpdateCameraInfo();
    
    // Always redraw dynamic objects
    VectorManager.DrawArrayLines2();
  }

  void RemoveLine(int i) {
    lines.RemoveAt(i);
    transforms.RemoveAt(i);
    --lineCount;
    DisableIfUnused();
  }

  public void RemoveLine(VectorLine vectorLine) {
    for (int i = 0; i < lineCount; i++) {
      if (vectorLine == lines[i]) {
        RemoveLine(i);
        break;
      }
    }
  }

  public void DisableIfUnused() {
    if (!destroyed) { // Prevent possible null reference exceptions
      if (lineCount <= 0 && VectorManager.arrayCount <= 0 && VectorManager.arrayCount2 <= 0) {
        enabled = false;
      }
    }
  }

  public void EnableIfUsed() {
    if (VectorManager.arrayCount >= 1 || VectorManager.arrayCount2 >= 1) {
      enabled = true;
    }
  }

  public void StartCheckDistance() {
    InvokeRepeating("CheckDistance", .01f, VectorManager.distanceCheckFrequency);
  }

  void CheckDistance() {
    VectorManager.CheckDistance();
  }

  void OnDestroy() {
    destroyed = true;
  }

  void OnLevelWasLoaded() {
    if (lineCount <= 0 && VectorManager.arrayCount <= 0 && VectorManager.arrayCount2 <= 0) {  
      Initialize();
    }
  }
}