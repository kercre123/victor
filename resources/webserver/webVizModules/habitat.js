/*  
 * habitat state and control system
 */

(function(myMethods, sendData) {
  
  myMethods.init = function(elem) {
    // Called once when the module is loaded.
    $(elem).append('<div id="habitat-title">Habitat Detection Info</div>');

    var setStateToUnknownButton = $('<input class="habitatButton" type="button" value="Set to Unknown"/>');
    var setStateToNotInHabitatButton = $('<input class="habitatButton" type="button" value="Set to NotInHabitat"/>');
    var setStateToInHabitatButton = $('<input class="habitatButton" type="button" value="Set to InHabitat"/>');

    setStateToUnknownButton.click( function(){
      var payload = { 'forceHabitatState' : 'Unknown' };
      sendData( payload );
    });
    setStateToUnknownButton.appendTo( elem );

    setStateToNotInHabitatButton.click( function(){
      var payload = { 'forceHabitatState' : 'NotInHabitat' };
      sendData( payload );
    });
    setStateToNotInHabitatButton.appendTo( elem );

    setStateToInHabitatButton.click( function(){
      var payload = { 'forceHabitatState' : 'InHabitat' };
      sendData( payload );
    });
    setStateToInHabitatButton.appendTo( elem );
    
    habitatInfoDiv = $('<h3 id="habitatInfo"></h3>').appendTo( elem );
  };

  myMethods.onData = function(data, elem) {
    // The engine has sent a new json blob.
    habitatInfoDiv.empty();
    habitatInfoDiv.append('<div class="detectionTitle">' + "Detection state: " + data["habitatState"] + '</div>');
    habitatInfoDiv.append('<div class="stopOnWhiteEnabled">' + "Stop-On-White enabled: " + data["stopOnWhiteEnabled"] + '</div>');
    habitatInfoDiv.append('<div class="reasonTitle">' + "Reason: " + data["reason"] + '</div>');
  };

  myMethods.update = function(dt, elem) { };

  myMethods.getStyles = function() {
    return `
      #habitat-title {
        font-size:16px;
        margin-bottom:20px;
      }
      
      .detectionTitle {
        margin-bottom:10px;
      }
      
      .reasonTitle {
        margin-bottom:10px;
      }
      
      .habitatButton {
        margin-bottom:20px;
        margin-right:10px;
      }
      
    `;
  };

})(moduleMethods, moduleSendDataFunc);

