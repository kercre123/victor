using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class BasicCharacterControllerGUI : MonoBehaviour 
	{
		private void OnGUI()
		{	
			GUILayout.TextArea("This is a small Sample Scene, showing how you might seamlessly transition from a Dynamic Gameplay camera into a cutscene.");
			GUILayout.Space(10.0f);
			GUILayout.TextArea("W, A, S and D : Move Character.");
			GUILayout.TextArea("Move into the RED trigger volume to trigger the small sequence.");
		}
	}
}