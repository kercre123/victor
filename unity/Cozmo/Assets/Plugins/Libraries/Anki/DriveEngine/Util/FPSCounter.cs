using UnityEngine;
using System.Collections;

public class FPSCounter : MonoBehaviour 
{
  public  float updateInterval = 1.0F;
  
  private float accum   = 0; // FPS accumulated over the interval
  private int   frames  = 0; // Frames drawn over the interval
  private float timeleft = 0; // Left time for current interval

  // Use this for initialization
  void Start () {
    UnityEngine.UI.Text text = this.GetComponentInParent<UnityEngine.UI.Text> ();
    if (!text) {
      Debug.Log("FPSCounter needs a Text component!");
      return;
    }
    timeleft = updateInterval;  
  }
  
  // Update is called once per frame
  void Update () 
  {
    UnityEngine.UI.Text text = this.GetComponentInParent<UnityEngine.UI.Text> ();
    if ( text )
    {
      timeleft -= Time.deltaTime;
      accum += Time.timeScale/Time.deltaTime;
      ++frames;

      // Interval ended - update GUI text and start new interval
      if( timeleft <= 0.0 )
      {
        // display two fractional digits (f2 format)
        float fps = accum/frames;
        string format = System.String.Format("{0:F2} FPS",fps);
        text.text = format;

        timeleft = updateInterval;
        accum = 0.0F;
        frames = 0;
      }
    }

  }
}
