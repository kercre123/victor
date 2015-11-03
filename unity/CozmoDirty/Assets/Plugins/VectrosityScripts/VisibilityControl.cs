// Version 4.1
// ©2015 Starscene Software. All rights reserved. Redistribution of source code without permission not allowed.

using UnityEngine;
using Vectrosity;

[AddComponentMenu("Vectrosity/VisibilityControl")]
public class VisibilityControl : MonoBehaviour {

  RefInt m_objectNumber;
  VectorLine m_vectorLine;
  bool m_destroyed = false;
  
  public RefInt objectNumber {
    get {return m_objectNumber;}
  }
  
  public void Setup (VectorLine line, bool makeBounds) {
    if (makeBounds) {
      VectorManager.SetupBoundsMesh (gameObject, line);
    }
    
    VectorManager.VisibilitySetup (transform, line, out m_objectNumber);
    m_vectorLine = line;
  }

  void OnBecameVisible () {
    m_vectorLine.active = true;
    
    // Draw line now, otherwise's there's a 1-frame delay before the line is actually drawn in the next LateUpdate
    VectorManager.DrawArrayLine2 (m_objectNumber.i);
  }
  
  void OnBecameInvisible () {
    m_vectorLine.active = false;
  }
  
  void OnDestroy () {
    if (m_destroyed) return;  // Paranoia check
    m_destroyed = true;
    VectorManager.VisibilityRemove (m_objectNumber.i);
    VectorLine.Destroy (ref m_vectorLine);
  }
}