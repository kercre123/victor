using UnityEngine;
using System.Collections;

[System.Serializable]
public class HexItem : ScriptableObject {
  public string InventoryId;
  public HexSet HexSet;
  public Color TileColor;
}
