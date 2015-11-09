using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SimpleFPSCounterLabel : MonoBehaviour {

  [SerializeField]
  private Text fpsLabel_;

  [SerializeField]
  private Text fpsAvgLabel_;

  [SerializeField]
  private Button closeButton_;

	// Use this for initialization
	private void Start () {
    closeButton_.onClick.AddListener(OnCloseButtonClick);
	}
  
  private void OnDestroy () {
    closeButton_.onClick.RemoveListener(OnCloseButtonClick);
  }

  public void SetFPS(float newFPS){
    fpsLabel_.text = newFPS.ToString();
  }

  public void SetAvgFPS(float newFPS){
    fpsAvgLabel_.text = newFPS.ToString("F2");
  }

  private void OnCloseButtonClick(){
    Destroy(gameObject);
  }
}
