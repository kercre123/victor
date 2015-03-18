using UnityEngine;
using System.Collections;

public class DebugObject : MonoBehaviour {

	void OnEnable () {
		if(PlayerPrefs.GetInt("ShowDebugInfo", 0) == 0) gameObject.SetActive(false);
	}

}
