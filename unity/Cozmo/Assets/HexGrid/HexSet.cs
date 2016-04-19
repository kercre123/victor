using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexSet : ScriptableObject {
  [SerializeField]
  private List<Coord> CoordsList = new List<Coord>();

  public HashSet<Coord> HexSetData = new HashSet<Coord>();

  // actually put the stuff in the serialized coordinates list into the hex set.
  public HexSet() {
    for (int i = 0; i < CoordsList.Count; ++i) {
      HexSetData.Add(CoordsList[i]);
    }
  }

}
