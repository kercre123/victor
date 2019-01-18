var hostname = "";  // (Effectively, the device IP)
var host = "";      // hostname and port (e.g. "192.168.42.82:8888")

$(function() {
  InitConfirmDialog();

  // Pull out hostname (device IP) so we can use it in here to call into ANY of the webservers
  if( hostname === "" ) {
    var urlObject = new URL(document.URL);
    host = urlObject.host;
    hostname = urlObject.hostname;
  }
  console.log('host is: ' + host);
  console.log('hostname is: ' + hostname);

  // get the state names from the console function
  $.get({url:"consolefunccall?func=TheBoxMoveToState&args=list",
         success:(function(data){
           data.trim().split(" ").forEach(function(state) {
             $('#DemoState')
               .append($("<option></option>")
                       .attr("value",state)
                       .text(state));
             console.log("adding demo state: " + state);
           });
         })});
});


var kUpdatePeriod_ms = 200;

var moduleLinks = {
  cpu : 'webVizModules/cpu.js',
  vision : 'webVizModules/vision.js',
  box : 'demoModules/box.js',
  touch : 'demoModules/touch.js',
}

var modules;

var originalNamesMap = {};
function makeTabs() {
  // add tabs and tab content
  // NOTE: these aren't actually tabs anymore (this code was all copied from WebViz which uses tabs)
  for( var moduleName in modules ) {
    var originalName = originalNamesMap[moduleName];
    $('ul.tabs').append('<li class="tab-link" data-tab="tab-' + moduleName + '">' 
                        + originalName 
                        + '</li>');
    var tab = $('<div id="tab-' + moduleName + '" class="tab-content"></div>').appendTo('#container');
    tab.append('<div class="popoutIcon"></div>');
  }
}

function connectTabs() {
  for( var moduleName in modules ) { 
  
    var tab_id = 'tab-' + moduleName;

    if( typeof modules[moduleName] === 'undefined' ) {
      console.log('module ' + moduleName + ' undefined!');
      return;
    }

    // subscribe to engine updates
    if( !isSubscribed(moduleName) ) {
      subscribe(moduleName);
      modules[moduleName].init($('#' + tab_id)[0]);

      if( typeof fakeData !== 'undefined' && typeof fakeData[moduleName] !== 'undefined' ) {
        fakeData[moduleName].forEach(function(entry){
          onNewData(moduleName, entry);
        });
      }
    }
  }
}

var isSubscribed = function(moduleName) {
  return (subscribedModules.indexOf(moduleName) >= 0);
}

var startUpdateLoop = function(dt) {
  setTimeout(function() {
    for( var moduleName in modules ) {
      if( isSubscribed(moduleName) ) {
        modules[moduleName].update(dt / 1000.0);
      }
    }
    startUpdateLoop(dt);
  }, dt);
}

var hasShownAlert = false;
var onNewData = function(moduleName, data) {
  if( typeof modules[moduleName] !== 'undefined' ) {
    var elem = $('#tab-' + moduleName)[0];
    try {
      modules[moduleName].onData(data, elem);
    } catch(err) {
      console.log(err);
      if( !hasShownAlert ) {
        alert('The module ' + moduleName + ' has an error. Displayed results may be invalid');
        hasShownAlert = true;
      }
    }
  }
}

var socket;
var subscribedModules = [];
function setupSocket() {
  if( typeof socket !== 'undefined' ) {
    console.log('no socket defined!');
    return;
  }
  var uri = "ws:" + "//" + host + "/socket";
  socket = new WebSocket(uri);
  socket.onopen = function(){
    // notify that we've connected
    $('#status').text('Connected').css('color','limegreen');

    // do all tab connections here: box demo only!
    connectTabs();
  }
  socket.onmessage = function(msg){
    var jsonData = JSON.parse(msg.data);
    var moduleName = jsonData["module"].toLowerCase();
    var data = jsonData["data"];
    onNewData(moduleName, data);
    return false; // keep open
  }
  socket.onclose = function(e){
    $('#status').text('Disconnected').css('color','red');
    for( var moduleName in modules ) {
      var idx = subscribedModules.indexOf(moduleName);
      if( idx >= 0 ) {
        subscribedModules.splice(idx, 1);
      }
    }
  } 
  socket.onerror=function (e) {
    $('#status').text('WebSockets error: ' + e.code + ' '  + e.reason).css('color','red');
  } 
}
var subscribe = function(moduleName) {
  if( !isSubscribed(moduleName) ) {
    var subscribeData = {"type": "subscribe", "module": moduleName};
    socket.send( JSON.stringify(subscribeData) ); 
    subscribedModules.push(moduleName);
  }
}
var unsubscribe = function(moduleName) {
  var unsubscribeData = {"type": "unsubscribe", "module": moduleName};
  socket.send( JSON.stringify(unsubscribeData) ); 
  var idx = subscribedModules.indexOf(moduleName);
  if( idx >= 0 ) {
    subscribedModules.splice(idx, 1);
  }
}
var sendToSocket = function(moduleName, data) {
  var payload = {"type": "data", "module": moduleName.toLowerCase(), "data": data};
  socket.send( JSON.stringify(payload) );
}
function parseQuery(queryString) {
  var query = {};
  var pairs = (queryString[0] === '?' ? queryString.substr(1) : queryString).split('&');
  for (var i = 0; i < pairs.length; i++) {
    var pair = pairs[i].split('=');
    query[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1] || '');
  }
  return query;
}

