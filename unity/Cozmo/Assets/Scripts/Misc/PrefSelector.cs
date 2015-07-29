using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// a version of our simple gameobject selector that uses an integer PlayerPref to determine which gameobject to activate
/// </summary>
public class PrefSelector : GameObjectSelector
{

	[SerializeField] string preferrence = null;

	bool optionsMenuWasOpenLastFrame = false;

	protected override void OnEnable()
	{
		RefreshFromPref();
	}

	void Update()
	{

		bool optionsOpenThisFrame = OptionsScreen.IsOpen;

		if(optionsMenuWasOpenLastFrame && !optionsOpenThisFrame)
		{
			RefreshFromPref();


		}

		optionsMenuWasOpenLastFrame = optionsOpenThisFrame;
	}

	void RefreshFromPref()
	{
		if(!string.IsNullOrEmpty(preferrence))
		{
			index = PlayerPrefs.GetInt(preferrence, 0);
		}
	}
}
