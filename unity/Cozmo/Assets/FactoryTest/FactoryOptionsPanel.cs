using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
  }
	
  // Update is called once per frame
  void Update() {
	
  }
}
