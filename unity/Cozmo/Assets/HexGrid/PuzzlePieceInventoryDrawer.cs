using UnityEngine;
using System.Collections;

public class PuzzlePieceInventoryDrawer : MonoBehaviour {
  [SerializeField]
  private string _PuzzlePieceId;

  [SerializeField]
  private HexSetDrawer _HexSetDrawer;

  void Start() {
    PuzzlePieceData puzzlePiece = Cozmo.HexItemList.PuzzlePiece(_PuzzlePieceId);
    _HexSetDrawer.Initialize(puzzlePiece.HexSet, puzzlePiece.TileColor);
  }

}
