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

      enum Direction {
        Up = 0,
        Right = 1,
        Left = 2,
        Down = 3
      }

      public Action<TronLight> OnLifeSpanEnd;
      private Direction _currDir = Direction.Up;
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
        _currDir = (Direction)UnityEngine.Random.Range(0, (int)Direction.Down);
        _currInterval = (_lifeSpanMax / (_currTurns + 1));
        HandleTween();
      }

      private void OnDestroy() {
        if (_currTween != null) {
          _currTween.Kill(false);
        }
      }

      private void FinishEffect() {
        if (_currTween != null) {
          _currTween.Kill(false);
        }
        if (OnLifeSpanEnd != null) {
          OnLifeSpanEnd.Invoke(this);
        }
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
        case Direction.Up:
          newT = new Vector3(0, _currDist, 0);
          break;
        case Direction.Right:
          newT = new Vector3(_currDist, 0, 0);
          break;
        case Direction.Down:
          newT = new Vector3(0, -_currDist, 0);
          break;
        case Direction.Left:
          newT = new Vector3(-_currDist, 0, 0);
          break;
        }
        return newT;
      }

      private Direction NewDir(Direction oldD) {
        Direction newD = oldD;
        float coinFlip = UnityEngine.Random.Range(0.0f, 1.0f);
        bool turnRight = (coinFlip > 0.5f);
        switch (oldD) {
        case Direction.Up:
          if (turnRight) {
            newD = Direction.Right;
          }
          else {
            newD = Direction.Left;
          }
          break;
        case Direction.Right:
          if (turnRight) {
            newD = Direction.Down;
          }
          else {
            newD = Direction.Up;
          }
          break;
        case Direction.Down:
          if (turnRight) {
            newD = Direction.Left;
          }
          else {
            newD = Direction.Right;
          }
          break;
        case Direction.Left:
          if (turnRight) {
            newD = Direction.Up;
          }
          else {
            newD = Direction.Down;
          }
          break;
        }
        return newD;
      }
    }
  }
}
