using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class VelocityComponent : FloatComponent {
	public override float MinimumValue { get { return -400.0f; } }
	public override float MaximumValue { get { return 400.0f; } }
	public override float MaxmimumSlope { get { return 100; } }
}
