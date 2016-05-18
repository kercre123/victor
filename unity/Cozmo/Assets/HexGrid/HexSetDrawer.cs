using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HexSetDrawer : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _SingleHexPrefab;
 
  private Color _HexColor;

  private List<UnityEngine.UI.Image> _HexGameObjects = new List<UnityEngine.UI.Image>();

  private float _HexSize;

  public void Initialize(HexSet hexSet, Color hexColor, float hexSize = 100.0f) {
    _HexSize = hexSize;
    _HexColor = hexColor;
    foreach (Coord hexCoord in hexSet.HexSetData) {
      UnityEngine.UI.Image hexInstance = GameObject.Instantiate(_SingleHexPrefab.gameObject).GetComponent<UnityEngine.UI.Image>();
      hexInstance.color = _HexColor;

      hexInstance.transform.SetParent(this.transform);
      hexInstance.transform.localPosition = GetLocalHexPosition(hexCoord);

      _HexGameObjects.Add(hexInstance);
    }
  }

  private Vector3 GetLocalHexPosition(Coord coord) {
    Vector3 localHexPosition = new Vector3(coord.X * _HexSize, coord.Y * _HexSize, 0.0f);
    if (coord.Y % 2 == 1) {
      // odd rows get shifted
      localHexPosition += Vector3.right * _HexSize * 0.5f;
    }
    return localHexPosition;
  }
}
