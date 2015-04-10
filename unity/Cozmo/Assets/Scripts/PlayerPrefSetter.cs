using UnityEngine;
using System.Collections;

public class PlayerPrefSetter : MonoBehaviour {

	[SerializeField] string pref = null;

	public void SetPlayerPref (int val) {
		PlayerPrefs.SetInt(pref, val);
	}
}
