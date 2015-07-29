using UnityEngine;
using System.Collections;

public class FaceCamera : MonoBehaviour
{
	void Update()
	{
		if(Camera.main == null) return;
		transform.LookAt(Camera.main.transform);
	}
}
