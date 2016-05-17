using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexMap {
  private Dictionary<Coord, PuzzlePiece> _OccupancyMap = new Dictionary<Coord, PuzzlePiece>();

  private HexSet _Map;

  public void Initialize(HexSet map) {
    _Map = map;
  }

  public Dictionary<Coord, PuzzlePiece> GetOccupancyMap() {
    return _OccupancyMap;
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
    HashSet<Coord> toRemoveLocalCoords = removedPiece.PieceData.HexSet.HexSetData;
    foreach (Coord localCoord in toRemoveLocalCoords) {
      // convert to mapCoord space by adding the map position of the puzzle piece we are
      // removing
      _OccupancyMap.Remove(removedPiece.LocalToMapCoord(localCoord));
    }

    return true;
  }

  public bool CanAdd(PuzzlePiece hexItem) {
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      // check to see if the coordinate has been occupied by another piece.
      if (_OccupancyMap.ContainsKey(hexItem.LocalToMapCoord(localCoord))) {
        return false;
      }
      // check to see if the coordinate exists in the map hex set.
      if (_Map.HexSetData.Contains(hexItem.LocalToMapCoord(localCoord)) == false) {
        return false;
      }
    }
    return true;
  }

  public bool TryAdd(PuzzlePiece hexItem) {
    if (!CanAdd(hexItem)) {
      return false;
    }

    // update the occupancy map
    foreach (Coord localCoord in hexItem.PieceData.HexSet.HexSetData) {
      _OccupancyMap.Add(hexItem.LocalToMapCoord(localCoord), hexItem);
    }
    return true;
  }
}
