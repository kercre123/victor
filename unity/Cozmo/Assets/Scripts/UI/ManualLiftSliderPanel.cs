using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// special case version of the ActionPanel that was used to give Hans a version of freeplay where the lifting was handled explicity and
///		without AI assistance
/// </summary>
public class ManualLiftSliderPanel : ActionPanel
{
	[SerializeField] public Slider slider = null;

}
