using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class GameObjectSelector : MonoBehaviour {

	[SerializeField] GameObject[] screens = null;
	[SerializeField] int defaultIndex = 0;
	[SerializeField] Text label = null;

	int _index = -1;
	protected int index {
		get {
			return _index;
		}

		set {
			if(value != _index) {
				if(_index >= 0 && _index < screens.Length) Debug.Log("ControlSchemeTester screen changed from " + screens[_index].name + " to " + screens[value].name );
				_index = value;
				Refresh();
			}
		}
	}

	void OnEnable() {
		if(screens == null || screens.Length == 0) {
			enabled = false;
			return;
		}

		index = Mathf.Clamp(defaultIndex, 0, screens.Length - 1);
	}

	void Refresh() {
		if(screens == null || screens.Length == 0) {
			enabled = false;
			return;
		}

		_index = Mathf.Clamp(_index, 0, screens.Length - 1);

		//first disable the old screen(s)
		for(int i=0; i<screens.Length; i++) {
			if(screens[i] == null) continue;
			if(index == i) continue;
			screens[i].SetActive(false);
		}

		//then enable the new one
		screens[index].SetActive(true);
		
		//then refresh our test title field
		if(label != null && label.text != screens[index].name) {
			label.text = screens[index].name;
		}

	}

	public void SetScreenIndex(int i) {
		Debug.Log("SetScreenIndex("+i+")");
		index = Mathf.Clamp(i, 0, screens.Length - 1);
	}

}
