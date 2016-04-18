using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, HexItem> _HexItems = new Dictionary<Coord, HexItem>();
  private HexSet _Map;

  // determines if hexItem at coord can fit inside of _Map.
  public bool CanAdd(HexItem hexItem, Coord coord) {
    return false;
  }

  // actually attempts to add hexItem at coord. returns false
  // if it does not fit. returns true when it has been successfully
  // added to _Map
  public bool TryAdd(HexItem hexItem, Coord coord) {
    if (!CanAdd(hexItem, coord)) {
      return false;
    }
    _HexItems.Add(coord, hexItem);
    return true;
  }
}
