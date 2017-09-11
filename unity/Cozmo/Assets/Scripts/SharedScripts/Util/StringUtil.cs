using System;

public static class StringUtil {

  public static string ToLowerUnderscore(this string str) {
    return str.ToHumanFriendly().ToLowerInvariant().Replace(" ", "_");
  }

  public static string ToHumanFriendly(this string str) {
    var result = string.Empty;
    bool wasLower = false;
    bool spaceInserted = true;
    for (int i = str.Length - 1; i >= 0; i--) {
      if (str[i] == '_') {
        result = " " + result;
        spaceInserted = true;
        wasLower = false;
        continue;
      }
      if (str[i].IsUpperCase()) {
        result = str[i] + result;
        if (wasLower && !spaceInserted) {
          spaceInserted = true;
          result = " " + result;
        }
        else {
          spaceInserted = false;
        }
        wasLower = false;
      }
      else {
        if (!wasLower && !spaceInserted) {
          result = " " + result;
          spaceInserted = true;
        }
        else {
          spaceInserted = false;
        }
        wasLower = true;
        result = str[i] + result;
      }
    }
    return result.Trim();
  }

  public static bool IsUpperCase(this string str) {
    return str.ToUpper() == str;
  }

  public static bool IsUpperCase(this char ch) {
    return ch.ToString().IsUpperCase();
  }

  public static bool IsLowerCase(this string str) {
    return str.ToLower() == str;
  }

  public static bool IsLowerCase(this char ch) {
    return ch.ToString().IsLowerCase();
  }

  public static string GenerateNextUniqueName(string input) {

    string newPath = "";
    System.Text.RegularExpressions.Match match = System.Text.RegularExpressions.Regex.Match(input, "[0-9]+$");

    if (match.Success) {
      string numberGroup = match.Groups[0].ToString();
      int number = System.Convert.ToInt32(numberGroup) + 1;
      newPath = input.Substring(0, input.Length - numberGroup.Length) + number.ToString();
    }
    else {
      newPath = input + "_2";
    }

    return newPath;
  }
}

