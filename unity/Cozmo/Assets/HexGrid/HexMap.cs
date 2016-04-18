using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, HexItem> _HexItems = new Dictionary<Coord, HexItem>();
  private HexSet _Map;

  public bool CanAdd(HexItem hexItem, Coord coord) {
    return false;
  }

  public bool TryAdd(HexItem hexItem, Coord coord) {
    if (!CanAdd(hexItem, coord)) {
      return false;
    }
    _HexItems.Add(coord, hexItem);
    return true;
  }
}
