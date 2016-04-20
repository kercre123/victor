using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, PuzzlePiece> _OccupancyMap = new Dictionary<Coord, PuzzlePiece>();
  private HexSet _Map;

  // determines if hexItem at coord can fit inside of _Map.
  public bool CanAdd(PuzzlePiece hexItem, Coord coord) {
    return false;
  }

  // actually attempts to add hexItem at coord. returns false
  // if it does not fit. returns true when it has been successfully
  // added to _Map
  public bool TryAdd(PuzzlePiece hexItem, Coord coord) {
    if (!CanAdd(hexItem, coord)) {
      return false;
    }
    _OccupancyMap.Add(coord, hexItem);
    return true;
  }
}
