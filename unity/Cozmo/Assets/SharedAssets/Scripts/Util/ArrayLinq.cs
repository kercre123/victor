using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

namespace Cozmo.Util { 
  public static class ArrayLinq {

    #region Int Math

    public static int Sum(this int[] array) {
      int sum = 0;
      for (int i = 0; i < array.Length; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static int Max(this int[] array) {
      int max = int.MinValue;
      for (int i = 0; i < array.Length; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static int Min(this int[] array) {
      int min = int.MaxValue;
      for (int i = 0; i < array.Length; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this int[] array, int[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this int[] array, int[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this int[] array, int[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this int[] array, int[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this int[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this int[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Long Math

    public static long Sum(this long[] array) {
      long sum = 0;
      for (int i = 0; i < array.Length; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static long Max(this long[] array) {
      long max = long.MinValue;
      for (int i = 0; i < array.Length; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static long Min(this long[] array) {
      long min = long.MaxValue;
      for (int i = 0; i < array.Length; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this long[] array, long[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this long[] array, long[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this long[] array, long[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this long[] array, long[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this long[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this long[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Float Math

    public static float Sum(this float[] array) {
      float sum = 0;
      for (int i = 0; i < array.Length; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static float Max(this float[] array) {
      float max = float.MinValue;
      for (int i = 0; i < array.Length; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static float Min(this float[] array) {
      float min = float.MaxValue;
      for (int i = 0; i < array.Length; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this float[] array, float[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this float[] array, float[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this float[] array, float[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this float[] array, float[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this float[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this float[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Double Math

    public static double Sum(this double[] array) {
      double sum = 0;
      for (int i = 0; i < array.Length; i++) {
        sum += array[i];
      }
      return sum;
    }

    public static double Max(this double[] array) {
      double max = double.MinValue;
      for (int i = 0; i < array.Length; i++) {
        max = (array[i] > max ? array[i] : max);
      }
      return max;
    }

    public static double Min(this double[] array) {
      double min = double.MaxValue;
      for (int i = 0; i < array.Length; i++) {
        min = (array[i] < min ? array[i] : min);
      }
      return min;
    }

    public static void Add(this double[] array, double[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] += second[i];
      }
    }

    public static void Subtract(this double[] array, double[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] -= second[i];
      }
    }

    public static void Multiply(this double[] array, double[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] *= second[i];
      }
    }

    public static void Divide(this double[] array, double[] second) {
      for (int i = 0; i < array.Length && i < second.Length; i++) {
        array[i] /= second[i];
      }
    }

    public static void Invert(this double[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = -array[i];
      }
    }

    public static void Zero(this double[] array) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = 0;
      }
    }

    #endregion

    #region Helper Methods
    public static T FirstOrDefault<T>(this T[] array) {
      if (array.Length > 0) {
        return array[0];
      }
      return default(T);
    }

    public static T FirstOrDefault<T>(this T[] array, Func<T, bool> selector) {
      if (array.Length > 0) {
        for (int i = 0; i < array.Length; i++) {
          if (selector == null || selector(array[i])) {
            return array[i];
          }
        }
      }
      return default(T);
    }

    public static TDest[] SelectArray<TSource, TDest>(this TSource[] array, Func<TSource, TDest> selector) {
      TDest[] result = new TDest[array.Length];
      for (int i = 0; i < array.Length; i++) {
        result[i] = selector(array[i]);
      }
      return result;
    }

    public static List<TDest> SelectList<TSource, TDest>(this TSource[] array, Func<TSource, TDest> selector) {
      List<TDest> result = new List<TDest>(array.Length);
      for (int i = 0; i < array.Length; i++) {
        result.Add(selector(array[i]));
      }
      return result;
    }
      
    public static List<T> ToList<T>(T[] array) {
      // passing the array into the constructor treats it as an IEnumerable, which allocates
      List<T> result = new List<T>(array.Length);
      for (int i = 0; i < array.Length; i++) {
        result.Add(array[i]);
      }
      return result;
    }

    public static void ForEach<T>(this T[] array, Action<T> action) {
      for (int i = 0; i < array.Length; i++) {
        action(array[i]);
      }
    }
      
    public static void UpdateEach<T>(this T[] array, Func<T,T> action) {
      for (int i = 0; i < array.Length; i++) {
        array[i] = action(array[i]);
      }
    }
      
    public static bool SequenceEquals<T>(this T[] a1, T[] a2, EqualityComparer<T> comparer = null)
    {
      if (ReferenceEquals(a1, a2))
        return true;

      if (a1 == null || a2 == null)
        return false;

      if (a1.Length != a2.Length)
        return false;

      if(comparer == null ) comparer = EqualityComparer<T>.Default;
      for (int i = 0; i < a1.Length; i++)
      {
        if (!comparer.Equals(a1[i], a2[i])) return false;
      }
      return true;
    }

    #endregion
  }
}
