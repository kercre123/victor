/*  
 * Cube connection information (i.e. are we connected to
 * a cube? Which one? Which ones do we know about?)
 */

(function(myMethods, sendData) {
  
  function flashCubeLights() {
    var payload = { 'flashCubeLights' : true };
    sendData( payload );
  }
  
  function resetBlockPool() {
    var payload = { 'resetBlockPool' : true };
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
    
    // Button to reset the "block pool"
    var resetBlockPoolButton = $('<input class="cubeButton" type="button" value="Reset Block Pool"/>');
    resetBlockPoolButton.click( function(){
      resetBlockPool();
    });
    resetBlockPoolButton.appendTo( elem );

    cubeInfoDiv = $('<h3 id="cubeInfo"></h3>').appendTo( elem );
   
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    
    cubeInfoDiv.empty(); // remove old
    cubeInfoDiv.append( '<div class="cubeTypeTitle">' + "Cubes:" + '</div>');
    data.forEach(function(entry) {
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
