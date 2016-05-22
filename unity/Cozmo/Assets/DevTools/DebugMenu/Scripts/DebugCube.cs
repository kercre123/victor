using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class DebugCube : MonoBehaviour {

  [SerializeField]
  private Button TapButton;

  [SerializeField]
  private Image[] Lights;

  private LightCube _Cube;

  private void Awake() {
    TapButton.onClick.AddListener(HandleTapButtonClicked);
  }

  public void Initialize(LightCube cube) {
    _Cube = cube;
  }

  private void HandleTapButtonClicked() {
    if (_Cube != null) {
      _Cube.Tapped(Singleton<Anki.Cozmo.ObjectTapped>.Instance.Initialize(0, (uint)_Cube.ID, _Cube.RobotID, 1));
    }
  }

  private void Update() {
    if (_Cube == null) {
      return;
    }

    _Cube.MakeActiveAndVisible(true, true);

    var timeMs = System.DateTime.UtcNow.Ticks * 10000L;


    for (int i = 0; i < Lights.Length; i++) {

      var cl = _Cube.Lights[i];

      var l = Lights[i];

      var onColor = cl.OnColor.ToColor();
      var offColor = cl.OffColor.ToColor();
      onColor.a = 1f;
      offColor.a = 1f;

      if (cl.OnPeriodMs == Robot.Light.FOREVER) {
        l.color = onColor;
      }
      else {
        long totalCycleTime = cl.OnPeriodMs + cl.OffPeriodMs + cl.TransitionOffPeriodMs + cl.TransitionOffPeriodMs;

        if (totalCycleTime == 0) {
          l.color = onColor;
          continue;
        }

        long phaseTime = timeMs % totalCycleTime;

        if (phaseTime < cl.OnPeriodMs) {
          l.color = onColor;
          continue;
        }
        phaseTime -= cl.OnPeriodMs;
        if (phaseTime < cl.TransitionOffPeriodMs) {
          l.color = Color.Lerp(onColor, offColor, phaseTime / (float)cl.TransitionOffPeriodMs);
          continue;
        }
        phaseTime -= cl.TransitionOffPeriodMs;

        if (phaseTime < cl.OffPeriodMs) {
          l.color = offColor;
          continue;
        }

        phaseTime -= cl.OffPeriodMs;

        l.color = Color.Lerp(offColor, onColor, phaseTime / (float)cl.TransitionOnPeriodMs);
      }
    }

  }

}
