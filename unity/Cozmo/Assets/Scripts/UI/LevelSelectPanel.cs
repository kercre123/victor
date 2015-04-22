using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public class LevelSelectPanel : MonoBehaviour {

	[SerializeField] Text textTitle;
	[SerializeField] string gameName = "Unknown";
	[SerializeField] int gameControllerIndex = 1;
	[SerializeField] int numLevels = 10;
	[SerializeField] GameObject levelSelectionPrefab;
	[SerializeField] ScrollRect scrollRect;
	[SerializeField] Sprite[] previews;

	List<LevelSelectButton> levelSelectButtons = new List<LevelSelectButton>();

	[SerializeField] GameObjectSelector gameMenuSelector;
	[SerializeField] GameObjectSelector gameControllerSelector;
	[SerializeField] GameLayoutTracker gameLayoutTracker;


	void Awake() {

		textTitle.text = gameName.ToUpper();

		float width = 0f;

		for(int i=0;i<numLevels;i++) {
			GameObject buttonObj = (GameObject)GameObject.Instantiate(levelSelectionPrefab);

			RectTransform rectT = buttonObj.transform as RectTransform;

			rectT.SetParent(scrollRect.content, false);
			width += rectT.sizeDelta.x;

			LevelSelectButton selectButton = buttonObj.GetComponent<LevelSelectButton>();

			Sprite preview = null;
			if(previews != null && previews.Length > 0) {
				int spriteIndex = Mathf.Clamp(i, 0, previews.Length-1);
				preview = previews[spriteIndex];
			}

			bool interactive = gameLayoutTracker.GameHasLayoutForThisIndex(gameName, i);

			selectButton.Initialize((i+1).ToString(), preview, interactive ? UnityEngine.Random.Range(1,3) : 0, interactive, delegate{LaunchLevel(i+1);});

			levelSelectButtons.Add(selectButton);
		}

		RectTransform scrollRectT = scrollRect.content.transform as RectTransform;
		Vector2 size = scrollRectT.sizeDelta;
		size.x = width;
		scrollRectT.sizeDelta = size;
	}

	void LaunchLevel(int level) {
		gameControllerSelector.SetScreenIndex(gameControllerIndex);
		gameMenuSelector.SetScreenIndex(1);
		gameLayoutTracker.SetLayoutForGame(gameName, level-1);
	}

}
