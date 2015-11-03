// Version 4.1
// ©2015 Starscene Software. All rights reserved. Redistribution of source code without permission not allowed.

using UnityEngine;
using System.Collections.Generic;
using Vectrosity;

public class VectorManager {

  public static float minBrightnessDistance = 500.0f;
  public static float maxBrightnessDistance = 250.0f;
  static int brightnessLevels = 32;
  public static float distanceCheckFrequency = .2f;
  static Color fogColor;
  public static bool useDraw3D = false;
  
  public static void SetBrightnessParameters (float fadeOutDistance, float fullBrightDistance, int levels, float frequency, Color color) {
    // Since we're using sqrMagnitude for speed (instead of Vector3.Distance), we need the squared distances
    minBrightnessDistance = fadeOutDistance * fadeOutDistance;
    maxBrightnessDistance = fullBrightDistance * fullBrightDistance;
    brightnessLevels = levels;
    distanceCheckFrequency = frequency;
    fogColor = color;
  }
  
  public static float GetBrightnessValue (Vector3 pos) {
    if (!VectorLine.camTransformExists) {
      VectorLine.SetCamera3D();
    }
    return Mathf.InverseLerp (minBrightnessDistance, maxBrightnessDistance, (pos - VectorLine.camTransformPosition).sqrMagnitude);
  }
  
  public static void ObjectSetup (GameObject go, VectorLine line, Visibility visibility, Brightness brightness) {
    ObjectSetup (go, line, visibility, brightness, true);
  }  
  
  public static void ObjectSetup (GameObject go, VectorLine line, Visibility visibility, Brightness brightness, bool makeBounds) {
    var vc = go.GetComponent(typeof(VisibilityControl)) as VisibilityControl;
    var vcs = go.GetComponent(typeof(VisibilityControlStatic)) as VisibilityControlStatic;
    var vca = go.GetComponent(typeof(VisibilityControlAlways)) as VisibilityControlAlways;
    var bc = go.GetComponent(typeof(BrightnessControl)) as BrightnessControl;
        
    if (visibility == Visibility.None) {
      if (vc) {
        MonoBehaviour.Destroy(vc);
      }
      if (vcs) {
        MonoBehaviour.Destroy(vcs);
      }
      if (vca) {
        MonoBehaviour.Destroy(vca);
      }
    }
    if (visibility == Visibility.Dynamic) {
      if (vc == null) {
        vc = go.AddComponent (typeof(VisibilityControl)) as VisibilityControl;
        vc.Setup (line, makeBounds);
        if (bc != null) {
          bc.SetUseLine (false);
        }
      }
    }
    else if (visibility == Visibility.Static) {
      if (vcs == null) {
        vcs = go.AddComponent (typeof(VisibilityControlStatic)) as VisibilityControlStatic;
        vcs.Setup (line, makeBounds);
        if (bc != null) {
          bc.SetUseLine (false);
        }
      }
    }
    else if (visibility == Visibility.Always) {
      if (vca == null) {
        vca = go.AddComponent (typeof(VisibilityControlAlways)) as VisibilityControlAlways;
        vca.Setup (line);
        if (bc != null) {
          bc.SetUseLine (false);
        }
      }
    }
    if (brightness == Brightness.Fog) {
      if (bc == null) {
        bc = go.AddComponent (typeof(BrightnessControl)) as BrightnessControl;
        if (vc == null && vcs == null && vca == null) {
          bc.Setup (line, true);
        }
        else {
          bc.Setup (line, false);
        }
      }
    }
    else {
      if (bc) {
        MonoBehaviour.Destroy (bc);
      }
    }
  }
  
  // It's quite a bit simpler just to have each VisibilityControlStatic script do its own check...however, running a lot of LateUpdate instances
  // is a fair bit slower than just running a centralized instance that checks all objects in a loop.
  // Hence it's worth the bother of tracking some Lists.
  static List<VectorLine> vectorLines;
  static List<RefInt> objectNumbers;
  static public int _arrayCount = 0;
  public static int arrayCount {
    get {return _arrayCount;}
  }
  
  public static void VisibilityStaticSetup (VectorLine line, out RefInt objectNum) {
    if (vectorLines == null) {
      vectorLines = new List<VectorLine>();
      objectNumbers = new List<RefInt>();
    }
    vectorLines.Add (line);
    objectNum = new RefInt(_arrayCount++); 
    objectNumbers.Add (objectNum);
    VectorLine.LineManagerEnable();
  }
  
