using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexSet : ScriptableObject {
  [SerializeField]
  private List<Coord> HexList = new List<Coord>();

  public HashSet<Coord> HexSetData = new HashSet<Coord>();

  public HexSet() {
    for (int i = 0; i < HexList.Count; ++i) {
      HexSetData.Add(HexList[i]);
    }
  }

}
