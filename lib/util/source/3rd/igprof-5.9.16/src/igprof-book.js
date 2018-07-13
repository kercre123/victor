function debug(str)
{
  //if (console)
  //  console.log(str);
  return str;
}

var need_custom_sort = (function () {
  // Fill the array with 3 values
  var array = new Array(3);
  for (var i = 0; i < 3; ++i) {
    array[i] = new Object();
  }

  // Override the toString method that counts how many times it is being called
  var count = 0;
  var save = Object.prototype.toString;
  Object.prototype.toString = function () { count += 1; return ""; };

  // Sort
  array.sort();
  Object.prototype.toString = save;

  // 3 times is good, more is bad!
  return (count === 3);
}());

String.prototype.startsWith = function(str){
    return (this.indexOf(str) === 0);
};

String.prototype.escapeHTML = function(str){
  return this.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
};

String.prototype.escapeCSS = function () {
  return this.replace(/[.:,]/g, "___");
};

String.prototype.shorten = function(max) {
  if (this.length > max-2)
    return this.slice(0, max-2) + "..";
  return this;
};

function RComma(S) { S = String(S)
  var RgX = /^(.*\s)?([-+\u00A3\u20AC]?\d+)(\d{3}\b)/
  return S == (S=S.replace(RgX, "$1$2'$3")) ? S : RComma(S); }

function NC(F) { F.out.value = RComma(F.inp.value); }

function str_repeat(i, m) { for (var o = []; m > 0; o[--m] = i); return(o.join('')); }