  public static void VisibilityStaticRemove (int objectNumber) {
    if (objectNumber >= vectorLines.Count) {
      Debug.LogError ("VectorManager: object number exceeds array length in VisibilityStaticRemove");
      return;
    }
    
    for (int i = objectNumber+1; i < _arrayCount; i++) {
      objectNumbers[i].i--;
    }
    vectorLines.RemoveAt (objectNumber);
    objectNumbers.RemoveAt (objectNumber);
    _arrayCount--;
    VectorLine.LineManagerDisable();
  }

  // Same as above, but for VisibilityControl
  static List<VectorLine> vectorLines2;
  static List<RefInt> objectNumbers2;
  static int _arrayCount2 = 0;
  public static int arrayCount2 {
    get {return _arrayCount2;}
  }
  
  public static void VisibilitySetup (Transform thisTransform, VectorLine line, out RefInt objectNum) {
    if (vectorLines2 == null) {
      vectorLines2 = new List<VectorLine>();
      objectNumbers2 = new List<RefInt>();
    }
    line.drawTransform = thisTransform;
    vectorLines2.Add (line);
    objectNum = new RefInt(_arrayCount2++); 
    objectNumbers2.Add (objectNum);
    VectorLine.LineManagerEnable();
  }
  
  public static void VisibilityRemove (int objectNumber) {
    if (objectNumber >= vectorLines2.Count) {
      Debug.LogError ("VectorManager: object number exceeds array length in VisibilityRemove");
      return;
    }
    for (int i = objectNumber+1; i < _arrayCount2; i++) {
      objectNumbers2[i].i--;
    }
    vectorLines2.RemoveAt (objectNumber);
    objectNumbers2.RemoveAt (objectNumber);
    _arrayCount2--;
    VectorLine.LineManagerDisable();
  }
  
  // Same as above (again)...better to have one CheckDistance instance here instead of using InvokeRepeating for every object
  static List<Transform> transforms3;
  static List<VectorLine> vectorLines3;
  static List<int> oldDistances;
  static List<Color> colors;
  static List<RefInt> objectNumbers3;
  static int _arrayCount3 = 0;
  
  public static void CheckDistanceSetup (Transform thisTransform, VectorLine line, Color color, RefInt objectNum) {
    VectorLine.LineManagerEnable();
    if (vectorLines3 == null) {
      vectorLines3 = new List<VectorLine>();
      transforms3 = new List<Transform>();
      oldDistances = new List<int>();
      colors = new List<Color>();
      objectNumbers3 = new List<RefInt>();
      VectorLine.LineManagerCheckDistance();
    }
    transforms3.Add (thisTransform);
    vectorLines3.Add (line);
    oldDistances.Add (-1);
    colors.Add (color);
    objectNum.i = _arrayCount3++;
    objectNumbers3.Add (objectNum);
  }
  
  public static void DistanceRemove (int objectNumber) {
    if (objectNumber >= vectorLines3.Count) {
      Debug.LogError ("VectorManager: object number exceeds array length in DistanceRemove");
      return;
    }
    
    for (int i = objectNumber+1; i < _arrayCount3; i++) {
      objectNumbers3[i].i--;
    }
    transforms3.RemoveAt (objectNumber);
    vectorLines3.RemoveAt (objectNumber);
    oldDistances.RemoveAt (objectNumber);
    colors.RemoveAt (objectNumber);
    objectNumbers3.RemoveAt (objectNumber);
    _arrayCount3--;
  }
  
  public static void CheckDistance () {
    for (int i = 0; i < _arrayCount3; i++) {
      SetDistanceColor(i);
    }
  }
  
  public static void SetOldDistance (int objectNumber, int val) {
    VectorManager.oldDistances[objectNumber] = val;
  }
  
  // This makes the color darker the farther away from the camera.  Just like fog.  However, fog won't work for vector objects,
  // hence we have to go through the effort of duplicating the effects.  But this also presents the opportunity to simulate limited
  // brightness levels being available, and the brightness only changes from minDistance to maxDistance units away and is constant otherwise.
  // (This also happens to make it a bit faster as a bonus, since Vector.SetColor isn't called as often.)
  // And vector objects can ignore the "fog" by just not having this script attached.
  // Currently it uses only one color per object, which it takes from line.segmentColors[0]
  
