using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class CozmoWalkerController : GameController {

  private bool gameReady = false;
  private bool gameOver = false;

  protected override void OnEnable() {
    base.OnEnable();
  }

  protected override void OnDisable() {
    base.OnDisable();
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
    gameReady = true;
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Enter_PRE_GAME() {
    base.Enter_PRE_GAME();
  }

  protected override void Update_PRE_GAME() {
    base.Update_PRE_GAME();
  }

  protected override void Exit_PRE_GAME() {
    base.Exit_PRE_GAME();
  }

  protected override void Enter_PLAYING() {
    base.Enter_PLAYING();
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();
    float cozmo_angle = (robot.Rotation.eulerAngles.z + 90.0f) % 360.0f;
    float phone_angle = (Input.gyro.attitude.eulerAngles.z);
    Debug.Log("cozmo: " + cozmo_angle + ", phone: " + phone_angle);
    float delta = phone_angle - cozmo_angle;
    if (Mathf.Abs(delta) > 20.0f) {
      if (RotateClockWise(cozmo_angle, phone_angle)) {
        robot.DriveWheels(25.0f, -25.0f);
      }
      else {
        robot.DriveWheels(-25.0f, 25.0f);
      }
    }
    else {
      robot.DriveWheels(0.0f, 0.0f);
    }

    if (Input.touchCount > 0 || Input.GetMouseButton(0)) {
      robot.DriveWheels(40.0f, 40.0f);
    }
  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();
  }

  protected override void Enter_RESULTS() {
    base.Enter_RESULTS();
  }

  protected override void Update_RESULTS() {
    base.Update_RESULTS();
  }

  protected override void Exit_RESULTS() {
    base.Exit_RESULTS();
  }

  protected override bool IsPreGameCompleted() {
    return true;
  }

  protected override bool IsGameReady() {
    return gameReady;
  }

  protected override bool IsGameOver() {
    return gameOver;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();
  }

  private bool RotateClockWise(float curRot, float newRot) {
    float r1 = curRot - newRot;
    float r2 = newRot - curRot;
    if (r1 < 0.0f) {
      r1 += 360.0f;
    }
    if (r2 < 0.0f) {
      r2 += 360.0f;
    }
    return r2 < r1;
  }
}
