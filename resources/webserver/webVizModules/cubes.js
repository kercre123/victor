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

  function setInteractableSubscription(subscribe) {
    var payload = { 'subscribeInteractable' : subscribe };
    sendData( payload );
  }

  function setTempInteractableSubscription() {
    var payload = { 'subscribeTempInteractable' : true };
    sendData( payload );
  }

  function setBackgroundSubscription(subscribe) {
    var payload = { 'subscribeBackground' : subscribe };
    sendData( payload );
  }

  function setTempBackgroundSubscription() {
    var payload = { 'subscribeTempBackground' : true };
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

    $(elem).append('<div id="ccc-title">CubeConnectionCoordinator</div>');

    var subInterButton = $('<input class="cccInteractableButton" type="button" value="Subscribe Interactable"/>');
    subInterButton.click( function(){
      setInteractableSubscription( true );
    });
    subInterButton.appendTo( elem );

    var unSubInterButton = $('<input class="cccInteractableButton" type="button" value="Unsubscribe Interactable"/>');
    unSubInterButton.click( function(){
      setInteractableSubscription( false );
    });
    unSubInterButton.appendTo( elem );

    var tempSubInterButton = $('<input class="cccInteractableButton" type="button" value="Temp Subscribe Interactable"/>');
    tempSubInterButton.click( function(){
      setTempInteractableSubscription();
    });
    tempSubInterButton.appendTo( elem );

    $(elem).append('<div id="cccButtonBreak"></div>');

     var subBackButton = $('<input class="cccBackgroundButton" type="button" value="Subscribe Background"/>');
    subBackButton.click( function(){
      setBackgroundSubscription( true );
    });
    subBackButton.appendTo( elem );

    var unSubBackButton = $('<input class="cccBackgroundButton" type="button" value="Unsubscribe Background"/>');
    unSubBackButton.click( function(){
      setBackgroundSubscription( false );
    });
    unSubBackButton.appendTo( elem );

    var tempSubBackButton = $('<input class="cccBackgroundButton" type="button" value="Temp Subscribe Background"/>');
    tempSubBackButton.click( function(){
      setTempBackgroundSubscription();
    });
    tempSubBackButton.appendTo( elem );

    cccInfoDiv = $('<h3 id="cccInfo"></h3>').appendTo( elem );

    $(elem).append('<div id="cit-title">CubeInteractionTracker</div>');

    citInfoDiv = $('<h3 id="citInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    
    if(data.hasOwnProperty('commInfo')){
      commInfo = data["commInfo"];
      cubeInfoDiv.empty(); // remove old
      cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Connection state: " + commInfo["connectionState"] + '</div>');
      cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Connected cube: " + commInfo["connectedCube"] + '</div>');
      cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Preferred cube: " + commInfo["preferredCube"] + '</div>');
      cubeInfoDiv.append('<div class="cubeTypeTitle">' + "Latest scan results (sorted by signal strength): " + '</div>');
      function compareRssi(a,b) {
        return (a.lastRssi < b.lastRssi ? 1 : -1)
      }
      cubeData = commInfo["cubeData"];
      cubeData.sort(compareRssi);
      cubeData.forEach(function(entry) {
        Object.keys(entry).forEach(function(key) {
          cubeInfoDiv.append( '<div class="cubeEntry">' + key + ': ' + entry[key] + '</div>');
        })
        cubeInfoDiv.append('<div class="cubeTypeTitle"></div>')
      });
    } else if (data.hasOwnProperty("cccInfo")){
      cccInfo = data["cccInfo"];
      cccInfoDiv.empty(); // clear out old data
      cccInfoDiv.append('<div class="cccTypeTitle">' + "Connection state: " + cccInfo["connectionState"] + '</div>');
      stateCountdown = cccInfo["stateCountdown"];
      stateCountdown.forEach(function(entry){
        Object.keys(entry).forEach(function(key){
          cccInfoDiv.append('<div class="cccTypeTitle">' + key + ': ' + entry[key] + '</div>');
        });
        cccInfoDiv.append('<div class="cccTypeTitle"></div>');
      });
      // loop over subs
      cccInfoDiv.append('<div class="cccTypeTitle">' + "Subscribers: " );
      subscriberData = cccInfo["subscriberData"];
      subscriberData.forEach(function(entry) {
        Object.keys(entry).forEach(function(key){
          cccInfoDiv.append('<div class="subscriberEntry">' + key + ': ' + entry[key] + '</div>');
        })
        cccInfoDiv.append('<div class="cccTypeTitle"></div>');
      });
    } else if (data.hasOwnProperty("citInfo")){
      citInfo = data["citInfo"];
      citInfoDiv.empty(); // clear out old data
      if(citInfo["noTarget"] == true){
        citInfoDiv.append('<div class="citTypeTitle">' + "NO TARGET" + '</div>');
      }
      citInfoDiv.append('<div class="citTypeTitle">' + "Tracking State: " + citInfo["trackingState"] + '</div>');
      citInfoDiv.append('<div class="citTypeTitle">' + "VSM Tracking Rate Request: " + citInfo["visTrackingRate"] + '</div>');
      citInfoDiv.append('<div class="citTypeTitle">' + "User Holding Cube: " + citInfo["userHoldingCube"] + '</div>');
      heldProbability = citInfo["heldProbability"].toFixed(0);
      citInfoDiv.append('<div class="citTypeTitle">' + "Cube Held Probability: " + heldProbability + "%" + '</div>');
      citInfoDiv.append('<div class="citTypeTitle">' + "Recently Moved: " + citInfo["movedRecently"] + '</div>');
      if(citInfo["movedFarRecently"] == true){
        citInfoDiv.append('<div class="citTypeTitle">' + "Recently Moved Far, assuming is held " + '</div>');
      }
      citInfoDiv.append('<div class="citTypeTitle">' + "Recently Visible: " + citInfo["visibleRecently"] + '</div>');
      if(citInfo["userHoldingCube"] != true){
        timeSinceHeld = citInfo["timeSinceHeld"].toFixed(2);
        citInfoDiv.append('<div class="citTypeTitle">' + "Time Since Held: " + timeSinceHeld + '</div>');
      }
      if(citInfo["movedRecently"] != true){
        timeSinceMoved = citInfo["timeSinceMoved"].toFixed(2);
        citInfoDiv.append('<div class="citTypeTitle">' + "Time Since Moved: " + timeSinceMoved + '</div>');
      }
      if(citInfo["visibleRecently"] != true){
        timeSinceSeen = citInfo["timeSinceObserved"].toFixed(2);
        citInfoDiv.append('<div class="citTypeTitle">' + "Time Since Seen: " + timeSinceSeen + '</div>');
      }
      timeSinceTapped = citInfo["timeSinceTapped"].toFixed(2);
      citInfoDiv.append('<div class="citTypeTitle">' + "Time Since Tapped: " + timeSinceTapped + '</div>');
      if(citInfo.hasOwnProperty("targetInfo")){
        targetInfo = citInfo["targetInfo"];
        citInfoDiv.append('<div class="citTypeTitle">' + "Dist To Target [mm]: " + targetInfo["distance"] + '</div>');
        citInfoDiv.append('<div class="citTypeTitle">' + "Dist Measured by prox: " + targetInfo["distMeasuredByProx"] + '</div>');
        citInfoDiv.append('<div class="citTypeTitle">' + "Angle To Target: " + targetInfo["angle"] + '</div>');
      }
    }

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

      #ccc-title {
        font-size:16px;
        margin-top:20px;
        margin-bottom:20px;
      }

      .cccTypeTitle {
        margin-bottom:10px;
      }

      .cccButtonBreak {
      }

      .cccInteractableButton {
        margin-bottom:20px;
        margin-right:10px;
      }

      .cccBackgroundButton {
        margin-bottom:20px;
        margin-right:10px;
      }
      
      .subscriberEntry{
        margin-left:10px;
      }

      #cit-title {
        font-size: 16px;
        margin-top: 20px;
        margin-bottom: 20px;
      }

      .citTypeTitle {
        margin-bottom:10px;
      }
    `;
  };

})(moduleMethods, moduleSendDataFunc);
