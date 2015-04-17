using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;


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

	public static uint ColorToUInt(Color color) {
		return (uint)(((uint)(255f * color.r) << 24) | ((uint)(255f * color.g) << 16) | ((uint)(255f * color.b) << 8)  | ((uint)(255f * color.a) << 0));
	}

	public uint GetUIntColorForActiveBlockType(ActiveBlockType activeType) {
		return ColorToUInt(activeBlockColors[(int)activeType]);
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
