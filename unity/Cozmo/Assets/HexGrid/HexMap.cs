using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, PuzzlePiece> _OccupancyMap = new Dictionary<Coord, PuzzlePiece>();

  private HexSet _Map;

  public void Initialize(HexSet map) {
    _Map = map;
  }

  // tries to remove the PuzzlePiece at mapCoord. returns true if a piece was successfully removed.
  // returns false if mapCoord was an available cell.
  public bool TryRemove(Coord mapCoord, out PuzzlePiece removedPiece) {
    if (!_OccupancyMap.ContainsKey(mapCoord)) {
      removedPiece = null;
      return false;
    }

    // save the puzzle piece we are removing
    removedPiece = _OccupancyMap[mapCoord];

    // get the coordinates to remove in local puzzle piece space
    HashSet<Coord> toRemoveLocalCoords = _OccupancyMap[mapCoord].PieceData.HexSet.HexSetData;
    foreach (Coord localCoord in toRemoveLocalCoords) {
      // convert to mapCoord space by adding the map position of the puzzle piece we are
      // removing
      _OccupancyMap.Remove(localCoord + _OccupancyMap[mapCoord].MapPosition);
    }

    return true;
  }

  public bool CanAdd(PuzzlePiece hexItem, Coord mapCoord) {
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      // check to see if the coordinate has been occupied by another piece.
      if (_OccupancyMap.ContainsKey(mapCoord + localCoord)) {
        return false;
      }
      // check to see if the coordinate exists in the map hex set.
      if (_Map.HexSetData.Contains(mapCoord + localCoord) == false) {
        return false;
      }
    }
    return true;
  }

  public bool TryAdd(PuzzlePiece hexItem, Coord mapCoord) {
    if (!CanAdd(hexItem, mapCoord)) {
      return false;
    }

    // update the occupancy map
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      _OccupancyMap.Add(mapCoord + localCoord, hexItem);
    }
    return true;
  }
}
