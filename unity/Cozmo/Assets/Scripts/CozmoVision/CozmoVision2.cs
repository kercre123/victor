using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	[SerializeField] protected SelectionButton2[] selectionButtons;

	protected void Update()
	{
		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		Dings();
		SetActionButtons();

		for( int i = 0; i < maxObservedObjects; ++i )
		{
			if( observedObjectsCount > i && robot.selectedObjects.Count == 0 && !robot.isBusy )
			{
				ObservedObject observedObject = robot.observedObjects[i];

				selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( observedObject.VizRect.width, observedObject.VizRect.height );
				selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.VizRect.x, -observedObject.VizRect.y );

				selectionButtons[i].text.text = "Select " + observedObject.ID;
				selectionButtons[i].observedObject = observedObject;

				selectionButtons[i].image.gameObject.SetActive( true );
			}
			else
			{
				selectionButtons[i].image.gameObject.SetActive( false );
			}
		}
	}
}
