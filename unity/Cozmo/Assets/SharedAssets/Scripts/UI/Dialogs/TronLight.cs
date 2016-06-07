using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.UI;
using Anki.Cozmo;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class TronLight : MonoBehaviour {

      enum DIR {
        UP = 0,
        RIGHT = 1,
        LEFT = 2,
        DOWN = 3
      }

      private DIR _currDir = DIR.UP;
      private float _currLifeSpan;
      private float _lifeSpanMin = 0.75f;
      private float _lifeSpanMax = 1.5f;
      private int _MinTurns = 1;
      private int _MaxTurns = 5;
      private float _MinTurnDist = 0.75f;
      private float _MaxTurnDist = 8.0f;

      private float _currDist = 0.0f;
      private float _currInterval = 0.0f;
      private int _currTurns = 0;
      private int _turnsDone = 0;
      [SerializeField]
      private TrailRenderer _trail;
      [SerializeField]
      private Image _spark;
      private Tweener _currTween = null;

      private void Awake() {
        _currTurns = UnityEngine.Random.Range(_MinTurns, _MaxTurns);
        // Don't destroy the universe by dividing by 0
        if (_currTurns < 1) {
          _currTurns = 1;
        }
        _currLifeSpan = UnityEngine.Random.Range(_lifeSpanMin, _lifeSpanMax);
        _currDir = (DIR)UnityEngine.Random.Range(0, (int)DIR.DOWN);
        _currInterval = (_lifeSpanMax / (_currTurns + 1));
        HandleTween();
      }

      private void OnDestroy() {
        if (_currTween != null) {
          _currTween.Kill(false);
        }
      }

      private void FinishEffect() {
        GameObject.Destroy(gameObject);
      }

      private void HandleTween() {
        if (_currLifeSpan <= 0) {
          _currTween = _spark.DOFade(0.0f, _trail.time).OnComplete(FinishEffect);
        }
        else {
          _currDist = UnityEngine.Random.Range(_MinTurnDist, _MaxTurnDist);
          _currLifeSpan -= _currInterval;
          _currTween = transform.DOMove(NewTarget(), _currInterval).SetRelative().OnComplete(HandleTween).SetEase(Ease.Linear);
          _turnsDone++;
        }
      }

      private Vector3 NewTarget() {
        Vector3 newT = new Vector3();
        _currDir = NewDir(_currDir);
        switch (_currDir) {
        case DIR.UP:
          newT = new Vector3(0, _currDist, 0);
          break;
        case DIR.RIGHT:
          newT = new Vector3(_currDist, 0, 0);
          break;
        case DIR.DOWN:
          newT = new Vector3(0, -_currDist, 0);
          break;
        case DIR.LEFT:
          newT = new Vector3(-_currDist, 0, 0);
          break;
        }
        return newT;
      }

      private DIR NewDir(DIR oldD) {
        DIR newD = oldD;
        float coinFlip = UnityEngine.Random.Range(0.0f, 1.0f);
        bool turnRight = (coinFlip > 0.5f);
        switch (oldD) {
        case DIR.UP:
          if (turnRight) {
            newD = DIR.RIGHT;
          }
          else {
            newD = DIR.LEFT;
          }
          break;
        case DIR.RIGHT:
          if (turnRight) {
            newD = DIR.DOWN;
          }
          else {
            newD = DIR.UP;
          }
          break;
        case DIR.DOWN:
          if (turnRight) {
            newD = DIR.LEFT;
          }
          else {
            newD = DIR.RIGHT;
          }
          break;
        case DIR.LEFT:
          if (turnRight) {
            newD = DIR.UP;
          }
          else {
            newD = DIR.DOWN;
          }
          break;
        }
        return newD;
      }
    }
  }
}