  public static void SetDistanceColor (int i) {
    if (!vectorLines3[i].active) return;
    float thisDistance = GetBrightnessValue(transforms3[i].position);
    int intDistance = (int)(thisDistance * brightnessLevels);
    if (intDistance != oldDistances[i]) {
      vectorLines3[i].SetColor(Color.Lerp(fogColor, colors[i], thisDistance));
    }
    oldDistances[i] = intDistance;
  }

  public static void DrawArrayLine (int i) {
    if (useDraw3D) {
      vectorLines[i].Draw3D();
    }
    else {
      vectorLines[i].Draw();
    }
  }

  public static void DrawArrayLine2 (int i) {
    if (useDraw3D) {
      vectorLines2[i].Draw3D();
    }
    else {
      vectorLines2[i].Draw();
    }
  }

  public static void DrawArrayLines () {
    if (useDraw3D) {
      for (int i = 0; i < _arrayCount; i++) {
        vectorLines[i].Draw3D();
      }
    }
    else {
      for (int i = 0; i < _arrayCount; i++) {
        vectorLines[i].Draw();
      }
    }
  }

  public static void DrawArrayLines2 () {
    if (useDraw3D) {
      for (int i = 0; i < _arrayCount2; i++) {
        vectorLines2[i].Draw3D();
      }
    }
    else {
      for (int i = 0; i < _arrayCount2; i++) {
        vectorLines2[i].Draw();
      }
    }
  }

  public static Bounds GetBounds (VectorLine line) {
    if (line.points3 == null) {
      Debug.LogError ("VectorManager: GetBounds can only be used with a Vector3 array");
      return new Bounds();
    }
    return GetBounds (line.points3);
  }
  
  public static Bounds GetBounds (List<Vector3> points3) {
    var bounds = new Bounds();
    var min = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);
    var max = new Vector3(float.MinValue, float.MinValue, float.MinValue);
    int end = points3.Count;
    
    for (int i = 0; i < end; i++) {
      if (points3[i].x < min.x) min.x = points3[i].x;
      else if (points3[i].x > max.x) max.x = points3[i].x;
      if (points3[i].y < min.y) min.y = points3[i].y;
      else if (points3[i].y > max.y) max.y = points3[i].y;
      if (points3[i].z < min.z) min.z = points3[i].z;
      else if (points3[i].z > max.z) max.z = points3[i].z;
    }
    
    bounds.min = min;
    bounds.max = max;    
    return bounds;
  }
  
  static Mesh MakeBoundsMesh (Bounds bounds) {
    var mesh = new Mesh();
    mesh.vertices = new[] {bounds.center + new Vector3(-bounds.extents.x,  bounds.extents.y,  bounds.extents.z),
                 bounds.center + new Vector3( bounds.extents.x,  bounds.extents.y,  bounds.extents.z),
                 bounds.center + new Vector3(-bounds.extents.x,  bounds.extents.y, -bounds.extents.z),
                 bounds.center + new Vector3( bounds.extents.x,  bounds.extents.y, -bounds.extents.z),
                 bounds.center + new Vector3(-bounds.extents.x, -bounds.extents.y,  bounds.extents.z),
                 bounds.center + new Vector3( bounds.extents.x, -bounds.extents.y,  bounds.extents.z),
                 bounds.center + new Vector3(-bounds.extents.x, -bounds.extents.y, -bounds.extents.z),
                 bounds.center + new Vector3( bounds.extents.x, -bounds.extents.y, -bounds.extents.z)};
    return mesh;
  }
  
  static Dictionary<string, Mesh> meshTable;
  
  public static void SetupBoundsMesh (GameObject go, VectorLine line) {
    var meshFilter = go.GetComponent<MeshFilter>();
    if (meshFilter == null) {
      meshFilter = go.AddComponent<MeshFilter>();
    }
    var meshRenderer = go.GetComponent<MeshRenderer>();
    if (meshRenderer == null) {
      meshRenderer = go.AddComponent<MeshRenderer>();
    }
    meshRenderer.enabled = true;
    
    if (meshTable == null) {
      meshTable = new Dictionary<string, Mesh>();
    }
    if (!meshTable.ContainsKey(line.name)) {
      meshTable.Add (line.name, MakeBoundsMesh (GetBounds (line)));
      meshTable[line.name].name = line.name + " Bounds";
    }
    meshFilter.mesh = meshTable[line.name];
  }
}