// Cozmo needs system configuration data exporter


// Includes functions for exporting active sheet or all sheets as JSON object (also Python object syntax compatible).
// Tweak the makePrettyJSON_ function to customize what kind of JSON to export.

var FORMAT_ONELINE   = 'One-line';
var FORMAT_MULTILINE = 'Multi-line';
var FORMAT_PRETTY    = 'Pretty';

var LANGUAGE_JS      = 'JavaScript';
var LANGUAGE_PYTHON  = 'Python';

var STRUCTURE_LIST = 'List';
var STRUCTURE_HASH = 'Hash (keyed by "id" column)';

/* Defaults for this particular spreadsheet, change as desired */
var DEFAULT_FORMAT = FORMAT_PRETTY;
var DEFAULT_LANGUAGE = LANGUAGE_JS;
var DEFAULT_STRUCTURE = STRUCTURE_LIST;


function onOpen() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var menuEntries = [
    {name: "Export JSON for MainConfig sheet", functionName: "exportMainConfigSheet"},
    {name: "Export JSON for ActionConfig sheet", functionName: "exportActionConfigSheet"},
    {name: "Export JSON for DecayConfig sheet", functionName: "exportDecayConfigSheet"},
//  {name: "Configure export", functionName: "exportOptions"},
  ];
  ss.addMenu("Export JSON", menuEntries);
}

function makeTextBox(app, name) { 
  var textArea = app.createTextArea().setWidth('100%').setHeight('200px').setId(name).setName(name);
  return textArea;
}

/*
function exportAllSheets(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheets = ss.getSheets();
  var sheetsData = {};
  for (var i = 0; i < sheets.length; i++) {
    var sheet = sheets[i];
    var rowsData = getRowsData_(sheet, getExportOptions(e));
    var sheetName = sheet.getName(); 
    sheetsData[sheetName] = rowsData;
  }
  var json = makeJSON_(sheetsData, getExportOptions(e));
  return displayText_(json);
}
*/

function exportMainConfigSheet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("MainConfig");
  var rowsData = getRowsOfKVPs_(sheet);
  var json = makeJSON_(rowsData, getExportOptions(e));
  return displayText_(json);
}

function exportActionConfigSheet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("ActionConfig");
  var rowsData = getRowsData_(sheet);
  var json = makeJSON_(rowsData, getExportOptions(e));
  json = '{ "actionDeltas":' + json + ' }';
  return displayText_(json);
}

function exportDecayConfigSheet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("DecayConfig");
  var data = getDecayData_(sheet);
  var json = makeJSON_(data, getExportOptions(e));
  return displayText_(json);
}

function getExportOptions(e) {
  var options = {};
  
  options.language = e && e.parameter.language || DEFAULT_LANGUAGE;
  options.format   = e && e.parameter.format || DEFAULT_FORMAT;
  options.structure = e && e.parameter.structure || DEFAULT_STRUCTURE;
  
  var cache = CacheService.getPublicCache();
  cache.put('language', options.language);
  cache.put('format',   options.format);
  cache.put('structure', options.structure);
  
  Logger.log(options);
  return options;
}

function makeJSON_(object, options) {
  if (options.format == FORMAT_PRETTY) {
    var jsonString = JSON.stringify(object, null, 2);
  } else if (options.format == FORMAT_MULTILINE) {
    var jsonString = Utilities.jsonStringify(object);
    jsonString = jsonString.replace(/},/gi, '},\n');
    jsonString = prettyJSON.replace(/":\[{"/gi, '":\n[{"');
    jsonString = prettyJSON.replace(/}\],/gi, '}],\n');
  } else {
    var jsonString = Utilities.jsonStringify(object);
  }
  if (options.language == LANGUAGE_PYTHON) {
    // add unicode markers
    jsonString = jsonString.replace(/"([a-zA-Z]*)":\s+"/gi, '"$1": u"');
  }
  return jsonString;
}

function displayText_(text) {
  var app = UiApp.createApplication().setTitle('Exported JSON');
  app.add(makeTextBox(app, 'json'));
  app.getElementById('json').setText(text);
  var ss = SpreadsheetApp.getActiveSpreadsheet(); 
  ss.show(app);
  return app; 
}

