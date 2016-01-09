using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

namespace Cozmo.Util { 
  public static class ListLinq {

    #region Int Math

    public static int Sum(this List<int> array) {
      int sum = 0;
      for (int i = 0, len = array.Count; i < len; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static int Max(this List<int> array) {
      int max = int.MinValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static int Min(this List<int> array) {
      int min = int.MaxValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this List<int> array, List<int> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this List<int> array, List<int> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this List<int> array, List<int> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this List<int> array, List<int> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this List<int> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this List<int> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Long Math

    public static long Sum(this List<long> array) {
      long sum = 0;
      for (int i = 0, len = array.Count; i < len; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static long Max(this List<long> array) {
      long max = long.MinValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static long Min(this List<long> array) {
      long min = long.MaxValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this List<long> array, List<long> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this List<long> array, List<long> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this List<long> array, List<long> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this List<long> array, List<long> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this List<long> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this List<long> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Float Math

    public static float Sum(this List<float> array) {
      float sum = 0;
      for (int i = 0, len = array.Count; i < len; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static float Max(this List<float> array) {
      float max = float.MinValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static float Min(this List<float> array) {
      float min = float.MaxValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this List<float> array, List<float> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this List<float> array, List<float> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this List<float> array, List<float> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this List<float> array, List<float> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this List<float> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this List<float> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Double Math

    public static double Sum(this List<double> array) {
      double sum = 0;
      for (int i = 0, len = array.Count; i < len; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static double Max(this List<double> array) {
      double max = double.MinValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static double Min(this List<double> array) {
      double min = double.MaxValue;
      for (int i = 0, len = array.Count; i < len; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this List<double> array, List<double> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this List<double> array, List<double> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this List<double> array, List<double> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this List<double> array, List<double> second) {
      for (int i = 0; i < array.Count && i < second.Count; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this List<double> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this List<double> array) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Helper Methods
    public static T FirstOrDefault<T>(this List<T> array) {
      if (array.Count > 0) {
        return array[0];
      }
      return default(T);
    }

    public static T FirstOrDefault<T>(this List<T> array, Func<T, bool> selector) {
      if (array.Count > 0) {
        for (int i = 0, len = array.Count; i < len; i++) {
          if (selector == null || selector(array[i])) {
            return array[i];
          }
        }
      }
      return default(T);
    }

    public static TDest[] SelectArray<TSource, TDest>(this List<TSource> array, Func<TSource, TDest> selector) {
      TDest[] result = new TDest[array.Count];
      for (int i = 0, len = array.Count; i < len; i++) {
        result[i] = selector(array[i]);
      }
      return result;
    }

    public static List<TDest> SelectList<TSource, TDest>(this List<TSource> array, Func<TSource, TDest> selector) {
      List<TDest> result = new List<TDest>(array.Count);
      for (int i = 0, len = array.Count; i < len; i++) {
        result.Add(selector(array[i]));
      }
      return result;
    }
      
    public static List<T> ToList<T>(List<T> array) {
      // passing the array into the constructor treats it as an IEnumerable, which allocates
      List<T> result = new List<T>(array.Count);
      for (int i = 0, len = array.Count; i < len; i++) {
        result.Add(array[i]);
      }
      return result;
    }

    public static void ForEach<T>(this List<T> array, Action<T> action) {
      for (int i = 0, len = array.Count; i < len; i++) {
        action(array[i]);
      }
    }

    public static void ForEach<T>(this IEnumerable<T> array, Action<T> action) {
      foreach(var element in array) {
        action(element);
      }
    }

    public static void UpdateEach<T>(this List<T> array, Func<T,T> action) {
      for (int i = 0, len = array.Count; i < len; i++) {
        array[i] = action(array[i]);
      }
    }

    public static bool SequenceEquals<T>(this List<T> a1, List<T> a2, EqualityComparer<T> comparer = null)
    {
      if (ReferenceEquals(a1, a2))
        return true;

      if (a1 == null || a2 == null)
        return false;

      if (a1.Count != a2.Count)
        return false;

      if(comparer == null ) comparer = EqualityComparer<T>.Default;
      for (int i = 0; i < a1.Count; i++)
      {
        if (!comparer.Equals(a1[i], a2[i])) return false;
      }
      return true;
    }

    #endregion
  }
}
