using System;
using UnityEngine;

public static class VectorUtil {

  public static Vector3 x0y(this Vector2 v) {
    return new Vector3(v.x, 0, v.y);
  }

  public static Vector2 xz(this Vector3 v) {
    return new Vector2(v.x, v.z);
  }

  // clear the z component
  public static Vector3 xy0(this Vector3 v) {
    return new Vector3(v.x, v.y, 0);
  }

  public static Quaternion xRotation(this Quaternion q) {
    return Quaternion.Euler(q.eulerAngles.x, 0, 0);
  }
  public static Quaternion yRotation(this Quaternion q) {
    return Quaternion.Euler(0, q.eulerAngles.y, 0);
  }
  public static Quaternion zRotation(this Quaternion q) {
    return Quaternion.Euler(0, 0, q.eulerAngles.z);
  }

}