// getRowsData iterates row by row in the input range and returns an array of objects.
// Each object contains all the data for a given row, indexed by its normalized column name.
// Arguments:
//   - sheet: the sheet object that contains the data to be processed
// Returns an Array of objects.
function getRowsData_(sheet, options) {
  Logger.log("We're inside getRowsData_");
  var headersRange = sheet.getRange(1, 1, sheet.getFrozenRows(), sheet.getMaxColumns());
  var headers = headersRange.getValues()[0];
  var dataRange = sheet.getRange(sheet.getFrozenRows()+1, 1, sheet.getMaxRows(), sheet.getMaxColumns());
  var objects = getObjects_(dataRange.getValues(), normalizeHeaders_(headers));
  return objects;
}

// Makes a single flat object of key-value pairs, where the keys are in the first column,
// and corresponding values are in the second column.
function getRowsOfKVPs_(sheet) {
  Logger.log("We're inside getRowsOfKVPs_");
  var dataRange = sheet.getRange(sheet.getFrozenRows()+1, 1, sheet.getMaxRows(), 2);
  var data = dataRange.getValues();
  var bigFlatObject = {};
  for (var row = 0; row < data.length; ++row) {
    var keyCellData = data[row][0];
    if (isCellEmpty_(keyCellData)) {
      continue;
    }
    bigFlatObject[keyCellData] = data[row][1];
    Logger.log("keyCell: " + keyCellData + "; value: " + bigFlatObject[keyCellData]);
  }
  return bigFlatObject;
}

// Parse the decay config sheet
function getDecayData_(sheet) {
  Logger.log("We're inside getDecayData_");
  var dataRange = sheet.getRange(1, 1, sheet.getMaxRows(), 4);
  var data = dataRange.getValues();
  var destObj = {};
  destObj["DecayRates"] = {};
  destObj["DecayModifiers"] = {};
  var parsingRates = true; // otherwise, parsing modifiers
  var lastKeyword = "";
  var lastThreshold = -1;
  for (var row = 0; row < data.length; ++row) {
    var keyCellData = data[row][0];
    var col1Data = data[row][1];
    if (isCellEmpty_(keyCellData) && isCellEmpty_(col1Data)) {
      continue;
    }
    if (keyCellData == 'DECAY RATES:') {
      parsingRates = true;
      continue;
    }
    if (keyCellData == 'DECAY MODIFIERS:') {
      parsingRates = false;
      continue;
    }
    var curKeyword = lastKeyword;
    if (!isCellEmpty_(keyCellData)) {
      curKeyword = keyCellData;
    }
    lastKeyword = curKeyword;
    var col2Data = data[row][2];
    if (parsingRates) {
      // Parse the rates columns
      var rateConfig = {};
      rateConfig["Threshold"] = col1Data;
      rateConfig["DecayPerMinute"] = col2Data;
      if (destObj["DecayRates"][curKeyword] === undefined) {
        destObj["DecayRates"][curKeyword] = [];
      }
      destObj["DecayRates"][curKeyword].push(rateConfig);
    } else {
      // Parse the modifiers columns
      var col3Data = data[row][3];
      if (destObj["DecayModifiers"][curKeyword] === undefined) {
        destObj["DecayModifiers"][curKeyword] = [];
      }
      var curThreshold = col1Data;
      if (curThreshold != lastThreshold) {
        var thresholdObj = {};
        thresholdObj["Threshold"] = curThreshold;
        thresholdObj["OtherNeedsAffected"] = [];
        destObj["DecayModifiers"][curKeyword].push(thresholdObj);
      }
      var modifierConfig = {};
      modifierConfig["OtherNeedID"] = col2Data;
      modifierConfig["Multiplier"] = col3Data;
      var size = destObj["DecayModifiers"][curKeyword].length;
      destObj["DecayModifiers"][curKeyword][size - 1]["OtherNeedsAffected"].push(modifierConfig);
      lastThreshold = curThreshold;
    }
  }
  return destObj;
}

