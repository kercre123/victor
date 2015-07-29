using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;


[Serializable]
public class ObjectkSymbolInfo
{
	public string Name = "Kitty";
	public Sprite sprite = null;
}

/// <summary>
/// Cozmo palette should always exist and provides a look up for color info about cozmo's Cubes
/// </summary>
[ExecuteInEditMode]
public class CozmoPalette : MonoBehaviour
{

	[SerializeField] Color[] activeBlockColors = new Color[(int)ActiveBlock.Mode.Count];
	[SerializeField] ObjectkSymbolInfo[] objectSymbolInfos = new ObjectkSymbolInfo[(int)CubeType.Count];

	public static CozmoPalette instance = null;

	void Awake()
	{
		if(instance != null)
		{
			GameObject.Destroy(gameObject);
			return;
		}
		DontDestroyOnLoad(gameObject);
		instance = this;
	}

	void OnDisable()
	{
		if(instance == this) instance = null;
	}

	public static uint ColorToUInt(Color color)
	{
		uint value = (uint)(((uint)(255f * color.r) << 24) | ((uint)(255f * color.g) << 16) | ((uint)(255f * color.b) << 8) | ((uint)(255f * color.a) << 0));
		//Debug.Log ("ColorToUInt("+color.ToString()+") return value("+value+") max("+uint.MaxValue+")");
		return value;
	}

	public uint GetUIntColorForActiveBlockType(ActiveBlock.Mode activeType)
	{
		return ColorToUInt(activeBlockColors[(int)activeType]);
	}

	public Color GetColorForActiveBlockMode(ActiveBlock.Mode activeType)
	{
		return activeBlockColors[(int)activeType];
	}

	public Sprite GetSpriteForObjectType(CubeType objType)
	{
		int index = (int)objType;
		if(index < 0 || index > objectSymbolInfos.Length - 1) return null;
		return objectSymbolInfos[index].sprite;
	}

	public string GetNameForObjectType(CubeType objType)
	{
		int index = (int)objType;
		if(index < 0 || index > objectSymbolInfos.Length - 1) return "Unknown Object";
		return objectSymbolInfos[index].Name;
	}

	public static uint CycleColors(int i) // cycle through all colors, but don't use black or white
	{
		int index = (i % (int)ActiveBlock.Mode.Count) + 2;
		
		return CozmoPalette.instance.GetUIntColorForActiveBlockType(index < (int)ActiveBlock.Mode.Count ? (ActiveBlock.Mode)index : (ActiveBlock.Mode)2);
	}
	
}
