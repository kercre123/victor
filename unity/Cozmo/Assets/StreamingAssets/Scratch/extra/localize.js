/**
 * Creates HTML tags necessary for localization
 *  - Adds localized string translations javascript file as script tag
 *  - Creates @font-face definitions for localized fonts in a style tag
 * @author Adam Platti
 * @since 7/13/2017
 */

// get the local string from the URL
var LOCALE = window.getUrlVars()['locale'];

// compute the URL for the translations javascript file
var translationsUrl = 'LocalizedStrings/' + LOCALE + '/CodeLabStrings.js';

// compute URL prefix for locale based font
var latinFontLocales = ['de-DE', 'en-US', 'fr-FR'];
var fontLocale = (latinFontLocales.indexOf(LOCALE) >= 0) ? 'latin' : LOCALE;
var fontSrcPrefix = 'fonts/Fonts-' + fontLocale;

// if in the 'extra' folder, add parent folder to relative paths for translations and fonts
if (window.location.pathname.indexOf('/extra/') >= 0) {
  translationsUrl = '../' + translationsUrl;
  fontSrcPrefix = '../' + fontSrcPrefix;
}

// create a map of font names to localized font files
var fonts = {};
fonts['Avenir Next'] = fontSrcPrefix + '/AvenirLTStd-Medium.otf';
fonts['Avenir Next Black'] = fontSrcPrefix + '/AvenirLTStd-Black.otf';
fonts['Avenir Next Bold'] = fontSrcPrefix + '/AvenirLTStd-Heavy.otf';

// write the locale specific translations javascript file
document.write('<script src="' + translationsUrl + '"></script>');

// add Scratch/Blockly translation file
var blocklyStrings = "en.js";
if (LOCALE == 'fr-FR') {
	blocklyStrings = "fr.js";
}
else if (LOCALE == 'de-DE') {
	blocklyStrings = "de.js";
}
else if (LOCALE == 'jp-JP') {
	blocklyStrings = "ja.js";
}
document.write('<script src="' + "./lib/blocks/msg/js/" + blocklyStrings + '"></script>');

// write out style tags with the localized @font-face declarations
document.write('<style>');
for (var family in fonts) {
  document.write('@font-face { font-family:\'' + family + '\'; src: url(\'' + fonts[family] + '\'); }\n');
}
document.write('</style>');
