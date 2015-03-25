using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	[SerializeField] protected GameObject selectionButtonPrefab;

	protected SelectionButton2[] selectionButtons;

	protected override void Awake() {
		base.Awake();

		GameObject obj = (GameObject)GameObject.Instantiate(selectionButtonPrefab);
		selectionButtons = obj.GetComponentsInChildren<SelectionButton2>(true);

		foreach(SelectionButton2 butt in selectionButtons) butt.gameObject.SetActive(false);
		DisableButtons();
	}

	protected void Update()
	{
		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		Dings();
		SetActionButtons();

		float w = imageRectTrans.sizeDelta.x;
		float h = imageRectTrans.sizeDelta.y;

		for( int i = 0; i < maxObservedObjects; ++i )
		{
			if( observedObjectsCount > i && robot.selectedObjects.Count == 0 && !robot.isBusy )
			{
				ObservedObject observedObject = robot.observedObjects[i];

				float boxX = (observedObject.VizRect.x / 320f) * w;
				float boxY = (observedObject.VizRect.y / 240f) * h;
				float boxW = (observedObject.VizRect.width / 320f) * w;
				float boxH = (observedObject.VizRect.height / 240f) * h;

				selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( boxW, boxH );
				selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( boxX, -boxY );

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
