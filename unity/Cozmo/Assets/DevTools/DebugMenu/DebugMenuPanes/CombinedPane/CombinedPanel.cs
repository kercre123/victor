using UnityEngine;
using System.Collections;

public class CombinedPanel : MonoBehaviour {

  [SerializeField]
  private GameObject[] _ChildPanels;

  [SerializeField]
  private RectTransform _ContentPanel;

	// Use this for initialization
	void Start () {
    foreach (var child in _ChildPanels) {
      var childObject = UIManager.CreateUIElement(child);
      childObject.transform.SetParent(_ContentPanel, false);
    }
	}	
}
