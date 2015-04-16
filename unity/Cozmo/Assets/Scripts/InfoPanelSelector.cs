using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class InfoPanelSelector : GameObjectSelector {

	public static InfoPanelSelector instance = null;

	protected override void OnEnable() {
		instance = this;

		base.OnEnable();
	}

	protected void OnDisable() {
		if(instance == this) instance = null;
	}
}
