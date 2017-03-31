using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System;

public class SampleScript : MonoBehaviour {
  public WheelDatePicker datePicker;

  void Start() {
    datePicker.date = DateTime.Now;
  }
}