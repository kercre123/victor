// Version 4.1
// ©2015 Starscene Software. All rights reserved. Redistribution of source code without permission not allowed.

using UnityEngine;
using Vectrosity;

[AddComponentMenu("Vectrosity/BrightnessControl")]
public class BrightnessControl : MonoBehaviour {

	RefInt m_objectNumber;
	VectorLine m_vectorLine;
	bool m_useLine = false;	// Normally false, since Visibility scripts take care of this
	bool m_destroyed = false;
	
	public RefInt objectNumber {
		get {return m_objectNumber;}
	}

	public void Setup (VectorLine line, bool m_useLine) {
		m_objectNumber = new RefInt(0);
		VectorManager.CheckDistanceSetup (transform, line, line.color, m_objectNumber);
		VectorManager.SetDistanceColor (m_objectNumber.i);
		if (m_useLine) {	// Only if there are no Visibility scripts being used
			this.m_useLine = true;
			m_vectorLine = line;
		}
	}
	
	public void SetUseLine (bool useLine) {
		this.m_useLine = useLine;
	}
	
	// Force the color to be set when becoming visible
	void OnBecameVisible () {
		VectorManager.SetOldDistance (m_objectNumber.i, -1);
		VectorManager.SetDistanceColor (m_objectNumber.i);
		if (!m_useLine) return;
		m_vectorLine.active = true;
	}
	
	public void OnBecameInvisible () {
		if (!m_useLine) return;
		m_vectorLine.active = false;
	}
	
	void OnDestroy () {
		if (m_destroyed) return;	// Paranoia check
		m_destroyed = true;
		VectorManager.DistanceRemove (m_objectNumber.i);
		if (m_useLine) {
			VectorLine.Destroy (ref m_vectorLine);
		}
	}
}