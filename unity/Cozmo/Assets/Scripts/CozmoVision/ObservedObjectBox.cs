using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectBox : MonoBehaviour
{
	public Image image;
	public Text text;

	[System.NonSerialized] public ObservedObject observedObject;
}
