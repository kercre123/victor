using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, PuzzlePiece> _OccupancyMap = new Dictionary<Coord, PuzzlePiece>();
  private HexSet _Map;

  public bool TryRemove(Coord mapCoord) {
    if (!_OccupancyMap.ContainsKey(mapCoord)) {
      return false;
    }

    HashSet<Coord> toRemoveLocalCoords = _OccupancyMap[mapCoord].PieceData.HexSet.HexSetData;
    foreach (Coord localCoord in toRemoveLocalCoords) {
      _OccupancyMap.Remove(localCoord + _OccupancyMap[mapCoord].MapPosition);
    }
    return true;
  }

  public bool CanAdd(PuzzlePiece hexItem, Coord mapCoord) {
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      if (_OccupancyMap.ContainsKey(mapCoord + localCoord)) {
        return false;
      }
    }
    return true;
  }

  public bool TryAdd(PuzzlePiece hexItem, Coord mapCoord) {
    if (!CanAdd(hexItem, mapCoord)) {
      return false;
    }
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      _OccupancyMap.Add(mapCoord + localCoord, hexItem);
    }
    return true;
  }
}
