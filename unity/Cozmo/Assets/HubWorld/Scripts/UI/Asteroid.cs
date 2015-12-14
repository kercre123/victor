using System;
using DG.Tweening;
using UnityEngine;
using System.Collections;
using Random = UnityEngine.Random;

public class Asteroid : MonoBehaviour {

  private Transform _Child;
  private Vector3 _SpinAxis;
  private Transform _Transform;

  // the closest distance to the center in our orbit
  private float _Apogee;
  // the furthest distance to the center in our orbit
  private float _Perigee;
  // time it takes to complete an orbit
  private float _OrbitLength;
  // the speed at which the asteroid spins in place
  private float _RotationSpeed;
  private float _CurrentAngle;
  private Quaternion _Orientation;

  // the major radii of our ellipse
  private float _MajorRadii;
  // the minor radii of our ellipse
  private float _MinorRadii;
  private Vector3 _OrbitCenter;

  private void Awake() {

    var index = Random.Range(0, 25);
    var prefab = Resources.Load<GameObject>("Prefabs/Asteroids/Asteroid_" + index);

    var child = (GameObject)GameObject.Instantiate(prefab);

    _Child = child.transform;

    _Child.SetParent(transform, false);

    _Child.rotation = Quaternion.LookRotation(Random.onUnitSphere, Random.onUnitSphere);

    _SpinAxis = Random.onUnitSphere;

    _Transform = transform;
  }

  // See https://en.wikipedia.org/wiki/Apsis
  public void Initialize(float apogee, float perigee, float orbitLength, float rotationSpeed, float startingAngle, Vector3 apogeeDirection) {
    _Apogee = apogee;
    _Perigee = perigee;

    float centerDistance = (_Apogee - _Perigee) * 0.5f;

    _MajorRadii = _Apogee - centerDistance;

    _MinorRadii = Mathf.Sqrt(_MajorRadii * _MajorRadii - centerDistance * centerDistance);

    _OrbitLength = orbitLength;
    _RotationSpeed = rotationSpeed;
    _CurrentAngle = startingAngle;

    _Orientation = Quaternion.FromToRotation(Vector3.up, apogeeDirection);
    _OrbitCenter = apogeeDirection * centerDistance;
    Update();
  }

  private void Update() {
    // apply spin
    var spinAngle = Mathf.LerpAngle(0f, _RotationSpeed, Time.deltaTime);
    _Child.Rotate(_SpinAxis, spinAngle);

    var orbitDeltaAngle = Mathf.LerpAngle(0f, 360 / _OrbitLength, Time.deltaTime);

    _CurrentAngle += orbitDeltaAngle;
    if (_CurrentAngle > 360) {
      _CurrentAngle -= 360;
    }

    Vector3 position = new Vector3(_MajorRadii * Mathf.Cos(_CurrentAngle * Mathf.Deg2Rad), _MinorRadii * Mathf.Sin(_CurrentAngle * Mathf.Deg2Rad), 0);

    _Transform.localPosition = _Orientation * position + _OrbitCenter;    
  }
}

