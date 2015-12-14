using System;
using UnityEngine;
using System.Collections.Generic;

public static class PolyhedronGenerator {

  #if UNITY_EDITOR
  [UnityEditor.MenuItem("Cozmo/Generate Asteroids")]
  public static void GenerateAstroids() {

    var material = Resources.Load<Material>("Asteroids/AsteroidMaterial");
    for (int i = 0; i < 25; i++) {
      var asteroid = new GameObject("Asteroid_"+i);
      var renderer = asteroid.AddComponent<MeshRenderer>();
      var filter = asteroid.AddComponent<MeshFilter>();
      var mesh = PolyhedronGenerator.GeneratePolyhedron();
      mesh.name = "AstroidMesh_" + i;
      filter.sharedMesh = mesh;
      renderer.sharedMaterial = material;

      asteroid.transform.localScale = Vector3.one * 20f;

      UnityEditor.AssetDatabase.CreateAsset(mesh, "Assets/HubWorld/Resources/Prefabs/Asteroids/_Mesh/AsteroidMesh_" + i + ".prefab");
      UnityEditor.PrefabUtility.CreatePrefab("Assets/HubWorld/Resources/Prefabs/Asteroids/Asteroid_" + i + ".prefab", asteroid);

      GameObject.DestroyImmediate(asteroid);
    }
  }
  #endif


  public static bool FindCircumscribedSphereCenter(Vector3 p1, Vector3 p2, Vector3 p3, out Vector3 center) {
    center = Vector3.zero;

    // make a plane half way along one side
    var planeIntersect = (p2 + p1) * 0.5f;
    var planeNormal = (p2 - p1).normalized;

    // now make a ray from half way along the other side in the triangle plane
    var rayOrigin = (p3 + p1) * 0.5f;

    var rayPerpendicular = (p3 - p1).normalized;

    var cosA = Vector3.Dot(rayPerpendicular, p2 - p1);

    var rayDirection = (p2 - rayPerpendicular * cosA - p1).normalized;
       
    var plane = new Plane(planeNormal, planeIntersect);

    var ray = new Ray(rayOrigin, rayDirection);

    var reverseRay = new Ray(rayOrigin, -rayDirection);

    float len = 0;
    if (plane.Raycast(ray, out len)) {
      center = ray.GetPoint(len);
      return true;
    }
    else if (plane.Raycast(reverseRay, out len)) {
      center = reverseRay.GetPoint(len);
      return true;
    }
    else {
      return false;
    }
  }
   

  public static void Triangulate(List<Vector3> points, out List<Vector3> vertices, out List<Vector3> normals, out List<int> triangles) {

    vertices = new List<Vector3>();
    normals = new List<Vector3>();
    triangles = new List<int>();

    int pCount = points.Count;
    for (int i = 0; i < pCount; i++) {
      for (int j = i + 1; j < pCount; j++) {
        for (int k = j + 1; k < pCount; k++) {
          Vector3 center;
          if (FindCircumscribedSphereCenter(points[i], points[j], points[k], out center)) {

            var cn = center.normalized;
            var r = (cn - points[i]).sqrMagnitude;
            bool valid = true;
            for (int l = 0; l < pCount; l++) {
              if (l == i || l == j || l == k) {
                continue;
              }

              var delta = cn - points[l];

              if (delta.sqrMagnitude < r) {                
                valid = false;
                break;
              }
            }
            if (valid) {
              // add both sided because of lazyness
              var normalA = Vector3.Cross(points[j] - points[i], points[k] - points[i]).normalized;
              triangles.Add(vertices.Count);
              vertices.Add(points[i]);
              normals.Add(normalA);
              triangles.Add(vertices.Count);
              vertices.Add(points[j]);
              normals.Add(normalA);
              triangles.Add(vertices.Count);
              vertices.Add(points[k]);
              normals.Add(normalA);


              var normalB = Vector3.Cross(points[k] - points[i], points[j] - points[i]).normalized;
              triangles.Add(vertices.Count);
              vertices.Add(points[i]);
              normals.Add(normalB);
              triangles.Add(vertices.Count);
              vertices.Add(points[k]);
              normals.Add(normalB);
              triangles.Add(vertices.Count);
              vertices.Add(points[j]);
              normals.Add(normalB);
            }
          }
          else {
            Debug.LogError("Points " + points[i] + ", " + points[j] + ", " + points[k] + " do not form a triangle");
          }
        }
      }
    }
  }


  public static Mesh GeneratePolyhedron() {

    List<Vector3> points = new List<Vector3>();

    int pointCount = UnityEngine.Random.Range(6, 20);

    var total = Vector3.zero;

    for (int i = 0; i < pointCount; i++) {
      var next = UnityEngine.Random.onUnitSphere;
      points.Add(next);
      total += next;
    }

    var center = total / pointCount;

    for (int i = 0; i < pointCount; i++) {
      var p = points[i] - center;
      points[i] = p.normalized;
    }

    List<Vector3> vertices, normals;
    List<int> triangles;
    Triangulate(points, out vertices, out normals, out triangles);

    var mesh = new Mesh();
    mesh.vertices = vertices.ToArray();
    mesh.normals = normals.ToArray();
    mesh.triangles = triangles.ToArray();

    return mesh;
  }


}

