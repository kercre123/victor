using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class HeadAngleComponent : FloatComponent {
	public override float MinimumValue { get { return -25.0f; } }
	public override float MaximumValue { get { return 35.0f; } }
	public override float MaxmimumSlope { get { return 5; } }
}
