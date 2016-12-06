using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;

public class HexPuzzleView : BaseModal {

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

    // draws the current map slots
    _MapDrawer.Initialize(mapHexSet, _MapColor);

    DrawOccupiedMapSlots();

    InitializeInventoryButtons();

  }

  private void DrawOccupiedMapSlots() {
    ClearOccupiedMapSlots();
    Dictionary<Coord, PuzzlePiece> occupancyMap = _HexMap.GetOccupancyMap();
    foreach (KeyValuePair<Coord, PuzzlePiece> kvp in occupancyMap) {
      HexSetDrawer hexDrawer = GameObject.Instantiate(_OccupiedMapDrawerPrefab.gameObject).GetComponent<HexSetDrawer>();
      hexDrawer.Initialize(kvp.Value.PieceData.HexSet, kvp.Value.PieceData.TileColor);
    }
  }

  private void ClearOccupiedMapSlots() {
    for (int i = 0; i < _OccupiedMapDrawer.Count; ++i) {
      GameObject.Destroy(_OccupiedMapDrawer[i].gameObject);
    }
    _OccupiedMapDrawer.Clear();
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

  protected override void Update() {
    base.Update();
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
