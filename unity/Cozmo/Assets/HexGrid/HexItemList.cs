using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  public class HexItemList : ScriptableObject {
    private static HexItemList _sInstance;

    public static void SetInstance(HexItemList instance) {
      _sInstance = instance;
      _sInstance.PopulateDictionary();
    }

    public static HexItemList Instance {
      get { return _sInstance; }
    }

    public PuzzlePiece[] HexItems;

    private Dictionary<string, PuzzlePiece> _IdToData;

    private void PopulateDictionary() {
      _IdToData = new Dictionary<string, PuzzlePiece>();
      foreach (PuzzlePiece data in HexItems) {
        if (!_IdToData.ContainsKey(data.PieceData.InventoryId)) {
          _IdToData.Add(data.PieceData.InventoryId, data);
        }
        else {
          DAS.Error("HexItemList.PopulateDictionary", "Trying to add item to dictionary, but the item already exists! item=" + data.PieceData.InventoryId);
        }
      }
    }

    public static PuzzlePiece PuzzlePiece(string puzzlePieceId) {
      PuzzlePiece data = null;
      if (!_sInstance._IdToData.TryGetValue(puzzlePieceId, out data)) {
        DAS.Error("HexItemList.GetData", "Could not find item='" + puzzlePieceId + "' in dictionary!");
      }
      return data;
    }

    public static bool IsPuzzlePiece(string itemId) {
      return _sInstance._IdToData.ContainsKey(itemId);
    }

    public static IEnumerable<string> GetPuzzlePieceIds() {
      return _sInstance._IdToData.Keys;
    }

    #if UNITY_EDITOR
    public IEnumerable<string> EditorGetPuzzlePieceIds() {
      List<string> puzzleIds = new List<string>();
      foreach (var data in HexItems) {
        puzzleIds.Add(data.PieceData.InventoryId);
      }
      return puzzleIds;
    }
    #endif
  }
}