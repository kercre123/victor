using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

//Red, Orange, Yellow, Green, Blue, Purple
public enum ActiveBlockType {
	Off,
	White,
	Red,
	Orange,
	Yellow,
	Green,
	Blue,
	Purple,
	NumTypes
}

[Serializable]
public class ObjectkSymbolInfo {
	public string Name = "Kitty";
	public int ObjectType = 1;
	public Sprite sprite = null;
}

[ExecuteInEditMode]
public class CozmoPalette : MonoBehaviour {

	[SerializeField] Color[] activeBlockColors = new Color[(int)ActiveBlockType.NumTypes];
	[SerializeField] List<ObjectkSymbolInfo> objectSymbolInfos = new List<ObjectkSymbolInfo>();
	[SerializeField] Sprite[] digitSprites;

	public static CozmoPalette instance = null;

	void OnEnable () {
		instance = this;
	}
	
	void OnDisable () {
		if(instance == this) instance = null;
	}


	public Color GetColorForActiveBlockType(ActiveBlockType activeType) {
		return activeBlockColors[(int)activeType];
	}

	public Sprite GetSpriteForObjectType(int objType) {
		ObjectkSymbolInfo info = objectSymbolInfos.Find(x => x.ObjectType == objType);
		
		return info != null ? info.sprite : null;
	}

	public Sprite GetDigitSprite(int digit) {
		digit = Mathf.Clamp(digit, 1, 6);
		return digitSprites[digit-1];
	}

}
