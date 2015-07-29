using UnityEngine;
using System.Collections;

public class AdoptedChildren : MonoBehaviour
{

	[Tooltip("These 'children' gameObjects will be enabled when this component is enabled and disabled when it is disabled.")]
	[SerializeField] GameObject[] children;
	[Tooltip("This reverses the relationship.  Ie., this child will be disabled when the parent is enabled and vice-versa.")]
	[SerializeField] bool reverse = false;

	void OnEnable()
	{
		for(int i = 0; i < children.Length; i++) if(children[i] != null) children[i].SetActive(!reverse);
	}

	void OnDisable()
	{
		for(int i = 0; i < children.Length; i++) if(children[i] != null) children[i].SetActive(reverse);
	}
}
