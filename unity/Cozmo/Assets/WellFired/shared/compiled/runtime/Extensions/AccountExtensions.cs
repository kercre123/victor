using System;
using System.Globalization;
using System.Text.RegularExpressions;

public static class AccountExtensions
{
    public static bool IsValidEmailAddress(string email)
    {
        if(String.IsNullOrEmpty(email))
        {
            return false;
        }

        var pattern = @"^(?!\.)(""([^""\r\\]|\\[""\r\\])*""|([-a-z0-9!#$%&'*+/=?^_`{|}~]|(?<!\.)\.)*)(?<!\.)@[a-z0-9][\w\.-]*[a-z0-9]\.[a-z][a-z\.]*[a-z]$";
        Regex regex = new Regex(pattern, RegexOptions.IgnoreCase);
        return regex.IsMatch(email);
	}

	public static string GetRandomName()
	{
		return names[UnityEngine.Random.Range(0, names.Length - 1)];
	}

	private static string[] names = 
	{
		"Penetrator",
		"Kenneth",
		"Palette",
		"Mark",
		"Parmesian",
		"Spitfire",
		"Eva",
		"Alpro",
		"Redtail",
		"Infiltrator",
		"RearEnd",
		"White Mice",
		"Unconventional",
		"Penis Man",
		"The Bin Man",
		"The Evacuator",
		"Giant Lump",
		"Ejaculatory",
		"The Sperminator",
		"Jester",
		"Maverick",
		"Ice Man",
		"Slider",
		"Ironside",
		"T-Bag",
		"Sundown",
		"Max",
		"Wizard",
		"Merlin",
		"BARRY",
		"Vitamin C",
		"Zoolander",
		"Coaster",
		"Plank",
		"Neo1988",
		"n3o",
		"ETC",
		"Read End Connection",
		"Blind",
		"Z - Ray",
		"Egg-man",
		"AXIX",
		"Sergio Georgini",
	};
}