function sprintf () {
  var i = 0, a, f = arguments[i++], o = [], m, p, c, x;
  while (f) {
    if (m = /^[^\x25]+/.exec(f)) o.push(m[0]);
    else if (m = /^\x25{2}/.exec(f)) o.push('%');
    else if (m = /^\x25(?:(\d+)\$)?(\+)?(0|'[^$])?(-)?(\d+)?(?:\.(\d+))?([b-fosuxX])/.exec(f)) {
      if (((a = arguments[m[1] || i++]) == null) || (a == undefined)) throw("Too few arguments.");
      if (/[^s]/.test(m[7]) && (typeof(a) != 'number'))
        throw("Expecting number but found " + typeof(a));
      switch (m[7]) {
        case 'b': a = a.toString(2); break;
        case 'c': a = String.fromCharCode(a); break;
        case 'd': a = parseInt(a); break;
        case 'e': a = m[6] ? a.toExponential(m[6]) : a.toExponential(); break;
        case 'f': a = m[6] ? parseFloat(a).toFixed(m[6]) : parseFloat(a); break;
        case 'o': a = a.toString(8); break;
        case 's': a = ((a = String(a)) && m[6] ? a.substring(0, m[6]) : a); break;
        case 'u': a = Math.abs(a); break;
        case 'x': a = a.toString(16); break;
        case 'X': a = a.toString(16).toUpperCase(); break;
      }
      a = (/[def]/.test(m[7]) && m[2] && a > 0 ? '+' + a : a);
      c = m[3] ? m[3] == '0' ? '0' : m[3].charAt(1) : ' ';
      x = m[5] - String(a).length;
      p = m[5] ? str_repeat(c, x) : '';
      o.push(m[4] ? a + p : p + a);
    }
    else throw ("Huh ?!");
    f = f.substring(m[0].length);
  }
  return o.join('');
}

// Read a page's GET URL variables and return them as an associative array.
function getUrlVars()
{
    var vars = [], hash;
    var hashes = window.location.href.substring(window.location.href.indexOf('?') + 1).split('&');
    for(var i = 0; i < hashes.length; i++)
    {
        hash = hashes[i].split('=');
        vars.push(hash[0]);
        vars[hash[0]] = hash[1];
    }
    return vars;
}


var plotInvariants = ["candle", "tier", "pileup", "counter", "events",
                      "architecture", "series", "conditions", "counter",
                      "sequence", "owner"];

// Filter definitions.
var filterSpecs = [{name: "counter", id: "c", description: "Counter"},
                   {name: "architecture", id: "a", description: "Architecture"},
                   {name: "candle", id: "cl", description: "Candle"},
                   {name: "tier", id: "t", description: "Data Tier"},
                   {name: "pileup", id: "p", description: "Pile-up"},
                   {name: "sequence", id: "sq", description: "Sequence"},
                   {name: "conditions", id: "cn", description: "Conditions"},
                   {name: "series", id: "s", description: "Series"},
                   {name: "events", id: "e", description: "# of events"},
                   {name: "owner", id: "o", description: "Owner"}];

var filterIdToName = {};
var filterNameToId = {};

for (var i = 0, e = filterSpecs.length; i != e; ++i)
{
  var spec = filterSpecs[i];
  filterIdToName[spec.id] = spec.name;
  filterNameToId[spec.name] = spec.id;
}

function filterDump(info, filterInfo)
{
  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
  {
    var spec = filterSpecs[fi];
    if (info[spec.name] != filterInfo[spec.id])
      return true;
  }
  return false;
}

function dump_family_sorter(a, b)
{
  return a < b && 1 || -1;
}

function historyPlots(dumps, filterInfo)
{
  var dumpFamilies = {};
  var globalInfo = {maxX: 2, maxY: 2, minY: 0}
  var globalXRemapping = {};
  var isPerfTicks = [];

  for (var di = 0, de = dumps.length; di != de; ++di)
    isPerfTicks[di] = dumps[di][1].counter == "PERF_TICKS";

  for (var i = 0, e = dumps.length; i != e; ++i)
  {
    dumpId = dumps[i][0];
    info = dumps[i][1];

    if (filterDump(info, filterInfo))
      continue;

    dumpFamily = "";
    for (var pii = 0, pie = plotInvariants.length; pii != pie; ++pii)
    {
      dumpFamily += (dumpFamily && ", " || "") + info[plotInvariants[pii]];
    }
    if (!dumpFamilies[dumpFamily])
      dumpFamilies[dumpFamily] = [[], {label: dumpFamily}];

    j = dumpFamilies[dumpFamily][0].length;

    // We need to group dumps which have the same "release" property
    // at the same X value on the plot.
    // For this reason if a given release already has a X value assigned,
    // we reuse that value instead of the one defined it's position in the
    // dumpFamilies[dumpFamily][0] array.
    // Notice that this will not guarantee that the "release" are strictly
    // growing, but simply that the same releases are grouped together,
    // this means we will have to sort the remappedX again later on.
    var remappedX = j;
    if (globalXRemapping[info["release"]])
      remappedX = globalXRemapping[info["release"]];
    else
      globalXRemapping[info["release"]] = j;

    if (isPerfTicks[i])
    {
      var v = [remappedX, Math.round(Number(info["total_count"])*info.tick_period * 100)/100,
                          Number(info["total_freq"]),
                          dumpId, info["name"]];
    }
    else
    {
      var v = [remappedX, Number(info["total_count"]),
                          Number(info["total_freq"]),
                          dumpId, info["name"]];
    }
    // Update the maximum of the scale.
    if (v[1] > globalInfo.maxY)
      globalInfo.maxY = v[1];
    if (!globalInfo.minY)
      globalInfo.minY = v[1];
    if (v[1] < globalInfo.minY)
      globalInfo.minY = v[1];
    if (j > globalInfo.maxX)
      globalInfo.maxX = j;
    dumpFamilies[dumpFamily][0][j] = v;
  }

  // Sort by release.
  var sortable = [];
  var j = 0;
  for (var i in globalXRemapping)
    sortable[j++] = [i, globalXRemapping[i]];
  sortable.sort();

  var orderedRemapping = [];
  for (var i = 0, e = sortable.length; i != e; ++i)
    orderedRemapping[sortable[i][1]] = i;

  var plots = [];
  var seriesOpts = [];
  var pi = 0;

  for (var dfi in dumpFamilies)
  {
    // Sort dump families by their release date.
    dumpFamilies[dfi][0].sort(dump_family_sorter);
    // Extract the information we are interested into.
    plots[pi] = dumpFamilies[dfi][0];
    for (var i = 0, e = plots[pi].length; i != e; ++i)
      plots[pi][i][0] = orderedRemapping[plots[pi][i][0]];
    plots[pi].sort(function(a,b) { return a[0] - b[0]; });
    plots[pi] = plots[pi].slice(-22);
    seriesOpts[pi] = dumpFamilies[dfi][1];
    seriesOpts[pi].color = pv.Colors.category20()(dfi);

    ++pi;
  }
  return [plots, seriesOpts, globalInfo, globalXRemapping, orderedRemapping];
}

var allPlots, allSeries;


function toggleList(l, what)
{
  if (!l)
    return [what];

  var result = [];
  var parts = l.split(",");

  if (typeof(parts) != typeof([]))
    parts = [parts];
  for (var i in parts)
    if (parts[i] == what)
      what = undefined;
    else
      result[result.length] = parts[i];
  if (what)
    result[result.length] = what;
  return result;
}

function dumpsClicked(d, state)
{
  var selectedDumpId = d[3];
  state.sel = toggleList(state.sel, d[3]).join(",");
  var result = "#m!";
  var start = "";
  for (var k in state)
  {
    result += start;
    result += k + "=" + state[k];
    start = "&";
  }
  window.location.hash = result;
  return d;
};

function dumpsMouseIn(obj, d)
{
//  obj.radius(7);
  return obj;
}

function dumpsMouseOut(obj, d)
{
//  obj.radius(5);
  return obj;
}

function aKeyIn(d)
{
  for (var k in d)
    return k;
}

function lastKeyIn(d)
{
  var last = undefined;
  for (var k in d)
    last = k;
  return last;
}


function events_sort(a, b)
{
  var ra = a.split("_");
  var rb = b.split("_");
  var diff = Number(ra[0]) - Number(rb[0]);
  if (diff != 0)
    return diff;
  return ra[1] && 1 || -1;
    return 1;
  return -1;
}

/** Invert sort series so that more recent releases come first*/
function series_sort(a, b)
{
  if (a == "unknown")
    return 1;
  if (b == "unknown")
    return -1;

  var ra = Number(a);
  var rb = Number(b);
  return ra > rb ? -1 : 1;
}


function filter_values_sorter(a, b)
{
  if (a == "unknown")
    return 1;
  if (b == "unknown")
    return -1;
  return a > b ? 1 : -1;
}

/** Given a set of @a dumps and the default filters selection @a defaultFilters
    calculate the unique sets of available filtering choices for that set of
    dumps, the set of all the existing filter choice paths, and a current
    filter selection where if a filter in @a defaultFilter is not available
    it is substituted with the first one in the same category.
  */
function getFilterInfo(dumps, defaultFilters)
{
  var filterInfo = defaultFilters;
  var availableFiltersPaths = {};
  var availableFilters = {};
  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
    availableFilters[filterSpecs[fi].name] = {};

  // Calculate the available filter options, avoiding repetitions.
  for (var di = 0, de = dumps.length; di != de; ++di)
  {
    var info = dumps[di][1];
    var uniquePath = "";
    var start = "";
    for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
    {
      var spec = filterSpecs[fi];
      var filterValue = info[spec.name];
      availableFilters[spec.name][filterValue] = true;
      uniquePath += start + spec.id + "=" + filterValue;
      start = "&";
    }

    availableFiltersPaths[uniquePath] = 1;
  }

  // Sort the available filters in a sensible manner rather than the order
  // they appear in the dumps.
  // In particular:
  // * Sort strings alphabetically.
  // * Sort events numerically.
  // * Have `unknown` always as last choice.
  for (var filterName in availableFilters)
  {
    var sortedValues = [];
    var i = 0;
    for (var filterValue in availableFilters[filterName])
      sortedValues[i++] = filterValue;

    if (filterName == "events")
      sortedValues.sort(events_sort);
    else if (filterName == "series")
      sortedValues.sort(series_sort)
    else
      sortedValues.sort(filter_values_sorter);
    availableFilters[filterName] = sortedValues;
  }


  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
  {
    var spec = filterSpecs[fi];
    if (!filterInfo[spec.id])
      filterInfo[spec.id] = availableFilters[spec.name][0];
  }
  var result = [filterInfo, availableFilters, availableFiltersPaths];
  return result;
}

function removeHiglight(e)
{
  $(".nextSelection").removeClass("nextSelection");
}

function highlightChanges(e)
{
  $(".nextSelection").removeClass("nextSelection");
  var selected = $("#" + e);
  var target = selected[0].hash;
  var state = parseState(selected[0].hash.split("!")[1]);
  for (var key in state)
  {
    var nextName = "#select_" + key + "_" + state[key].escapeCSS();
    var next = $(nextName);
    if (next.hasClass("selected_filter") == false)
    {
      $("#select_" + key + "_" + state[key].escapeCSS()).addClass("nextSelection");
    }
  }
}

/** Generates the best possible anchor to go to when clicking on a given filter
    selector.

    @a filter_key the key of the filter which gets clicked.
    @a filter_value the value of the filter which gets clicked.
    @a filterInfo the current filter selection.
    @a availableFiltersPaths all the possible paths which have at least one dump.

    @return a div
  */
function createFilterSelector(filter_value, filter_key, filterInfo, availableFiltersPaths)
{
  var filter_id = "select_" + filter_key.escapeCSS() + "_" +
                  filter_value.escapeCSS();
  var txt = "<div><a onmouseout=removeHiglight('" + filter_id + "') onmouseover=highlightChanges('" + filter_id + "') id='" + filter_id + "'" + "href='#m!";
  var selectedFilter = "";
  var additionalPart = "";
  var oldFilterPath = "";

  var start = "";
  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
  {
    var spec = filterSpecs[fi];
    oldFilterPath += start + spec.id + "=" + filterInfo[spec.id];
    start = "&";
  }

  // When preparing the part which does not change (e.g. selection and whether
  // or not the viewer should be shown), we always make sure that the filter
  // selector is expanded on click (ssd=1).
  start = "";
  for (var key in filterInfo)
  {
    if (filterIdToName[key])
      continue;
    if (key == "ssd")
      additionalPart += start + "ssd=1";
    else
      additionalPart += start +  key + "=" + filterInfo[key];
    start = "&";
  }
  var oldFilter = filter_key + "=" + filterInfo[filter_key];
  var newFilter = filter_key + "=" + filter_value;
  var extra = (filter_key == "o" ? "" : "&");
  newFilterPath = oldFilterPath.replace(oldFilter + extra, newFilter + extra);

  // Check if the new filter is actually a valid one (i.e., one which has
  // some dumps available). If it is, simply point to the new dumps.
  // If it is not, find the best matching filter combination which would
  // have at least one dump in it.
  if (!availableFiltersPaths[newFilterPath])
  {
    var bestChoice = newFilterPath;
    var bestChoiceScore = 0;

    // Calculate the best (longest) match (word by word) for the requested filter
    // among the available ones.
    for (var existingFilter in availableFiltersPaths)
    {
      // If an existing filter does not have the bit we clicked on, we
      // ignore it.
      if (existingFilter.indexOf(newFilter + (filter_key == "o" ? "" : "&")) == -1)
        continue;

      // Assign a score to a filter by calculating how much it matches the
      // requested one (by word).
      var orderedMatchingScore = 0;
      for (var ci = 0, ce = existingFilter.length; ci != ce; ++ci)
      {
        if (existingFilter[ci] != newFilterPath[ci])
          break;
        if (existingFilter[ci] == "&")
          orderedMatchingScore += 1;
      }

      // Calculate additional score by matching key-value pairs, even if
      // unordered.
      var unorderedMatchingScore = 0;
      for (var key in filterInfo)
      {
        var tmp = key + "=" +  filterInfo[key] + (filter_key == "o" && "" || "&");
        if (existingFilter.indexOf(tmp) != -1)
          unorderedMatchingScore += 1;
      }

      // We give priority to matching things in order, if that does not happen
      // we match the choice which has the least amount of changes.
      var newScore = ((orderedMatchingScore * 100) + unorderedMatchingScore);

      if (newScore >= bestChoiceScore)
      {
        bestChoice = existingFilter;
        bestChoiceScore = newScore;
      }
    }
    txt += bestChoice + "&" + additionalPart + "'";
    txt += " class='unavailable_choice'";
  }
  else
    txt += newFilterPath + "&" + additionalPart +"'";

  txt += ">" + filter_value + "</a></div>";
  return txt;
}

function toggleFilterSelector(show)
{
  var state = parseState(window.location.hash.split("!")[1]);
  state.ssd = show;
  putState(state);
}

/** Uses the model in filterSpecs to generate the top filter selector. */
function generateFilterSelector()
{
  var result = "";
  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
  {
    var spec = filterSpecs[fi];
    result += "<div id='" + spec.name + "_filter_container' class='filter_selector'>" +
              "<span>" + spec.description + ":</span></div>";
  }
  return result;
}

function displayMainList(dumps, state)
{
  var filtersAndSelection = parseState(state);
  $("#main").html("<div id='filters_selector'>" +
                           generateFilterSelector() +
                           "<div id='toggle_show_hide' onclick='toggleFilterSelector(true)'>(change)</div>" +
                           "</div>" +
                           "<div id='dumps_selector' style='clear: both;'>" +
                              "<div id='chartdiv' style='height:420px;'></div>" +
                              "<div id='flatlist_container'>" +
                                "<div id='selected_dumps_list'></div>" +
                              "</div>" +
                           "</div>" +
                           "<div id='db_info' style='clear: both; font-size:70%; padding-top: 10px'></div>");
  var wantedFilters = filtersAndSelection;
  var dumpsInfo = {};
  $("#db_info").append("A total of " + dumps.length + " profile dumps in database.");

  for (var di = 0, de = dumps.length; di != de; ++di)
    dumpsInfo[dumps[di][0]] = dumps[di][1];

  // Handle selection.
  var selection = filtersAndSelection.sel;
  var selectedDumps = [];

  if (selection)
    selectedDumps = selection.split(",");

  if (selectedDumps.length)
  {
    var parts = selectedDumps;
    if (parts.length > 1)
      $("#selected_dumps_list").append("<b id='selected_dumps_header'>Show details of the following dumps:<b>");
    else
      $("#selected_dumps_list").append("<b id='selected_dumps_header'>Show details of the following dump:<b>");

    for (var pi = 0, pe = parts.length; pi != pe; ++pi)
    {
      var sdid = parts[pi];
      var selected_dump_id = "selected_dump_" + sdid;
      var extra = "";
      var detailLink = "<a class='action_link' href='#" + sdid +
                       "!0,self_count,compare'>Show details</a>";

      var info = dumpsInfo[sdid]
      var prettyName = [];
      var prettyNameFields = ["release", "counter", "architecture", "candle",
                     "tier", "sequence", "pileup", "conditions", "events"];

      for (var i in prettyNameFields)
      {
        var key = prettyNameFields[i];
        var value = info[key];
        if (!value)
          continue;

        if (key == "events")
        {
          var events = Number(value);
          if (events == 1)
            prettyName[i] = value + " event";
          else
            prettyName[i] = value + " events";
          continue;
        }
        prettyName[i] = info[key];
      }
      prettyName = prettyName.join(", ");

      $("#selected_dumps_list").append("<li class='selected_dump' id='" +
                                       selected_dump_id +
                                       "'>" +
                                       prettyName +
                                       detailLink +
                                       "</li>");
      function createRemoveHandler (dumpId,s) {
        return function () { dumpsClicked(dumpId, s); }
      }
      var handler = createRemoveHandler([sdid, sdid, sdid,sdid], filtersAndSelection)
      var removeHandler = $("<a class='action_link'>Remove</a>").click(handler);
      $("#"+selected_dump_id).append(removeHandler);
    }

    if (parts.length == 2)
      $("#selected_dumps_header").append("<br/>(<a class='action_link' href='#" +
                                         parts.join(",") + "!0,self_count,compare" +
                                         "'>side by side</a>");
    if (parts.length == 2)
    {
      $("#selected_dumps_header").append(", <a class='action_link' href='#" +
                                         parts.join(",") + "!0,self_count,diff" +
                                         "'>difference</a>" +
                                         ", <a class='action_link' href='#" +
                                         parts.join(",") + "!0,self_count,sum" +
                                         "'>sum</a>" +
                                         ", <a class='action_link' href='#" +
                                         parts.join(",") + "!0,self_count,mul" +
                                         "'>product</a>" +
                                         ", <a class='action_link' href='#" +
                                         parts.join(",") + "!0,self_count,ratio" +
                                         "'>ratio</a>)");
    }
  }
  else
    $("#selected_dumps_list").append("<b id='selected_dumps_header'>Select dumps from plot to start.<b>");

  // Generate the filtering GUI.
  // FIXME: store on server-side calculation of available paths?

  var t = getFilterInfo(dumps, wantedFilters);
  var filterInfo = t[0];
  var availableFilters = t[1];
  var availableFiltersPaths = t[2];

  // No particular selection was done. Select the first possible combination
  // of filters.
  if (window.location.hash == "#m")
  {
    window.location.hash = "m!" + lastKeyIn(availableFiltersPaths);
    return;
  }

  for (var k in availableFilters)
  {
    for (var j in availableFilters[k])
    {
      var filterName = availableFilters[k][j];
      var selector = "#" + k + "_filter_container";
      $(selector).append(createFilterSelector(filterName, filterNameToId[k], filterInfo, availableFiltersPaths));
    }
  }

  $(".filter_selector > div > a").addClass("filters_hidden");

  for (var fi = 0, fe = filterSpecs.length; fi != fe; ++fi)
  {
    var spec = filterSpecs[fi];
    $("#select_" + spec.id + "_" + filterInfo[spec.id].escapeCSS()).addClass("selected_filter");
    $("#select_" + spec.id + "_" + filterInfo[spec.id].escapeCSS()).removeClass("filters_hidden");
  }

  if (filtersAndSelection.ssd != 1)
  {
    $(".filter_selector > div > a:not(.selected_filter)").addClass("filters_hidden").removeClass("filter_is_title");
    $("#toggle_show_hide").replaceWith("<div id='toggle_show_hide' onclick='toggleFilterSelector(1)'>(change)</div>");
  }
  else
  {
    $(".filter_selector > div > a").removeClass("filters_hidden").addClass("filter_is_title");
    $("#toggle_show_hide").replaceWith("<div id='toggle_show_hide' onclick='toggleFilterSelector(0)'>(hide)</div>");
  }

  results = historyPlots(dumps, filterInfo);
  allPlots = results[0];
  allSeries = results[1];
  var globalInfo = results[2];
  var globalXRemapping = results[3];
  var orderedRemapping = results[4];

  // Inverse map for the x labels.
  var xlabels = [];
  for (var gxri in globalXRemapping)
    xlabels[orderedRemapping[globalXRemapping[gxri]]] = gxri;

  var identity = function(x) {return x;}
  function getter(i){return function(x){x[i];}}


  /* Sizing and scales. */
  var deltaY = (globalInfo.maxY - globalInfo.minY) * 0.3 + 2;

  var w = 700,
      h = 300,
      x = pv.Scale.linear(Math.max(0, globalInfo.maxX-21), globalInfo.maxX).range(0, w),
      y = pv.Scale.linear(globalInfo.minY-deltaY, globalInfo.maxY+deltaY).range(0, h);

  /* The root panel. */
  var vis = new pv.Panel().canvas("chartdiv")
      .width(w)
      .height(h)
      .bottom(150)
      .left(90)
      .right(10)
      .top(50);

  vis.add(pv.Label)
      .left(30)
      .top(-5)
      .textAlign("center")
      .textBaseline("bottom")
      .textAngle(0)
      .font("17px sans-serif")
      .text(filterInfo["c"] == "PERF_TICKS" && "seconds" || "bytes");

  /* Y-axis and ticks. */
  vis.add(pv.Rule)
      .data(y.ticks(5))
      .bottom(y)
      .strokeStyle(function(d){return d ? "#eee" : "#000";})
    .anchor("left").add(pv.Label)
      .text(y.tickFormat);

  /* X-axis and ticks. */
  vis.add(pv.Rule)
      .data(x.ticks(globalInfo.maxX))
      .left(x)
      .bottom(-5)
      .height(5)
    .anchor("bottom").add(pv.Label)
      .text(function(i) {
        if (!xlabels[i])
          return undefined;
        if (xlabels[i].indexOf("pre") != -1)
          return xlabels[i].shorten(16);
        return xlabels[i].replace(/CMSSW_[0-9]+_[0-9]+_[0-9X]+_/,"").shorten(16);
      })
      .textAngle(-Math.PI/10).textAlign("right");

  /* A panel for each data series. */
  var panel = vis.add(pv.Panel)
        .data(allPlots);

  var plotNames = allSeries.map(function(f) {return f.label;});
  c = pv.Colors.category20(plotNames);

  function createClickCallback(state) {
    return function(d) {
      dumpsClicked(d, state);
    }
  };

  function selectedPointsHighlighter(points) {
    return function(d) {
      for (var i = 0, e = points.length; i != e; ++i)
        if (points[i] == d[3])
          return 7;
      return 4;
    }
  };

  function selectedPointsLineWidth(points) {
    return function(d) {
      for (var i = 0, e = points.length; i != e; ++i)
        if (points[i] == d[3])
          return 4;
      return 2;
    }
  };

  /* The area with top line. */
  panel.add(pv.Line)
       .data(identity)
       .left(function(d) {return x(d[0]);})
       .bottom(function(d) {return y(d[1]);})
       .strokeStyle(pv.Colors.category20())
       .lineWidth(0.1);
  var dots = panel.add(pv.Dot)
       .data(identity)
       .lineWidth(5)
       .left(function(d) {return x(d[0]);})
       .bottom(function(d) {return y(d[1]);})
       .strokeStyle(function()  {return c(this.parent.index);})
       .fillStyle(function()  {return c(this.parent.index);})
       .lineWidth(selectedPointsLineWidth(selectedDumps))
       .shape("cross")
       .cursor("pointer")
       .radius(selectedPointsHighlighter(selectedDumps))
       .text(getter(0))
       .event("click", createClickCallback(filtersAndSelection))
       .event("mouseover", function(d) {return dumpsMouseIn(this, d)})
       .event("mouseout", function(d) {return dumpsMouseOut(this, d)});

  vis.render();
}

function formatDumpInfo(info)
{
  return "<div class='dump_header'>" +
        (info.candle && "<div><span>Release:</span> " + info.release + "</div>" +
                        "<div><span>Candle:</span>" + info.candle + "</div>" +
                        "<div><span>Sequence:</span>" + info.sequence + "</div>" +
                        "<div><span>Tier:</span>" + info.tier + "</div>" +
                        "<div><span>Pileup:</span>" + info.pileup + "</div>" +
                        "<div><span>Conditions:</span>" + info.conditions + "</div>" ||
                        "Filename: " + info.name) +
                        "<div><span>Counter:</span>" + info.counter + "</div></div>";
}

function formatDumpHeader(info)
{
  return "<div class='dump_table_header'>" +
        (info.candle && info.release + ", " + info.candle + ", " +
                        info.sequence + ", " + info.tier + ", " + info.pileup +
                        ", " + info.conditions + ", " + info.events + " event(s)" ||
                        info.name) +
                        "<br/><b>" + info.counter + "</b></div>";
}


function switchSelection(dumpIndex, column, type)
{
  var selection = "!" + dumpIndex + "," + column + "," + type;
  return function() {
    window.location.hash = window.location.hash.replace(/[!].*$/, selection)
  };
}

function createTableHeader(dumpIndex, counter, label, type)
{
  var headerId="sort_by_" + dumpIndex + "___" + counter + "___" + type;
  var columnHeader = $("<th id='" + headerId + "'>").addClass("sortable_column")
                                                    .append(label)
                                                    .click(switchSelection(dumpIndex, counter, type));
  $("#dump_column_header").append(columnHeader);
}

var sortColumnMapping = {
  "self_count":2,
  "cumulative_count":5,
  "self_calls":3,
  "total_calls":6
};

/** Generate a table header given an array of information about N dumps.
  */
function generateTableHeaders(flatInfo, tickPeriod)
{
  var result = "<tr><th rowspan=3 colspan=1>%&nbsp;total</th>";

  for (var di in flatInfo)
  {
    if (tickPeriod[di])
      result += "<th colspan=2>" + formatDumpHeader(flatInfo[di]) + "</th>";
    else
      result += "<th colspan=4 class='dump_table_header'>" + formatDumpHeader(flatInfo[di]) + "</th>";
  }
  result += "<th rowspan=3 colspan=1 class='sn'>Name</th></tr>";
  result += "<tr>";
  for (var di in flatInfo)
    if (tickPeriod[di])
      result += "<th colspan=2>(seconds)</th>";
    else
      result += "<th colspan=2>(bytes)</th>" +
                "<th colspan=2>(calls)</th>";

  result += "</tr>";
  result += "<tr id='dump_column_header'></tr>";
  return result;
}

var prettyOperatorNames = {
  "diff": "minus",
  "mul": "times",
  "ratio": "divided by",
  "sum": "plus"
}

var prettyOperatorUnits = {
  "diff": function(a,b) {
    if ((a==undefined) && (b==undefined))
      return "<th colspan=2>(&Delta; bytes)</th><th colspan=2>(&Delta; calls)</th>";
    if (a && b)
      return "<th colspan=2>(&Delta; seconds)</th>";
    return "<th colspan=2>(Apples minus Oranges)</th><th colspan=2>(Apples minus Oranges)</th>";
  },

  "mul": function(a,b) {
    if ((a==undefined) && (b==undefined))
      return "<th colspan=2>(bytes<sup>2</sup>)</th><th colspan=2>(calls<sup>2</sup>)</th>";
    if (a && b)
      return "<th colspan=2>(seconds<sup>2</sup>)</th>";
    if (a)
      return "<th colspan=2>(bytes*seconds)</th><th colspan=2>(calls*seconds)</th>";
    if (b)
      return "<th colspan=2>(seconds*bytes)</th><th colspan=2>(calls*seconds)</th>";
  },

  "ratio": function(a,b) {
    if ((a==undefined) && (b==undefined))
      return "<th colspan=2>(%)</th><th colspan=2>(% calls)</th>";
    if (a && b)
      return "<th colspan=2>(%)</th>";
    if (a)
      return "<th colspan=2>(seconds/bytes)</th><th colspan=2>(seconds/calls)</th>";
    if (b)
      return "<th colspan=2>(bytes/seconds)</th><th colspan=2>(calls/seconds)</th>";
  },

  "sum": function(a,b) {
    if ((a==undefined) && (b==undefined))
      return "<th colspan=2>(&Sigma; bytes)</th><th colspan=2>(&Sigma; calls)</th>";
    if (a && b)
      return "<th colspan=2>(&Sigma; seconds)</th>";
    return "<th colspan=2>(Apples plus Oranges)</th><th colspan=2>(Apples plus Oranges)</th>";
  },
}

/** Generate the table header for the case in which we are comparing two
    dumps.
  */
function generateDiffTableHeaders(flatInfo, displayStyle, tickPeriod)
{
  var result = "<tr><th rowspan=3 colspan=1>%&nbsp;total</th>";
  var showCalls = false;

  for (var i = 0, e = tickPeriod.length; i != e; ++i)
    if (!tickPeriod[i])
      showCalls = true;

  if (showCalls)
    result += "<th colspan=4 class='dump_table_header'>" +
              formatDumpHeader(flatInfo[1])  +
              prettyOperatorNames[displayStyle] +
              formatDumpHeader(flatInfo[0]) +
              "</th>";
  else
    result += "<th colspan=2>" + formatDumpHeader(flatInfo[1]) +
              prettyOperatorNames[displayStyle] +
              formatDumpHeader(flatInfo[0]) + "</th>";
  result += "<th rowspan=3 colspan=1 class='sn'>Name</th></tr><tr>";

  result += prettyOperatorUnits[displayStyle](tickPeriod[1], tickPeriod[0]);

  result += "</tr>";
  result += "<tr id='dump_column_header'></tr>";

  return result;
}


function createFlatTable(flatInfo, si, perfticks)
{
  var result = "";
  var isPerfTicks = [];
  var tickPeriod = [];
  for (var di in flatInfo)
  {
    isPerfTicks[di] = flatInfo[di][0].counter == "PERF_TICKS";
    tickPeriod[di] = Number(flatInfo[di][0].tick_period);
  }

  for (var di in flatInfo)
  {
    var info = flatInfo[di][1][si][2];
    if (isPerfTicks[di])
      result += "</td><td>" + RComma(Math.round(info[0]*tickPeriod[di]*1000)/1000) +
                "</td><td>" + RComma(Math.round(info[1]*tickPeriod[di]*1000)/1000);
    else
      result += "</td><td>" + RComma(info[0]) +
                "</td><td>" + RComma(info[1]) +
                "</td><td>" + RComma(info[2]) +
                "</td><td>" + RComma(info[3]);
  }
  return result;
}

var cachedNodes = {};

function createUpdater(nodeChunks) {
  return function(nodesTxt) {
    var nodes = JSON.parse(nodesTxt);
    for (var i = 0, e = nodeChunks.length; i != e; ++i)
    {
      var symbolName = nodes[i][0].escapeHTML();
      var filename = nodes[i][1].escapeHTML();
      cachedNodes[nodeChunks[i]] = [symbolName, filename];
      $("#" + nodeChunks[i]).replaceWith(symbolName);
    }
  };
}
/*
 */
function createSymbolNamesUpdater() {
  var missingNodes = $(".deferred_node_hook").map(function(k, v){return v.id;}).toArray();
  missingNodes.sort();
  var requests = [];
  for (var ci = 0, ce = Math.ceil(missingNodes.length/100); ci != ce; ++ci)
  {
    var nodeChunks = missingNodes.slice(ci*100, (ci+1)*100);
    var url = nodeChunks.join(",");

    requests[requests.length] = { url: url,
                                  success: createUpdater(nodeChunks)};
  }
  return requests;
}

/** Create a sorter which will sort elements according to the absolute value
    found in column @a columnIndex -th element in the array.
    Notice that we do this only for Chrome, since for other browsers it's faster
    to convert array values to strings and then sort them.
    Also notice that given we can have NaNs (i.e. 0/0), we sort them as if
    they were zeros, since most likely we don't care about them.
  */
function createFlatSorter(columnIndex, desc)
{
  return function (a, b) {
    var aValue = a[columnIndex];
    var bValue = b[columnIndex];
    if (isNaN(aValue))
      aValue = 0;
    if (isNaN(bValue))
      bValue = 0;

    var aValue = Math.abs(aValue);
    var bValue = Math.abs(bValue);
    if (aValue == bValue)
      return 0;
    return aValue < bValue ? desc : -desc;
  }
}

// Create a sorter for the flat list which sorts elements by key (i.e. the element
// 1 of an item) as they are sorted in @a sortedIds.
function createSorterById(sortedIds) {
  var sortIndex = {};

  for (var sii = 0, sie = sortedIds.length; sii != sie; ++sii)
    sortIndex[sortedIds[sii][1]] = sii;
  return function(a, b) {
    var aIndex = sortIndex[a[1]];
    var bIndex = sortIndex[b[1]];
    if (aIndex == undefined)
      aIndex = sortedIds.length;
    if (bIndex == undefined)
      bIndex = sortedIds.length;
    return aIndex > bIndex ? 1 : -1;
  }
}

var sortColumnFlatMapping = {
  "self_count": 0,
  "cumulative_count": 1,
  "self_calls": 2,
  "total_calls": 3
}

// Combine two dumps flattened in @a data (each dump spans 6 columns, starting
// from the 3rd one in @a data) into one single dump using the operator
// @a op.
function createCombinedResults(data, combinerSpec)
{
  var op = combinerSpec.op;
  var filter = combinerSpec.filter;

  sortable = [];

  for (var i = 0, e = data.length; i != e; ++i)
  {
    sortable[i] = [data[i][0], data[i][1]];
    for (var j = 0; j < 6; j++)
      sortable[i][1+j] = op(data[i][1 + j], data[i][1 + 6 + j])
  }

  if (!filter)
    return sortable;

  var filtered = [];
  var j = 0;
  for (var i = 0, e = sortable.length; i != e; ++i)
    if (filter(sortable[i]))
      filtered[j++] = sortable[i];

  return filtered;
}

function isBinaryOperator(op)
{
  return op == "diff" || op == "mul" || op == "sum" || op == "ratio";
}

var combinedOperators = {
  diff: {op: function(a,b){return Math.round((b-a)*10000)/10000;}},
  mul: {op: function(a,b){return Math.round((b*a)*10000)/10000;}},
  ratio: {op: function(a,b){return Math.round(b*10000/a)/10000; }},
  sum: {op:function(a,b){return Math.round((b+a)*10000)/10000;}}
}

var binaryOperatorsActionText = {
  diff: "difference",
  mul: "product",
  ratio: "ratio",
  sum: "sum",
  compare: "side by side"
};


function injectBinaryOperatorAction(currentDisplayStyle)
{
  $("#display_style_choice").append("<span class='action_label'>Show: </span>")
  for (var i in binaryOperatorsActionText)
  {
    if (i == currentDisplayStyle)
      continue;
    $("#display_style_choice").append("<a class='action_link' href='" +
                                      window.location.hash.replace(/,[^,]*$/, "," + i) +
                                      "'>" + binaryOperatorsActionText[i] + "</a>");
  }

  var swapped = window.location.hash.replace(/#([^,]+),([^!]+)!(.*)/, "#$2,$1!$3");
  $("#display_style_choice").append("<a class='action_link' href='" + swapped + "'>(swap)</a>");
}

/** A faster sort for array of fixed length tuples.
    - Find the maximum of the requested @a column.
    - Change the Array.toString method to return fixed lenght strings.
    - Sort by string.
    - Put back the old Array.toString.
  */
var sortTableFast = function(column, desc) {
  var max = 0;

  for (var i = 0, e = this.length; i != e; ++i)
  {
    var t = this[i][column];
    if (t == Infinity || isNaN(t))
      continue;
    t = t >= 0 ? t : -t;
    max = t > max ? t : max;
  }

  var maxLength = Math.ceil(Math.log(max+2) / Math.log(10))+4;

  var zeros = [ "" ];
  for (var i = 1, e = maxLength; i != e; ++i)
    zeros[i] = zeros[i - 1]  + "0";

  function customToString(column, max) {
    return function () {
      var v = this[column];
      if (v == Infinity)
        v = "Infinity";
      else if (isNaN(v))
        v = "Nan";
      else
        v = Math.round(v*1000);
      var text = String(v >= 0 ? v : -v);
      return zeros[max - text.length] + text;
    }
  }

  var save = Array.prototype.toString;
  Array.prototype.toString = customToString(column, maxLength);
  var result = this.sort();
  Array.prototype.toString = save;
  if (desc)
    return result.reverse();
  return result;
}

/** Display the flat table with profile counts (and calls) for
    each symbol / profiled entity.

    @a data the data to be visualized, in the format:

    data := [<dumps>, <infos>, <perfPeriod>, <totals>, <flattened data>]
    dumps := [{<dump information>}, ...]
    infos := [<info>, ... ]
    info := [<payload id>, <node id>,
             <self count>, <total count>,
             <self calls>, <total calls>,
             <self paths>, <total paths>]
    perfPeriod := [<dump-perf-ticks-period>, [...]]
    totals := []

    @a sortInfoTxt is a comma-separated list which specifies, in order,
     the dump to use for the sorting. The column to use of the previously mentioned
     dump to use for the sorting, the mode to use to display. Mode can be:

     compare: compare the various dumps side to side.
     diff: subtract the first dump from the second.
     mul: multiply the first dump by the second.
     ratio: divide the second dump by the second.
     sum: sum the first and the second dump.

     Notice that in case a given dump measures the igprof PERF_TICKS counter,
     the tick period is extracted from the dump information and used to multiply
     the value of the counts for that dump.

     FIXME: sum should probably sum as many dumps as provided in data.
  */
function displayFlat(data, sortInfoTxt) {
  // Decide the various parameters for sorting.
  // FIXME: this should be done outside here.
  if (sortInfoTxt)
    var sortInfo = sortInfoTxt.split(",");
  else
  {
    window.location.hash = window.location.hash += "!0,self_count,compare";
    return;
  }
  var sortDump = Number(sortInfo[0]);
  var sortColumnName = sortInfo[1];
  var displayStyle = sortInfo[2];
  var minResult = 0;
  var maxResult = 100;

  var sortColumn = sortColumnFlatMapping[sortColumnName];

  var tickPeriod = data[2];
  var totals = data[3];

  // Extract the dumpIds.
  var requestedDumpIds = extractDumpIds();
  var dumpIds = [];

  for (var rdi = 0, rde = requestedDumpIds.length; rdi != rde; ++rdi)
    dumpIds[rdi] = "p" + requestedDumpIds[rdi];

  // Because we want the ability to display multiple dumps at the same time
  // we first need to sort one, by a given column, and then we sort the others
  // using the same payload id ordering.
  // All this is complicated by the fact that we want the ability to do some
  // operation on two or more dumps and show that instead.
  // What we do is the following:
  // * We copy in "sortable" the dump (or combination of dumps) we want to
  //   sort.
  // * We sort "sortable" according to some criteria and slice the visible amount
  //   of rows, putting the result in "sorted".
  // * We copy in "results" a sliced version of all the dumps, sorted according
  //   to "sorted".

  // Decide what is considered to be the sortable.

  // There shouldn't be any need to recalculate these when only sorting changes
  // however for the time being we leave it here for semplicity.
  // There should be some mechanism of triggering some level of calculation
  // and its dependency only when a given portion of the state changes.
  if (isBinaryOperator(displayStyle))
  {
    var sortable = createCombinedResults(data[4], combinedOperators[displayStyle]);
    sortDump = 0;
  }
  else
    var sortable = data[4];

  Array.prototype.sortAsTable = sortTableFast;
  var sorted = undefined;

  if (need_custom_sort)
    sorted = sortable.sortAsTable(1 + sortDump*6 + sortColumn, 1);
  else
    sorted = sortable.sort(createFlatSorter(1 + sortDump*6 + sortColumn, 1));

  sorted = sorted.slice(minResult, maxResult);

  delete Array.prototype.sortAsTable;

  $("#main").replaceWith("<div id='main'>" +
      "<h4><a href='.'>Back to list</a></h4>" +
      "<div id='display_style_choice'/>" +
      "<form id='baselineForm'/>" +
      "<table id='symbols'>" +
      "<thead>" +
      (isBinaryOperator(displayStyle) &&
       generateDiffTableHeaders(data[0], displayStyle, tickPeriod) ||
       generateTableHeaders(data[0], tickPeriod)) +
      "</thead>" +
      "</table></div>");

  var numDumpsInResults = (sorted[0].length-1) / 6;

  if (tickPeriod.length == 2)
    injectBinaryOperatorAction(displayStyle);


  for (var di = 0; di < numDumpsInResults; di++)
  {
    createTableHeader(di, "self_count", "self", displayStyle);
    createTableHeader(di, "cumulative_count", "cumulative", displayStyle);
    if (tickPeriod[di])
      continue;
    createTableHeader(di, "self_calls", "self", displayStyle);
    createTableHeader(di, "total_calls", "cumulative", displayStyle);
  }

  for (var si = 0, se = sorted.length; si != se; ++si)
  {
    var node_id = sorted[si][0];
    var payloadIds = [];

    for (var di = 0, de = dumpIds.length; di != de; ++di)
      payloadIds[di] = dumpIds[di] + "-" + node_id;

    var symbolHtml = "<a href='#" + payloadIds.join(",") + "!0,to_self_counts," +
                     displayStyle + "'>";

    // In case the node symbol name is not there, we put a placeholder and load
    // it from the server subsequently.
    if (!cachedNodes[node_id])
      symbolHtml += "<span class='deferred_node_hook' id='n" + node_id +
                    "'>loading symbol name</span></a>";
    else
      symbolHtml += cachedNodes[node_id][0];

    symbolHtml += "</a>";

    // Iterate over the sorted results and construct the table.
    var rowHtml = "";
    for (var ri = 0; ri < numDumpsInResults; ri++)
    {
      var startIndex = 1+ri*6;

      if (tickPeriod[ri])
      {
        rowHtml += "</td><td  style='padding-left: 20px;'>" + RComma(sprintf("%.2f", sorted[si][startIndex + 0])) +
                  "</td><td>" + RComma(sprintf("%.2f", sorted[si][startIndex + 1]));
      }
      else
        rowHtml += "</td><td style='padding-left: 20px;'>" + RComma(sorted[si][startIndex + 0]) +
                   "</td><td>" + RComma(sorted[si][startIndex + 1]) +
                   "</td><td style='padding-left: 20px;'>" + RComma(sorted[si][startIndex + 2]) +
                   "</td><td>" + RComma(sorted[si][startIndex + 3]);
    }
    $("#symbols").append("<tr><td>" +
                         RComma(sprintf("%.2f", sorted[si][startIndex + sortColumn]/totals[sortDump][Math.floor(sortColumn/2)]*100)) +
                         "%" + rowHtml +
                         "</td><td class='s' style='padding-left: 20px;'>" + symbolHtml + "</td></tr>");
  }

  $("#sort_by_" + sortInfoTxt.escapeCSS()).addClass("selected_column_header");

  var updaters = createSymbolNamesUpdater();
  for (var ui = 0, ue = updaters.length; ui != ue; ++ui)
    $.ajax(updaters[ui]);
}

function getNodeId(edge, which)
{
  var edgeId = edge.split("-");
  return "-P-" + edgeId[2] + "-" + edgeId[which];
}

function columnSorter(a, b, desc, column)
{
  return desc * (b[column] - a[column]);
}

var nodes_debug;

function formatPerfTicks(x)
{
  return RComma(sprintf("%.2f", x));
}


function renderEdges(edgeData, hook, tickPeriods, sortInfo, isMain, total_counts)
{
  var result = "";

  // Extract the dumpIds.
  var requestedDumpIds = extractDumpIds();
  var dumpIds = [];

  for (var rdi = 0, rde = requestedDumpIds.length; rdi != rde; ++rdi)
    dumpIds[rdi] = "p" + requestedDumpIds[rdi];

  for (var ei = 0, ee = edgeData.length; ei != ee; ++ei)
  {
    var numDumpsInResults = (edgeData[ei].length-1) / 6;
    var node_id = edgeData[ei][0];

    edgeIds = [];

    for (var di = 0, de = dumpIds.length; di != de; ++di)
      edgeIds[di] = dumpIds[di] + "-" + node_id;

    result = "<tr>";
    for (var i = 0, e = numDumpsInResults; i != e; ++i)
    {
      var startOffset = 1 + i * 6;
      var formatter = RComma;
      if (tickPeriods[i])
        formatter = formatPerfTicks;

      result += "<td>" + sprintf("%.2f", edgeData[ei][startOffset + 0] / total_counts * 100)  + "</td>";
      result += "<td style='padding-left: 15px;'>" + (isMain && formatter(edgeData[ei][startOffset + 1]) || "-") + "</td>";

      result += "<td style='padding-left: 25px;'>" + formatter(edgeData[ei][startOffset + 0]) +"</td>";
      result += "<td>/</td>";
      result += "<td style='padding-right: 15px;'>" + (isMain && formatter(edgeData[ei][startOffset + 1] - edgeData[ei][startOffset + 0]) || formatter(edgeData[ei][startOffset + 1])) + "</td>";

      if (!tickPeriods[i])
        result += "<td style='padding-left: 15px;'>" + RComma(edgeData[ei][startOffset + 2]) +"</td>" +
                           "<td>/</td><td  style='padding-right: 15px;'>" +
                           RComma(edgeData[ei][startOffset + 3]) + "</td>";

      result += "<td style='padding-left: 15px;'>" + RComma(edgeData[ei][startOffset + 4]) +"</td>" +
                         "<td>/</td><td style='padding-right: 15px;'>" +
                         RComma(edgeData[ei][startOffset + 5]) +"</td>";
    }

    result += "<td class='s'><a href='#" + edgeIds.join(",") + "!" + sortInfo + "'>";

    if (cachedNodes[node_id])
      result += cachedNodes[node_id][0];
    else
      result += "<span class='deferred_node_hook' id='n" + node_id + "'>loading</span>";

    result += "</a></td></tr>";
    $(hook).append(result);
  }
}

var treeColumns = {
  0: "#self_count_sort",
  1: "#total_count_sort",
  2: "#self_freq_sort",
  3: "#total_freq_sort",
  4: "#self_path_sort",
  5: "#total_path_sort"
};

function generateNodeTableHeaders(tickPeriods, data)
{
  var result = "";
  var numDumpsInResults = (data.length-1)/6;
  for (var di = 0, de = numDumpsInResults; di != de; ++di)
  {
    result += "<colgroup class='counts" + di + "' span='5'></colgroup>";
    if (!tickPeriods[di])
      result += "<colgroup class='freq" + di + "' span='3'></colgroup>";
    result += "<colgroup class='paths" + di + "' span='3'></colgroup>";
  }

  result += "<colgroup class='symbol'></colgroup>";
  result += "<thead><tr>";

  for (var di = 0, de = numDumpsInResults; di != de; ++di)
  {
    result += "<th colspan='5'>Count " + (tickPeriods[di] && "(s)" || "(bytes)") + "</th>";
    if (!tickPeriods[di])
      result += "<th colspan='3'>Calls</th>";

    result += "<th colspan='3'>Paths</th>";
  }

  result += "<th class='sn' rowspan='2'>Symbol Name</th></tr><tr class='moreinfo'>";

  for (var di = 0, de = numDumpsInResults; di != de; ++di)
  {
    result += "<th>% total</th>";
    result += "<th align='right'>Self</th>";
    result += "<th id='self_count_sort" + di + "' align='right'>Self</th><th>/</th>" +
              "<th id='total_count_sort" +  di + "' align='left'>Children</th>";
    if (!tickPeriods[di])
      result += "<th id='self_freq_sort" + di + "' align='right'>Self</th><th>/</th>" +
                "<th id='total_freq_sort" + di + "' align='left'>Children</th>";

    result += "<th id='self_path_sort" + di + "' align='right'>Paths</th><th>/</th>" +
              "<th id='total_path_sort" + di + "' align='left'>Total</th>";
  }

  result += "</tr></thead>";
  return result;
}

var treeColumnMapping = {
  "to_self_counts": 0,
  "total_self_counts": 1,
  "to_self_calls": 2,
  "total_self_calls": 3,
  "to_self_paths": 4,
  "total_self_paths": 5,
};

function updateSortStatus(sortDump, sortColumnName, displayStyle)
{
  return function () {
    var nextUrl = window.location.hash;
    window.location.hash = nextUrl.replace(/[!].*/, "!" + [sortDump,sortColumnName,displayStyle].join(","));
  }
}

function displayNode(data, stateTxt) {
  if (!stateTxt)
  {
    updateSortStatus(0, "to_self_counts", "compare");
    return;
  }
  var sortInfo = stateTxt.split(",");
  if (sortInfo.length != 3)
  {
    updateSortStatus(0, "to_self_counts", "compare");
    return;
  }

  var sortDump = Number(sortInfo[0]) || 0;
  var sortColumn = treeColumnMapping[sortInfo[1]] || 0;
  var displayStyle = isBinaryOperator(sortInfo[2]) && sortInfo[2] || "compare";

  var dumpInfos = data[0];
  var isPerfTick = [];
  var tickPeriods = [];

  var flatReport = [];
  for (var di = 0, de = dumpInfos.length; di != de; ++di)
  {
    isPerfTick[di] = dumpInfos[di].info.counter;
    tickPeriods[di] = Number(dumpInfos[di].info.tick_period);
    flatReport[di] = dumpInfos[di].id;
  }

  var total_counts = dumpInfos[0].info.total_count;
  if (tickPeriods[di])
    total_count *= tickPeriods[di];

  $("title").replaceWith("<title>" + "igprof-explorer" + "</title>");
  $("#main").replaceWith(
    "<div id='main'>" +
    "<h3><a href='#" + flatReport.join(",") +
    "!0,self_count," + displayStyle + "'>Back to summary</a></h3>" +
    "<div id='display_style_choice'/>" +
    "<table cellspacing='0' rules='groups' id='node' class='tablesorter'>" +
    "<thead>" +
    "</thead>" +
    "</table>" +
    "</div>");

  if (tickPeriods.length == 2)
    injectBinaryOperatorAction(displayStyle);

  // Get the various tick periods.
  var tickPeriods = data[2];

  var column_id = treeColumns[sortColumn];
  $(".moreinfo th").css("color", "black");
  $(column_id).css("color", "red");

  $("#node #parents").remove();
  $("#node #mainrow").remove();
  $("#node #children").remove();

  var parentsBody = $("#node").append("<tbody id='parents' class='parents'/>");
  var nodeBody = $("#node").append("<tbody id='mainrow' class='mainrow'/>");
  var childrenBody = $("#node").append("<tbody id='children' class='children'/>");
  var minResult = 0;
  var maxResult = 1000;

  var sortable = [];

  if (isBinaryOperator(displayStyle))
  {
    var sortable = createCombinedResults(data[4].parents, combinedOperators[displayStyle]);
    sortDump = 0;
  }
  else
    var sortable = data[4].parents;

  var sorted = sortable.sort(createFlatSorter(1 + sortDump*6 + sortColumn, -1));

  missingNodes = renderEdges(sorted, "#parents", tickPeriods, sortInfo, false, total_counts);

  if (isBinaryOperator(displayStyle))
  {
    var sorted = createCombinedResults(data[4].main, combinedOperators[displayStyle]);
    sortDump = 0;
  }
  else
    var sorted = data[4].main;

  renderEdges(sorted, "#mainrow", tickPeriods, stateTxt, true, total_counts);

  if (isBinaryOperator(displayStyle))
    $("#node > thead").replaceWith(generateNodeTableHeaders(tickPeriods, sorted[0]));
  else
    $("#node > thead").replaceWith(generateNodeTableHeaders(tickPeriods, sorted[0]));

  var numDumpsInResults = (sorted[0].length-1)/6;

  for (var i = 0; i < numDumpsInResults; ++i)
  {
    $("#self_count_sort" + i).click(updateSortStatus(i,"to_self_counts",displayStyle));
    $("#total_count_sort" + i).click(updateSortStatus(i,"total_self_counts",displayStyle));
    $("#self_freq_sort" + i).click(updateSortStatus(i,"to_self_calls",displayStyle));
    $("#total_freq_sort" + i).click(updateSortStatus(i,"total_self_calls",displayStyle));
    $("#self_path_sort" + i).click(updateSortStatus(i, "to_self_paths",displayStyle));
    $("#total_path_sort" + i).click(updateSortStatus(i, "total_self_paths",displayStyle));
  }

  // There shouldn't be any need to do this every time sorting changes, but
  // only when the operation / filtering changes. However for the time being
  // we leave it here.
  if (isBinaryOperator(displayStyle))
  {
    var sortable = createCombinedResults(data[4].children, combinedOperators[displayStyle]);
    sortDump = 0;
  }
  else
    var sortable = data[4].children;

  var sorted = sortable.sort(createFlatSorter(1 + sortDump*6 + sortColumn, 1));

  renderEdges(sorted, "#children", tickPeriods, stateTxt, false, total_counts);

  var updaters = createSymbolNamesUpdater();
  for (var ui = 0, ue = updaters.length; ui != ue; ++ui)
    $.ajax(updaters[ui]);
}

/* Splits a filter string with the following format:

   key1=value1,key2=value2

   and makes it a dictionary.
*/
function parseState(txt)
{
  if (!txt)
    return {}
  var parts = txt.split("&");
  var results = {};

  for (var pi = 0, pe = parts.length; pi != pe; ++pi)
  {
    var option = parts[pi].split("=");
    results[option[0]] = option[1];
  }
  return results;
}

function putState(state)
{
  var oldHash = window.location.hash.replace(/[!].*/, '');
  var tmpState = [];
  for (var i in state)
    tmpState[tmpState.length] = String(i) + "=" + String(state[i]);

  window.location.hash = oldHash + "!" + tmpState.join("&");
}

function createDataCallback(controller, state, callback, manipulator)
{
  return function (jsonTxt) {
    controller.currentData = manipulator ? manipulator(JSON.parse(jsonTxt)) : JSON.parse(jsonTxt);
    return callback(controller.currentData, state);
  }
}

function extractPeriodsAndTotals(origin, tickPeriod, totals)
{
  for (var di = 0, de = origin.length; di != de; ++di)
  {
    totals[di] = [];
    totals[di][0] = Number(origin[di].total_count);
    totals[di][1] = Number(origin[di].total_freq);
    tickPeriod[di] = undefined;
    if (origin[di].counter == "PERF_TICKS")
    {
      tickPeriod[di] = Number(origin[di].tick_period);
      totals[di][0] *= tickPeriod[di];
    }
  }
}

/** Scale counts using the tick period found in the dump summary.

   @a source a flat table of counts and calls.

 */
function scalePerfTicks(source, tickPeriod)
{
  // We need to scale according to the tick period every time based profile
  // so that we can merge the information.
  for (var di = 0, de = source.length; di != de; ++di)
  {
    if (!tickPeriod[di])
      continue;
    for (var i = 0, e = source[di][1].length; i != e; ++i)
      for (var j = 0; j != 4; ++j)
        source[di][1][i][j] = source[di][1][i][j]*tickPeriod[di];
  }
}

/** Merge sort the list of ids found in the @a source which has the following
    structure

    source := [[[id1, id2,...], ...], [[payload1, payload2, ...], ...]]

    @return a single, sorted list with all the unique ids found in all the
    dumps.
    FIXME: since the arrays are already sorted, I could simply do some sort
           of merge sorting.
  */
function mergeIds(source)
{
  var ids = [];
  // Get all the ids and sort them.
  for (var di = 0, de = source.length; di != de; ++di)
    for (var i = 0, e = source[di][0].length; i != e; ++i)
      ids[ids.length] = source[di][0][i];
  ids.sort();

  // Prune the ids which are the same.
  var j = 0, last = undefined;
  for (var i = 0, e = ids.length; i != e; ++i)
    if (ids[i] != last)
    {
      last = ids[i];
      ids[j++] = last;
    }
  ids = ids.slice(0, j);
  return ids;
}

function extractDumpIds()
{
  var parts = window.location.hash.substring(1).split("!")[0].split(",");
  var ids = [];
  for (var i = 0, e = parts.length; i != e; ++i)
    ids[i] = parts[i].substring(1).split("-")[0];

  return ids;
}

/**  Merges all the dumps passed as an array in @a source into a single array
     or rows, using ids as keys to do the joining. I guess this is a poor man
     javascript JOIN.
  */
function mergeDumps(source, dest, ids)
{
  if (!ids.length)
    return;

  var index = [];
  var dumpIds = [];
  var requestedDumpIds = extractDumpIds();

  for (var rdi = 0, rde = requestedDumpIds.length; rdi != rde; ++rdi)
  {
    index[rdi] = 0;
    dumpIds[rdi] = "p" + requestedDumpIds[rdi];
  }


  for (var i = 0, e = ids.length; i != e; ++i)
    dest[i] = [ids[i]];

  for (var di = 0, de = source.length; di != de; ++di)
  {
    var j = 0;
    for (var i = 0, e = ids.length; i != e; ++i)
    {
      var row;
      if (source[di][0][j] && source[di][0][j] == ids[i])
        row = source[di][1][j++];
      else
        row = [0, 0, 0, 0, 0, 0];

      for (var rii = 0; rii != 6; ++rii)
        dest[i][1 + di*6 + rii] = row[rii];
    }
  }
}

/**
  */
function handleMainData(data)
{
  return data;
}

/** Manipulates the input data once per time it is loaded.
    This includes doing the transformation from counts to seconds, where
    required, and merging the data columns together, matching their symbol names
    so that I need to do that only once. The actual formatting / sorting will
    be done later later on the returned, predigested, data.
    We also save in data[2] the periods and in data[3] the total counts.
  */
function handleFlatData(data)
{
  data[2] = [];
  data[3] = [];

  var tickPeriod = data[2];
  var totals = data[3];
  extractPeriodsAndTotals(data[0], tickPeriod, totals);

  // We need to scale according to the tick period every time based profile
  // so that we can merge the information.
  scalePerfTicks(data[1], tickPeriod);

  // Merge dumps together inserting zeros where a symbol appears only in some
  // dump and not in others.
  data[4] = [];
  var ids = mergeIds(data[1]);
  mergeDumps(data[1], data[4], ids);
  return data;
}

/** Handles the input data for a tree node once its loaded. Notice that
    we basically do what we do for the handleFlatData case, we simply have
    to repeat everything 3 times, once for the parents, once for the children
    and once for the main rows.
  */
function handleNodeData(data)
{
  var tickPeriod = [];
  var totals = [];
  var infos = [];
  for (var i = 0, e = data[0].length; i != e; ++i)
     infos[i] = data[0][i].info;

  extractPeriodsAndTotals(infos, tickPeriod, totals);
  data[2] = tickPeriod;
  data[3] = totals;

  scalePerfTicks(data[1].main, tickPeriod);
  scalePerfTicks(data[1].children, tickPeriod);
  scalePerfTicks(data[1].parents, tickPeriod);

  data[4] = {children: [], parents: [], main: []};

  var parentIds = mergeIds(data[1].parents);
  var childrenIds = mergeIds(data[1].children);
  var mainIds = mergeIds(data[1].main);

  mergeDumps(data[1].parents, data[4].parents, parentIds);
  mergeDumps(data[1].children, data[4].children, childrenIds);
  mergeDumps(data[1].main, data[4].main, mainIds);
  return data;
}

/** The specification of what to do with a given request.
    * @a manipulator which does a one time processing of the data as it arrives
      from the server.
    * @a callback to be invoked on the processed data.
  */
var requestedDataSpecs = {
  d: {callback: displayFlat, manipulator: handleFlatData, noData: displayEmptyFlat},
  m: {url: "dumps", callback: displayMainList, manipulator: handleMainData, noData: displayEmptyMain},
  p: {callback: displayNode, manipulator: handleNodeData}
};

function displayEmptyMain()
{
  $("#main").replaceWith("<div id='main'>Loading data from server...</div>");
}

function displayEmptyFlat()
{
  $("#main").replaceWith("<div id='main'>Loading data from server...</div>");
}

function hashChangedHandler()
{
  var state = window.location.hash.substring(1);

  if (! state)
  {
    window.location.hash = "#m";
    return;
  }

  var request = state.replace(/^#/, '');
  var parts = request.split("!");

  var requestedData = parts[0];
  var state = parts[1];
  debug("requestedData.substring(0,1)");
  debug(requestedData.substring(0,1));
  var requestSpec = requestedDataSpecs[requestedData.substring(0,1)];

  // Do not reload data if only the part of the url after the ! changed,
  // but reuse the one which was already fetched. This way we don't fetch
  // anything if only sorting / diff mode changes.
  if (this.currentRequest != requestedData)
  {
    // If there is an handler for the case no data has yet to been downloaded,
    // execute it.
    if (requestSpec.noData)
      requestSpec.noData();

    this.currentRequest = requestedData;

    var url = requestSpec.url;
    $.ajax({url: url || requestedData,
            success: createDataCallback(this, state,
                                        requestSpec.callback,
                                        requestSpec.manipulator)});
  }
  else
    requestSpec.callback(this.currentData, state)
}

$(document).ready(function() {
  $(window).hashchange(function() {
    // Alerts every time the hash changes!
    hashChangedHandler();
  });
  hashChangedHandler();
  }
);
