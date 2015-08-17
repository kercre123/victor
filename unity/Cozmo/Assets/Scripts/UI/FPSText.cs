using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class FPSText : MonoBehaviour {
  // Attach this to any object to make a frames/second indicator.
  //
  // It calculates frames/second over each updateInterval,
  // so the display does not keep changing wildly.

  [SerializeField] bool updateColor = true; // Do you want the color to change if the FPS gets low
  [SerializeField] float frequency = 0.5F; // The update frequency of the fps
  [SerializeField] int nbDecimal = 1; // How many decimal do you want to display

  float fps = 0;
  float accum   = 0f; // FPS accumulated over the interval
  int frames  = 0; // Frames drawn over the interval

  string red = "<color=red>"; // The color of the GUI, depending of the FPS ( R < 10, Y < 30, G >= 30 )
  string yellow = "<color=yellow>"; // The color of the GUI, depending of the FPS ( R < 10, Y < 30, G >= 30 )
  string green = "<color=green>"; // The color of the GUI, depending of the FPS ( R < 10, Y < 30, G >= 30 )
  string endColorTag = "</color>"; // The color of the GUI, depending of the FPS ( R < 10, Y < 30, G >= 30 )

  string sFPS = ""; // The fps formatted into a string.
  string prefix = "fps: "; // The fps formatted into a string.
  Text text;

  void Awake () {
    text = GetComponent<Text>();
  }

  void OnEnable() {
    StartCoroutine( "FPS" );
  }
  
  void Update() {
    sFPS = fps.ToString( "f" + Mathf.Clamp( nbDecimal, 0, 10 ) );

    if(updateColor) {
      //Update the color
      string setColorTag = (fps >= 30) ? green : ((fps > 10) ? red : yellow);

      text.text = setColorTag + prefix + sFPS + endColorTag;
    }
    else {
      text.text = prefix + sFPS;
    }

    accum += Time.timeScale/ Time.deltaTime;
    ++frames;
  }

  void OnDisable() {
    StopCoroutine( "FPS" );
  }
  
  IEnumerator FPS() {
    // Infinite loop executed every "frenquency" secondes.
    while( true )
    {
      // Update the FPS
      fps = accum/frames;
      accum = 0.0F;
      frames = 0;

      yield return new WaitForSeconds( frequency );
    }
  }


}
