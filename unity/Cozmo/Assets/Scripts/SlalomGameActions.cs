using UnityEngine;
using System.Collections;

public class SlalomGameActions : GameActions
{
	public override void SetActionButtons( bool isSlider = false ) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if( ActionPanel.instance == null ) return;
		
		ActionPanel.instance.DisableButtons();
	}
}
