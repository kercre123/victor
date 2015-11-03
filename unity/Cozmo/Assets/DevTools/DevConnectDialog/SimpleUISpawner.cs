using UnityEngine;
using System.Collections;

public class SimpleUISpawner : MonoBehaviour {

	[SerializeField]
	private GameObject uiPrefab_;

	private void Start() {
		UIManager.CreateUI (uiPrefab_);
	}
}
