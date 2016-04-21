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

  [SerializeField]
  private Color _MapColor;

  private HexMap _HexMap;

  // a handle to the game object of the puzzle piece that is
  // currently selected by the user. null if nothing is being dragged.
  private HexSetDrawer _SelectedPuzzlePiece = null;

  private List<HexSetDrawer> _OccupiedMapDrawer = new List<HexSetDrawer>();

  public void Initialize(HexSet mapHexSet) {
    _HexMap = new HexMap();
    _HexMap.Initialize(mapHexSet);

    _MapDrawer.Initialize(mapHexSet, _MapColor);

    InitializeInventoryButtons();

  }

  private void InitializeInventoryButtons() {
    for (int i = 0; i < _PuzzlePeiceDrawers.Length; ++i) {
      
    }
  }

  private void HandleOnPuzzlePieceInventorySelect(PuzzlePieceInventoryDrawer inventoryDrawer) {
    
  }

  // the user stopped dragging, try to see if the piece is close to a spot to fill on the map
  // if not, then lerp the piece back to its inventory slot.
  private void HandleOnPuzzlePieceDropped() {
    if (_SelectedPuzzlePiece != null) {
      
    }
  }

  private void Update() {
    if (_SelectedPuzzlePiece != null) {
      // update selected puzzle piece position based on dragging.
      // highlight available close by slots
    }
  }

  protected override void CleanUp() {
    foreach (HexSetDrawer hexDrawer in _OccupiedMapDrawer) {
      GameObject.Destroy(hexDrawer.gameObject);
    }
  }
}
