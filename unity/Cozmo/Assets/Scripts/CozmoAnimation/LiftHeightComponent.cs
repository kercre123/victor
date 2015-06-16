using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class LiftHeightComponent : FloatComponent {

	public override float MinimumValue { get { return 30.0f; } }
	public override float MaximumValue { get { return 100.0f; } }
	public override float MaxmimumSlope { get { return 5.0f; } }
}
