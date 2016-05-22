using UnityEngine;
using System.Collections;

[System.Serializable]
public class Coord {
  public int X;
  public int Y;

  public Coord(int x, int y) {
    X = x;
    Y = y;
  }

  public static Coord operator+(Coord lhs, Coord rhs) {
    return new Coord(lhs.X + rhs.X, lhs.Y + rhs.Y);
  }

  public static Coord operator-(Coord lhs, Coord rhs) {
    return new Coord(lhs.X - rhs.X, lhs.Y - rhs.Y);
  }

}
