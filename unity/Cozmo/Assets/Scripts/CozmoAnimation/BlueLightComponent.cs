using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class BlueLightComponent : ColorComponent {

	// Totally picking these out of the air
	private static readonly Color[] fixedColors = new Color[]
	{
		new Color32(0, 0, 255, 255),
		new Color32(0, 0, 248, 255),
		new Color32(0, 0, 240, 255),
		new Color32(0, 0, 232, 255),
		new Color32(0, 0, 224, 255),
		new Color32(0, 0, 216, 255),
		new Color32(0, 0, 208, 255),
		new Color32(0, 0, 200, 255),
		new Color32(0, 0, 192, 255),
	};

	public override Color[] FixedColors {
		get {
			return fixedColors;
		}
	}
}