var modulesContexts = {};
var moduleMethods = {}; // global passed to modules
var moduleSendDataFunc; // global passed to modules
$(function(){

  modules = moduleLinks;
  var missingTabsStr = '<h3>Looking for ';
  $('#tab-overview').append( missingTabsStr );


  // load module data
  var moduleIndex = 0;
  var moduleNames = Object.keys(modules).sort();
  
  var loadJS = function(moduleIndex, onLoad, elem){

    // add an empty object for methods and callbacks
    modulesContexts[moduleNames[moduleIndex]] = {
      methods: {},
      sendFunc: {}
    };

    // bind moduleName to the function passed to the module
    modulesContexts[moduleNames[moduleIndex]].sendFunc 
      = (function (moduleName) { 
        return function(data){
          sendToSocket(moduleName, data);
        } 
      })(moduleNames[moduleIndex]);

    // these refs will be sent to the module
    moduleMethods = modulesContexts[moduleNames[moduleIndex]].methods;
    moduleSendDataFunc = modulesContexts[moduleNames[moduleIndex]].sendFunc;

    var url = modules[moduleNames[moduleIndex]];

    var scriptTag = document.createElement('script');
    scriptTag.src = url;

    scriptTag.onload = onLoad;
    scriptTag.onreadystatechange = onLoad;

    elem.appendChild(scriptTag);
  };
  var onModuleLoad = function(){
    if( (typeof moduleMethods.init === 'undefined') ||
        (typeof moduleMethods.onData === 'undefined') ||
        (typeof moduleMethods.update === 'undefined') ) {
      alert('Module ' + moduleNames[moduleIndex] + ' has a syntax error or does not expose required methods. Bailing');
      delete modules[moduleNames[moduleIndex]]
      moduleNames.splice(moduleIndex,1);
      moduleIndex = moduleIndex - 1;
    } else {
      // replace file path with functions, and convert to lowercase if needed
      if( moduleNames[moduleIndex] != moduleNames[moduleIndex].toLowerCase() ) {
        delete modules[moduleNames[moduleIndex]]
        modules[moduleNames[moduleIndex].toLowerCase()] = moduleMethods;
        originalNamesMap[moduleNames[moduleIndex].toLowerCase()] = moduleNames[moduleIndex];
        moduleNames[moduleIndex] = moduleNames[moduleIndex].toLowerCase();
      } else {
        modules[moduleNames[moduleIndex]] = moduleMethods;
        originalNamesMap[moduleNames[moduleIndex]] = moduleNames[moduleIndex];
      }
    }
    if( moduleIndex >= moduleNames.length - 1 ) {
      // done loading modules.

      // make tabs
      makeTabs();

      // connect to websockets
      setupSocket();

      startUpdateLoop(kUpdatePeriod_ms);

      // See if url had specification of which tab to auto-navigate to
      autoOpenTabRef = "";
      var fullUrl = document.URL;
      var index = fullUrl.indexOf('?');
      if (index >= 0) {
        var queryPairs = parseQuery(fullUrl.substring(index + 1));
        for (var key in queryPairs) {
          var value = queryPairs[key];
          if (key === 'tab') {
            // Save the tab reference, so when the socket finishes
            // connecting, we can auto-navigate to that tab
            autoOpenTabRef = 'tab-' + value;
            break;
          }
        }
      }
      
    } else {
      // increment and continue
      ++moduleIndex;
      loadJS(moduleIndex, onModuleLoad, document.body);
    }
  }
  loadJS(moduleIndex, onModuleLoad, document.body);

});

$(function() {
  $('#MasterVolumeLevel').selectmenu({
    create: function(event, ui) {
      $.post('consolevarget?key=MasterVolumeLevel', function(result) {
        var value = parseInt(result);
        $('#MasterVolumeLevel').prop("selectedIndex", value).selectmenu('refresh');
      });
    },
    change: function(event, data) {
      $.post("consolevarset", {key: "MasterVolumeLevel", value: data.item.index}, function(result) {
        $.post("consolefunccall", {func: "DebugSetMasterVolume", args: ""}, function(result) {});
      });
    }
  });
  $('#DemoState').selectmenu({
    change: function(event, data) {
      if(data.item.value) {
        $.post("consolefunccall", {func: "TheBoxMoveToState", args: data.item.value}, function(result){
          // go back to none after sending
          $('#DemoState').prop("selectedIndex", "none").selectmenu('refresh');
        });
      }
    }
  });
});

function InitConfirmDialog() {
  $("#confirm_dialog").dialog({ autoOpen: false, modal: true, resizable: false,
                                draggable: true,
                                width: 'auto',
                                buttons: {
                                  "OK": function() {
                                    $("#confirm_dialog").dialog("close");
                                    if (dialogOKFunction !== undefined) {
                                      dialogOKFunction();
                                      dialogOKFunction = undefined;
                                    }
                                  },
                                  "Cancel": function () {
                                    $("#confirm_dialog").dialog("close");
                                    dialogOKFunction = undefined;
                                  }
                                } });
}

function StartConfirmDialog(title, body, okfunction) {
  dialogOKFunction = okfunction;
  $("#confirm_dialog_body").html(body);
  var d = $("#confirm_dialog");
  d.dialog("option", "title", title);
  d.dialog("open");
}
