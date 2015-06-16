using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class TurnRadiusComponent : FloatComponent {
	public override float MinimumValue { get { return -100.0f; } }
	public override float MaximumValue { get { return 100.0f; } }
	public override float MaxmimumSlope { get { return 10; } }
}
