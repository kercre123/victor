using UnityEngine;
using System.Collections;

public class DontDestroyOnLoad : MonoBehaviour
{
  private void Awake()
  {
    DontDestroyOnLoad( gameObject ); 
  }
}
