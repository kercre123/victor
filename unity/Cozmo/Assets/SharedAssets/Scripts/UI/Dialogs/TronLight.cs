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

      #region Settings

      [SerializeField]
      private float _LifeSpanMin = 0.75f;
      [SerializeField]
      private float _LifeSpanMax = 1.5f;
      [SerializeField]
      private int _MinTurns = 1;
      [SerializeField]
      private int _MaxTurns = 4;
      [SerializeField]
      private float _MinTurnDist = 1.0f;
      [SerializeField]
      private float _MaxTurnDist = 8.0f;

      [SerializeField]
      private TrailRenderer _Trail;
      [SerializeField]
      private Image _Spark;

      #endregion

      public Action<TronLight> OnLifeSpanEnd;
      private Direction _CurrDir = Direction.Up;
      private float _CurrLifeSpan;
      private float _CurrDist = 0.0f;
      private float _CurrInterval = 0.0f;
      private int _CurrTurns = 0;
      private Tweener _CurrTween = null;

      public void Initialize() {
        _CurrTurns = UnityEngine.Random.Range(_MinTurns, _MaxTurns);
        // Don't destroy the universe by dividing by 0
        if (_CurrTurns < 1) {
          _CurrTurns = 1;
        }
        _Spark.DOFade(1.0f, 0.0f);
        transform.position = transform.parent.position;
        _CurrLifeSpan = UnityEngine.Random.Range(_LifeSpanMin, _LifeSpanMax);
        _CurrDir = (Direction)UnityEngine.Random.Range(0, (int)Direction.Down);
        _CurrInterval = (_LifeSpanMax / (_CurrTurns + 1));
        _Trail.Clear();
        HandleTweenEnd();
        _Trail.enabled = true;
      }

      public void OnPool() {
        if (_CurrTween != null) {
          _CurrTween.Kill(false);
        }
        _Trail.Clear();
        _Trail.enabled = false;
        transform.position = transform.parent.position;
      }

      private void FinishEffect() {
        if (OnLifeSpanEnd != null) {
          OnLifeSpanEnd.Invoke(this);
        }
      }

      // If lifespan is remaining, turn towards a new target and tween to it.
      // If lifespan is exhausted, fade out over the time that trail will take to
      // disappear, then trigger FinishEffect.
      private void HandleTweenEnd() {
        // R.A. -  Not sure why this is what fixes this instead of all the other code that should.
        // Before we got tons of extra trails shooting across the screen with pooling.
        // So I built this shrine and killed a Tween as an sacrificial offering
        // this appeases Unitron the Magnanimous.
        if (_CurrTween != null) {
          _CurrTween.Kill(false);
        }
        if (_CurrLifeSpan <= 0) {
          _CurrTween = _Spark.DOFade(0.0f, _Trail.time).OnComplete(FinishEffect);
        }
        else {
          _CurrDist = UnityEngine.Random.Range(_MinTurnDist, _MaxTurnDist);
          _CurrLifeSpan -= _CurrInterval;
          _CurrTween = transform.DOMove(NewTarget(), _CurrInterval).SetRelative().OnComplete(HandleTweenEnd).SetEase(Ease.Linear);
        }
      }

      private Vector3 NewTarget() {
        Vector3 newT = new Vector3();
        _CurrDir = NewDir(_CurrDir);
        switch (_CurrDir) {
        case Direction.Up:
          newT = new Vector3(0, _CurrDist, 0);
          break;
        case Direction.Right:
          newT = new Vector3(_CurrDist, 0, 0);
          break;
        case Direction.Down:
          newT = new Vector3(0, -_CurrDist, 0);
          break;
        case Direction.Left:
          newT = new Vector3(-_CurrDist, 0, 0);
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
