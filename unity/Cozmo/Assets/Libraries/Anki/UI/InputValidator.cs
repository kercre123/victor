using UnityEngine;
using System.Collections;
using System.Text.RegularExpressions;

namespace Anki {
  namespace UI {
    // Class that checks for valid inputs for certain types of fields
    public static class InputValidator {

      // Basic email validation
      // Just checking that its 4 letters long, contains a @ and a .
      public static bool IsValidEmail(string email) {
        return (email.Length >= 4 && email.Contains("@") && email.Contains("."));
      }

      // Check if phone number is valid (US only), TODO: Need to localize this
      // found here http://www.regexlib.com/Search.aspx?k=phone&AspxAutoDetectCookieSupport=1
      // Checking for xxxxxxxxxx or xxx-xxx-xxxx or (xxx)xxx-xxxx
      public static bool IsValidPhoneNumber(string phoneNumber) {
        return Regex.IsMatch(phoneNumber, @"^\D?(\d{3})\D?\D?(\d{3})\D?(\d{4})$");
      }

      // Return whether the text input is valid, right now only checking to make sure that it's not empty
      public static bool IsValidCity(string s) {
        return !string.IsNullOrEmpty(s);
      }
      
      // Return whether a string is a valid state, currently only checking if it has 2 letters
      // TODO: This needs to be changed to a combo box
      public static bool IsValidState(string state) {
        return state.Length == 2;
      }
      
      // Return whether its a valid zip
      public static bool IsValidZip(string zip) {
        return zip.Length == 5;
      }

      // Need to check that password is at least 6 characters and doesn't contain driver name
      public static bool IsValidPassword(string pass, string driverName) {
        return pass.Length >= 6 && !pass.Contains(driverName);
      }
    }
  }
}
