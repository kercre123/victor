using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;

public class HexPuzzleView : BaseView {

  [SerializeField]
  private HexSetDrawer _OccupiedMapDrawerPrefab;

  [SerializeField]
  private PuzzlePieceInventoryDrawer[] _PuzzlePeiceDrawers;

  [SerializeField]
  private HexSetDrawer _MapDrawer;

  private HexMap _HexMap;

  // a handle to the game object of the puzzle piece that is
  // currently selected by the user. null if nothing is being dragged.
  private HexSetDrawer _SelectedPuzzlePiece;

  private List<HexSetDrawer> _OccupiedMapDrawer = new List<HexSetDrawer>();

  public void Initialize(HexSet mapHexSet) {
    _HexMap = new HexMap();
    _HexMap.Initialize(mapHexSet);

    _MapDrawer.Initialize(mapHexSet);

  }

  protected override void CleanUp() {
    foreach (HexSetDrawer hexDrawer in _OccupiedMapDrawer) {
      GameObject.Destroy(hexDrawer.gameObject);
    }
  }
}
