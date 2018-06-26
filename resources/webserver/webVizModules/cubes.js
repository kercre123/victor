/*  
 * Cube connection information (i.e. are we connected to
 * a cube? Which one? Which ones do we know about?)
 */

(function(myMethods, sendData) {
  
  function flashCubeLights() {
    var payload = { 'flashCubeLights' : true };
    sendData( payload );
  }
  
  function connectCube() {
    var payload = { 'connectCube' : true };
    sendData( payload );
  }
  
  function disconnectCube() {
    var payload = { 'disconnectCube' : true };
    sendData( payload );
  }
  
  function forgetPreferredCube() {
    var payload = { 'forgetPreferredCube' : true };
    sendData( payload );
  }

  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="cubes-title">Cube connection info</div>');

    // Button to flash lights on currently-connected cube
    var flashCubesButton = $('<input class="cubeButton" type="button" value="Flash Cube Lights"/>');
    flashCubesButton.click( function(){
      flashCubeLights();
    });
    flashCubesButton.appendTo( elem );
    
    // Button to request a connection to a cube
    var connectCubeButton = $('<input class="cubeButton" type="button" value="Connect To Cube"/>');
    connectCubeButton.click( function(){
      connectCube();
    });
    connectCubeButton.appendTo( elem );
    
    // Button to disconnect the current cube
    var disconnectCubeButton = $('<input class="cubeButton" type="button" value="Disconnect From Cube"/>');
    disconnectCubeButton.click( function(){
      disconnectCube();
    });
    disconnectCubeButton.appendTo( elem );
    
    // Button to reset the "block pool"
    var forgetPreferredCubeButton = $('<input class="cubeButton" type="button" value="Forget Preferred Cube"/>');
    forgetPreferredCubeButton.click( function(){
      forgetPreferredCube();
    });
    forgetPreferredCubeButton.appendTo( elem );

    cubeInfoDiv = $('<h3 id="cubeInfo"></h3>').appendTo( elem );
   
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    
    cubeInfoDiv.empty(); // remove old
    cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Connection state: " + data["connectionState"] + '</div>');
    cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Connected cube: " + data["connectedCube"] + '</div>');
    cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Preferred cube: " + data["preferredCube"] + '</div>');
    cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Latest scan results (sorted by signal strength): " + '</div>');
    function compareRssi(a,b) {
      return (a.lastRssi < b.lastRssi ? 1 : -1)
    }
    cubeData = data["cubeData"];
    cubeData.sort(compareRssi);
    cubeData.forEach(function(entry) {
      Object.keys(entry).forEach(function(key) {
        cubeInfoDiv.append( '<div class="cubeEntry">' + key + ': ' + entry[key] + '</div>');
      })
      cubeInfoDiv.append('<div class="cubeTypeTitle"></div>')
    });
  };

  myMethods.update = function(dt, elem) { };

  myMethods.getStyles = function() {
    return `
      #cubes-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .cubeTypeTitle {
        margin-bottom:10px;
      }
      
      .cubeButton {
        margin-bottom:20px;
        margin-right:10px;
      }
      
      .cubeEntry { 
        margin-left:10px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
