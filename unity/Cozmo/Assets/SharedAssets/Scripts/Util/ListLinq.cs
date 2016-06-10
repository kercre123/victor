using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

namespace Cozmo.Util {
  public static class IListLinq {

    #region Int Math

    public static int Sum(this IList<int> list) {
      int sum = 0;
      for (int i = 0, len = list.Count; i < len; i++) {
        sum += list[i];
      }
      return sum;
    }

    public static int Max(this IList<int> list) {
      int max = int.MinValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        max = (list[i] > max ? list[i] : max);
      }
      return max;
    }

    public static int Min(this IList<int> list) {
      int min = int.MaxValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        min = (list[i] < min ? list[i] : min);
      }
      return min;
    }

    public static void Add(this IList<int> list, IList<int> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] += second[i];
      }
    }

    public static void Subtract(this IList<int> list, IList<int> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] -= second[i];
      }
    }

    public static void Multiply(this IList<int> list, IList<int> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] *= second[i];
      }
    }

    public static void Divide(this IList<int> list, IList<int> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] /= second[i];
      }
    }

    public static void Invert(this IList<int> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = -list[i];
      }
    }

    public static void Zero(this IList<int> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = 0;
      }
    }

    #endregion

    #region Long Math

    public static long Sum(this IList<long> list) {
      long sum = 0;
      for (int i = 0, len = list.Count; i < len; i++) {
        sum += list[i];
      }
      return sum;
    }

    public static long Max(this IList<long> list) {
      long max = long.MinValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        max = (list[i] > max ? list[i] : max);
      }
      return max;
    }

    public static long Min(this IList<long> list) {
      long min = long.MaxValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        min = (list[i] < min ? list[i] : min);
      }
      return min;
    }

    public static void Add(this IList<long> list, IList<long> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] += second[i];
      }
    }

    public static void Subtract(this IList<long> list, IList<long> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] -= second[i];
      }
    }

    public static void Multiply(this IList<long> list, IList<long> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] *= second[i];
      }
    }

    public static void Divide(this IList<long> list, IList<long> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] /= second[i];
      }
    }

    public static void Invert(this IList<long> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = -list[i];
      }
    }

    public static void Zero(this IList<long> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = 0;
      }
    }

    #endregion

    #region Float Math

    public static float Sum(this IList<float> list) {
      float sum = 0;
      for (int i = 0, len = list.Count; i < len; i++) {
        sum += list[i];
      }
      return sum;
    }

    public static float Max(this IList<float> list) {
      float max = float.MinValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        max = (list[i] > max ? list[i] : max);
      }
      return max;
    }

    public static float Min(this IList<float> list) {
      float min = float.MaxValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        min = (list[i] < min ? list[i] : min);
      }
      return min;
    }

    public static void Add(this IList<float> list, IList<float> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] += second[i];
      }
    }

    public static void Subtract(this IList<float> list, IList<float> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] -= second[i];
      }
    }

    public static void Multiply(this IList<float> list, IList<float> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] *= second[i];
      }
    }

    public static void Divide(this IList<float> list, IList<float> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] /= second[i];
      }
    }

    public static void Invert(this IList<float> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = -list[i];
      }
    }

    public static void Zero(this IList<float> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = 0;
      }
    }

    #endregion

    #region Double Math

    public static double Sum(this IList<double> list) {
      double sum = 0;
      for (int i = 0, len = list.Count; i < len; i++) {
        sum += list[i];
      }
      return sum;
    }

    public static double Max(this IList<double> list) {
      double max = double.MinValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        max = (list[i] > max ? list[i] : max);
      }
      return max;
    }

    public static double Min(this IList<double> list) {
      double min = double.MaxValue;
      for (int i = 0, len = list.Count; i < len; i++) {
        min = (list[i] < min ? list[i] : min);
      }
      return min;
    }

    public static void Add(this IList<double> list, IList<double> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] += second[i];
      }
    }

    public static void Subtract(this IList<double> list, IList<double> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] -= second[i];
      }
    }

    public static void Multiply(this IList<double> list, IList<double> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] *= second[i];
      }
    }

    public static void Divide(this IList<double> list, IList<double> second) {
      for (int i = 0; i < list.Count && i < second.Count; i++) {
        list[i] /= second[i];
      }
    }

    public static void Invert(this IList<double> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = -list[i];
      }
    }

    public static void Zero(this IList<double> list) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = 0;
      }
    }

    #endregion

    #region Helper Methods
    public static bool Any<T>(this IList<T> list, Func<T, bool> selector = null) {
      for (int i = 0, len = list.Count; i < len; i++) {
        if (selector == null || selector(list[i])) {
          return true;
        }
      }
      return false;
    }

    public static T FirstOrDefault<T>(this IList<T> list) {
      if (list.Count > 0) {
        return list[0];
      }
      return default(T);
    }

    public static T FirstOrDefault<T>(this IList<T> list, Func<T, bool> selector) {
      for (int i = 0, len = list.Count; i < len; i++) {
        if (selector == null || selector(list[i])) {
          return list[i];
        }
      }
      return default(T);
    }

    public static T LastOrDefault<T>(this IList<T> list) {
      if (list.Count > 0) {
        return list[list.Count - 1];
      }
      return default(T);
    }

    public static T LastOrDefault<T>(this IList<T> list, Func<T, bool> selector) {
      for (int i = list.Count - 1; i >= 0; i--) {
        if (selector == null || selector(list[i])) {
          return list[i];
        }
      }
      return default(T);
    }

    public static TDest[] SelectArray<TSource, TDest>(this IList<TSource> list, Func<TSource, TDest> selector) {
      TDest[] result = new TDest[list.Count];
      for (int i = 0, len = list.Count; i < len; i++) {
        result[i] = selector(list[i]);
      }
      return result;
    }

    public static IList<TDest> SelectList<TSource, TDest>(this IList<TSource> list, Func<TSource, TDest> selector) {
      List<TDest> result = new List<TDest>(list.Count);
      for (int i = 0, len = list.Count; i < len; i++) {
        result.Add(selector(list[i]));
      }
      return result;
    }

    public static List<T> ToList<T>(IList<T> list) {
      // passing the list into the constructor treats it as an IEnumerable, which allocates
      List<T> result = new List<T>(list.Count);
      for (int i = 0, len = list.Count; i < len; i++) {
        result.Add(list[i]);
      }
      return result;
    }

    public static void ForEach<T>(this IList<T> list, Action<T> action) {
      for (int i = 0, len = list.Count; i < len; i++) {
        action(list[i]);
      }
    }

    public static void ForEach<T>(this IEnumerable<T> list, Action<T> action) {
      foreach (var element in list) {
        action(element);
      }
    }

    public static void UpdateEach<T>(this IList<T> list, Func<T, T> action) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = action(list[i]);
      }
    }

    public static void Fill<T>(this IList<T> list, T value) {
      for (int i = 0, len = list.Count; i < len; i++) {
        list[i] = value;
      }
    }

    public static void FillDefault<T>(this IList<T> list) {
      list.Fill(default(T));
    }

    public static void ReverseInPlace<T>(this IList<T> list) {
      for (int i = 0, end = list.Count - 1, len = list.Count / 2; i < len; i++) {
        var tmp = list[i];
        list[i] = list[end - i];
        list[end - i] = tmp;
      }
    }

    public static IEnumerable<T> Unique<T>(this IList<T> list) {
      HashSet<T> hashSet = new HashSet<T>();
      for (int i = 0; i < list.Count; i++) {
        hashSet.Add(list[i]);
      }
      return hashSet;
    }

    public static T[] UniqueArray<T>(this IList<T> list) {
      // We can't just call Unique here because of AOT.
      HashSet<T> hashSet = new HashSet<T>();
      for (int i = 0; i < list.Count; i++) {
        hashSet.Add(list[i]);
      }
      T[] result = new T[hashSet.Count];
      int j = 0;
      foreach (var item in hashSet) {
        result[j] = item;
        j++;
      }
      return result;
    }

    public static IEnumerable<T> Reversed<T>(this IList<T> list) {
      for (int i = list.Count - 1; i >= 0; i--) {
        yield return list[i];
      }
    }

    public static bool SequenceEquals<T>(this IList<T> a1, IList<T> a2, EqualityComparer<T> comparer = null) {
      if (ReferenceEquals(a1, a2))
        return true;

      if (a1 == null || a2 == null)
        return false;

      if (a1.Count != a2.Count)
        return false;

      if (comparer == null) comparer = EqualityComparer<T>.Default;
      for (int i = 0; i < a1.Count; i++) {
        if (!comparer.Equals(a1[i], a2[i])) return false;
      }
      return true;
    }

    public static void Shuffle<T>(this IList<T> list) {
      int n = list.Count;
      while (n > 1) {
        n--;
        int k = UnityEngine.Random.Range(0, n + 1);
        T value = list[k];
        list[k] = list[n];
        list[n] = value;
      }
    }

    #endregion
  }
}
