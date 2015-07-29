using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class GameObjectSelector : MonoBehaviour
{

	[SerializeField] GameObject[] gameObjects = null;
	[SerializeField] int defaultIndex = 0;
	[SerializeField] Text label = null;

	bool set = false;

	int _index = -1;

	protected int index
	{
		get {
			return _index;
		}

		set {
			if(value != _index)
			{
				//if(_index >= 0 && _index < gameObjects.Length) Debug.Log(gameObject.name + " GameObjectSelector changed from " + gameObjects[_index].name + " to " + gameObjects[value].name );
				_index = value;
				Refresh();
			}
		}
	}

	protected virtual void Awake()
	{
		InitializeDefault();
	}

	protected virtual void OnEnable()
	{
		InitializeDefault();
	}

	protected void InitializeDefault()
	{
		if(gameObjects == null || gameObjects.Length == 0)
		{
			enabled = false;
			return;
		}

		if(!set) index = Mathf.Clamp(defaultIndex, 0, gameObjects.Length - 1);
	}

	void Refresh()
	{
		if(gameObjects == null || gameObjects.Length == 0)
		{
			enabled = false;
			return;
		}

		_index = Mathf.Clamp(_index, 0, gameObjects.Length - 1);

		//first disable the old screen(s)
		for(int i = 0; i < gameObjects.Length; i++)
		{
			if(gameObjects[i] == null) continue;
			if(index == i) continue;
			gameObjects[i].SetActive(false);
		}

		//then enable the new one
		gameObjects[index].SetActive(true);
		
		//then refresh our test title field
		if(label != null && label.text != gameObjects[index].name)
		{
			label.text = gameObjects[index].name;
		}

	}

	public void SetScreenIndex(int i)
	{
		//Debug.Log(gameObject.name + " SetScreenIndex("+i+")");
		index = Mathf.Clamp(i, 0, gameObjects.Length - 1);
		set = true;
	}

}
