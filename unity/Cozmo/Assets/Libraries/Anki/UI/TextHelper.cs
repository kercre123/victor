using System.Collections.Generic;
namespace Anki {
  
  namespace UI {
  
    public static class TextHelper {

      private static string _Language = "en";
      private static Dictionary<string,OrdinalStrategy> _OrdinalStrategies = new Dictionary<string,OrdinalStrategy>();
      private static List<string> _SupportedLanguages = new List<string>();

      public static void LoadStrategies() {
        _OrdinalStrategies.Clear();
        _SupportedLanguages.Clear();
        
        _OrdinalStrategies.Add("en", new EnglishOrdinalStrategy());
        _OrdinalStrategies.Add("de", new GermanOrdinalStrategy());
        
        // update supported languages
        _SupportedLanguages.Add("en");
        _SupportedLanguages.Add("de");
        
        _Language = "en";
      }

      public static void SetLanguage(string language) {
        DAS.Event("ui.text_helper.set_language", language);
        if( _SupportedLanguages.Contains(language) ) {
          _Language = language;
        }
        else {
          // detected unsupported language; default to en
          DAS.Event("ui.text_helper.unsupported_language", language);
          _Language = "en";
        }
      }

      // returns localized ordinal string for 1-based rank
      public static string OrdinalString(uint rank, bool includeRankInString = true) {
        OrdinalStrategy strategy = _OrdinalStrategies[_Language];
        if (strategy != null) {
          string ordinal = strategy.OrdinalString(rank, includeRankInString);
          return ordinal;
        }
        else {
          return null;
        }
      }

      // returns time string for number of seconds
      public static string TimeString(float seconds) {
        int minuteValue = (int)(seconds / 60);
        int secondValue = (int)(seconds - minuteValue * 60);
        return string.Format("{0:00}:{1:00}", minuteValue, secondValue);
      }

      // returns time string for number of seconds
      public static string TimeStringWithTenths(float seconds) {
        int minuteValue = (int)(seconds / 60);
        int secondValue = (int)(seconds - minuteValue * 60);
        int tenthsValue = (int)((seconds - ((int)seconds)) * 10);
        return string.Format("{0:00}:{1:00}.{2}", minuteValue, secondValue, tenthsValue);
      }


    public interface OrdinalStrategy {
      string OrdinalString(uint rank, bool includeRankInString = true);
    }

    public class EnglishOrdinalStrategy : OrdinalStrategy {
      public string OrdinalString(uint rank, bool includeRankInString = true) {
        string ordinal = null;
        if (rank == 1) {
          ordinal = "st";
        }
        else if (rank == 2) {
          ordinal = "nd";
        }
        else if (rank == 3) {
          ordinal = "rd";
        }
        else {
          ordinal = "th";
        }

        if (includeRankInString) {
          ordinal = rank + ordinal;
        }

        return ordinal;
      }
    }

    public class GermanOrdinalStrategy : OrdinalStrategy {
      public string OrdinalString(uint rank, bool includeRankInString = true) {
        string ordinal = ".";

        if (includeRankInString) {
          ordinal = rank + ordinal;
        }

        return ordinal;
      }
    }

  }
  
  }
}
