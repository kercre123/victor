#define DEBUG
 
using System;
using System.Diagnostics;

// http://answers.unity3d.com/questions/19122/assert-function.html 
public class DebugUtils {
  [Conditional("DEBUG")]
  public static void Assert(bool condition) {
    if (!condition)
      throw new Exception();
  }
}
