/**
 * Creates HTML tags necessary for localization
 *  - Adds localized string translations javascript file as script tag
 *  - Creates @font-face definitions for localized fonts in a style tag
 * @author Adam Platti
 * @since 7/13/2017
 */

// get the local string from the URL
var LOCALE = window.getUrlVars()['locale'];

// write the locale specific translations javascript file
document.write('<script src="../LocalizedStrings/' + LOCALE + '/CodeLabStrings.js"></script>');

// compute URL prefix for locale based font
var latinFontLocales = ['de-DE', 'en-US', 'fr-FR'];
var fontLocale = (latinFontLocales.indexOf(LOCALE) >= 0) ? 'latin' : LOCALE;
var fontSrcPrefix = '../fonts/Fonts-' + fontLocale;

// create a map of font names to localized font files
var fonts = {};
fonts['Avenir Next'] = fontSrcPrefix + '/AvenirLTStd-Medium.otf';
fonts['Avenir Next Black'] = fontSrcPrefix + '/AvenirLTStd-Black.otf';
fonts['Avenir Next Bold'] = fontSrcPrefix + '/AvenirLTStd-Heavy.otf';

// write out style tags with the localized @font-face declarations
document.write('<style>');
for (var family in fonts) {
  document.write('@font-face { font-family:\'' + family + '\'; src: url(\'' + fonts[family] + '\'); }\n');
}
document.write('</style>');
