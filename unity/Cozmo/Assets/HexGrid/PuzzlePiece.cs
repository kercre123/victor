using UnityEngine;
using System.Collections;

public class PuzzlePiece {
  public Coord MapPosition;
  public PuzzlePieceData PieceData;

  public Coord LocalToMapCoord(Coord localCoord) {
    return MapPosition + localCoord;
  }
}