// getColumnsData iterates column by column in the input range and returns an array of objects.
// Each object contains all the data for a given column, indexed by its normalized row name.
// Arguments:
//   - sheet: the sheet object that contains the data to be processed
//   - range: the exact range of cells where the data is stored
//   - rowHeadersColumnIndex: specifies the column number where the row names are stored.
//       This argument is optional and it defaults to the column immediately left of the range; 
// Returns an Array of objects.
function getColumnsData_(sheet, range, rowHeadersColumnIndex) {
  rowHeadersColumnIndex = rowHeadersColumnIndex || range.getColumnIndex() - 1;
  var headersTmp = sheet.getRange(range.getRow(), rowHeadersColumnIndex, range.getNumRows(), 1).getValues();
  var headers = normalizeHeaders_(arrayTranspose_(headersTmp)[0]);
  return getObjects(arrayTranspose_(range.getValues()), headers);
}


// For every row of data in data, generates an object that contains the data. Names of
// object fields are defined in keys.
// Arguments:
//   - data: JavaScript 2d array
//   - keys: Array of Strings that define the property names for the objects to create
function getObjects_(data, keys) {
  var objects = [];
  for (var i = 0; i < data.length; ++i) {
    var object = {};
    var hasData = false;
    for (var j = 0; j < data[i].length; ++j) {
      var cellData = data[i][j];
      if (isCellEmpty_(cellData)) {
        continue;
      }
      object[keys[j]] = cellData;
      hasData = true;
    }
    if (hasData) {
      objects.push(object);
    }
  }
  return objects;
}

// Returns an Array of normalized Strings.
// Arguments:
//   - headers: Array of Strings to normalize
function normalizeHeaders_(headers) {
  var keys = [];
  for (var i = 0; i < headers.length; ++i) {
    var key = normalizeHeader_(headers[i]);
    if (key.length > 0) {
      keys.push(key);
    }
  }
  return keys;
}

// Normalizes a string, by removing all alphanumeric characters and using mixed case
// to separate words. The output will always start with a lower case letter.
// This function is designed to produce JavaScript object property names.
// Arguments:
//   - header: string to normalize
// Examples:
//   "First Name" -> "firstName"
//   "Market Cap (millions) -> "marketCapMillions
//   "1 number at the beginning is ignored" -> "numberAtTheBeginningIsIgnored"
function normalizeHeader_(header) {
  var key = "";
  var upperCase = false;
  for (var i = 0; i < header.length; ++i) {
    var letter = header[i];
    if (letter == " " && key.length > 0) {
      upperCase = true;
      continue;
    }
    if (!isAlnum_(letter)) {
      continue;
    }
    if (key.length == 0 && isDigit_(letter)) {
      continue; // first character must be a letter
    }
    if (upperCase) {
      upperCase = false;
      key += letter.toUpperCase();
    } else {
      key += letter.toLowerCase();
    }
  }
  return key;
}

// Returns true if the cell where cellData was read from is empty.
// Arguments:
//   - cellData: string
function isCellEmpty_(cellData) {
  return typeof(cellData) == "string" && cellData == "";
}

// Returns true if the character char is alphabetical, false otherwise.
function isAlnum_(char) {
  return char >= 'A' && char <= 'Z' ||
    char >= 'a' && char <= 'z' ||
    isDigit_(char);
}

// Returns true if the character char is a digit, false otherwise.
function isDigit_(char) {
  return char >= '0' && char <= '9';
}

// Given a JavaScript 2d Array, this function returns the transposed table.
// Arguments:
//   - data: JavaScript 2d Array
// Returns a JavaScript 2d Array
// Example: arrayTranspose([[1,2,3],[4,5,6]]) returns [[1,4],[2,5],[3,6]].
function arrayTranspose_(data) {
  if (data.length == 0 || data[0].length == 0) {
    return null;
  }

  var ret = [];
  for (var i = 0; i < data[0].length; ++i) {
    ret.push([]);
  }

  for (var i = 0; i < data.length; ++i) {
    for (var j = 0; j < data[i].length; ++j) {
      ret[j][i] = data[i][j];
    }
  }

  return ret;
}
