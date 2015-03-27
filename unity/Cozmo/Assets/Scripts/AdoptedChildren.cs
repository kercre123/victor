using UnityEngine;
using System.Collections;

public class AdoptedChildren : MonoBehaviour {

	[SerializeField] GameObject[] children;

	void OnEnable () {
		for(int i=0; i<children.Length; i++) children[i].SetActive(true);
	}

	void OnDisable () {
		for(int i=0; i<children.Length; i++) { if(children[i] != null) { children[i].SetActive(false); } }
	}
}
